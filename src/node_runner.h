#ifndef NODE_RUNNER_H_
#define NODE_RUNNER_H_

#include <memory>
#include <vector>

namespace node {
class Environment;
class MultiIsolatePlatform;
class MultiIsolatePlatform;
class IsolateData;
class ArrayBufferAllocator;
}  // namespace node

namespace v8 {
class Isolate;
}  // namespace v8

struct uv_loop_s;

struct SnapshotData;
struct NodeIsolate;

class __declspec(dllexport) Runner final {
 public:
  Runner(const Runner&) = delete;
  Runner& operator=(const Runner&) = delete;

  static Runner* GetInstance();

  void SetDebugEnable(bool isEnabled);

  void Init(int argc, const char** argv);

  void RunScript(const char* script);

 private:
  Runner() = default;
  ~Runner();

  node::Environment* CreateMainEnvironment();
  SnapshotData* GetSnapshot() const;
  NodeIsolate* GetNodeIsolate(uv_loop_s* eventLoop);

  void RunEnvironment(NodeIsolate* nodeIsolate, node::Environment* env) const;

 private:
  SnapshotData *snapshotData = nullptr;
  node::MultiIsolatePlatform* platform = nullptr;
  node::ArrayBufferAllocator* allocator = nullptr;
  NodeIsolate *nodeIsolate = nullptr;
  node::Environment *env = nullptr;

  bool isDeserealizeMode = false;

  bool isDebug = false;
};

int __declspec(dllexport) RunNode(int argc, const char* argv[]);

#endif  // NODE_RUNNER_H_
