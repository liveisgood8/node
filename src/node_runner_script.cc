#include "node_runner_script.h"

#include <list>
#include <mutex>

#include "node_runner.h"
#include "util.h"

namespace node {

static std::list<RunnerScript*> scripts;
static std::mutex freeDoneMutex;

RunnerScript* RunnerScript::Create() {
  auto script = new RunnerScript();
  scripts.push_back(script);
  return script;
}

void RunnerScript::Free(RunnerScript* script) {
  DCHECK(script);

  scripts.remove(script);

  script->Clean();
  delete script;
}

RunnerScript* RunnerScript::SetScript(const char* code) {
  inputData.scriptOrFilePath = code;
  return this;
}

RunnerScript* RunnerScript::AddInputInteger(const char* name, int value) {
  inputData.inputVariables.integers[name] = value;
  return this;
}

RunnerScript* RunnerScript::AddInputBoolean(const char* name, bool value) {
  inputData.inputVariables.booleans[name] = value;
  return this;
}

RunnerScript* RunnerScript::AddInputString(const char* name,
                                           const char* value) {
  inputData.inputVariables.strings[name] = value;
  return this;
}

RunnerScript* RunnerScript::WantOutputInteger(const char* name) {
  outputData.outputVariables.integers[name] = 0;
  return this;
}

RunnerScript* RunnerScript::WantOutputBoolean(const char* name) {
  outputData.outputVariables.booleans[name] = false;
  return this;
}

RunnerScript* RunnerScript::WantOutputString(const char* name) {
  outputData.outputVariables.strings[name] = std::string();
  return this;
}

int RunnerScript::GetOuputInteger(const char* name) const {
  const auto element = outputData.outputVariables.integers.find(name);
  return element != outputData.outputVariables.integers.end() ? element->second
                                                              : 0;
}

bool RunnerScript::GetOuputBoolean(const char* name) const {
  const auto element = outputData.outputVariables.booleans.find(name);
  return element != outputData.outputVariables.booleans.end() ? element->second
                                                              : false;
}

const char* RunnerScript::GetOuputString(const char* name) const {
  static std::string emptyString;
  const auto element = outputData.outputVariables.strings.find(name);
  if (element != outputData.outputVariables.strings.end()) {
    return element->second.c_str();
  }
  return emptyString.c_str();
}

void RunnerScript::DeleteLater() {
  outputData.isDone = true;
}

const char* RunnerScript::GetError() const {
  return outputData.error.c_str();
}

bool RunnerScript::Run() {
  Runner::GetInstance()->RunScript(this);
  FreeDone();
  return outputData.isSuccess;
}

void RunnerScript::FreeDone() {
  std::lock_guard<std::mutex> guard(freeDoneMutex);

  auto it = scripts.begin();
  while (it != scripts.end()) {
    bool isDone = (*it)->outputData.isDone;
    if (isDone) {
      (*it)->Clean();
      delete (*it);
      it = scripts.erase(it);
    } else {
      it++;
    }
  }
}

void RunnerScript::FreeAll() {
  for (auto& e : scripts) {
    e->Clean();
    delete e;
  }
  scripts.clear();
}

void RunnerScript::Clean() {
  inputData.inputVariables.integers.clear();
  inputData.inputVariables.booleans.clear();
  inputData.inputVariables.strings.clear();
  outputData.outputVariables.integers.clear();
  outputData.outputVariables.booleans.clear();
  outputData.outputVariables.strings.clear();
}

}  // namespace node
