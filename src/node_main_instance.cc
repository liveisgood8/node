#include <memory>

#include "node_internals.h"
#include "node_main_instance.h"
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
      event_loop(event_loop),
      owns_isolate_(false),
      deserialize_mode_(false) {
  isolate_data_ =
      std::make_unique<IsolateData>(isolate_, event_loop, platform, nullptr);

  SetIsolateMiscHandlers(isolate_, {});
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
      event_loop(event_loop),
      owns_isolate_(true) {
  Isolate::CreateParams createParamsCopy = *params;
  createParamsCopy.array_buffer_allocator = array_buffer_allocator_.get();

  isolate_ = Isolate::Allocate();
  CHECK_NOT_NULL(isolate_);
  // Register the isolate on the platform before the isolate gets initialized,
  // so that the isolate can access the platform during initialization.
  platform->RegisterIsolate(isolate_, event_loop);
  SetIsolateCreateParamsForNode(&createParamsCopy);
  Isolate::Initialize(isolate_, createParamsCopy);

  Locker locker(isolate_);
  isolate_->Enter();

  deserialize_mode_ = per_isolate_data_indexes != nullptr;
  // If the indexes are not nullptr, we are not deserializing
  CHECK_IMPLIES(deserialize_mode_,
                createParamsCopy.external_references != nullptr);
  isolate_data_ = std::make_unique<IsolateData>(isolate_,
                                                event_loop,
                                                platform,
                                                array_buffer_allocator_.get(),
                                                per_isolate_data_indexes);
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
  isolate_->Exit();

  bool platform_finished = false;
  platform_->AddIsolateFinishedCallback(
      isolate_,
      [](void* data) { *static_cast<bool*>(data) = true; },
      &platform_finished);

  platform_->UnregisterIsolate(isolate_);
  isolate_->Dispose();

  DCHECK(event_loop);
  while (!platform_finished) {
    uv_run(event_loop, UV_RUN_ONCE);
  }
}

int NodeMainInstance::Run(
    const std::function<void(Environment* env)> envPreparator,
    const std::function<void(Environment* env, bool isRunSuccess)> envHandler) {
#if HAVE_INSPECTOR && defined(__POSIX__) && !defined(NODE_SHARED_MODE)
#define CLEAN_UP(is_run_success)                                               \
  struct sigaction act;                                                        \
  memset(&act, 0, sizeof(act));                                                \
  for (unsigned nr = 1; nr < kMaxSignal; nr += 1) {                            \
    if (nr == SIGKILL || nr == SIGSTOP || nr == SIGPROF) continue;             \
    act.sa_handler = (nr == SIGPIPE) ? SIG_IGN : SIG_DFL;                      \
    CHECK_EQ(0, sigaction(nr, &act, nullptr));                                 \
  }                                                                            \
  if (envHandler) {                                                            \
    envHandler(env.get(), is_run_success);                                     \
  }
#else
#define CLEAN_UP(is_run_success)                                               \
  if (envHandler) {                                                            \
    envHandler(env.get(), is_run_success);                                     \
  }
#endif

  Locker locker(isolate_);
  Isolate::Scope isolate_scope(isolate_);
  HandleScope handle_scope(isolate_);

  int exit_code = 0;

  DeleteFnPtr<Environment, FreeEnvironment> env =
      CreateMainEnvironment(&exit_code, envPreparator);

  CHECK_NOT_NULL(env);
  Context::Scope context_scope(env->context());

  try {
    if (exit_code == 0) {
      LoadEnvironment(env.get());

      env->set_trace_sync_io(env->options()->trace_sync_io);

      {
        SealHandleScope seal(isolate_);
        bool more;
        env->performance_state()->Mark(
            node::performance::NODE_PERFORMANCE_MILESTONE_LOOP_START);
        do {
          uv_run(env->event_loop(), UV_RUN_DEFAULT);

          per_process::v8_platform.DrainVMTasks(isolate_);

          more = uv_loop_alive(env->event_loop());
          if (more && !env->is_stopping()) continue;

          if (!uv_loop_alive(env->event_loop())) {
            EmitBeforeExit(env.get());
          }

          // Emit `beforeExit` if the loop became alive either after emitting
          // event, or after running some callbacks.
          more = uv_loop_alive(env->event_loop());
        } while (more == true && !env->is_stopping());
        env->performance_state()->Mark(
            node::performance::NODE_PERFORMANCE_MILESTONE_LOOP_EXIT);
      }

      env->set_trace_sync_io(false);
      exit_code = EmitExit(env.get());
    }

  } catch (const NodeException&) {
    CLEAN_UP(false);
    throw;
  }

  if (env->GetExitCode() != 0) {
    exit_code = env->GetExitCode();
  }

  CLEAN_UP(true);
  return exit_code;
}

DeleteFnPtr<Environment, FreeEnvironment>
NodeMainInstance::CreateMainEnvironment(
    int* exit_code, const std::function<void(Environment* env)> envPreparator) {
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
    SetIsolateErrorHandlers(isolate_, {});
  } else {
    context = NewContext(isolate_);
  }

  CHECK(!context.IsEmpty());
  Context::Scope context_scope(context);

  DeleteFnPtr<Environment, FreeEnvironment> env{
      CreateEnvironment(isolate_data_.get(),
                        context,
                        args_,
                        exec_args_,
                        EnvironmentFlags::kDefaultFlags)};

  if (envPreparator) {
    envPreparator(env.get());
  }

  if (*exit_code != 0) {
    return env;
  }

  if (env == nullptr) {
    *exit_code = 1;
  }

  return env;
}

}  // namespace node
