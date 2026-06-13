# Concurrent-Banking-System-OS
C++ Concurrent Banking System implementing 5 OS concepts: CPU Scheduling (FCFS, Priority, Round Robin), Process Synchronization (Mutex, Semaphore), Deadlock Avoidance (Banker's Algorithm), IPC (Message Queue), and Memory Management (FIFO &amp; LRU page replacement).

# 🏦 Concurrent Banking System

![C++](https://img.shields.io/badge/Language-C%2B%2B17-blue)
![OS](https://img.shields.io/badge/Subject-Operating%20Systems-red)
![Threads](https://img.shields.io/badge/Concurrency-pthreads%20%2B%20std%3A%3Athread-green)
![University](https://img.shields.io/badge/FAST-OS%20Lab%20Project-orange)

<img width="843" height="1071" alt="image" src="https://github.com/user-attachments/assets/213dd05a-36fb-47f3-92a2-0bb1248c1842" />

<img width="1527" height="943" alt="image" src="https://github.com/user-attachments/assets/a7b38a5d-e212-4e79-8d50-1c82550287ff" />

<img width="833" height="1037" alt="image" src="https://github.com/user-attachments/assets/964d0a7f-1bff-4c04-afdb-bce8eb70c942" />

<img width="567" height="1022" alt="image" src="https://github.com/user-attachments/assets/cc00ec78-ba92-455d-9d73-fecf2af44bbe" />


A C++ application that simulates a concurrent banking environment implementing **5 core Operating Systems concepts** in a single modular codebase — CPU Scheduling, Process Synchronization, Deadlock Avoidance, IPC, and Memory Management.

**Authors:** 24F-0617 · 24F-0535 · Muhammad Humd (24F-0542)  
National University of Computing and Emerging Sciences (FAST) — OS Lab, CL2006, 2024–25

---

## 📋 Overview

Multiple customer threads perform banking transactions concurrently. Each OS module is self-contained and produces measurable console output including Gantt charts, metrics tables, transaction logs, and page fault traces.

| Module | OS Concept | Technique |
|--------|-----------|-----------|
| 1 | CPU Scheduling | FCFS, Priority, Round Robin |
| 2 | Process Synchronization | Mutex locks, POSIX Semaphores |
| 3 | Deadlock Avoidance | Banker's Algorithm |
| 4 | Inter-Process Communication | Thread-safe Message Queue |
| 5 | Memory Management | FIFO & LRU Page Replacement |

---

## 🗂️ Repository Structure

```
Concurrent-Banking-System-OS/
├── README.md
├── main.cpp                  ← full source code (single file)
└── report/
    └── OS_Project_Report.pdf ← submitted project report
```

---

## 👥 Customer Types

| Type | Priority | Description |
|------|----------|-------------|
| Regular | 1 | Standard deposits and withdrawals |
| Premium | 3 | Higher scheduling priority |
| VIP | 5 | Highest priority; preempts all others |
| Loan Applicant | — | Handled by Banker's Algorithm |

---

## 🧩 Module 1 — CPU Scheduling

Customer requests are modelled as processes with arrival time, burst time, and priority. Three scheduling algorithms are implemented and compared.

**Test Customers:**

| Name | Type | Arrival | Burst | Priority |
|------|------|---------|-------|----------|
| Alice | Regular | 0 | 4 | 1 |
| Bob | Regular | 1 | 3 | 1 |
| Carol | Premium | 2 | 5 | 3 |
| Dave | VIP | 0 | 2 | 5 |
| Eve | Premium | 1 | 4 | 3 |

### FCFS (First-Come First-Served)
Customers processed in order of arrival. Non-preemptive.
```
Gantt: | Alice | Dave | Bob | Eve | Carol |
Time:  0      4     6    9   13   18

Avg Waiting Time: 5.00  |  Avg Turnaround: 9.40
```

### Priority Scheduling
VIP customers always execute first, then Premium, then Regular.
```
Gantt: | Dave | Eve | Carol | Bob | Alice |
Time:  0     2    6     11   14   18

Avg Waiting Time: 4.00  |  Avg Turnaround: 8.40  ✅ Best
```

### Round Robin (Quantum = 2ms)
Each customer gets equal 2ms time slices in cyclic order.
```
Avg Waiting Time: 10.80  |  Avg Turnaround: 14.80
```

**Comparison:**

| Algorithm | Avg Waiting | Avg Turnaround | Fairness |
|-----------|------------|----------------|----------|
| FCFS | 5.00 ms | 9.40 ms | Arrival-order |
| Priority | **4.00 ms** | **8.40 ms** | Priority-based |
| Round Robin | 10.80 ms | 14.80 ms | Time-shared (most fair) |

---

## 🔒 Module 2 — Process Synchronization

Protects shared `BankAccount` from race conditions using two mechanisms.

### Mutex Lock
Each `BankAccount` has a `std::mutex`. Both `deposit()` and `withdraw()` acquire a `lock_guard` before modifying the balance — ensuring only one thread writes at a time.

```cpp
void deposit(const string& name, double amount) {
    lock_guard<mutex> lock(mtx);
    balance += amount;
}
```

Without the mutex: two simultaneous deposits could both read the same balance, both add to it, and write back — losing one deposit entirely (lost-update race condition).

### POSIX Semaphore
Initialized with value `2` — limits concurrent deposits to 2 threads at a time. A third thread blocks until one finishes.

```cpp
sem_init(&sem, 0, 2);   // max 2 concurrent deposits
sem_wait(&sem);         // acquire slot
// ... deposit ...
sem_post(&sem);         // release slot
```

**Test results (initial balance = 1000):**

| Customer | Operation | Amount | Result |
|----------|-----------|--------|--------|
| Alice | Deposit | 300 | balance = 1300 |
| Bob | Withdrawal | 200 | balance = 1100 |
| Carol | Deposit | 500 | balance = 1600 |
| Dave | Withdrawal | 800 | balance = 800 |

---

## ⚠️ Module 3 — Deadlock Avoidance (Banker's Algorithm)

Loan requests are treated as resource allocation requests. Before granting any loan, the system checks whether the resulting state is **safe**. Unsafe requests are denied.

**Initial state:** 3 available credit units

| Customer | Maximum | Allocated | Need |
|----------|---------|-----------|------|
| Customer A | 7 | 0 | 7 |
| Customer B | 5 | 2 | 3 |
| Customer C | 3 | 3 | 0 |

**Request outcomes:**

| Test | Customer | Request | Result | Reason |
|------|----------|---------|--------|--------|
| TC-D1 | Customer B | 2 units | ✅ GRANTED | Safe sequence exists |
| TC-D2 | Customer A | 5 units | ❌ DENIED | Insufficient available resources |
| TC-D3 | Customer A | 2 units | ❌ DENIED | Insufficient available resources |

**Safety check logic:**
```cpp
bool isSafe(const BankerState& s, vector<int>& safeSeq) {
    vector<int> work = s.available;
    // find customer whose need <= work
    // simulate release, mark finished
    // return true only if all customers finish
}
```

---

## 📨 Module 4 — Inter-Process Communication (IPC)

Implements a **producer-consumer** pattern. Customer threads (producers) send transaction requests to a bank server thread (consumer) via a thread-safe `MessageQueue`.

```
Customer threads → [MessageQueue] → Bank Server thread → [ResponseQueue] → Customers
```

**MessageQueue** uses `std::queue` + `std::mutex` + `std::condition_variable`. No busy waiting — threads block until a message is available.

**Test transactions (Account #123 initial=1000, Account #456 initial=500):**

| Customer | Operation | Account | Amount | Server Response |
|----------|-----------|---------|--------|-----------------|
| Alice | Deposit | #123 | 500 | ✅ Success \| balance=1500 |
| Bob | Withdrawal | #123 | 200 | ✅ Success \| balance=1300 |
| Carol | Deposit | #456 | 300 | ✅ Success \| balance=800 |
| Dave | Withdrawal | #456 | 700 | ❌ Failed \| insufficient funds |
| Eve (VIP) | Deposit | #123 | 100 | ✅ Success \| balance=1400 |

---

## 💾 Module 5 — Memory Management

Simulates virtual memory page replacement. Measures page faults and hits to compare algorithm efficiency.

**Configuration:**
- Page Reference String: `1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5`
- Number of Frames: `3`
- Total References: `12`

### FIFO (First-In First-Out)
Evicts the oldest loaded page when memory is full.

| Page | Action | Frames |
|------|--------|--------|
| 1 | FAULT | {1} |
| 2 | FAULT | {1 2} |
| 3 | FAULT | {1 2 3} |
| 4 | FAULT — evict 1 | {2 3 4} |
| 1 | FAULT — evict 2 | {3 4 1} |
| 2 | FAULT — evict 3 | {4 1 2} |
| 5 | FAULT — evict 4 | {1 2 5} |
| 1 | HIT | {1 2 5} |
| 2 | HIT | {1 2 5} |
| 3 | FAULT — evict 1 | {2 5 3} |
| 4 | FAULT — evict 2 | {5 3 4} |
| 5 | HIT | {5 3 4} |

**FIFO Result: Faults = 9 | Hits = 3 | Hit Ratio = 25.00%**

### LRU (Least Recently Used)
Evicts the page that was used least recently. Exploits temporal locality.

**LRU Result: Faults = 8 | Hits = 4 | Hit Ratio = 33.33% ✅ Better**

**Comparison:**

| Algorithm | Page Faults | Page Hits | Hit Ratio | Note |
|-----------|------------|-----------|-----------|------|
| FIFO | 9 | 3 | 25.00% | Simple; may suffer Belady's anomaly |
| LRU | **8** | **4** | **33.33%** | Better; exploits temporal locality |

---

## 🚀 How to Run

**Linux / macOS:**
```bash
g++ -std=c++17 -pthread -o banking main.cpp
./banking
```

**Windows (MinGW):**
```bash
g++ -std=c++17 -pthread -o banking.exe main.cpp
banking.exe
```

> **Note:** POSIX semaphores (`sem_init`) require Linux or WSL on Windows. On macOS, use `-lpthread`.

---

## 🛠️ Dependencies

| Library | Purpose |
|---------|---------|
| `<thread>` | Customer and server threads |
| `<mutex>` | Per-account mutex locks |
| `<condition_variable>` | MessageQueue blocking |
| `<semaphore.h>` | POSIX semaphore (concurrent deposit limit) |
| `<atomic>` | Thread-safe flags |
| `<queue>`, `<list>` | Scheduling and page replacement structures |

All dependencies are part of the C++17 standard library or POSIX — no external packages needed.

---

## 📄 References

1. Silberschatz, Galvin, Gagne — *Operating System Concepts*, 10th Edition, Wiley
2. cppreference.com — `std::thread`, `std::mutex`, `std::condition_variable`
3. The Open Group Base Specifications — POSIX Semaphores: `sem_init`, `sem_wait`, `sem_post`
