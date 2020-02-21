#include <iostream>

#include <windows.h>

#include "node_runner.h"

int main(int argc, char* argv[]) {
  SetConsoleOutputCP(CP_UTF8);

  SetEnvironmentVariableA("NODE_DISABLE_COLORS", "1");

  RunNode(argc, const_cast<const char**>(argv));

  //Runner::GetInstance()->Init(argc, const_cast<const char**>(argv));
  //Runner::GetInstance()->RunScript("console.log('test')");
}
