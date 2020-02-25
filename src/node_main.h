#ifndef NODE_MAIN_H_
#define NODE_MAIN_H_

#include "node_runner.h"

#ifdef _WIN32
#ifndef BUILDING_NODE_EXTENSION
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#endif
#else
#define API __attribute__((visibility("default")))
#endif

API node::Runner* GetRunner();
API int RunNode(int argc, const char *argv[]);

#endif  // NODE_MAIN_H_
