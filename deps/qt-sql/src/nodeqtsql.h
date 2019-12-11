#ifndef NODEQTSQL_H
#define NODEQTSQL_H

#include <node.h>

#if !defined(NODEQTSQL_STATIC)
#   if defined(NODEQTSQL_LIBRARY)
#       define NODEQTSQL_EXPORT __declspec(dllexport)
#   else
#       define NODEQTSQL_EXPORT __declspec(dllimport)
#   endif
#else
#define NODEQTSQL_EXPORT
#endif

namespace nodeqtsql {

NODEQTSQL_EXPORT bool openConnection();
NODEQTSQL_EXPORT void initAll(v8::Local<v8::Object> exports);

} // namespace nodeqtsql

#endif // NODEQTSQL_H
