#ifndef NODE_RUNNER_H_
#define NODE_RUNNER_H_

#include "node.h"

#include <string>
#include <list>
#include <map>


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
  struct Variables {
    std::map<std::string, int> integers;
    std::map<std::string, bool> booleans;
    std::map<std::string, std::string> strings;
  };

  struct InputData {
    std::string scriptOrFilePath;
    bool isFile = false;
    bool isDebug = false;
    Variables inputVariables;
  };

  struct OutputData {
    std::string error;
    Variables outputVariables;
  };

 public:
  Runner(const Runner&) = delete;
  Runner& operator=(const Runner&) = delete;

  static Runner* GetInstance();

  void Init(int argc, const char** argv);
  bool RunScript(const InputData *inputData, OutputData *outputData = nullptr);

  void AppendLastError(const std::string& err);

  // Data must be freed by user
  InputData* CreateEmptyInputData() const;
  OutputData* CreateEmptyOutputData() const;
  void FreeInputData(InputData *data) const;
  void FreeOutputData(OutputData *data) const;

 private:
  Runner() = default;
  ~Runner();

  SnapshotData* GetSnapshot() const;
  // NodeIsolate* GetNodeIsolate(uv_loop_s* eventLoop);

 private:
  SnapshotData* snapshotData = nullptr;
};
}  // namespace node

#endif  // NODE_RUNNER_H_
