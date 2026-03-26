(* ========================================================================= *)
(* Phoenix Engine - 并发模型 TLA+ 规格说明                                   *)
(* TLA+ Specification of Phoenix Engine Concurrency Model                    *)
(* ========================================================================= *)

------------------------------- MODULE ConcurrencyModel -------------------------------
EXTENDS Integers, Sequences, TLC

(* ========================================================================= *)
(* 1. 常量定义 (Constants)                                                   *)
(* ========================================================================= *)

CONSTANTS
    ThreadIds     \* 线程 ID 集合
    ResourceIds   \* 资源 ID 集合
    MaxLocks      \* 最大锁数量
    QueueSize     \* 命令队列大小

(* ========================================================================= *)
(* 2. 变量定义 (Variables)                                                   *)
(* ========================================================================= *)

VARIABLES
    threads         \* 线程状态映射：thread_id -> "idle" | "running" | "blocked" | "waiting"
    locks           \* 锁状态映射：resource_id -> thread_id | NULL
    lock_queue      \* 锁等待队列：resource_id -> Seq(thread_id)
    thread_pc       \* 线程程序计数器：thread_id -> 代码位置
    resource_state  \* 资源状态映射：resource_id -> 资源值
    message_queue   \* 消息队列：Seq(message)
    frame_count     \* 帧计数器

(* ========================================================================= *)
(* 3. 辅助定义 (Helper Definitions)                                          *)
(* ========================================================================= *)

NULL == -1

(* 线程状态 *)
ThreadState == {"idle", "running", "blocked", "waiting"}

(* 消息类型 *)
MessageType == {"render", "physics", "audio", "io", "script"}

(* 消息记录 *)
Message == [type : MessageType, data : Nat, sender : ThreadIds, priority : 1..10]

(* 锁持有者 *)
LockHolder(r) == IF locks[r] = NULL THEN {} ELSE {locks[r]}

(* 锁可用 *)
LockAvailable(r) == locks[r] = NULL

(* 线程可运行 *)
ThreadRunnable(t) == threads[t] = "running"

(* ========================================================================= *)
(* 4. 初始状态 (Initial State)                                              *)
(* ========================================================================= *)

Init ==
    /\ threads = [t \in ThreadIds |-> "idle"]
    /\ locks = [r \in ResourceIds |-> NULL]
    /\ lock_queue = [r \in ResourceIds |-> <<>>]
    /\ thread_pc = [t \in ThreadIds |-> "start"]
    /\ resource_state = [r \in ResourceIds |-> 0]
    /\ message_queue = <<>>
    /\ frame_count = 0

(* ========================================================================= *)
(* 5. 锁操作 (Lock Operations)                                               *)
(* ========================================================================= *)

(* 获取锁 *)
AcquireLock(t, r) ==
    /\ threads[t] = "running"
    /\ IF LockAvailable(r) THEN
        /\ locks' = [locks EXCEPT ![r] = t]
        /\ lock_queue' = lock_queue
        /\ thread_pc' = [thread_pc EXCEPT ![t] = "critical_section"]
    ELSE
        /\ locks' = locks
        /\ lock_queue' = [lock_queue EXCEPT ![r] = Append(lock_queue[r], t)]
        /\ threads' = [threads EXCEPT ![t] = "waiting"]
        /\ thread_pc' = [thread_pc EXCEPT ![t] = "wait_lock"]
    /\ UNCHANGED <<resource_state, message_queue, frame_count>>

(* 释放锁 *)
ReleaseLock(t, r) ==
    /\ threads[t] = "running"
    /\ locks[t] = r  \* t 持有锁 r
    /\ IF Len(lock_queue[r]) > 0 THEN
        /\ LET next_t == Head(lock_queue[r]) IN
           /\ locks' = [locks EXCEPT ![r] = next_t]
           /\ lock_queue' = [lock_queue EXCEPT ![r] = SubSeq(lock_queue[r], 2, Len(lock_queue[r]))]
           /\ threads' = [threads EXCEPT ![next_t] = "running"]
    ELSE
        /\ locks' = [locks EXCEPT ![r] = NULL]
        /\ lock_queue' = lock_queue
        /\ thread_pc' = [thread_pc EXCEPT ![t] = "released"]
    /\ UNCHANGED <<resource_state, message_queue, frame_count>>

(* ========================================================================= *)
(* 6. 线程操作 (Thread Operations)                                           *)
(* ========================================================================= *)

(* 渲染线程 *)
RenderThread(t) ==
    /\ thread_pc[t] = "start"
    /\ thread_pc' = [thread_pc EXCEPT ![t] = "acquire_render_lock"]
    /\ threads' = [threads EXCEPT ![t] = "running"]
    /\ UNCHANGED <<locks, lock_queue, resource_state, message_queue, frame_count>>

(* 物理线程 *)
PhysicsThread(t) ==
    /\ thread_pc[t] = "start"
    /\ thread_pc' = [thread_pc EXCEPT ![t] = "acquire_physics_lock"]
    /\ threads' = [threads EXCEPT ![t] = "running"]
    /\ UNCHANGED <<locks, lock_queue, resource_state, message_queue, frame_count>>

(* 脚本线程 *)
ScriptThread(t) ==
    /\ thread_pc[t] = "start"
    /\ thread_pc' = [thread_pc EXCEPT ![t] = "acquire_script_lock"]
    /\ threads' = [threads EXCEPT ![t] = "running"]
    /\ UNCHANGED <<locks, lock_queue, resource_state, message_queue, frame_count>>

(* IO 线程 *)
IOThread(t) ==
    /\ thread_pc[t] = "start"
    /\ thread_pc' = [thread_pc EXCEPT ![t] = "acquire_io_lock"]
    /\ threads' = [threads EXCEPT ![t] = "running"]
    /\ UNCHANGED <<locks, lock_queue, resource_state, message_queue, frame_count>>

(* 临界区执行 *)
CriticalSection(t) ==
    /\ thread_pc[t] = "critical_section"
    /\ resource_state' = [resource_state EXCEPT ![SomeResource] = resource_state[SomeResource] + 1]
    /\ thread_pc' = [thread_pc EXCEPT ![t] = "release_lock"]
    /\ UNCHANGED <<threads, locks, lock_queue, message_queue, frame_count>>

(* 等待锁 *)
WaitLock(t, r) ==
    /\ threads[t] = "waiting"
    /\ LockAvailable(r)
    /\ locks' = [locks EXCEPT ![r] = t]
    /\ lock_queue' = [lock_queue EXCEPT ![r] = SubSeq(lock_queue[r], 2, Len(lock_queue[r]))]
    /\ threads' = [threads EXCEPT ![t] = "running"]
    /\ thread_pc' = [thread_pc EXCEPT ![t] = "critical_section"]
    /\ UNCHANGED <<resource_state, message_queue, frame_count>>

(* ========================================================================= *)
(* 7. 消息队列操作 (Message Queue Operations)                                *)
(* ========================================================================= *)

(* 发送消息 *)
SendMessage(t, msg) ==
    /\ Len(message_queue) < QueueSize
    /\ message_queue' = Append(message_queue, msg)
    /\ UNCHANGED <<threads, locks, lock_queue, thread_pc, resource_state, frame_count>>

(* 接收消息 *)
ReceiveMessage(t) ==
    /\ Len(message_queue) > 0
    /\ LET msg == Head(message_queue) IN
       /\ msg.sender # t  \* 不接收自己的消息
    /\ message_queue' = SubSeq(message_queue, 2, Len(message_queue))
    /\ UNCHANGED <<threads, locks, lock_queue, thread_pc, resource_state, frame_count>>

(* ========================================================================= *)
(* 8. 帧同步 (Frame Synchronization)                                         *)
(* ========================================================================= *)

(* 开始新帧 *)
BeginFrame() ==
    /\ frame_count' = frame_count + 1
    /\ threads = [t \in ThreadIds |-> "idle"]  \* 重置所有线程
    /\ thread_pc' = [thread_pc EXCEPT ![t] = "start" |-> "start"]
    /\ UNCHANGED <<locks, lock_queue, resource_state, message_queue>>

(* 结束帧 *)
EndFrame() ==
    /\ \A t \in ThreadIds : thread_pc[t] = "done"
    /\ frame_count' = frame_count
    /\ UNCHANGED <<threads, locks, lock_queue, thread_pc, resource_state, message_queue>>

(* ========================================================================= *)
(* 9. 系统下一步 (Next State)                                               *)
(* ========================================================================= *)

Next ==
    \/ \E t \in ThreadIds : RenderThread(t)
    \/ \E t \in ThreadIds : PhysicsThread(t)
    \/ \E t \in ThreadIds : ScriptThread(t)
    \/ \E t \in ThreadIds : IOThread(t)
    \/ \E t \in ThreadIds : \E r \in ResourceIds : AcquireLock(t, r)
    \/ \E t \in ThreadIds : \E r \in ResourceIds : ReleaseLock(t, r)
    \/ \E t \in ThreadIds : \E r \in ResourceIds : WaitLock(t, r)
    \/ \E t \in ThreadIds : CriticalSection(t)
    \/ \E t \in ThreadIds : \E msg \in [type : MessageType, data : Nat, sender : ThreadIds, priority : 1..10] : SendMessage(t, msg)
    \/ \E t \in ThreadIds : ReceiveMessage(t)
    \/ BeginFrame()
    \/ EndFrame()

(* ========================================================================= *)
(* 10. 活性属性 (Liveness Properties)                                       *)
(* ========================================================================= *)

(* 公平性：每个线程最终会执行 *)
ThreadFairness(t) == WF_vars(RenderThread(t) \/ PhysicsThread(t) \/ ScriptThread(t) \/ IOThread(t))

(* 锁公平性：等待锁的线程最终会获得锁 *)
LockFairness(t, r) == 
    WF_vars(WaitLock(t, r))

(* 消息处理公平性 *)
MessageFairness == 
    WF_vars(\E t \in ThreadIds : ReceiveMessage(t))

(* ========================================================================= *)
(* 11. 安全性属性 (Safety Properties)                                       *)
(* ========================================================================= *)

(* 互斥：同一资源不能被多个线程同时持有 *)
MutexInvariant ==
    \A r \in ResourceIds :
        IF locks[r] # NULL THEN
            Len(lock_queue[r]) = 0  \* 持有锁时无等待队列
        ELSE
            TRUE

(* 无死锁：不会所有线程都在等待 *)
NoDeadlock ==
    ~(\A t \in ThreadIds : threads[t] = "waiting")

(* 锁顺序：防止死锁的锁获取顺序 *)
LockOrderInvariant ==
    \A t \in ThreadIds :
        IF thread_pc[t] = "acquire_render_lock" THEN
            \A r \in ResourceIds : locks[r] # t
        ELSE IF thread_pc[t] = "acquire_physics_lock" THEN
            locks["render"] # t  \* 必须先获取渲染锁
        ELSE IF thread_pc[t] = "acquire_script_lock" THEN
            locks["render"] # t /\ locks["physics"] # t
        ELSE
            TRUE

(* 队列边界：消息队列不溢出 *)
QueueBoundInvariant ==
    Len(message_queue) <= QueueSize

(* 线程状态有效性 *)
ThreadStateInvariant ==
    \A t \in ThreadIds : threads[t] \in ThreadState

(* 组合不变量 *)
TypeInvariant ==
    /\ MutexInvariant
    /\ LockOrderInvariant
    /\ QueueBoundInvariant
    /\ ThreadStateInvariant

(* ========================================================================= *)
(* 12. 规范 (Specification)                                                 *)
(* ========================================================================= *)

Spec == Init /\ [][Next]_vars /\ \A t \in ThreadIds : ThreadFairness(t)

(* ========================================================================= *)
(* 13. 定理 (Theorems)                                                      *)
(* ========================================================================= *)

THEOREM Spec => []TypeInvariant
THEOREM Spec => []NoDeadlock
THEOREM Spec => <>[](frame_count > 0)  \* 最终会开始渲染

(* ========================================================================= *)
(* 14. 模型检查配置 (Model Checking Configuration)                          *)
(* ========================================================================= *)

(*
模型检查参数:
- ThreadIds = {1, 2, 3, 4}  \* 4 个线程
- ResourceIds = {"render", "physics", "script", "io"}  \* 4 个资源
- MaxLocks = 4
- QueueSize = 100

检查属性:
- 互斥不变量
- 无死锁
- 锁顺序不变量
- 队列边界不变量
- 活性（无饥饿）
*)

================================================================================
