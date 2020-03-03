#ifndef NODE_RUNNER_H_
#define NODE_RUNNER_H_

#include "node.h"
#include "node_runner_script.h"

namespace v8 {
class Isolate;
}  // namespace v8

struct uv_loop_s;

struct SnapshotData;
// struct NodeIsolate;

namespace node {
class Environment;

class NODE_EXTERN Runner {
 public:
  Runner(const Runner&) = delete;
  Runner& operator=(const Runner&) = delete;

  static Runner* GetInstance();

  void Init(int argc, const char** argv);
  void RunScript(RunnerScript *script);

  void AppendLastError(const std::string& err);

  // Data must be freed by user
  //InputData* CreateEmptyInputData() const;
  //OutputData* CreateEmptyOutputData() const;
  //void FreeInputData(InputData *data) const;
  //void FreeOutputData(OutputData *data) const;

 private:
  Runner() = default;
  ~Runner();

  void InitDefault();

  void PrepareEnvironment(const RunnerScript::InputData* data,
                          Environment* env);
  void HandleEnvironment(RunnerScript::OutputData* data,
                         Environment* env,
                         bool isRunSuccess);

  SnapshotData* GetSnapshot() const;
  // NodeIsolate* GetNodeIsolate(uv_loop_s* eventLoop);

 private:
  SnapshotData* snapshotData = nullptr;
  bool isInit = false;
};
}  // namespace node

#endif  // NODE_RUNNER_H_
