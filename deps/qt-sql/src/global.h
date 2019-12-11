#ifndef NODEQTSQL_GLOBAL_H
#define NODEQTSQL_GLOBAL_H


#if !defined(STATIC_LIB)
#   if defined(NODEQTSQL_LIBRARY)
#       define NODEQTSQL_EXPORT __declspec(dllexport)
#   else
#       define NODEQTSQL_EXPORT __declspec(dllimport)
#   endif
#else
#define NODEQTSQL_EXPORT
#endif

#endif // NODEQTSQL_GLOBAL_H
