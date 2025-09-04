#pragma once

#include "base/apple/scoped_cftyperef.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "experiments/message_loop/message_pump.h"
#include <CoreFoundation/CoreFoundation.h>
#include <memory>
#include <optional>

namespace base {

class MessagePumpCFRunLoopBase : public MessagePump {
public:
    MessagePumpCFRunLoopBase(const MessagePumpCFRunLoopBase&) = delete;
    MessagePumpCFRunLoopBase& operator=(const MessagePumpCFRunLoopBase&) = delete;

    // Initializes features for this class. See `base::features::Init()`.
    static void InitializeFeatures();

    // MessagePump:
    void Run(Delegate* delegate) override;
    void Quit() override;
    void ScheduleWork() override;
    void ScheduleDelayedWork(const Delegate::NextWorkInfo& next_work_info) override;
    TimeTicks AdjustDelayedRunTime(TimeTicks earliest_time,
                                   TimeTicks run_time,
                                   TimeTicks latest_time) override;

protected:
    // Needs access to CreateAutoreleasePool.
    friend class OptionalAutoreleasePool;
    friend class TestMessagePumpCFRunLoopBase;

    // Tasks will be pumped in the run loop modes described by
    // |initial_mode_mask|, which maps bits to the index of an internal array of
    // run loop mode identifiers.
    explicit MessagePumpCFRunLoopBase(int initial_mode_mask);
    ~MessagePumpCFRunLoopBase() override;

    // Subclasses should implement the work they need to do in MessagePump::Run
    // in the DoRun method.  MessagePumpCFRunLoopBase::Run calls DoRun directly.
    // This arrangement is used because MessagePumpCFRunLoopBase needs to set
    // up and tear down things before and after the "meat" of DoRun.
    virtual void DoRun(Delegate* delegate) = 0;

    // Similar to DoRun, this allows subclasses to perform custom handling when
    // quitting a run loop. Return true if the quit took effect immediately;
    // otherwise call OnDidQuit() when the quit is actually applied (e.g., a
    // nested native runloop exited).
    virtual bool DoQuit() = 0;

    // Should be called by subclasses to signal when a deferred quit takes place.
    void OnDidQuit();

    // Accessors for private data members to be used by subclasses.
    CFRunLoopRef run_loop() const { return run_loop_.get(); }
    int nesting_level() const { return nesting_level_; }
    int run_nesting_level() const { return run_nesting_level_; }
    bool keep_running() const { return keep_running_; }

    // Sets this pump's delegate.  Signals the appropriate sources if
    // |delegateless_work_| is true.  |delegate| can be NULL.
    void SetDelegate(Delegate* delegate);

    // Return whether an autorelease pool should be created to wrap around any
    // work being performed. If false is returned to prevent an autorelease pool
    // from being created, any objects autoreleased by work will fall into the
    // current autorelease pool.
    virtual bool ShouldCreateAutoreleasePool();

    // Enable and disable entries in |enabled_modes_| to match |mode_mask|.
    void SetModeMask(int mode_mask);

    // Get the current mode mask from |enabled_modes_|.
    int GetModeMask() const;

protected:
    Delegate* delegate() { return delegate_; }

private:
    class ScopedModeEnabler;

    // The maximum number of run loop modes that can be monitored.
    static constexpr int kNumModes = 3;

    // Timer callback scheduled by ScheduleDelayedWork.  This does not do any
    // work, but it signals |work_source_| so that delayed work can be performed
    // within the appropriate priority constraints.
    static void RunDelayedWorkTimer(CFRunLoopTimerRef timer, void* info);

    // Perform highest-priority work.  This is associated with |work_source_|
    // signalled by ScheduleWork or RunDelayedWorkTimer.  The static method calls
    // the instance method; the instance method returns true if it resignalled
    // |work_source_| to be called again from the loop.
    static void RunWorkSource(void* info);
    bool RunWork();

    // Perform idle-priority work.  This is normally called by PreWaitObserver,
    // but can also be invoked from RunNestingDeferredWork when returning from a
    // nested loop.  When this function actually does perform idle work, it will
    // re-signal the |work_source_|.
    void RunIdleWork();

    // Perform work that may have been deferred because it was not runnable
    // within a nested run loop.  This is associated with
    // |nesting_deferred_work_source_| and is signalled by
    // MaybeScheduleNestingDeferredWork when returning from a nested loop,
    // so that an outer loop will be able to perform the necessary tasks if it
    // permits nestable tasks.
    static void RunNestingDeferredWorkSource(void* info);
    void RunNestingDeferredWork();

    // Called before the run loop goes to sleep to notify delegate.
    void BeforeWait();

    // Schedules possible nesting-deferred work to be processed before the run
    // loop goes to sleep, exits, or begins processing sources at the top of its
    // loop.  If this function detects that a nested loop had run since the
    // previous attempt to schedule nesting-deferred work, it will schedule a
    // call to RunNestingDeferredWorkSource.
    void MaybeScheduleNestingDeferredWork();

    // Observer callback responsible for performing idle-priority work, before
    // the run loop goes to sleep.  Associated with |pre_wait_observer_|.
    static void PreWaitObserver(CFRunLoopObserverRef observer,
                                CFRunLoopActivity activity,
                                void* info);

    static void AfterWaitObserver(CFRunLoopObserverRef observer,
                                  CFRunLoopActivity activity,
                                  void* info);

    // Observer callback called before the run loop processes any sources.
    // Associated with |pre_source_observer_|.
    static void PreSourceObserver(CFRunLoopObserverRef observer,
                                  CFRunLoopActivity activity,
                                  void* info);

    // Observer callback called when the run loop starts and stops, at the
    // beginning and end of calls to CFRunLoopRun.  This is used to maintain
    // |nesting_level_|.  Associated with |enter_exit_observer_|.
    static void EnterExitObserver(CFRunLoopObserverRef observer,
                                  CFRunLoopActivity activity,
                                  void* info);

    // Called by EnterExitObserver after performing maintenance on
    // |nesting_level_|. This allows subclasses an opportunity to perform
    // additional processing on the basis of run loops starting and stopping.
    virtual void EnterExitRunLoop(CFRunLoopActivity activity);

    // Gets rid of the top work item scope.
    void PopWorkItemScope();

    // Starts tracking a new work item.
    void PushWorkItemScope();

    // The thread's run loop.
    apple::ScopedCFTypeRef<CFRunLoopRef> run_loop_;

    // The enabled modes. Posted tasks may run in any non-null entry.
    std::unique_ptr<ScopedModeEnabler> enabled_modes_[kNumModes];

    // The timer, sources, and observers are described above alongside their
    // callbacks.
    apple::ScopedCFTypeRef<CFRunLoopTimerRef> delayed_work_timer_;
    apple::ScopedCFTypeRef<CFRunLoopSourceRef> work_source_;
    apple::ScopedCFTypeRef<CFRunLoopSourceRef> nesting_deferred_work_source_;
    apple::ScopedCFTypeRef<CFRunLoopObserverRef> pre_wait_observer_;
    apple::ScopedCFTypeRef<CFRunLoopObserverRef> after_wait_observer_;
    apple::ScopedCFTypeRef<CFRunLoopObserverRef> pre_source_observer_;
    apple::ScopedCFTypeRef<CFRunLoopObserverRef> enter_exit_observer_;

    // (weak) Delegate passed as an argument to the innermost Run call.
    Delegate* delegate_ = nullptr;

    // Time at which `delayed_work_timer_` is set to fire.
    base::TimeTicks delayed_work_scheduled_at_ = base::TimeTicks::Max();
    base::TimeDelta delayed_work_leeway_;

    // The recursion depth of the currently-executing CFRunLoopRun loop on the
    // run loop's thread.  0 if no run loops are running inside of whatever scope
    // the object was created in.
    int nesting_level_ = 0;

    // The recursion depth (calculated in the same way as |nesting_level_|) of the
    // innermost executing CFRunLoopRun loop started by a call to Run.
    int run_nesting_level_ = 0;

    // The deepest (numerically highest) recursion depth encountered since the
    // most recent attempt to run nesting-deferred work.
    int deepest_nesting_level_ = 0;

    // Whether we should continue running application tasks. Set to false when
    // Quit() is called for the innermost run loop.
    bool keep_running_ = true;

    // "Delegateless" work flags are set when work is ready to be performed but
    // must wait until a delegate is available to process it.  This can happen
    // when a MessagePumpCFRunLoopBase is instantiated and work arrives without
    // any call to Run on the stack.  The Run method will check for delegateless
    // work on entry and redispatch it as needed once a delegate is available.
    bool delegateless_work_ = false;

    // Used to keep track of the native event work items processed by the message
    // pump. Made of optionals because tracking can be suspended when it's
    // determined the loop is not processing a native event but the depth of the
    // stack should match |nesting_level_| at all times. A nullopt is also used
    // as a stand-in during delegateless operation.
    base::stack<std::optional<base::MessagePump::Delegate::ScopedDoWorkItem>> stack_;
};

class MessagePumpCFRunLoop : public MessagePumpCFRunLoopBase {
public:
    MessagePumpCFRunLoop();

    MessagePumpCFRunLoop(const MessagePumpCFRunLoop&) = delete;
    MessagePumpCFRunLoop& operator=(const MessagePumpCFRunLoop&) = delete;

    ~MessagePumpCFRunLoop() override;

    void DoRun(Delegate* delegate) override;
    bool DoQuit() override;

private:
    void EnterExitRunLoop(CFRunLoopActivity activity) override;

    // True if Quit is called to stop the innermost MessagePump
    // (|innermost_quittable_|) but some other CFRunLoopRun loop
    // (|nesting_level_|) is running inside the MessagePump's innermost Run call.
    bool quit_pending_;
};

class MessagePumpNSApplication : public MessagePumpCFRunLoopBase {
public:
    MessagePumpNSApplication();

    MessagePumpNSApplication(const MessagePumpNSApplication&) = delete;
    MessagePumpNSApplication& operator=(const MessagePumpNSApplication&) = delete;

    ~MessagePumpNSApplication() override;

    void DoRun(Delegate* delegate) override;
    bool DoQuit() override;

private:
    // A source that doesn't do anything but provide something signalable
    // attached to the run loop.  This source will be signalled when Quit
    // is called, to cause the loop to wake up so that it can stop.
    apple::ScopedCFTypeRef<CFRunLoopSourceRef> quit_source_;
};

// While in scope, permits posted tasks to be run in private AppKit run loop
// modes that would otherwise make the UI unresponsive. E.g., menu fade out.
class ScopedPumpMessagesInPrivateModes {
public:
    ScopedPumpMessagesInPrivateModes();

    ScopedPumpMessagesInPrivateModes(const ScopedPumpMessagesInPrivateModes&) = delete;
    ScopedPumpMessagesInPrivateModes& operator=(const ScopedPumpMessagesInPrivateModes&) = delete;

    ~ScopedPumpMessagesInPrivateModes();

    int GetModeMaskForTest();
};

class MessagePumpNSApplication : public MessagePumpCFRunLoopBase {
public:
    MessagePumpNSApplication();

    MessagePumpNSApplication(const MessagePumpNSApplication&) = delete;
    MessagePumpNSApplication& operator=(const MessagePumpNSApplication&) = delete;

    ~MessagePumpNSApplication() override;

    void DoRun(Delegate* delegate) override;
    bool DoQuit() override;

private:
    friend class ScopedPumpMessagesInPrivateModes;

    void EnterExitRunLoop(CFRunLoopActivity activity) override;

    // True if DoRun is managing its own run loop as opposed to letting
    // -[NSApplication run] handle it.  The outermost run loop in the application
    // is managed by -[NSApplication run], inner run loops are handled by a loop
    // in DoRun.
    bool running_own_loop_ = false;

    // True if Quit() was called while a modal window was shown and needed to be
    // deferred.
    bool quit_pending_ = false;
};

}  // namespace base
