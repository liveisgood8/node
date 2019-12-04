// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "env-inl.h"

#include <../deps/easylogging/easylogging++.h>


INITIALIZE_EASYLOGGINGPP

namespace node {
namespace logger {

using v8::Context;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;
using v8::Function;
using v8::JSON;

static bool isLoggerInitialized = false;


std::string Stringify(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);

  std::string message = "__InnerLog__: ";

  const auto dateObject =
      env->context()
          ->Global()
          ->Get(env->context(), FIXED_ONE_BYTE_STRING(env->isolate(), "Date"))
          .ToLocalChecked()
          .As<Object>();

  for (int i = 0; i < args.Length(); i++) {
    if (i != 0) {
      message += ", ";
    }

    if (args[i]->IsObject() && !args[i]->InstanceOf(env->context(), dateObject).FromJust()) {
      auto jsString =
          JSON::Stringify(env->context(),
                          args[i],
                          FIXED_ONE_BYTE_STRING(env->isolate(), "\t"))
              .ToLocalChecked();

      String::Utf8Value jsStringValue(env->isolate(), jsString);
      message += *jsStringValue;
    }
    else {
      String::Utf8Value jsStringValue(env->isolate(), args[i]);
      message += *jsStringValue;
    }
  }

  return message;
}

static void Trace(const FunctionCallbackInfo<Value>& args) {
  LOG(TRACE) << Stringify(args);
}

static void Debug(const FunctionCallbackInfo<Value>& args) {
  LOG(DEBUG) << Stringify(args);
}

static void Info(const FunctionCallbackInfo<Value>& args) {
  LOG(INFO) << Stringify(args);
}

static void Warning(const FunctionCallbackInfo<Value>& args) {
  LOG(WARNING) << Stringify(args);
}

static void Error(const FunctionCallbackInfo<Value>& args) {
  LOG(ERROR) << Stringify(args);
}

void InitializeLogger() {
#ifdef WIN32
  constexpr const char* kLogFileName = "Log\\lisnode.log";
#else
  constexpr const char* kLogFileName = "Log/lisnode.log";
#endif  // WIN32

  if (!isLoggerInitialized) {
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Filename,
                                       kLogFileName);
    el::Loggers::reconfigureAllLoggers(
        el::Level::Error,
        el::ConfigurationType::Format,
        "%datetime %level [%logger]: %func %msg");

    isLoggerInitialized = true;
  }

}

void Initialize(Local<Object> target,
                Local<Value> unused,
                Local<Context> context,
                void* priv) {
  Environment* env = Environment::GetCurrent(context);

  InitializeLogger();

  env->SetMethod(target, "trace", Trace);
  env->SetMethod(target, "debug", Debug);
  env->SetMethod(target, "info", Info);
  env->SetMethod(target, "warning", Warning);
  env->SetMethod(target, "error", Error);
}

}  // namespace logger
}  // namespace node

NODE_MODULE_CONTEXT_AWARE_INTERNAL(logger, node::logger::Initialize)
