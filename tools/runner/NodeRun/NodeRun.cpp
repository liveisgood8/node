#include <iostream>
#include <thread>

#include <windows.h>

#include "node_main.h"

int main(int argc, char* argv[]) {
  SetConsoleOutputCP(CP_UTF8);

  SetEnvironmentVariableA("NODE_DISABLE_COLORS", "1");

  auto runner = GetRunner();
  runner->Init(argc, const_cast<const char**>(argv));
  runner->RunScript("console.log('test')");

  //return RunNode(argc, const_cast<const char**>(argv));
}
