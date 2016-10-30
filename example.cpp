#include <cassert>
#include <iostream>
#include <dlfcn.h>
#include <string>
#include <unistd.h>
#include <thread>

#include "node.h"

namespace {

using node::AtExit;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Number;
using v8::String;
using v8::HandleScope;
using v8::Local;
using v8::Value;
using v8::TryCatch;
using v8::Context;
using v8::Script;


int add() {
  static int i = 0;
  return i++;
}

void AddMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  args.GetReturnValue().Set(add());
}

void HelloMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, "world"));
}

void CallbackMethod(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();
  v8::Local<v8::Value> result;

  if (args[0]->IsFunction()) {
    std::vector<v8::Local<v8::Value>> argv;
    v8::Local<v8::Value> argument = Number::New(isolate, add());
    argv.push_back(argument);

    auto method = args[0].As<v8::Function>();
    result = node::MakeCallback(isolate,
      isolate->GetCurrentContext()->Global(), method,
      argv.size(), argv.data());
  }
  else {
    result = Undefined(isolate);
  }

  args.GetReturnValue().Set(result);
}


void CompileMethod(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();
  Local<Value> result;

  const char* source = "function __c() {"
                       "  this.x = 42;"
                       "};"
                       "var __val = 43;";

  HandleScope handle_scope(isolate);
  TryCatch try_catch(isolate);

  Local<Context> context(isolate->GetCurrentContext());

  Local<String> script = String::NewFromUtf8(isolate, source);
  Local<Script> compiled_script;
  if (!Script::Compile(context, script).ToLocal(&compiled_script)) {
    String::Utf8Value error(try_catch.Exception());
    return;
  }

  if (!compiled_script->Run(context).ToLocal(&result)) {
    String::Utf8Value error(try_catch.Exception());
    return;
  }

  args.GetReturnValue().Set(result);
}

static void atExitCB(void* arg) {
  Isolate* isolate = static_cast<Isolate*>(arg);
  HandleScope handle_scope(isolate);
  Local<Object> obj = Object::New(isolate);
  assert(!obj.IsEmpty());  // Assert VM is still alive.
  assert(obj->IsObject());
  printf("bye~\n");
}

void init(Local<Object> exports) {
  AtExit(atExitCB, exports->GetIsolate());
  NODE_SET_METHOD(exports, "hello", HelloMethod);
  NODE_SET_METHOD(exports, "add", AddMethod);
  NODE_SET_METHOD(exports, "callback", CallbackMethod);
  NODE_SET_METHOD(exports, "compile", CompileMethod);
}

NODE_MODULE_CONTEXT_AWARE_BUILTIN(soy, init)
}  // namespace

void _node() {
  int (*Start)(int, char **);
  void *handle = dlopen("libnode.51.dylib", RTLD_LAZY | RTLD_NODELETE);
  Start = (int (*)(int, char **))dlsym(handle, "Start");

  int _argc = 2;
  char* _argv[2];
  _argv[0] = "a.out";
  _argv[1] = "app.js";

  node::Start(_argc, _argv);
}


int main(int argc, char *argv[]) {
  std::thread n(_node);
  n.detach();

  // dummy loop
  while(true) {
    usleep(1000*1000);
    //printf("main\n");
  }

  return 0;
}
