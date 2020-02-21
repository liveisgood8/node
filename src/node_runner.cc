#include "node_runner.h"

#include "env.h"
#include "node_internals.h"
#include "node_main_instance.h"
#include "node_options.h"
#include "node_v8_platform-inl.h"
#include "uv.h"
#include "v8.h"

Runner* Runner::GetInstance() {
  static Runner runner;
  return &runner;
}

void Runner::SetDebugEnable(bool isEnabled) {
  isDebug = isEnabled;
}

void Runner::Init(int argc,
                  const char** argv,
                  int execArgc,
                  const char** execArgv) {
  node::Init(&argc, argv, &execArgc, &execArgv);
  snapshotData = GetSnapshot();
  allocator.reset(node::CreateArrayBufferAllocator());
  // isolate = node::NewIsolate(allocator, uv_default_loop(), nullptr);
  // isolateData.reset(node::CreateIsolateData(isolate, uv_default_loop()));

  snapshotData.params.array_buffer_allocator = allocator.get();
  platform = node::per_process::v8_platform.Platform();

  isDeserealizeMode = snapshotData.indexes != nullptr;

  nodeIsolate = GetNodeIsolate(uv_default_loop());
}

void Runner::RunScript(const char* script) {
  auto env = CreateMainEnvironment();
  env->options()->eval_string = script;

  RunEnvironment(&nodeIsolate, env.get());
}

std::unique_ptr<node::Environment> Runner::CreateMainEnvironment() {
  v8::HandleScope handle_scope(nodeIsolate.isolate);

  if (nodeIsolate.isolateData->options()->track_heap_objects) {
    nodeIsolate.isolate->GetHeapProfiler()->StartTrackingHeapObjects(true);
  }

  v8::Local<v8::Context> context;
  if (isDeserealizeMode) {
    context =
        v8::Context::FromSnapshot(nodeIsolate.isolate,
                                  node::NodeMainInstance::kNodeContextIndex)
            .ToLocalChecked();
    node::InitializeContextRuntime(context);
    node::IsolateSettings s;
    SetIsolateErrorHandlers(nodeIsolate.isolate, s);
  } else {
    context = node::NewContext(nodeIsolate.isolate);
  }

  CHECK(!context.IsEmpty());
  v8::Context::Scope contextScope(context);

  // TEMP
  const std::vector<std::string> argv;
  const std::vector<std::string> execArgv;
  std::unique_ptr<node::Environment> env = std::make_unique<node::Environment>(
      nodeIsolate.isolateData.get(),
      context,
      argv,
      execArgv,
      static_cast<node::Environment::Flags>(
          node::Environment::kIsMainThread |
          node::Environment::kOwnsProcessState |
          node::Environment::kOwnsInspector));
  env->InitializeLibuv(node::per_process::v8_is_profiling);
  env->InitializeDiagnostics();

  // TODO(joyeecheung): when we snapshot the bootstrapped context,
  // the inspector and diagnostics setup should after after deserialization.
#if HAVE_INSPECTOR
  *exit_code = env->InitializeInspector({});
#endif

  return env;
}

Runner::SnapshotData Runner::GetSnapshot() const {
  SnapshotData data;
  bool forceNoSnapshot =
      node::per_process::cli_options->per_isolate->no_node_snapshot;
  if (!forceNoSnapshot) {
    v8::StartupData* blob = node::NodeMainInstance::GetEmbeddedSnapshotBlob();
    if (blob != nullptr) {
      data.externalReferences.push_back(reinterpret_cast<intptr_t>(nullptr));
      data.params.external_references = data.externalReferences.data();
      data.params.snapshot_blob = blob;
      data.indexes = node::NodeMainInstance::GetIsolateDataIndexes();
    }
  }
  return data;
}

Runner::NodeIsolate Runner::GetNodeIsolate(uv_loop_s* eventLoop) {
  NodeIsolate nodeIsolate;

  nodeIsolate.isolate = v8::Isolate::Allocate();
  CHECK_NOT_NULL(nodeIsolate.isolate);

  // Register the isolate on the platform before the isolate gets initialized,
  // so that the isolate can access the platform during initialization.
  platform->RegisterIsolate(nodeIsolate.isolate, uv_default_loop());
  node::SetIsolateCreateParamsForNode(&snapshotData.params);
  v8::Isolate::Initialize(nodeIsolate.isolate, snapshotData.params);

  // If the indexes are not nullptr, we are not deserializing
  CHECK_IMPLIES(isDeserealizeMode,
                snapshotData.params.external_references != nullptr);

  nodeIsolate.isolateData =
      std::make_unique<node::IsolateData>(nodeIsolate.isolate,
                                          eventLoop,
                                          platform,
                                          allocator.get(),
                                          snapshotData.indexes);
  node::IsolateSettings s;
  SetIsolateMiscHandlers(nodeIsolate.isolate, s);
  if (!isDeserealizeMode) {
    // If in deserialize mode, delay until after the deserialization is
    // complete.
    SetIsolateErrorHandlers(nodeIsolate.isolate, s);
  }
  return nodeIsolate;
}

void Runner::RunEnvironment(Runner::NodeIsolate* nodeIsolate, node::Environment* env) const {
  v8::Locker locker(nodeIsolate->isolate);
  v8::Isolate::Scope isolate_scope(nodeIsolate->isolate);
  v8::HandleScope handle_scope(nodeIsolate->isolate);

  DCHECK_NOT_NULL(env);
  v8::Context::Scope context_scope(env->context());

  {
    node::InternalCallbackScope callback_scope(
        env,
        v8::Object::New(nodeIsolate->isolate),
        {1, 0},
        node::InternalCallbackScope::kSkipAsyncHooks);
    LoadEnvironment(env);

    env->set_trace_sync_io(env->options()->trace_sync_io);

    {
      v8::SealHandleScope seal(nodeIsolate->isolate);
      bool more;
      env->performance_state()->Mark(
          node::performance::NODE_PERFORMANCE_MILESTONE_LOOP_START);
      do {
        uv_run(env->event_loop(), UV_RUN_DEFAULT);

        node::per_process::v8_platform.DrainVMTasks(nodeIsolate->isolate);

        more = uv_loop_alive(env->event_loop());
        if (more && !env->is_stopping()) continue;

        if (!uv_loop_alive(env->event_loop())) {
          node::EmitBeforeExit(env);
        }

        // Emit `beforeExit` if the loop became alive either after emitting
        // event, or after running some callbacks.
        more = uv_loop_alive(env->event_loop());
      } while (more == true && !env->is_stopping());
      env->performance_state()->Mark(
          node::performance::NODE_PERFORMANCE_MILESTONE_LOOP_EXIT);
    }

    env->set_trace_sync_io(false);
    int exitCode = node::EmitExit(env); // IF 0 then success
  }

  env->set_can_call_into_js(false);
  env->stop_sub_worker_contexts();
  node::ResetStdio();
  env->RunCleanup();

  // TODO(addaleax): Neither NODE_SHARED_MODE nor HAVE_INSPECTOR really
  // make sense here.
#if HAVE_INSPECTOR && defined(__POSIX__) && !defined(NODE_SHARED_MODE)
  struct sigaction act;
  memset(&act, 0, sizeof(act));
  for (unsigned nr = 1; nr < kMaxSignal; nr += 1) {
    if (nr == SIGKILL || nr == SIGSTOP || nr == SIGPROF) continue;
    act.sa_handler = (nr == SIGPIPE) ? SIG_IGN : SIG_DFL;
    CHECK_EQ(0, sigaction(nr, &act, nullptr));
  }
#endif

  node::RunAtExit(env);

  node::per_process::v8_platform.DrainVMTasks(nodeIsolate->isolate);

#if defined(LEAK_SANITIZER)
  __lsan_do_leak_check();
#endif

  //return exit_code;
}
