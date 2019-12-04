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

static void ShowMessageBox(const FunctionCallbackInfo<Value>& args) {
#ifdef WIN32
  Environment* env = Environment::GetCurrent(args);

  CHECK_EQ(args.Length(), 3);
  CHECK(args[0]->IsString()); // Text
  CHECK(args[1]->IsString()); // Title
  CHECK(args[2]->IsUint32()); // Flags

  const auto Utf8ToWin1251 = [](const char* str) {
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
  };

  String::Utf8Value jsText(env->isolate(), args[0]);
  String::Utf8Value jsTitle(env->isolate(), args[1]);
  const UINT flags = args[2]->Uint32Value(env->context()).FromJust();

  int result = MessageBoxW(nullptr,
                           Utf8ToWin1251(*jsText).c_str(),
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
              FIXED_ONE_BYTE_STRING(env->isolate(), "warning"),
              Uint32::New(env->isolate(), MB_ICONWARNING));

  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "error"),
              Uint32::New(env->isolate(), MB_ICONERROR));

  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "asterisk"),
              Uint32::New(env->isolate(), MB_ICONASTERISK));

  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "info"),
              Uint32::New(env->isolate(), MB_ICONINFORMATION));
#else
  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "warning"),
              Uint32::New(env->isolate(), 0));

  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "error"),
              Uint32::New(env->isolate(), 0));

  iconsObject->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "asterisk"),
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
                     FIXED_ONE_BYTE_STRING(env->isolate(), "abortRetryIgnore"),
                     Uint32::New(env->isolate(), MB_ABORTRETRYIGNORE));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "cancelTryContinue"),
                     Uint32::New(env->isolate(), MB_CANCELTRYCONTINUE));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "help"),
                     Uint32::New(env->isolate(), MB_HELP));


  buttonsObject->Set(env->context(),
                   FIXED_ONE_BYTE_STRING(env->isolate(), "ok"),
                   Uint32::New(env->isolate(), MB_OK));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "retryCancel"),
                     Uint32::New(env->isolate(), MB_RETRYCANCEL));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "yesNo"),
                     Uint32::New(env->isolate(), MB_YESNO));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "yesNoCancel"),
                     Uint32::New(env->isolate(), MB_YESNOCANCEL));
#else
  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "abortRetryIgnore"),
                     Uint32::New(env->isolate(), 0));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "cancelTryContinue"),
                     Uint32::New(env->isolate(), 0));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "help"),
                     Uint32::New(env->isolate(), 0));

  buttonsObject->Set(env->context(),
                   FIXED_ONE_BYTE_STRING(env->isolate(), "ok"),
                   Uint32::New(env->isolate(), 0));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "retryCancel"),
                     Uint32::New(env->isolate(), 0));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "yesNo"),
                     Uint32::New(env->isolate(), 0));

  buttonsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "yesNoCancel"),
                     Uint32::New(env->isolate(), 0));
#endif  // WIN32

  return scope.Escape(buttonsObject);
}

Local<Object> CreateResultsConstObject(Environment* env) {
  EscapableHandleScope scope(env->isolate());

  auto resultsObject = Object::New(env->isolate());

#ifdef WIN32
  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "abort"),
                     Uint32::New(env->isolate(), IDABORT));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "cancel"),
                     Uint32::New(env->isolate(), IDCANCEL));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "continue"),
                     Uint32::New(env->isolate(), IDCONTINUE));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "ignore"),
                     Uint32::New(env->isolate(), IDIGNORE));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "no"),
                     Uint32::New(env->isolate(), IDNO));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "ok"),
                     Uint32::New(env->isolate(), IDOK));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "retry"),
                     Uint32::New(env->isolate(), IDRETRY));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "tryAgain"),
                     Uint32::New(env->isolate(), IDTRYAGAIN));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "yes"),
                     Uint32::New(env->isolate(), IDYES));
#else
  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "abort"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "cancel"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "continue"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "ignore"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "no"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "ok"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "retry"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "tryAgain"),
                     Uint32::New(env->isolate(), 0));

  resultsObject->Set(env->context(),
                     FIXED_ONE_BYTE_STRING(env->isolate(), "yes"),
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
              FIXED_ONE_BYTE_STRING(env->isolate(), "kIcons"),
              CreateIconsConstObject(env));

  target->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "kButtons"),
              CreateButtonsConstObject(env));

  target->Set(env->context(),
              FIXED_ONE_BYTE_STRING(env->isolate(), "kResults"),
              CreateResultsConstObject(env));
}

}  // namespace message
}  // namespace node

NODE_MODULE_CONTEXT_AWARE_INTERNAL(message, node::message::Initialize)
