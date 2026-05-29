/** @file
 *
 * UEFI Protocol interface for SMP scheduler services in UEFI
 *
 * Copyright (c) 2020 The Linux Foundation. All rights reserved.
 *
 * Portions Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
**/

/*=============================================================================
                              EDIT HISTORY

 when       who     what, where, why
 --------   ---     -----------------------------------------------------------
 10/22/19   md      Added API's to support unsafe stack.
 01/15/19   yg      Add Thread/CPU stat API's
 09/15/18   yg      Add WDog interface
 02/21/18   yg      Updated to latest interface
 06/14/17   yg      Initial version

=============================================================================*/

#include <Uefi.h>

#ifndef ___EFI_KERNEL_PROTOCOL_H__
#define ___EFI_KERNEL_PROTOCOL_H__

/* External global reference to the Kernel Protocol GUID */
extern EFI_GUID gEfiKernelProtocolGuid;

/* Protocol GUID definition. { B5062BE7-170B-4A32-BE21-689262FF4399 } */
#define EFI_KERNEL_PROTOCOL_GUID \
    { 0xB5062BE7, 0x170B, 0x4A32, { 0xBE, 0x21, 0x68, 0x92, 0x62, 0xFF, 0x43, \
      0x99 } }

/*
 *   Document the version changes here
 *   V 1.0
 *      Initial version
 *
 *   V 1.1
 *      Added API ConfigureInterrupt
 *
 *   V 1.2
 *      Removed unsupported thread interface API's
 *      Added Power transition notification registration API's
 *
 *   V 1.3
 *      Added Watchdog interface
 *
 *   V 1.4
 *      Added Thread level stats API, renamed the ThreadStats to CPU Stats to
 *      correct the reference. Retained old API for backward compatibility
 *      Added Lib version member.
 *
 *   V 1.5
 *      Add Simple lock API's with recursive lock support
 *
 *   V 1.6
 *      Add API's to support unsafe stack.
 *
 * */
#define EFI_KERNEL_PROTOCOL_VER_INTR_CONFIG        0x00010001
#define EFI_KERNEL_PROTOCOL_VER_PWR_NOTIFY         0x00010002
#define EFI_KERNEL_PROTOCOL_VER_WDOG_INTF          0x00010003
#define EFI_KERNEL_PROTOCOL_VER_THR_CPU_STATS      0x00010004
#define EFI_KERNEL_PROTOCOL_VER_LIB_VER_API        0x00010004
#define EFI_KERNEL_PROTOCOL_VER_LOCK_API           0x00010005
#define EFI_KERNEL_PROTOCOL_VER_UNSAFE_STACK_APIS  0x00010006

/* Current protocol version */
#define EFI_KERNEL_PROTOCOL_VERSION    EFI_KERNEL_PROTOCOL_VER_UNSAFE_STACK_APIS

/*******************************************************************************
 *
 *   Interface type definitions
 *
 ******************************************************************************/

typedef INT32 KStatus;

#define NO_ERROR                (0)
#define ERR_GENERIC             (-1)
#define ERR_NOT_FOUND           (-2)
#define ERR_NOT_READY           (-3)
#define ERR_NO_MSG              (-4)
#define ERR_NO_MEMORY           (-5)
#define ERR_ALREADY_STARTED     (-6)
#define ERR_NOT_VALID           (-7)
#define ERR_INVALID_ARGS        (-8)
#define ERR_NOT_ENOUGH_BUFFER   (-9)
#define ERR_NOT_SUSPENDED       (-10)
#define ERR_OBJECT_DESTROYED    (-11)
#define ERR_NOT_BLOCKED         (-12)
#define ERR_TIMED_OUT           (-13)
#define ERR_ALREADY_EXISTS      (-14)
#define ERR_CHANNEL_CLOSED      (-15)
#define ERR_OFFLINE             (-16)
#define ERR_NOT_ALLOWED         (-17)
#define ERR_BAD_PATH            (-18)
#define ERR_ALREADY_MOUNTED     (-19)
#define ERR_IO                  (-20)
#define ERR_NOT_DIR             (-21)
#define ERR_NOT_FILE            (-22)
#define ERR_RECURSE_TOO_DEEP    (-23)
#define ERR_NOT_SUPPORTED       (-24)
#define ERR_TOO_BIG             (-25)
#define ERR_CANCELLED           (-26)
#define ERR_NOT_IMPLEMENTED     (-27)
#define ERR_CHECKSUM_FAIL       (-28)
#define ERR_CRC_FAIL            (-29)
#define ERR_CMD_UNKNOWN         (-30)
#define ERR_BAD_STATE           (-31)
#define ERR_BAD_LEN             (-32)
#define ERR_BUSY                (-33)
#define ERR_THREAD_DETACHED     (-34)
#define ERR_I2C_NACK            (-35)
#define ERR_ALREADY_EXPIRED     (-36)
#define ERR_OUT_OF_RANGE        (-37)
#define ERR_NOT_CONFIGURED      (-38)
#define ERR_NOT_MOUNTED         (-39)
#define ERR_FAULT               (-40)
#define ERR_NO_RESOURCES        (-41)
#define ERR_BAD_HANDLE          (-42)
#define ERR_ACCESS_DENIED       (-43)
#define ERR_PARTIAL_WRITE       (-44)

typedef struct
{
    UINT32 PreemptCount;
    UINT32 BlockedCount;
    UINT64 PreemptDuration;
    UINT64 BlockedDuration;
    UINT64 ActiveDuration;
    UINT64 SleepDuration;
}ThreadStats;

typedef struct
{
    UINT64 IdleTime;
    UINT64 LastIdleTimestamp;
    UINT64 Reschedules;
    UINT64 ContextSwitches;
    UINT64 Preempts;
    UINT64 Yields;
    UINT64 Interrupts;    /* platform code increment this */
    UINT64 TimerInts;     /* timer code increment this */
    UINT64 Timers;        /* timer code increment this */
    UINT64 RescheduleIpis;
}CpuSchedStats;

enum PwrTxnType{
    DevicePwrNone    = 0,     // None
    DevicePwrOFF     = 1,     // Turn the device power OFF
    DevicePwrON      = 2,     // Turn the device power ON
};

typedef VOID (*UnsafeStackCb) (void* Arg);

typedef struct {
   VOID          *unsafe_sp_base; 
   UINT64        unsafe_stack_size;
   UnsafeStackCb cb;
   VOID          *unsafe_stack_cb_data;
   VOID          *unsafe_stack_top;
}ThrUnsafeStackIntf;
/******************************************************************************
 *   Interrupt handler interface
 *
 *   Note that the interrupt handlers registered should not conflict with
 *   scheduler registered interrupts for PPI, SGI and SPI interrupt vectors
 *
 *   Handlers execute in ISR context, so only events can be posted with some
 *   restrictions.
 *
 ******************************************************************************/
enum HandlerStatus
{
  HANDLER_NO_RESCHEDULE = 0,
  HANDLER_RESCHEDULE
};

enum IntrConfig
{
   INTR_CONFIG_NONE             = 0,
   INTR_CONFIG_EDGE_TRIGGER     = 1,
   INTR_CONFIG_MAX              = 2,
};

typedef enum HandlerStatus (*IntrHandler)(VOID *Arg);

typedef VOID (*REGISTER_INTR_HANDLER) (UINT32 Vector, IntrHandler Handler,
    VOID *Arg);

typedef KStatus (*MASK_INTERRUPT) (UINT32 Vector);
typedef KStatus (*UNMASK_INTERRUPT) (UINT32 Vector);
typedef KStatus (*CONFIGURE_INTERRUPT) (UINT32 Vector,
    enum IntrConfig Config, VOID* Arg);

typedef struct {
  REGISTER_INTR_HANDLER    RegisterIntrHandler;
  MASK_INTERRUPT           MaskInterrupt;
  UNMASK_INTERRUPT         UnmaskInterrupt;
  CONFIGURE_INTERRUPT      ConfigureInterrupt;
} InterruptIntf;

/******************************************************************************
 *
 *   Scheduler supported Event Services
 *
 ******************************************************************************/

/* Rules for Events:
 * - Events may be signaled from interrupt context *but* the reschedule
 *     parameter must be false in that case.
 * - Events may not be waited upon from interrupt context.
 * - Events *without* FLAG_AUTOUNSIGNAL:
 *   - Wake up any waiting threads when signaled.
 *   - Continue to do so (no threads will wait) until unsignaled.
 * - Events *with* FLAG_AUTOUNSIGNAL:
 *   - If one or more threads are waiting when signaled, one thread will
 *     be woken up and return.  The signaled state will not be set.
 *   - If no threads are waiting when signaled, the Event will remain
 *     in the signaled state until a thread attempts to wait (at which
 *     time it will unsignal atomicly and return immediately) or
 *     event_unsignal() is called.
*/

#define SCHED_EVENT_FLAG_AUTOUNSIGNAL             1

#define SCHED_EVENT_STATE_UNSIGNALED              0
#define SCHED_EVENT_STATE_SIGNALED                1

struct event;
typedef struct event Event;

typedef UINT32 TimeDuration;

#ifndef INFINITE_TIME
#define INFINITE_TIME              0xFFFFFFFF
#endif

typedef Event* (*EVENT_INIT)(UINT64 Id OPTIONAL, _Bool Initial, UINT32 Flags);
typedef VOID (*EVENT_DESTROY)(Event *);

typedef _Bool (*EVENT_INITIALIZED)(Event *Evt);

typedef KStatus (*EVENT_WAIT)(Event *Evt);
typedef KStatus (*EVENT_WAIT_TIMEOUT)(Event *, TimeDuration);

typedef KStatus (*EVENT_SIGNAL)(Event *, _Bool Reschedule);
typedef KStatus (*EVENT_UNSIGNAL)(Event *Evt);

typedef _Bool (*EVENT_GET_SIGNAL_STATE) (Event *Evt);

typedef struct {
  EVENT_INIT              EventInit;
  EVENT_DESTROY           EventDestroy;
  EVENT_INITIALIZED       EventInitialized;
  EVENT_WAIT              EventWait;
  EVENT_WAIT_TIMEOUT      EventWaitTimeout;
  EVENT_SIGNAL            EventSignal;
  EVENT_UNSIGNAL          EventUnsignal;
  EVENT_GET_SIGNAL_STATE  EventGetSignalState;
}EventIntf;

/******************************************************************************
     Mutex
*******************************************************************************/
/* Rules for Mutexes:
 * - Mutexes are only safe to use from thread context.
 * - Mutexes are non-recursive.
*/
struct mutex;
typedef struct mutex Mutex;

typedef Mutex* (*MUTEX_INIT)(UINT64 Id OPTIONAL);
typedef VOID (*MUTEX_DESTROY)(Mutex *Mtx);

typedef KStatus (*MUTEX_ACQUIRE)(Mutex *Mtx);
/* try to acquire the mutex with a timeout value */
typedef KStatus (*MUTEX_ACQUIRE_TIMEOUT)(Mutex *, TimeDuration);

typedef KStatus (*MUTEX_RELEASE)(Mutex *Mtx);

/* does the current thread hold the mutex? */
typedef _Bool (*IS_MUTEX_HELD)(Mutex *Mtx);

typedef struct {
  MUTEX_INIT               MutexInit;
  MUTEX_DESTROY            MutexDestroy;
  MUTEX_ACQUIRE            MutexAcquire;
  MUTEX_ACQUIRE_TIMEOUT    MutexAcquireTimeout;
  MUTEX_RELEASE            MutexRelease;
  IS_MUTEX_HELD            IsMutexHeld;

}MutexIntf;

/******************************************************************************
     Semaphore
******************************************************************************/
struct semaphore;
typedef struct semaphore Semaphore;

typedef Semaphore* (*SEM_INIT)(UINT64 Id OPTIONAL, UINT32 Cnt);
typedef VOID (*SEM_DESTROY)(Semaphore *);

typedef INT32 (*SEM_POST)(Semaphore *, _Bool Resched);
typedef KStatus (*SEM_WAIT)(Semaphore *);
typedef KStatus (*SEM_TRYWAIT)(Semaphore *);
typedef KStatus (*SEM_TIMEDWAIT)(Semaphore *, TimeDuration);

typedef struct {
  SEM_INIT             SemInit;
  SEM_DESTROY          SemDestroy;
  SEM_POST             SemPost;
  SEM_WAIT             SemWait;
  SEM_TRYWAIT          SemTryWait;
  SEM_TIMEDWAIT        SemTimedWait;

}SemIntf;

/******************************************************************************
     Spinlock
******************************************************************************/

struct spinlock;
typedef struct spinlock Spinlock;

typedef Spinlock* (*SPINLOCK_INIT)(UINT64 Id OPTIONAL);
typedef _Bool (*SPINLOCK_HELD)(Spinlock*);
typedef INTN (*SPINLOCK_TRY_LOCK)(Spinlock*);
typedef VOID (*SPINLOCK_LOCK)(Spinlock*);
typedef VOID (*SPINLOCK_UNLOCK)(Spinlock*);

typedef struct {
  SPINLOCK_INIT          SpinLockInit;
  SPINLOCK_HELD          SpinLockHeld;
  SPINLOCK_TRY_LOCK      SpinTryLock;
  SPINLOCK_LOCK          SpinLock;
  SPINLOCK_UNLOCK        SpinUnlock;

}SpinlockIntf;

/******************************************************************************
     Thread
******************************************************************************/

/* Thread priority */
#define NUM_PRIORITIES                  32

#define LOWEST_PRIORITY                 0
#define HIGHEST_PRIORITY                (NUM_PRIORITIES - 1)

#define IDLE_PRIORITY                   LOWEST_PRIORITY

#define DPC_PRIORITY                    (NUM_PRIORITIES - 2)

#define LOW_PRIORITY                    (NUM_PRIORITIES / 4)

#define DEFAULT_PRIORITY                (NUM_PRIORITIES / 2)

#define HIGH_PRIORITY                   ((NUM_PRIORITIES / 4) * 3)

#define UEFI_THREAD_PRIORITY            HIGH_PRIORITY

/* stack size */
#ifdef CUSTOM_DEFAULT_STACK_SIZE
#define DEFAULT_STACK_SIZE              CUSTOM_DEFAULT_STACK_SIZE
#else
#define DEFAULT_STACK_SIZE              0x1000
#endif

struct thread;
typedef struct thread Thread;

typedef INT32 (*ThreadStartRoutine) (VOID *Arg);

typedef Thread* (*THREAD_CREATE)(const char *Name,
    ThreadStartRoutine EntryPoint, VOID *Arg, INT32 Priority, UINTN Stack_size);
typedef KStatus (*THREAD_RESUME)(Thread *);
typedef VOID (*THREAD_EXIT)(INT32 RetCode);

/* the current thread */
typedef Thread* (*GET_CURRENT_THREAD)(VOID);

typedef INT32 (*THREAD_GET_CURR_CPU)(Thread*);
typedef INT32 (*THREAD_GET_PINNED_CPU)(Thread*);
typedef VOID (*THREAD_SET_PINNED_CPU)(Thread*, INT32 Cpu);
typedef VOID (*THREAD_SET_NAME)(const char *Name);
typedef VOID (*THREAD_SET_PRIORITY)(INT32 Priority);

typedef VOID (*THREAD_SLEEP)(TimeDuration Delay);

typedef KStatus (*THREAD_DETACH)(Thread *);
typedef KStatus (*THREAD_JOIN)(Thread *, INT32 *RetCode,
    TimeDuration Timeout);
typedef KStatus (*THREAD_DETACH_AND_RESUME)(Thread *);

typedef ThreadStats* (*THREAD_GET_STATS) (Thread *, ThreadStats* Tsp);
typedef UINT64 (*THREAD_GET_TIMESTAMP) (VOID);

/* Get thread unsafe stack pointer current */
typedef VOID** (*THREAD_GET_UNSAFE_SP_CURRENT) (Thread *);

/* Get thread unsafe stack base */
typedef VOID* (*THREAD_GET_UNSAFE_SP_BASE) (Thread *Thr);

/* Returns size of padding that will be used in unsafe stack for bound check */
typedef UINT64 (*THREAD_GET_UNSAFE_SP_PADDING_SIZE) (VOID);

/* set thread unsafe stack pointer */
/* For bound check, allocate unsafe stack of size =  required stack size +
                                                     Padding size
   Get Padding size by calling ThreadGetUnsafeStackPaddingSize ()
   In Size parameter pass allocated unsafe stack size  */

/* NOTE: Client should pass the memory for unsafe stack and it is the
   responsibility of client to free up that space. */
typedef KStatus (*THREAD_SET_THREAD_UNSAFE_SP) (Thread *,
    ThrUnsafeStackIntf *);

typedef struct {
  THREAD_CREATE                ThreadCreate;
  THREAD_RESUME                ThreadResume;
  THREAD_EXIT                  ThreadExit;
  GET_CURRENT_THREAD           GetCurrentThread;
  THREAD_GET_CURR_CPU          ThreadGetCurrCpu;
  THREAD_GET_PINNED_CPU        ThreadGetPinnedCpu;
  VOID*                        Reserved3;      // Deprecated API filler
  THREAD_SET_PINNED_CPU        ThreadSetPinnedCpu;
  THREAD_SET_NAME              ThreadSetName;
  THREAD_SET_PRIORITY          ThreadSetPriority;
  THREAD_SLEEP                 ThreadSleep;
  THREAD_DETACH                ThreadDetach;
  THREAD_JOIN                  ThreadJoin;
  THREAD_DETACH_AND_RESUME     ThreadDetachAndResume;
  VOID*                        Reserved0;   // Deprecated API filler
  VOID*                        Reserved1;   // Deprecated API filler
  VOID*                        Reserved2;   // Deprecated API filler
  VOID*                        Reserved4;   // Deprecated API filler
  THREAD_GET_TIMESTAMP         ThreadGetTimeStamp;
  THREAD_GET_STATS             ThreadGetStats;
  THREAD_GET_UNSAFE_SP_CURRENT ThreadGetUnsafeSPCurrent;
  THREAD_GET_UNSAFE_SP_BASE    ThreadGetUnsafeSPBase;
  THREAD_SET_THREAD_UNSAFE_SP  ThreadSetUnsafeSP;
  THREAD_GET_UNSAFE_SP_PADDING_SIZE ThreadGetUnsafeStackPaddingSize;

}ThreadIntf;

/******************************************************************************
     Timer
******************************************************************************/
/* Rules for Timers:
 * - Timer callbacks occur from interrupt context
 * - Timers may be programmed or canceled from interrupt or thread context
 * - Timers may be canceled or reprogrammed from within their callback
 * - Timers currently are dispatched from a 10ms periodic tick
*/

struct timer;
typedef struct timer Timer;

typedef enum HandlerStatus (*TimerCallback) (Timer *Tmr, TimeDuration Now,
    VOID *Arg);

typedef Timer* (*TIMER_INIT)(VOID);
typedef VOID (*TIMER_SET_ONESHOT)(Timer *, TimeDuration Delay,
    TimerCallback Cb, VOID *arg);
typedef VOID (*TIMER_SET_PERIODIC)(Timer *, TimeDuration Period,
    TimerCallback Cb, VOID *arg);
typedef VOID (*TIMER_CANCEL)(Timer *);


typedef struct {
  TIMER_INIT            TimerInit;
  TIMER_SET_ONESHOT     TimerSetOneshot;
  TIMER_SET_PERIODIC    TimerSetPeriodic;
  TIMER_CANCEL          CancelTimer;

}TimerIntf;


/******************************************************************************
     MP Cpu
******************************************************************************/

/*
 *  Active CPU :
 *  Busy CPU :
 *  Idle CPU :
 *
 * */
typedef UINT32 (*MPCORE_GET_MAX_CPU_COUNT) (VOID);
typedef UINT32 (*MPCORE_GET_AVAIL_CPU_COUNT) (VOID);
typedef UINT32 (*MPCORE_INIT_DEFERRED_CORES) (UINT32 Cpu_Mask);

typedef UINT32 (*MPCORE_GET_CURR_CPU) (VOID);

typedef UINT32 (*MPCORE_IS_CPU_ACTIVE) (UINT32 Cpu);

typedef UINT32 (*MPCORE_GET_ACTIVE_MASK) (VOID);
typedef UINT32 (*MPCORE_GET_ONLINE_MASK) (VOID);

typedef VOID (*MPCORE_SHUTDOWN_SCHEDULER) (VOID);
typedef VOID (*MPCORE_POWER_OFF_CPU) (UINT32 CpuMask);
typedef VOID (*MPCORE_POWER_ON_CPU) (UINT32 CpuMask);

typedef INT32 (*MPCORE_SLEEP_CPU) (UINT64 DurationMs);

typedef CpuSchedStats* (*MPCORE_GET_CPU_SCHED_STATS) (UINT32 Cpu,
    CpuSchedStats* Csp);

/* Should be able to receive the power transition notification on any of the
 * cores
 * */
typedef void (*PwrTxnNotifyFn) (enum PwrTxnType Evt, VOID* Arg);

typedef EFI_STATUS (*REGISTER_PWR_TRANSITION_NOTIFY) (PwrTxnNotifyFn CbFn,
    VOID* Arg);
typedef EFI_STATUS (*UNREGISTER_PWR_TRANSITION_NOTIFY)
    (PwrTxnNotifyFn CbFn);

typedef struct {
    MPCORE_GET_MAX_CPU_COUNT        MpcoreGetMaxCpuCount;

    MPCORE_GET_AVAIL_CPU_COUNT      MpcoreGetAvailCpuCount;

    MPCORE_INIT_DEFERRED_CORES      MpcoreInitDeferredCores;

    MPCORE_GET_CURR_CPU             MpcoreGetCurrCpu;

    MPCORE_IS_CPU_ACTIVE            MpcoreIsCpuActive;

    MPCORE_GET_ACTIVE_MASK          MpcoreGetActiveMask;
    MPCORE_GET_ONLINE_MASK          MpcoreGetOnlineMask;

    MPCORE_SHUTDOWN_SCHEDULER       MpcoreShutdownScheduler;
    MPCORE_POWER_OFF_CPU            MpcorePowerOffCpu;
    MPCORE_POWER_ON_CPU             MpcorePowerOnCpu;

    /* Sleep related API's
     * NOTE: Only supported in Non retail test configuration */
    MPCORE_SLEEP_CPU                MpcoreSleepCpu;

    REGISTER_PWR_TRANSITION_NOTIFY     RegisterPwrTransitionNotify;
    UNREGISTER_PWR_TRANSITION_NOTIFY   UnRegisterPwrTransitionNotify;

    MPCORE_GET_CPU_SCHED_STATS      MpcoreGetCpuSchedStats;

}MpCpuIntf;

/******************************************************************************
     Watch Dog
******************************************************************************/

typedef EFI_STATUS (*WDOG_ENABLE) (VOID);
typedef VOID       (*WDOG_DISABLE) (VOID);
typedef EFI_STATUS (*WDOG_SET_BITE_TIMEOUT) (UINT32 Timeoutms);
typedef VOID       (*WDOG_FORCE_PET) (VOID);
typedef VOID       (*WDOG_FORCE_BITE) (VOID);
typedef EFI_STATUS (*WDOG_SET_PET_TIMER_PERIOD) (UINT32 Timeoutms);

typedef struct {
    WDOG_ENABLE               WdogEnable;
    WDOG_DISABLE              WdogDisable;
    WDOG_SET_BITE_TIMEOUT     WdogSetBiteTimeout;
    WDOG_FORCE_PET            WdogForcePet;
    WDOG_FORCE_BITE           WdogFirceBite;
    WDOG_SET_PET_TIMER_PERIOD WdogSetPetTimerPeriod;
}WDogIntf;


/******************************************************************************
      Simple and Recursible Locks
******************************************************************************/
struct LockType;
typedef struct LockType LockHandle;

/* Creates a lock instance. If the LockStrID has a valid string name, then
 * the named lock is created if it already doesn't exist, if the named lock
 * already exists, the lock created points to the same underlying instance.
 * The Lock String name should be of length 8 chars or less, if its above 8
 * it results into error to avoid ambiguity since underlying mutex ID len is
 * same 8 bytes and it could be pointing to the same instance which may not
 * be the intended
 *
 * The option can be provided to get the lock that is recursive (ie the same
 * thread can lock it many times and has to unlock the same number of times) */
typedef EFI_STATUS  (*INIT_LOCK) (IN  CONST CHAR8 *LockStrID   OPTIONAL,
                                  OUT LockHandle** LockHandlePtr);

typedef EFI_STATUS  (*INIT_RECURSIVE_LOCK)
    (IN  CONST CHAR8 *LockStrID   OPTIONAL, OUT LockHandle** LockHandlePtr);

typedef VOID        (*DESTROY_LOCK) (LockHandle*);
typedef VOID        (*ACQUIRE_LOCK) (LockHandle*);
typedef VOID        (*RELEASE_LOCK) (LockHandle*);

typedef struct {
    INIT_LOCK               InitLock;
    INIT_RECURSIVE_LOCK     InitRecursiveLock;
    DESTROY_LOCK            DestroyLock;
    ACQUIRE_LOCK            AcquireLock;
    RELEASE_LOCK            ReleaseLock;
}LockIntf;

/*===========================================================================
                    PROTOCOL INTERFACE
===========================================================================*/

typedef UINT32 (*GET_LIB_VERSION) (VOID);

/*
 *  Scheduler protocol interface in UEFI. Provides the standard scheduler
 *  services for driver/applications consumption.
 */
typedef struct {
  UINT64                         Version;
  InterruptIntf                  *Interrupt;
  TimerIntf                      *Timer;
  ThreadIntf                     *Thread;
  EventIntf                      *Event;
  MutexIntf                      *Mutex;
  SemIntf                        *Sem;
  SpinlockIntf                   *Spinlock;
  MpCpuIntf                      *MpCpu;
  WDogIntf                       *WDog;
  GET_LIB_VERSION                 GetLibVersion; // Major [31:16], Minor {15:0]
  LockIntf                       *Lock;
}EFI_KERNEL_PROTOCOL;

extern EFI_KERNEL_PROTOCOL* gKernel;

#endif  /* ___EFI_KERNEL_PROTOCOL_H__ */

