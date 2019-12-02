#include "node_main_instance.h"
#include "node_internals.h"
#include "node_options-inl.h"
#include "node_v8_platform-inl.h"
#include "util-inl.h"
#if defined(LEAK_SANITIZER)
#include <sanitizer/lsan_interface.h>
#endif

#if HAVE_INSPECTOR
#include "inspector/worker_inspector.h"  // ParentInspectorHandle
#endif

namespace node {

using v8::Context;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::Locker;
using v8::Object;
using v8::SealHandleScope;

NodeMainInstance::NodeMainInstance(Isolate* isolate,
                                   uv_loop_t* event_loop,
                                   MultiIsolatePlatform* platform,
                                   const std::vector<std::string>& args,
                                   const std::vector<std::string>& exec_args)
    : args_(args),
      exec_args_(exec_args),
      array_buffer_allocator_(nullptr),
      isolate_(isolate),
      platform_(platform),
      isolate_data_(nullptr),
      owns_isolate_(false),
      deserialize_mode_(false) {
  isolate_data_.reset(new IsolateData(isolate_, event_loop, platform, nullptr));

  IsolateSettings misc;
  SetIsolateMiscHandlers(isolate_, misc);
}

std::unique_ptr<NodeMainInstance> NodeMainInstance::Create(
    Isolate* isolate,
    uv_loop_t* event_loop,
    MultiIsolatePlatform* platform,
    const std::vector<std::string>& args,
    const std::vector<std::string>& exec_args) {
  return std::unique_ptr<NodeMainInstance>(
      new NodeMainInstance(isolate, event_loop, platform, args, exec_args));
}

NodeMainInstance::NodeMainInstance(
    Isolate::CreateParams* params,
    uv_loop_t* event_loop,
    MultiIsolatePlatform* platform,
    const std::vector<std::string>& args,
    const std::vector<std::string>& exec_args,
    const std::vector<size_t>* per_isolate_data_indexes)
    : args_(args),
      exec_args_(exec_args),
      array_buffer_allocator_(ArrayBufferAllocator::Create()),
      isolate_(nullptr),
      platform_(platform),
      isolate_data_(nullptr),
      owns_isolate_(true),
      loop(event_loop) {
  params->array_buffer_allocator = array_buffer_allocator_.get();
  isolate_ = Isolate::Allocate();
  CHECK_NOT_NULL(isolate_);

  // Register the isolate on the platform before the isolate gets initialized,
  // so that the isolate can access the platform during initialization.
  platform->RegisterIsolate(isolate_, event_loop);
  SetIsolateCreateParamsForNode(params);
  Isolate::Initialize(isolate_, *params);

  deserialize_mode_ = per_isolate_data_indexes != nullptr;
  // If the indexes are not nullptr, we are not deserializing
  CHECK_IMPLIES(deserialize_mode_, params->external_references != nullptr);

  Locker locker(isolate_);

  isolate_data_.reset(new IsolateData(isolate_,
                                      event_loop,
                                      platform,
                                      array_buffer_allocator_.get(),
                                      per_isolate_data_indexes));
  IsolateSettings s;
  SetIsolateMiscHandlers(isolate_, s);
  if (!deserialize_mode_) {
    // If in deserialize mode, delay until after the deserialization is
    // complete.
    SetIsolateErrorHandlers(isolate_, s);
  }
}

void NodeMainInstance::Dispose() {
  CHECK(!owns_isolate_);
  platform_->DrainTasks(isolate_);
}

NodeMainInstance::~NodeMainInstance() {
  if (!owns_isolate_) {
    return;
  }

  bool platform_finished = false;

  platform_->AddIsolateFinishedCallback(
      isolate_,
      [](void* data) { *static_cast<bool*>(data) = true; },
      &platform_finished);

  isolate_->Dispose();
  platform_->UnregisterIsolate(isolate_);

  DCHECK(loop);
  while (!platform_finished) {
    uv_run(loop, UV_RUN_ONCE);
  }
}

void NodeMainInstance::SetScript(const std::string& script) {
  this->script = script;
}

void NodeMainInstance::SetInputArgsJson(const std::string& inputArgsJson) {
  this->inputArgsJson = inputArgsJson;
}

void NodeMainInstance::SetInspectorState(bool state) {
  isInspectorEnabled = state;
}

int NodeMainInstance::Run() {
  Locker locker(isolate_);
  Isolate::Scope isolate_scope(isolate_);
  HandleScope handle_scope(isolate_);

  int exit_code = 0;
  auto env_ = CreateMainEnvironment(&exit_code);

  CHECK_NOT_NULL(env_);
  Context::Scope context_scope(env_->context());

  if (exit_code == 0) {
    {
      InternalCallbackScope callback_scope(
          env_.get(),
          Local<Object>(),
          { 1, 0 },
          InternalCallbackScope::kAllowEmptyResource |
              InternalCallbackScope::kSkipAsyncHooks);
      LoadEnvironment(env_.get());
    }

    env_->set_trace_sync_io(env_->options()->trace_sync_io);

    {
      SealHandleScope seal(isolate_);
      bool more;
      env_->performance_state()->Mark(
          node::performance::NODE_PERFORMANCE_MILESTONE_LOOP_START);
      do {
        uv_run(env_->event_loop(), UV_RUN_DEFAULT);

        per_process::v8_platform.DrainVMTasks(isolate_);

        more = uv_loop_alive(env_->event_loop());
        if (more && !env_->is_stopping()) continue;

        if (!uv_loop_alive(env_->event_loop())) {
          EmitBeforeExit(env_.get());
        }

        // Emit `beforeExit` if the loop became alive either after emitting
        // event, or after running some callbacks.
        more = uv_loop_alive(env_->event_loop());
      } while (more == true && !env_->is_stopping());
      env_->performance_state()->Mark(
          node::performance::NODE_PERFORMANCE_MILESTONE_LOOP_EXIT);
    }

    env_->set_trace_sync_io(false);
    exit_code = EmitExit(env_.get());
  }

  env_->set_can_call_into_js(false);
  env_->stop_sub_worker_contexts();
  ResetStdio();
  env_->RunCleanup();

  // TODO(addaleax): Neither NODE_SHARED_MODE nor HAVE_INSPECTOR really
  // make sense here.
#if HAVE_INSPECTOR && defined(__POSIX__) && !defined(NODE_SHARED_MODE)
  struct sigaction act;
  memset(&act, 0, sizeof(act));
  for (unsigned nr = 1; nr < kMaxSignal; nr += 1) {
    if (nr == SIGKILL || nr == SIGSTOP || nr == SIGPROF)
      continue;
    act.sa_handler = (nr == SIGPIPE) ? SIG_IGN : SIG_DFL;
    CHECK_EQ(0, sigaction(nr, &act, nullptr));
  }
#endif

  RunAtExit(env_.get());

  per_process::v8_platform.DrainVMTasks(isolate_);

#if defined(LEAK_SANITIZER)
  __lsan_do_leak_check();
#endif

  return exit_code;
}


// TODO(joyeecheung): align this with the CreateEnvironment exposed in node.h
// and the environment creation routine in workers somehow.
std::unique_ptr<Environment> NodeMainInstance::CreateMainEnvironment(
    int* exit_code) {
  *exit_code = 0;  // Reset the exit code to 0

  HandleScope handle_scope(isolate_);

  // TODO(addaleax): This should load a real per-Isolate option, currently
  // this is still effectively per-process.
  if (isolate_data_->options()->track_heap_objects) {
    isolate_->GetHeapProfiler()->StartTrackingHeapObjects(true);
  }

  Local<Context> context;
  if (deserialize_mode_) {
    context =
        Context::FromSnapshot(isolate_, kNodeContextIndex).ToLocalChecked();
    InitializeContextRuntime(context);
    IsolateSettings s;
    SetIsolateErrorHandlers(isolate_, s);
  } else {
    context = NewContext(isolate_);
  }

  CHECK(!context.IsEmpty());
  Context::Scope context_scope(context);

  auto env_ = std::unique_ptr<Environment>(new Environment(
      isolate_data_.get(),
      context,
      args_,
      exec_args_,
      static_cast<Environment::Flags>(Environment::kIsMainThread |
                                      Environment::kOwnsProcessState |
                                      Environment::kOwnsInspector)));
  env_->InitializeLibuv(per_process::v8_is_profiling);
  env_->InitializeDiagnostics();

  // Set force eval script
  if (!script.empty()) {
    env_->options()->has_eval_string = true;
    env_->options()->eval_string = script;
  }

  // Set force input args
  if (!inputArgsJson.empty()) {
    env_->options()->input_args_json = inputArgsJson;
  }

  // Set inspector
  if (isInspectorEnabled) {
    env_->options()->get_debug_options()->EnableBreakFirstLine();
    env_->options()->get_debug_options()->break_node_first_line = true;
  }

  // TODO(joyeecheung): when we snapshot the bootstrapped context,
  // the inspector and diagnostics setup should after after deserialization.
#if HAVE_INSPECTOR
  *exit_code = env_->InitializeInspector({});
#endif
  if (*exit_code != 0) {
    return env_;
  }

  if (env_->RunBootstrapping().IsEmpty()) {
    *exit_code = 1;
  }

  return env_;
}

}  // namespace node
