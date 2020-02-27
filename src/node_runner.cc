#include "node_runner.h"

#include "node.h"
#include "node_internals.h"
#include "node_main_instance.h"
#include "node_v8_platform-inl.h"

struct SnapshotData {
  v8::Isolate::CreateParams params;
  const std::vector<size_t>* indexes = nullptr;
  std::vector<intptr_t> externalReferences;
};

// struct NodeIsolate {
//  v8::Isolate* isolate = nullptr;
//  std::unique_ptr<node::IsolateData> isolateData;
//};

namespace node {

static node::InitializationResult initResult;
static thread_local std::string lastNodeError;

void Runner::PrepareEnvironment(const RunnerScript::InputData* data,
                                Environment* env) {
  if (!data->scriptOrFilePath.empty()) {
    if (data->isFile) {
      // TODO Run js file
    } else {
      env->options()->has_eval_string = true;
      env->options()->eval_string = data->scriptOrFilePath;
    }

    if (!data->isDebug) {
      env->options()->force_no_inspector_init = true;
    } else {
      env->options()->get_debug_options()->EnableBreakFirstLine();
    }

    auto isolate = env->isolate();
    auto context = env->context();
    auto global = env->context()->Global();
    for (const auto& pair : data->inputVariables.integers) {
      global->Set(
          context,
          v8::String::NewFromUtf8(isolate, pair.first.c_str()).ToLocalChecked(),
          v8::Int32::New(isolate, pair.second));
    }
    for (const auto& pair : data->inputVariables.booleans) {
      global->Set(
          context,
          v8::String::NewFromUtf8(isolate, pair.first.c_str()).ToLocalChecked(),
          v8::Boolean::New(isolate, pair.second));
    }
    for (const auto& pair : data->inputVariables.strings) {
      global->Set(
          context,
          v8::String::NewFromUtf8(isolate, pair.first.c_str()).ToLocalChecked(),
          v8::String::NewFromUtf8(isolate, pair.second.c_str())
              .ToLocalChecked());
    }
  }
}

void Runner::HandleEnvironment(RunnerScript::OutputData* data,
                               Environment* env,
                               bool isRunSuccess) {
  if (!data || !isRunSuccess) {
    return;
  }

  auto isolate = env->isolate();
  auto context = env->context();
  auto global = env->context()->Global();
  for (auto& pair : data->outputVariables.integers) {
    auto value = global
                     ->Get(context,
                           v8::String::NewFromUtf8(isolate, pair.first.c_str())
                               .ToLocalChecked())
                     .ToLocalChecked();
    if (value->IsInt32()) {
      pair.second = value->Int32Value(context).FromJust();
    }
  }
  for (auto& pair : data->outputVariables.booleans) {
    auto value = global
                     ->Get(context,
                           v8::String::NewFromUtf8(isolate, pair.first.c_str())
                               .ToLocalChecked())
                     .ToLocalChecked();
    if (value->IsBoolean()) {
      pair.second = value->BooleanValue(isolate);
    }
  }
  for (auto& pair : data->outputVariables.strings) {
    auto value = global
                     ->Get(context,
                           v8::String::NewFromUtf8(isolate, pair.first.c_str())
                               .ToLocalChecked())
                     .ToLocalChecked();
    if (value->IsString()) {
      pair.second = *v8::String::Utf8Value(isolate, value);
    }
  }
}

Runner* Runner::GetInstance() {
  static Runner runner;
  return &runner;
}

void Runner::Init(int argc, const char** argv) {
  initResult = node::InitializeOncePerProcess(argc, const_cast<char**>(argv));
  snapshotData = GetSnapshot();
}

void Runner::RunScript(RunnerScript* script) {
  DCHECK(script);

  auto loop = std::unique_ptr<uv_loop_t>(new uv_loop_t());
  uv_loop_init(loop.get());

  {
    NodeMainInstance main_instance(&snapshotData->params,
                                   loop.get(),
                                   per_process::v8_platform.Platform(),
                                   initResult.args,
                                   initResult.exec_args,
                                   snapshotData->indexes);

    const auto envPreparator = [this, script](Environment* env) {
      PrepareEnvironment(&script->inputData, env);
    };

    const auto envHandler = [this, script](Environment* env, bool isRunSucces) {
      HandleEnvironment(&script->outputData, env, isRunSucces);
    };

    try {
      int exit_code = main_instance.Run(envPreparator, envHandler);
      script->outputData.isSuccess = exit_code == 0;
    } catch (const NodeException& err) {
      script->outputData.error =
          lastNodeError.empty() ? err.what() : lastNodeError;
      lastNodeError.clear();
    }
  }

  int close_result = uv_loop_close(loop.get());
  DCHECK(close_result == 0);
}

SnapshotData* Runner::GetSnapshot() const {
  auto data = new SnapshotData();
  bool forceNoSnapshot =
      node::per_process::cli_options->per_isolate->no_node_snapshot;
  if (!forceNoSnapshot) {
    v8::StartupData* blob = node::NodeMainInstance::GetEmbeddedSnapshotBlob();
    if (blob != nullptr) {
      data->externalReferences.push_back(reinterpret_cast<intptr_t>(nullptr));
      data->params.external_references = data->externalReferences.data();
      data->params.snapshot_blob = blob;
      data->indexes = node::NodeMainInstance::GetIsolateDataIndexes();
    }
  }
  return data;
}

void Runner::AppendLastError(const std::string& err) {
  lastNodeError += err;
}

// InputData* Runner::CreateEmptyInputData() const {
//  return new InputData();
//}
//
// OutputData* Runner::CreateEmptyOutputData() const {
//  return new OutputData();
//}
//
// void Runner::FreeInputData(InputData* data) const {
//  DCHECK(data);
//  delete data;
//}
//
// void Runner::FreeOutputData(OutputData* data) const {
//  DCHECK(data);
//  delete data;
//}

// NodeIsolate* Runner::GetNodeIsolate(uv_loop_s* eventLoop) {
//  auto nIsolate = new NodeIsolate();
//
//  nIsolate->isolate = v8::Isolate::Allocate();
//  CHECK_NOT_NULL(nIsolate->isolate);
//
//  platform->RegisterIsolate(nIsolate->isolate, uv_default_loop());
//
//  // Register the isolate on the platform before the isolate gets initialized,
//  // so that the isolate can access the platform during initialization.
//  auto& params = snapshotData->params;
//  node::SetIsolateCreateParamsForNode(&params);
//  v8::Isolate::Initialize(nIsolate->isolate, snapshotData->params);
//
//  // If the indexes are not nullptr, we are not deserializing
//  CHECK_IMPLIES(isDeserealizeMode,
//                snapshotData->params.external_references != nullptr);
//
//  nIsolate->isolateData =
//      std::make_unique<node::IsolateData>(nIsolate->isolate,
//                                          eventLoop,
//                                          platform,
//                                          allocator,
//                                          snapshotData->indexes);
//  node::IsolateSettings s;
//  SetIsolateMiscHandlers(nIsolate->isolate, s);
//  if (!isDeserealizeMode) {
//    // If in deserialize mode, delay until after the deserialization is
//    // complete.
//    SetIsolateErrorHandlers(nIsolate->isolate, s);
//  }
//  return nIsolate;
//}

Runner::~Runner() {
  if (snapshotData) {
    delete snapshotData;
  }
  if (per_process::v8_initialized) {
    TearDownOncePerProcess();
  }
}

}  // namespace node
