#include <iostream>
#include <thread>
#include <string.h>

#include <windows.h>
#include <shlwapi.h>

#include "node_main.h"

int main(int argc, char* argv[]) {
  SetConsoleOutputCP(CP_UTF8);

  char modulePath[MAX_PATH];
  ::GetModuleFileNameA(NULL, modulePath, MAX_PATH);
  ::PathRemoveFileSpecA(modulePath);

  const std::string fullNodePath = std::string(modulePath) + "\\node_modules";

  SetEnvironmentVariableA("NODE_DISABLE_COLORS", "1");
  SetEnvironmentVariableA("NODE_PATH", fullNodePath.c_str());

  return RunNode(argc, const_cast<const char**>(argv));
}
