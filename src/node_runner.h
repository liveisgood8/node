#ifndef NODE_RUNNER_H_
#define NODE_RUNNER_H_

#include "node.h"

#include <memory>
#include <vector>

namespace v8 {
class Isolate;
}  // namespace v8

struct uv_loop_s;

struct SnapshotData;
//struct NodeIsolate;

namespace node {
 class Environment;

 class NODE_EXTERN Runner {
  public:
   Runner(const Runner&) = delete;
   Runner& operator=(const Runner&) = delete;

   static Runner* GetInstance();

   void Init(int argc, const char** argv);
   void RunScript(const char* script);

  private:
   Runner() = default;
   ~Runner();

   SnapshotData* GetSnapshot() const;
   //NodeIsolate* GetNodeIsolate(uv_loop_s* eventLoop);

  private:
   SnapshotData* snapshotData = nullptr;
 };
 }  // namespace node

#endif  // NODE_RUNNER_H_
