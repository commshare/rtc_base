/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_BASE_THREAD_H_
#define WEBRTC_BASE_THREAD_H_

#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <vector>
//
//#define WEBRTC_WIN

#if defined(WEBRTC_POSIX)
#include <pthread.h>
#endif
#include "constructormagic.h"
#include "event.h"
#include "messagequeue.h"

#if defined(WEBRTC_WIN)
#include "win32.h"
#endif

namespace rtc {

class Thread;

class ThreadManager {
 public:
  static const int kForever = -1;

  ThreadManager();
  ~ThreadManager();

  static ThreadManager* Instance();

  Thread* CurrentThread();
  void SetCurrentThread(Thread* thread);

  // Returns a thread object with its thread_ ivar set
  // to whatever the OS uses to represent the thread.
  // If there already *is* a Thread object corresponding to this thread,
  // this method will return that.  Otherwise it creates a new Thread
  // object whose wrapped() method will return true, and whose
  // handle will, on Win32, be opened with only synchronization privileges -
  // if you need more privilegs, rather than changing this method, please
  // write additional code to adjust the privileges, or call a different
  // factory method of your own devising, because this one gets used in
  // unexpected contexts (like inside browser plugins) and it would be a
  // shame to break it.  It is also conceivable on Win32 that we won't even
  // be able to get synchronization privileges, in which case the result
  // will have a NULL handle.
  Thread *WrapCurrentThread();
  void UnwrapCurrentThread();

 private:
#if defined(WEBRTC_POSIX)
  pthread_key_t key_;
#endif

#if defined(WEBRTC_WIN)
  DWORD key_;
#endif

  RTC_DISALLOW_COPY_AND_ASSIGN(ThreadManager);
};

struct _SendMessage {
  _SendMessage() {}
  Thread *thread;
  Message msg;
  bool *ready;
};

class Runnable {
 public:
  virtual ~Runnable() {}
  virtual void Run(Thread* thread) = 0;

 protected:
  Runnable() {}

 private:
  RTC_DISALLOW_COPY_AND_ASSIGN(Runnable);
};

// WARNING! SUBCLASSES MUST CALL Stop() IN THEIR DESTRUCTORS!  See ~Thread().
// 继承自此类的子类，必须在其析构中主动调用 Stop 方法
class LOCKABLE Thread : public MessageQueue {
 public:
  // 已弃用的默认构造函数，不要使用，应该使用下面的静态方法进行创建类实例
  // 建议在开发过程中使用 CreateWithSocketServer 创建	 
  // Create a new Thread and optionally assign it to the passed SocketServer.
  Thread();
  explicit Thread(SocketServer* ss);
  explicit Thread(std::unique_ptr<SocketServer> ss);

  // NOTE: ALL SUBCLASSES OF Thread MUST CALL Stop() IN THEIR DESTRUCTORS (or
  // guarantee Stop() is explicitly called before the subclass is destroyed).
  // This is required to avoid a data race between the destructor modifying the
  // vtable, and the Thread::PreRun calling the virtual method Run().
  // 所有的子类均必须保证在其析构函数中或者之前明确调用 Stop() 方法
  // 这样做是为了避免 Thread::PreRun 调用 Run() 方法时，析构函数对虚函数表的修改
  ~Thread() override;

  // rtc::Thread 提供的创建类实例的静态方法
  static std::unique_ptr<Thread> CreateWithSocketServer();
  static std::unique_ptr<Thread> Create();

  // 返回当前线程的实例指针
  static Thread* Current();

  // Used to catch performance regressions. Use this to disallow blocking calls
  // (Invoke) for a given scope.  If a synchronous call is made while this is in
  // effect, an assert will be triggered.
  // Note that this is a single threaded class.
   // 单实例类，在 Thread 内部作为一个友元类，辅助 Thread 用于避免在指定的作用域中同步调用 Invoke 方法
  // 如果发生了同步方法的调用，则会触发一个断言
  class ScopedDisallowBlockingCalls {
   public:
    ScopedDisallowBlockingCalls();
    ~ScopedDisallowBlockingCalls();
   private:
    Thread* const thread_;
    const bool previous_state_;
  };
  // 判断调用者所在线程是否处于本线程实例中
  bool IsCurrent() const {
    return Current() == this;
  }

  // Sleeps the calling thread for the specified number of milliseconds, during
  // which time no processing is performed. Returns false if sleeping was
  // interrupted by a signal (POSIX only).
  // 将线程睡眠指定的毫秒数，默认返回 true，除非被 POSIX 发送的信号终止，它才会返回 false
  static bool SleepMs(int millis);

  // Sets the thread's name, for debugging. Must be called before Start().
  // If |obj| is non-NULL, its value is appended to |name|.
	// 设置线程名字，它必须在 调用 Start() 之前调用，这个方法是为了方便调试时查看
  // 如果 @param obj 为空，它将追加到 @param name 后面
  const std::string& name() const { return name_; }
  bool SetName(const std::string& name, const void* obj);

  // Starts the execution of the thread.
  // 开始执行此线程，如果 runnable 不为空，则运行其内部的 thread，否则启动本线程
  // 一般传入 nullptr，即表示启动线程实例本身
  bool Start(Runnable* runnable = NULL);

  // Tells the thread to stop and waits until it is joined.
  // Never call Stop on the current thread.  Instead use the inherited Quit
  // function which will exit the base MessageQueue without terminating the
  // underlying OS thread.
	// 通知线程结束，并阻塞等待；如果调用继承的 Quit 方法，则只会结束消息队列循环，而不会结束线程
  // 注意：一定不要在线程内部调用此方法    
  virtual void Stop();

  // By default, Thread::Run() calls ProcessMessages(kForever).  To do other
  // work, override Run().  To receive and dispatch messages, call
  // ProcessMessages occasionally.
  // 默认情况下，将调用 ProcessMessages(kForever),如果要做其它工作，则需要重写 Run()
  // 如果需要接受或者处理消息，则需要自己主动调用 ProcessMessages 方法
  virtual void Run();

  // 向消息队列发消息，并阻塞等待其执行完毕
  virtual void Send(const Location& posted_from,
                    MessageHandler* phandler,
                    uint32_t id = 0,
                    MessageData* pdata = NULL);

  // Convenience method to invoke a functor on another thread.  Caller must
  // provide the |ReturnT| template argument, which cannot (easily) be deduced.
  // Uses Send() internally, which blocks the current thread until execution
  // is complete.
  // Ex: bool result = thread.Invoke<bool>(RTC_FROM_HERE,
  // &MyFunctionReturningBool);
  // NOTE: This function can only be called when synchronous calls are allowed.
  // See ScopedDisallowBlockingCalls for details. 只有在允许同步调用时才能调用此函数。
  // 在其它线程保证某方法被此线程调用，相当于消息的异步通知，即 POST
  template <class ReturnT, class FunctorT>
  ReturnT Invoke(const Location& posted_from, const FunctorT& functor) {
    FunctorMessageHandler<ReturnT, FunctorT> handler(functor);
    InvokeInternal(posted_from, &handler);
    return handler.MoveResult();
  }

  // From MessageQueue
  // 继承与 MessageQueue
  void Clear(MessageHandler* phandler,
             uint32_t id = MQID_ANY,
             MessageList* removed = NULL) override;
  void ReceiveSends() override;

  // ProcessMessages will process I/O and dispatch messages until:
  //  1) cms milliseconds have elapsed (returns true)
  //  2) Stop() is called (returns false)
  bool ProcessMessages(int cms);

  // Returns true if this is a thread that we created using the standard
  // constructor, false if it was created by a call to
  // ThreadManager::WrapCurrentThread().  The main thread of an application
  // is generally not owned, since the OS representation of the thread
  // obviously exists before we can get to it.
  // You cannot call Start on non-owned threads.
	// 判断此线程是否是我们自己通过标准的构造函数进行创建的，如果是就返回 true
  // 如果是通过 ThreadManager::WrapCurrentThread() 创建的则返回 false
  // 不能对 IsOwned 返回 false 线程对象调用 Start
  bool IsOwned();

#if defined(WEBRTC_WIN)
  HANDLE GetHandle() const {
    return thread_;
  }
  DWORD GetId() const {
    return thread_id_;
  }
#elif defined(WEBRTC_POSIX)
  pthread_t GetPThread() {
    return thread_;
  }
#endif

  // Expose private method running() for tests.
  //
  // DANGER: this is a terrible public API.  Most callers that might want to
  // call this likely do not have enough control/knowledge of the Thread in
  // question to guarantee that the returned value remains true for the duration
  // of whatever code is conditionally executing because of the return value!
  bool RunningForTest() { return running(); }

  // Sets the per-thread allow-blocking-calls flag and returns the previous
  // value. Must be called on this thread.
  bool SetAllowBlockingCalls(bool allow);

  // These functions are public to avoid injecting test hooks. Don't call them
  // outside of tests.
  // This method should be called when thread is created using non standard
  // method, like derived implementation of rtc::Thread and it can not be
  // started by calling Start(). This will set started flag to true and
  // owned to false. This must be called from the current thread.
  bool WrapCurrent();
  void UnwrapCurrent();

 protected:
  // Same as WrapCurrent except that it never fails as it does not try to
  // acquire the synchronization access of the thread. The caller should never
  // call Stop() or Join() on this thread.
  void SafeWrapCurrent();

  // Blocks the calling thread until this thread has terminated.
  void Join();

  static void AssertBlockingIsAllowedOnCurrentThread();

  friend class ScopedDisallowBlockingCalls;

 private:
#if defined(WEBRTC_WIN)
  static DWORD WINAPI PreRun(LPVOID context);
#else
  static void *PreRun(void *pv);
#endif

  // ThreadManager calls this instead WrapCurrent() because
  // ThreadManager::Instance() cannot be used while ThreadManager is
  // being created.
  // The method tries to get synchronization rights of the thread on Windows if
  // |need_synchronize_access| is true.
  bool WrapCurrentWithThreadManager(ThreadManager* thread_manager,
                                    bool need_synchronize_access);

  // Return true if the thread was started and hasn't yet stopped.
  bool running() { return running_.Wait(0); }

  // Processes received "Send" requests. If |source| is not NULL, only requests
  // from |source| are processed, otherwise, all requests are processed.
  void ReceiveSendsFromThread(const Thread* source);

  // If |source| is not NULL, pops the first "Send" message from |source| in
  // |sendlist_|, otherwise, pops the first "Send" message of |sendlist_|.
  // The caller must lock |crit_| before calling.
  // Returns true if there is such a message.
  bool PopSendMessageFromThread(const Thread* source, _SendMessage* msg);

  void InvokeInternal(const Location& posted_from, MessageHandler* handler);

  std::list<_SendMessage> sendlist_;
  std::string name_;
  Event running_;  // Signalled means running.

#if defined(WEBRTC_POSIX)
  pthread_t thread_;
#endif

#if defined(WEBRTC_WIN)
  HANDLE thread_;
  DWORD thread_id_;
#endif

  bool owned_;
  bool blocking_calls_allowed_;  // By default set to |true|.

  friend class ThreadManager;

  RTC_DISALLOW_COPY_AND_ASSIGN(Thread); // 禁用拷贝构造和操作运算符 =
};

// AutoThread automatically installs itself at construction
// uninstalls at destruction, if a Thread object is
// _not already_ associated with the current OS thread.

class AutoThread : public Thread {
 public:
  AutoThread();
  ~AutoThread() override;

 private:
  RTC_DISALLOW_COPY_AND_ASSIGN(AutoThread);
};

// Win32 extension for threads that need to use COM
#if defined(WEBRTC_WIN)
class ComThread : public Thread {
 public:
  ComThread() {}
  ~ComThread() override { Stop(); }

 protected:
  void Run() override;

 private:
  RTC_DISALLOW_COPY_AND_ASSIGN(ComThread);
};
#endif

// Provides an easy way to install/uninstall a socketserver on a thread.
class SocketServerScope {
 public:
  explicit SocketServerScope(SocketServer* ss) {
    old_ss_ = Thread::Current()->socketserver();
    Thread::Current()->set_socketserver(ss);
  }
  ~SocketServerScope() {
    Thread::Current()->set_socketserver(old_ss_);
  }

 private:
  SocketServer* old_ss_;

  RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(SocketServerScope);
};

}  // namespace rtc

#endif  // WEBRTC_BASE_THREAD_H_
