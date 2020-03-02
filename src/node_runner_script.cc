#include "node_runner_script.h"

#include <list>
#include <mutex>

#ifdef WIN32
#include <windows.h>
#endif  // WIN32

#include "../deps/easylogging/easylogging++.h"
#include "node_runner.h"
#include "util.h"

namespace node {

static std::list<RunnerScript*> scripts;
static std::mutex freeDoneMutex;

bool IsUtf8(const char* str);
#ifdef WIN32
std::string ToUtf8FromWin1251(const char* str);
#endif  // WIN32

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
#ifdef WIN32
  if (!IsUtf8(code)) {
    inputData.scriptOrFilePath = ToUtf8FromWin1251(code);
  }
#else
  if (!IsUtf8(code)) {
    LOG(WARNING) << "Script must be in UTF-8 encoding";
  }
  inputData.scriptOrFilePath = code;
#endif  // WIN32

  return this;
}

RunnerScript* RunnerScript::EnableDebugger() {
  inputData.isDebug = true;
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
#ifdef WIN32
  if (!IsUtf8(value)) {
    inputData.inputVariables.strings[name] = ToUtf8FromWin1251(value);
  }
#else
  inputData.inputVariables.strings[name] = value;
#endif  // WIN32
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

bool IsUtf8(const char* str) {
  if (!str) {
    return true;
  }

  const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str);
  unsigned int cp;
  int num;

  while (*bytes != 0x00) {
    if ((*bytes & 0x80) == 0x00) {
      // U+0000 to U+007F
      cp = (*bytes & 0x7F);
      num = 1;
    } else if ((*bytes & 0xE0) == 0xC0) {
      // U+0080 to U+07FF
      cp = (*bytes & 0x1F);
      num = 2;
    } else if ((*bytes & 0xF0) == 0xE0) {
      // U+0800 to U+FFFF
      cp = (*bytes & 0x0F);
      num = 3;
    } else if ((*bytes & 0xF8) == 0xF0) {
      // U+10000 to U+10FFFF
      cp = (*bytes & 0x07);
      num = 4;
    } else
      return false;

    bytes += 1;
    for (int i = 1; i < num; ++i) {
      if ((*bytes & 0xC0) != 0x80) return false;
      cp = (cp << 6) | (*bytes & 0x3F);
      bytes += 1;
    }

    if ((cp > 0x10FFFF) || ((cp >= 0xD800) && (cp <= 0xDFFF)) ||
        ((cp <= 0x007F) && (num != 1)) ||
        ((cp >= 0x0080) && (cp <= 0x07FF) && (num != 2)) ||
        ((cp >= 0x0800) && (cp <= 0xFFFF) && (num != 3)) ||
        ((cp >= 0x10000) && (cp <= 0x1FFFFF) && (num != 4)))
      return false;
  }

  return true;
}

#ifdef WIN32
std::string ToUtf8FromWin1251(const char* str) {
  if (IsUtf8(str)) {
    return str;
  }

  bool error = false;

  int result_u = ::MultiByteToWideChar(1251, 0, str, -1, 0, 0);
  if (!result_u) {
    LOG(WARNING) << "Encoding: Ошибка при преобразовании в UTF-8 (ш. 1)";
    return 0;
  }

  wchar_t* ures = new wchar_t[result_u];
  if (!::MultiByteToWideChar(1251, 0, str, -1, ures, result_u)) {
    delete[] ures;
    LOG(WARNING) << "Encoding: Ошибка при преобразовании в UTF-8 (ш. 2)";
    return 0;
  }

  int result_c = ::WideCharToMultiByte(CP_UTF8, 0, ures, -1, 0, 0, 0, 0);
  if (!result_c) {
    delete[] ures;
    LOG(WARNING) << "Encoding: Ошибка при преобразовании в UTF-8 (ш. 3)";
    return 0;
  }

  char* cres = new char[result_c];
  if (!::WideCharToMultiByte(CP_UTF8, 0, ures, -1, cres, result_c, 0, 0)) {
    delete[] cres;
    LOG(WARNING) << "Encoding: Ошибка при преобразовании в UTF-8 (ш. 4)";
    return 0;
  }
  delete[] ures;

  std::string result(cres);
  delete[] cres;

  return result;
}
#endif  // WIN32

}  // namespace node
