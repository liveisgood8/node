#ifndef NODE_RUNNER_H_
#define NODE_RUNNER_H_

#include <memory>

#include "node.h"

class NODE_EXTERN Runner {
 private:
  struct SnapshotData {
    v8::Isolate::CreateParams params;
    const std::vector<size_t>* indexes = nullptr;
    std::vector<intptr_t> externalReferences;
  };

  struct NodeIsolate {
    v8::Isolate* isolate = nullptr;
    std::unique_ptr<node::IsolateData> isolateData;
  };

 public:
  Runner(const Runner&) = delete;
  Runner& operator=(const Runner&) = delete;

  static Runner* GetInstance();

  void SetDebugEnable(bool isEnabled);

  void Init(int argc, const char** argv, int execArgc, const char** execArgv);

  void RunScript(const char* script);

 private:
  Runner() = default;

  std::unique_ptr<node::Environment> CreateMainEnvironment();
  SnapshotData GetSnapshot() const;
  NodeIsolate GetNodeIsolate(uv_loop_s *eventLoop);

  void RunEnvironment(NodeIsolate *nodeIsolate, node::Environment* env) const;

 private:
  SnapshotData snapshotData;
  node::MultiIsolatePlatform* platform = nullptr;
  std::shared_ptr<node::ArrayBufferAllocator> allocator;
  NodeIsolate nodeIsolate;
  std::unique_ptr<node::Environment> env;

  bool isDeserealizeMode = false;

  bool isDebug = false;
};

#endif  // NODE_RUNNER_H_
