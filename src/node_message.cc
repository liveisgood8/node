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

#ifdef WIN32
#include <windows.h>
#endif  // WIN32


namespace node {
namespace message {

using v8::FunctionCallbackInfo;
using v8::Value;
using v8::Object;
using v8::Context;
using v8::String;
using v8::Local;
using v8::Isolate;
using v8::EscapableHandleScope;
using v8::Uint32;
using v8::JSON;

static std::wstring Utf8ToWin1251(const char *str) {
  DWORD result_u = ::MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
  if (!result_u) {
    std::cerr << "showMessageBox: cannot convert utf8 string to win1251";
    return std::wstring();
  }

  wchar_t* ures = new wchar_t[result_u];
  if (!::MultiByteToWideChar(CP_UTF8, 0, str, -1, ures, result_u)) {
    std::cerr << "showMessageBox: cannot convert utf8 string to win1251";
    delete[] ures;
    return std::wstring();
  }

  std::wstring result(ures);
  delete[] ures;

  return result;
}

static void ShowMessageBox(const FunctionCallbackInfo<Value>& args) {
#ifdef WIN32
  Environment* env = Environment::GetCurrent(args);

  CHECK_EQ(args.Length(), 3);
  CHECK(args[0]->IsString());
  CHECK(args[1]->IsString());
  CHECK(args[2]->IsInt32());


  String::Utf8Value jsMessage(env->isolate(), args[0]);
  String::Utf8Value jsTitle(env->isolate(), args[1]);
  UINT flags = args[2]->Uint32Value(env->context()).FromJust();

  int result = ::MessageBoxW(nullptr,
                             Utf8ToWin1251(*jsMessage).c_str(),
                             Utf8ToWin1251(*jsTitle).c_str(),
                             flags);
  args.GetReturnValue().Set(result);
#else
  args.GetReturnValue().Set(0);
#endif  // WIN32
}

Local<Object> CreateIconsConstObject(Environment *env) {
  EscapableHandleScope scope(env->isolate());

  auto iconsObject = Object::New(env->isolate());

#ifdef WIN32
  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "WARNING"),
              Uint32::New(env->isolate(), MB_ICONWARNING));

  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "ERROR"),
              Uint32::New(env->isolate(), MB_ICONERROR));

  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "ASTERISK"),
              Uint32::New(env->isolate(), MB_ICONASTERISK));

  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "info"),
              Uint32::New(env->isolate(), MB_ICONINFORMATION));
#else
  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "WARNING"),
              Uint32::New(env->isolate(), 0));

  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "ERROR"),
              Uint32::New(env->isolate(), 0));

  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "ASTERISK"),
              Uint32::New(env->isolate(), 0));

  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "info"),
              Uint32::New(env->isolate(), 0));
#endif  // WIN32

  return scope.Escape(iconsObject);
}

Local<Object> CreateButtonsConstObject(Environment* env) {
  EscapableHandleScope scope(env->isolate());

  auto buttonsObject = Object::New(env->isolate());

#ifdef WIN32
  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "ABORT_RETRY_IGNORE"),
                     Uint32::New(env->isolate(), MB_ABORTRETRYIGNORE));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "CANCEL_TRY_CONTINUE"),
                     Uint32::New(env->isolate(), MB_CANCELTRYCONTINUE));

  buttonsObject->Set(env->context(),
                   FIXED_ONE_BYTE_STRING(env->isolate(), "OK"),
                   Uint32::New(env->isolate(), MB_OK));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "OK_CANCEL"),
                     Uint32::New(env->isolate(), MB_OKCANCEL));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "RETRY_CANCEL"),
                     Uint32::New(env->isolate(), MB_RETRYCANCEL));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "YES_NO"),
                     Uint32::New(env->isolate(), MB_YESNO));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "YES_NO_CANCEL"),
                     Uint32::New(env->isolate(), MB_YESNOCANCEL));
#else
  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "ABORT_RETRY_IGNORE"),
                     Uint32::New(env->isolate(), 0));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "CANCEL_TRY_CONTINUE"),
                     Uint32::New(env->isolate(), 0));


  buttonsObject->Set(env->context(),
                   FIXED_ONE_BYTE_STRING(env->isolate(), "OK"),
                   Uint32::New(env->isolate(), 0));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "OK_CANCEL"),
                     Uint32::New(env->isolate(), 0));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "RETRY_CANCEL"),
                     Uint32::New(env->isolate(), 0));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "YES_NO"),
                     Uint32::New(env->isolate(), 0));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "YES_NO_CANCEL"),
                     Uint32::New(env->isolate(), 0));
#endif  // WIN32

  return scope.Escape(buttonsObject);
}

Local<Object> CreateResultsConstObject(Environment* env) {
  EscapableHandleScope scope(env->isolate());

  auto resultsObject = Object::New(env->isolate());

#ifdef WIN32
  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "ABORT"),
                     Uint32::New(env->isolate(), IDABORT));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "CANCEL"),
                     Uint32::New(env->isolate(), IDCANCEL));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "CONTINUE"),
                     Uint32::New(env->isolate(), IDCONTINUE));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "IGNORE"),
                     Uint32::New(env->isolate(), IDIGNORE));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "NO"),
                     Uint32::New(env->isolate(), IDNO));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "OK"),
                     Uint32::New(env->isolate(), IDOK));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "RETRY"),
                     Uint32::New(env->isolate(), IDRETRY));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "TRYAGAIN"),
                     Uint32::New(env->isolate(), IDTRYAGAIN));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "YES"),
                     Uint32::New(env->isolate(), IDYES));
#else
  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "ABORT"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "CANCEL"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "CONTINUE"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "IGNORE"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "NO"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "OK"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "RETRY"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "TRYAGAIN"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "YES"),
                     Uint32::New(env->isolate(), 0));
#endif  // WIN32

  return scope.Escape(resultsObject);
}

void Initialize(Local<Object> target,
                Local<Value> unused,
                Local<Context> context,
                void* priv) {
  Environment* env = Environment::GetCurrent(context);
  env->SetMethod(target, "showMessageBox", ShowMessageBox);

  target->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "Icons"),
              CreateIconsConstObject(env));

  target->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "Buttons"),
              CreateButtonsConstObject(env));

  target->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "Results"),
              CreateResultsConstObject(env));
}

}  // namespace message
}  // namespace node

NODE_MODULE_CONTEXT_AWARE_INTERNAL(message, node::message::Initialize)
