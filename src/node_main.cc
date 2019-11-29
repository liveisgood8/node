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

#include <stdio.h>
#include <iostream>
#include <sstream>

#include "node.h"


extern "C" NODE_EXTERN int __stdcall MainStart(int argc, char* argv[]) {
  return node::Start(argc, argv);
}

extern "C" NODE_EXTERN int __stdcall Init(int argc, char* argv[]) {
  return node::InitFully(argc, argv);
}

extern "C" NODE_EXTERN int __stdcall Eval(const char* script, const char* inputArgsJson, bool isDebug) {
  return node::EvalScript(script, inputArgsJson, isDebug);
}

extern "C" NODE_EXTERN void __stdcall TearDown() {
  node::TearDown();
}

extern "C" NODE_EXTERN bool __stdcall IsInspectorSupported() {
#if HAVE_INSPECTOR
  return true;
#endif // HAVE_INSPECTOR
  return false;
}

std::stringstream errorStreamBuffer;
std::streambuf* oldErrorStream = nullptr;

extern "C" NODE_EXTERN void __stdcall RedirectStderrToMemory() {
  oldErrorStream = std::cerr.rdbuf(errorStreamBuffer.rdbuf());
}

extern "C" NODE_EXTERN void __stdcall GetErrorStack(char *buffer, int size) {
  const auto errorString = errorStreamBuffer.str();
  errorStreamBuffer.str(std::string());

  strcpy_s(buffer, size, errorString.c_str());
}

#ifdef _WIN32
#include <VersionHelpers.h>
#include <WinError.h>
#include <windows.h>

int wmain(int argc, wchar_t* wargv[]) {
  if (!IsWindows7OrGreater()) {
    fprintf(stderr,
            "This application is only supported on Windows 7, "
            "Windows Server 2008 R2, or higher.");
    exit(ERROR_EXE_MACHINE_TYPE_MISMATCH);
  }

  // Convert argv to UTF8
  char** argv = new char*[argc + 1];
  for (int i = 0; i < argc; i++) {
    // Compute the size of the required buffer
    DWORD size = WideCharToMultiByte(
        CP_UTF8, 0, wargv[i], -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) {
      // This should never happen.
      fprintf(stderr, "Could not convert arguments to utf8.");
      exit(1);
    }
    // Do the actual conversion
    argv[i] = new char[size];
    DWORD result = WideCharToMultiByte(
        CP_UTF8, 0, wargv[i], -1, argv[i], size, nullptr, nullptr);
    if (result == 0) {
      // This should never happen.
      fprintf(stderr, "Could not convert arguments to utf8.");
      exit(1);
    }
  }
  argv[argc] = nullptr;
  // Now that conversion is done, we can finally start.
  auto res = node::Start(argc, argv);

  return res;
 /* Init(argc, argv);

   Eval("console.log('test')", nullptr, true);

   TearDown();

   return 0;*/
}
#else
// UNIX
#ifdef __linux__
#include <elf.h>
#ifdef __LP64__
#define Elf_auxv_t Elf64_auxv_t
#else
#define Elf_auxv_t Elf32_auxv_t
#endif  // __LP64__
extern char** environ;
#endif  // __linux__
#if defined(__POSIX__) && defined(NODE_SHARED_MODE)
#include <signal.h>
#include <string.h>
#endif

namespace node {
namespace per_process {
extern bool linux_at_secure;
}  // namespace per_process
}  // namespace node

int main(int argc, char* argv[]) {
#if defined(__POSIX__) && defined(NODE_SHARED_MODE)
  // In node::PlatformInit(), we squash all signal handlers for non-shared lib
  // build. In order to run test cases against shared lib build, we also need
  // to do the same thing for shared lib build here, but only for SIGPIPE for
  // now. If node::PlatformInit() is moved to here, then this section could be
  // removed.
  {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, nullptr);
  }
#endif

#if defined(__linux__)
  char** envp = environ;
  while (*envp++ != nullptr) {
  }
  Elf_auxv_t* auxv = reinterpret_cast<Elf_auxv_t*>(envp);
  for (; auxv->a_type != AT_NULL; auxv++) {
    if (auxv->a_type == AT_SECURE) {
      node::per_process::linux_at_secure = auxv->a_un.a_val;
      break;
    }
  }
#endif
  // Disable stdio buffering, it interacts poorly with printf()
  // calls elsewhere in the program (e.g., any logging from V8.)
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);
  return node::Start(argc, argv);
}
#endif

