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

void PrepareEnvironment(const Runner::InputData* data, Environment* env) {
  if (!data->scriptOrFilePath.empty()) {
    if (data->isFile) {
      // TODO Run js file
    } else {
      env->options()->has_eval_string = true;
      env->options()->eval_string = data->scriptOrFilePath;
    }
  }
}

void HandleEnvironment(Runner::OutputData* data, Environment* env, bool isRunSuccess) {
  if (!data) {
    return;
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

bool Runner::RunScript(const InputData* inputData, OutputData* outputData) {
  DCHECK(inputData);

  auto loop = std::unique_ptr<uv_loop_t>(new uv_loop_t());
  uv_loop_init(loop.get());

  bool isSuccess = false;
  {
    NodeMainInstance main_instance(&snapshotData->params,
                                   loop.get(),
                                   per_process::v8_platform.Platform(),
                                   initResult.args,
                                   initResult.exec_args,
                                   snapshotData->indexes);

    const auto envPreparator = [inputData](Environment* env) {
      PrepareEnvironment(inputData, env);
    };

    const auto envHandler = [outputData](Environment* env, bool isRunSucces) {
      HandleEnvironment(outputData, env, isRunSucces);
    };

    try {
      int exit_code = main_instance.Run(envPreparator, envHandler);
      isSuccess = exit_code == 0;
    } catch (const NodeException& err) {
      if (outputData) {
        outputData->error = lastNodeError.empty() ? err.what() : lastNodeError;
      }
      lastNodeError.clear();
    }
  }

  int close_result = uv_loop_close(loop.get());
  DCHECK(close_result == 0);

  return isSuccess;
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


Runner::InputData* Runner::CreateEmptyInputData() const {
  return new InputData();
}

Runner::OutputData* Runner::CreateEmptyOutputData() const {
  return new OutputData();
}

void Runner::FreeInputData(InputData* data) const {
  DCHECK(data);
  delete data;
}


void Runner::FreeOutputData(OutputData* data) const {
  DCHECK(data);
  delete data;
}

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
  TearDownOncePerProcess();
}

}  // namespace node
