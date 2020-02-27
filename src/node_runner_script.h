#ifndef NODE_RUNNER_SCRIPT_H_
#define NODE_RUNNER_SCRIPT_H_

#include "node.h"

#include <map>
#include <string>

namespace node {
class Environment;

class NODE_EXTERN RunnerScript {
  friend class Runner;
  friend void OnExit();

 private:
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
    bool isDone = false;
    bool isSuccess = false;
    std::string error;
    Variables outputVariables;
  };

 public:
  // Script should be marked as DeleteLater
  // if user want to free memory on next script run
  // or user can free it manually by Runner::Free
  // At program exit all resources used by script freed automatically
  static RunnerScript* Create();
  static void Free(RunnerScript* script);

  RunnerScript(const RunnerScript&) = delete;
  RunnerScript& operator=(const RunnerScript&) = delete;
  RunnerScript(RunnerScript&&) = delete;
  RunnerScript& operator=(RunnerScript&&) = delete;

  RunnerScript* SetScript(const char* code);

  RunnerScript* AddInputInteger(const char* name, int value);
  RunnerScript* AddInputBoolean(const char* name, bool value);
  RunnerScript* AddInputString(const char* name, const char* value);

  RunnerScript* WantOutputInteger(const char* name);
  RunnerScript* WantOutputBoolean(const char* name);
  RunnerScript* WantOutputString(const char* name);

  int GetOuputInteger(const char* name) const;
  bool GetOuputBoolean(const char* name) const;
  const char* GetOuputString(const char* name) const;

  void DeleteLater();

  const char* GetError() const;

  bool Run();

 private:
  static void FreeDone();
  static void FreeAll();

  RunnerScript() = default;

  void Clean();

 private:
  InputData inputData;
  OutputData outputData;
};
}  // namespace node

#endif  // NODE_RUNNER_SCRIPT_H_
