(* ========================================================================= *)
(* Phoenix Engine - 死锁检测 TLA+ 规格说明                                   *)
(* TLA+ Specification for Deadlock Detection in Phoenix Engine              *)
(* ========================================================================= *)

------------------------------- MODULE DeadlockDetection -------------------------------
EXTENDS Integers, Sequences, TLC, GraphTheory

(* ========================================================================= *)
(* 1. 常量定义 (Constants)                                                   *)
(* ========================================================================= *)

CONSTANTS
    Threads     \* 线程集合
    Locks       \* 锁集合
    MaxThreads  \* 最大线程数

(* ========================================================================= *)
(* 2. 变量定义 (Variables)                                                   *)
(* ========================================================================= *)

VARIABLES
    held_locks      \* 线程持有的锁：thread -> Set(lock)
    waiting_for     \* 线程等待的锁：thread -> lock | NULL
    lock_holders    \* 锁的持有者：lock -> thread | NULL
    wait_graph      \* 等待图：thread -> Set(thread)

(* ========================================================================= *)
(* 3. 辅助定义 (Helper Definitions)                                          *)
(* ========================================================================= *)

NULL == 0

(* 等待图边：t1 -> t2 表示 t1 等待 t2 持有的锁 *)
WaitGraphEdge(t1, t2) ==
    \E l \in Locks :
        /\ waiting_for[t1] = l
        /\ lock_holders[l] = t2

(* 更新等待图 *)
UpdateWaitGraph ==
    wait_graph' = [t1 \in Threads |-> 
        {t2 \in Threads : WaitGraphEdge(t1, t2)}]

(* 检测循环 *)
HasCycle(g) ==
    \E t \in Threads : Reachable(g, t, t)

(* 可达性：从 s 到 t 是否可达 *)
Reachable(g, s, t) ==
    LET RECURSIVE ReachHelper(n, visited) ==
        IF n = t THEN TRUE
        ELSE IF n \in visited THEN FALSE
        ELSE \E next \in g[n] : ReachHelper(next, visited \cup {n})
    IN
    ReachHelper(s, {})

(* 死锁状态 *)
IsDeadlock ==
    /\ \E t \in Threads : waiting_for[t] # NULL  \* 有线程在等待
    /\ \A t \in Threads : 
        waiting_for[t] # NULL => 
        \E t2 \in Threads : WaitGraphEdge(t, t2)  \* 所有等待的线程都在等某个锁
    /\ HasCycle(wait_graph)  \* 等待图有环

(* 安全状态：无死锁 *)
IsSafe == ~IsDeadlock

(* ========================================================================= *)
(* 4. 初始状态 (Initial State)                                              *)
(* ========================================================================= *)

Init ==
    /\ held_locks = [t \in Threads |-> {}]
    /\ waiting_for = [t \in Threads |-> NULL]
    /\ lock_holders = [l \in Locks |-> NULL]
    /\ wait_graph = [t \in Threads |-> {}]

(* ========================================================================= *)
(* 5. 锁操作 (Lock Operations)                                               *)
(* ========================================================================= *)

(* 请求锁 *)
RequestLock(t, l) ==
    /\ waiting_for[t] = NULL  \* 当前没有在等待
    /\ IF lock_holders[l] = NULL THEN
        \* 锁可用，直接获取
        /\ lock_holders' = [lock_holders EXCEPT ![l] = t]
        /\ held_locks' = [held_locks EXCEPT ![t] = held_locks[t] \cup {l}]
        /\ waiting_for' = waiting_for
    ELSE
        \* 锁被占用，进入等待
        /\ lock_holders' = lock_holders
        /\ held_locks' = held_locks
        /\ waiting_for' = [waiting_for EXCEPT ![t] = l]
    /\ UNCHANGED <<wait_graph>>

(* 获取锁（从等待状态） *)
AcquireLock(t, l) ==
    /\ waiting_for[t] = l
    /\ lock_holders[l] = t  \* 现在持有锁
    /\ lock_holders' = lock_holders
    /\ held_locks' = [held_locks EXCEPT ![t] = held_locks[t] \cup {l}]
    /\ waiting_for' = [waiting_for EXCEPT ![t] = NULL]
    /\ wait_graph' = [wait_graph EXCEPT ![t] = {}]  \* 移除等待边

(* 释放锁 *)
ReleaseLock(t, l) ==
    /\ l \in held_locks[t]
    /\ lock_holders' = [lock_holders EXCEPT ![l] = NULL]
    /\ held_locks' = [held_locks EXCEPT ![t] = held_locks[t] \ {l}]
    /\ waiting_for' = waiting_for
    /\ UNCHANGED <<wait_graph>>

(* 带超时的锁请求 *)
RequestLockWithTimeout(t, l, timeout) ==
    /\ waiting_for[t] = NULL
    /\ IF lock_holders[l] = NULL THEN
        /\ lock_holders' = [lock_holders EXCEPT ![l] = t]
        /\ held_locks' = [held_locks EXCEPT ![t] = held_locks[t] \cup {l}]
        /\ waiting_for' = waiting_for
    ELSE
        \* 设置超时计时器（简化：标记为可能死锁）
        /\ lock_holders' = lock_holders
        /\ held_locks' = held_locks
        /\ waiting_for' = [waiting_for EXCEPT ![t] = l]
    /\ UNCHANGED <<wait_graph>>

(* ========================================================================= *)
(* 6. 死锁检测算法 (Deadlock Detection Algorithm)                           *)
(* ========================================================================= *)

(* 等待图遍历检测循环 *)
DetectDeadlockDFS ==
    LET RECURSIVE DFS(node, visited, rec_stack) ==
        IF node \in rec_stack THEN TRUE  \* 发现循环
        ELSE IF node \in visited THEN FALSE
        ELSE
            /\ visited' = visited \cup {node}
            /\ rec_stack' = rec_stack \cup {node}
            /\ \E next \in wait_graph[node] : DFS(next, visited', rec_stack')
    IN
    \E start \in Threads : DFS(start, {}, {})

(* 死锁恢复：选择一个牺牲者线程 *)
ChooseVictim() ==
    \E t \in Threads :
        /\ waiting_for[t] # NULL
        /\ \E l \in held_locks[t] :
            /\ lock_holders' = [lock_holders EXCEPT ![l] = NULL]
            /\ held_locks' = [held_locks EXCEPT ![t] = held_locks[t] \ {l}]
            /\ waiting_for' = [waiting_for EXCEPT ![t] = NULL]

(* 死锁恢复：抢占锁 *)
PreemptLock(victim, lock) ==
    /\ victim \in Threads
    /\ lock \in held_locks[victim]
    /\ lock_holders' = [lock_holders EXCEPT ![lock] = NULL]
    /\ held_locks' = [held_locks EXCEPT ![victim] = held_locks[victim] \ {lock}]
    /\ waiting_for' = waiting_for
    /\ UNCHANGED <<wait_graph>>

(* ========================================================================= *)
(* 7. 死锁预防策略 (Deadlock Prevention Strategies)                         *)
(* ========================================================================= *)

(* 策略 1: 锁排序 - 按固定顺序获取锁 *)
RequestLockOrdered(t, l, order) ==
    /\ waiting_for[t] = NULL
    /\ \A l2 \in held_locks[t] : order[l2] < order[l]  \* 必须按顺序
    /\ IF lock_holders[l] = NULL THEN
        /\ lock_holders' = [lock_holders EXCEPT ![l] = t]
        /\ held_locks' = [held_locks EXCEPT ![t] = held_locks[t] \cup {l}]
    ELSE
        /\ UNCHANGED <<lock_holders, held_locks>>
    /\ waiting_for' = [waiting_for EXCEPT ![t] = IF lock_holders[l] # NULL THEN l ELSE NULL]
    /\ UNCHANGED <<wait_graph>>

(* 策略 2: 超时重试 *)
RequestLockWithRetry(t, l, max_retries) ==
    /\ waiting_for[t] = NULL
    /\ IF lock_holders[l] = NULL THEN
        /\ lock_holders' = [lock_holders EXCEPT ![l] = t]
        /\ held_locks' = [held_locks EXCEPT ![t] = held_locks[t] \cup {l}]
        /\ waiting_for' = waiting_for
    ELSE
        \* 等待并重试（简化：标记重试计数）
        /\ UNCHANGED <<lock_holders, held_locks, waiting_for>>
    /\ UNCHANGED <<wait_graph>>

(* 策略 3: 银行家算法 - 资源分配安全性检查 *)
IsSafeState(available, allocation, max_need) ==
    LET work == available
        finish == [t \in Threads |-> FALSE]
        RECURSIVE FindSafeSeq(work, finish) ==
            IF \A t \in Threads : finish[t] THEN TRUE
            ELSE
                \E t \in Threads :
                    /\ ~finish[t]
                    /\ max_need[t] <= work  \* 需求可满足
                    /\ FindSafeSeq(work + allocation[t], [finish EXCEPT ![t] = TRUE])
    IN
    FindSafeSeq(work, finish)

BankerRequest(t, l, available, allocation, max_need) ==
    /\ IsSafeState(available, allocation, max_need)  \* 检查安全性
    /\ IF lock_holders[l] = NULL THEN
        /\ lock_holders' = [lock_holders EXCEPT ![l] = t]
        /\ held_locks' = [held_locks EXCEPT ![t] = held_locks[t] \cup {l}]
    ELSE
        /\ UNCHANGED <<lock_holders, held_locks>>
    /\ UNCHANGED <<waiting_for, wait_graph>>

(* ========================================================================= *)
(* 8. 系统下一步 (Next State)                                               *)
(* ========================================================================= *)

Next ==
    \/ \E t \in Threads : \E l \in Locks : RequestLock(t, l)
    \/ \E t \in Threads : \E l \in held_locks[t] : ReleaseLock(t, l)
    \/ \E t \in Threads : \E l \in Locks : AcquireLock(t, l)
    \/ DetectDeadlockDFS
    \/ \E t \in Threads : ChooseVictim()
    \/ UpdateWaitGraph

(* ========================================================================= *)
(* 9. 安全性属性 (Safety Properties)                                        *)
(* ========================================================================= *)

(* 无死锁不变量 *)
NoDeadlockInvariant == ~IsDeadlock

(* 等待图无循环 *)
WaitGraphAcyclic == ~HasCycle(wait_graph)

(* 锁互斥 *)
MutexInvariant ==
    \A l \in Locks :
        IF lock_holders[l] # NULL THEN
            \A t \in Threads : t # lock_holders[l] => l \notin held_locks[t]
        ELSE
            TRUE

(* 等待一致性 *)
WaitingConsistency ==
    \A t \in Threads :
        waiting_for[t] # NULL =>
        lock_holders[waiting_for[t]] # NULL /\ lock_holders[waiting_for[t]] # t

(* 组合不变量 *)
TypeInvariant ==
    /\ NoDeadlockInvariant
    /\ WaitGraphAcyclic
    /\ MutexInvariant
    /\ WaitingConsistency

(* ========================================================================= *)
(* 10. 活性属性 (Liveness Properties)                                       *)
(* ========================================================================= *)

(* 锁最终会被释放 *)
LockEventuallyReleased(l) ==
    [](lock_holders[l] # NULL => <>lock_holders[l] = NULL)

(* 等待的线程最终会获得锁 *)
WaitEventuallySatisfied(t) ==
    [](waiting_for[t] # NULL => <>waiting_for[t] = NULL)

(* 无饥饿 *)
NoStarvation ==
    \A t \in Threads : WF_vars(RequestLock(t, SOME(l \in Locks)))

(* ========================================================================= *)
(* 11. 规范 (Specification)                                                 *)
(* ========================================================================= *)

Spec == Init /\ [][Next]_vars /\ \A t \in Threads : WaitEventuallySatisfied(t)

(* ========================================================================= *)
(* 12. 定理 (Theorems)                                                      *)
(* ========================================================================= *)

THEOREM Spec => []TypeInvariant
THEOREM Spec => []NoDeadlockInvariant
THEOREM Spec => NoStarvation

(* ========================================================================= *)
(* 13. 模型检查配置 (Model Checking Configuration)                          *)
(* ========================================================================= *)

(*
模型检查参数:
- Threads = {1, 2, 3, 4}
- Locks = {"render", "physics", "audio", "io", "resource"}
- MaxThreads = 4

检查属性:
- 无死锁不变量
- 等待图无循环
- 锁互斥
- 等待一致性
- 无饥饿

死锁场景测试:
1. 经典死锁：T1 持有 L1 等 L2, T2 持有 L2 等 L1
2. 三线程死锁：T1->L1->L2, T2->L2->L3, T3->L3->L1
3. 资源竞争：多线程竞争同一资源
*)

================================================================================
