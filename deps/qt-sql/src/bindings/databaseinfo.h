#ifndef NODEDATABSEINFO_H
#define NODEDATABSEINFO_H

#include <node.h>


namespace bindings {

class NodeDatabaseInfo final
{
public:
  static void init(v8::Local<v8::Object> exports);

private:
  static void databaseType(const v8::FunctionCallbackInfo<v8::Value>& args);
};

} // namespace bindings

#endif // NODEDATABSEINFO_H
