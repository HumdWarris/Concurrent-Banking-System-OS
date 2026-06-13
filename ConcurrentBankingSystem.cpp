#include <iostream>
#include <vector>
#include <queue>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <semaphore.h>
#include <algorithm>
#include <iomanip>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// common types
// ─────────────────────────────────────────────────────────────────────────────

enum CustomerType  { REGULAR = 1, PREMIUM = 3, VIP = 5 };
enum TransactionType { DEPOSIT, WITHDRAWAL };

struct Customer {
    int id, arrivalTime, burstTime, priority;
    int startTime, finishTime, waitingTime, turnaroundTime;
    string name;
};

// ─────────────────────────────────────────────────────────────────────────────
// module 1: scheduling
// ─────────────────────────────────────────────────────────────────────────────

void printGantt(const vector<pair<string,int>>& gantt) {
    cout << "\n--- Gantt Chart ---\n|";
    for (auto& g : gantt) cout << " " << g.first << " |";
    cout << "\n0";
    int t = 0;
    for (auto& g : gantt) { t += g.second; cout << setw(4) << t; }
    cout << "\n";
}

void printMetrics(vector<Customer>& customers) {
    cout << "\n--- Scheduling Metrics ---\n";
    cout << left << setw(20) << "Name" << setw(10) << "Arrival"
         << setw(10) << "Burst" << setw(10) << "Start"
         << setw(10) << "Finish" << setw(12) << "Waiting" << "Turnaround\n";
    double avgWT = 0, avgTAT = 0;
    for (auto& c : customers) {
        cout << setw(20) << c.name << setw(10) << c.arrivalTime
             << setw(10) << c.burstTime << setw(10) << c.startTime
             << setw(10) << c.finishTime << setw(12) << c.waitingTime
             << c.turnaroundTime << "\n";
        avgWT += c.waitingTime; avgTAT += c.turnaroundTime;
    }
    int n = customers.size();
    cout << fixed << setprecision(2);
    cout << "Avg Waiting Time: " << avgWT/n << "  |  Avg Turnaround: " << avgTAT/n << "\n";
}

void runFCFS(vector<Customer> customers) {
    cout << "\n========== FCFS Scheduling ==========\n";
    sort(customers.begin(), customers.end(),
        [](const Customer& a, const Customer& b){ return a.arrivalTime < b.arrivalTime; });
    vector<pair<string,int>> gantt;
    int clock = 0;
    for (auto& c : customers) {
        if (clock < c.arrivalTime) clock = c.arrivalTime;
        c.startTime      = clock;
        c.waitingTime    = c.startTime - c.arrivalTime;
        c.finishTime     = c.startTime + c.burstTime;
        c.turnaroundTime = c.finishTime - c.arrivalTime;
        clock = c.finishTime;
        gantt.push_back({c.name, c.burstTime});
        cout << "Running: " << c.name << " [arrival=" << c.arrivalTime << " burst=" << c.burstTime << "]\n";
    }
    printGantt(gantt);
    printMetrics(customers);
}

void runPriority(vector<Customer> customers) {
    cout << "\n========== Priority Scheduling ==========\n";
    vector<pair<string,int>> gantt;
    int clock = 0, completed = 0, n = customers.size();
    vector<bool> done(n, false);
    while (completed < n) {
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (!done[i] && customers[i].arrivalTime <= clock)
                if (best == -1 || customers[i].priority > customers[best].priority) best = i;
        }
        if (best == -1) { clock++; continue; }
        auto& c = customers[best];
        c.startTime      = clock;
        c.waitingTime    = c.startTime - c.arrivalTime;
        c.finishTime     = c.startTime + c.burstTime;
        c.turnaroundTime = c.finishTime - c.arrivalTime;
        clock = c.finishTime;
        done[best] = true; completed++;
        gantt.push_back({c.name, c.burstTime});
        cout << "Running: " << c.name << " [priority=" << c.priority << " burst=" << c.burstTime << "]\n";
    }
    printGantt(gantt);
    printMetrics(customers);
}

void runRoundRobin(vector<Customer> customers, int quantum = 2) {
    cout << "\n========== Round Robin Scheduling (quantum=" << quantum << ") ==========\n";
    int n = customers.size();
    vector<int> remaining(n);
    for (int i = 0; i < n; i++) remaining[i] = customers[i].burstTime;
    vector<pair<string,int>> gantt;
    vector<bool> started(n, false);
    int clock = 0, completed = 0;
    for (auto& c : customers) { c.startTime = -1; c.finishTime = 0; }
    while (completed < n) {
        bool anyRan = false;
        for (int i = 0; i < n; i++) {
            if (remaining[i] <= 0 || customers[i].arrivalTime > clock) continue;
            if (!started[i]) { customers[i].startTime = clock; started[i] = true; }
            int run = min(quantum, remaining[i]);
            gantt.push_back({customers[i].name, run});
            clock += run; remaining[i] -= run; anyRan = true;
            if (remaining[i] == 0) {
                customers[i].finishTime     = clock;
                customers[i].turnaroundTime = clock - customers[i].arrivalTime;
                customers[i].waitingTime    = customers[i].turnaroundTime - customers[i].burstTime;
                completed++;
            }
        }
        if (!anyRan) clock++;
    }
    printGantt(gantt);
    printMetrics(customers);
}

void runThreadedDemo(vector<Customer>& customers) {
    cout << "\n========== Threaded Execution Demo ==========\n";
    mutex printMtx;
    vector<thread> threads;
    for (auto& c : customers) {
        threads.emplace_back([&c, &printMtx]() {
            this_thread::sleep_for(chrono::milliseconds(c.arrivalTime * 10));
            { lock_guard<mutex> lock(printMtx); cout << "[Thread] " << c.name << " started\n"; }
            this_thread::sleep_for(chrono::milliseconds(c.burstTime * 10));
            { lock_guard<mutex> lock(printMtx); cout << "[Thread] " << c.name << " finished\n"; }
        });
    }
    for (auto& t : threads) t.join();
}

// ─────────────────────────────────────────────────────────────────────────────
// module 2: synchronization
// ─────────────────────────────────────────────────────────────────────────────

struct BankAccount {
    int id;
    double balance;
    vector<string> history;
    mutex mtx;

    BankAccount(int id, double bal) : id(id), balance(bal) {}

    void deposit(const string& name, double amount) {
        lock_guard<mutex> lock(mtx);
        balance += amount;
        string log = name + " deposited " + to_string((int)amount)
                   + " | balance=" + to_string((int)balance);
        history.push_back(log);
        cout << "[Mutex] " << log << "\n";
    }

    bool withdraw(const string& name, double amount) {
        lock_guard<mutex> lock(mtx);
        if (balance < amount) {
            cout << "[Mutex] " << name << " withdrawal DENIED (insufficient funds)\n";
            return false;
        }
        balance -= amount;
        string log = name + " withdrew " + to_string((int)amount)
                   + " | balance=" + to_string((int)balance);
        history.push_back(log);
        cout << "[Mutex] " << log << "\n";
        return true;
    }

    void printHistory() {
        cout << "\n--- Account #" << id << " Transaction History ---\n";
        for (auto& h : history) cout << "  " << h << "\n";
        cout << "Final Balance: " << balance << "\n";
    }
};

void runSynchronizationDemo() {
    cout << "\n========== Synchronization Demo ==========\n";
    BankAccount account(101, 1000.0);
    sem_t sem; sem_init(&sem, 0, 2); // allow max 2 concurrent deposits

    vector<pair<string, pair<TransactionType, double>>> ops = {
        {"Alice (Regular)",  {DEPOSIT,    300}},
        {"Bob (Regular)",    {WITHDRAWAL, 200}},
        {"Carol (Premium)",  {DEPOSIT,    500}},
        {"Dave (Regular)",   {WITHDRAWAL, 800}},
    };

    vector<thread> threads;
    for (auto& op : ops) {
        threads.emplace_back([&, op]() {
            if (op.second.first == DEPOSIT) {
                sem_wait(&sem);
                this_thread::sleep_for(chrono::milliseconds(50));
                account.deposit(op.first, op.second.second);
                sem_post(&sem);
            } else {
                account.withdraw(op.first, op.second.second);
            }
        });
    }
    for (auto& t : threads) t.join();
    account.printHistory();
    sem_destroy(&sem);
}

// ─────────────────────────────────────────────────────────────────────────────
// module 3: banker's algorithm
// ─────────────────────────────────────────────────────────────────────────────

struct BankerState {
    int n;                    // number of customers
    vector<int> available, maximum, allocation, need;
    vector<string> names;
};

bool isSafe(const BankerState& s, vector<int>& safeSeq) {
    vector<int> work = s.available;
    vector<bool> finish(s.n, false);
    safeSeq.clear();
    while ((int)safeSeq.size() < s.n) {
        bool progress = false;
        for (int i = 0; i < s.n; i++) {
            if (!finish[i] && s.need[i] <= work[0]) {
                work[0] += s.allocation[i];
                finish[i] = true;
                safeSeq.push_back(i);
                progress = true;
            }
        }
        if (!progress) return false;
    }
    return true;
}

bool requestLoan(BankerState& s, int idx, int request) {
    cout << "\n[Banker] " << s.names[idx] << " requests " << request << " units\n";
    if (request > s.need[idx])      { cout << "[Banker] ERROR: exceeds maximum need\n"; return false; }
    if (request > s.available[0])   { cout << "[Banker] DENIED: not enough resources available\n"; return false; }
    s.available[0] -= request; s.allocation[idx] += request; s.need[idx] -= request;
    vector<int> safeSeq;
    if (isSafe(s, safeSeq)) {
        cout << "[Banker] GRANTED. Safe sequence: ";
        for (int i : safeSeq) cout << s.names[i] << " ";
        cout << "\n"; return true;
    }
    s.available[0] += request; s.allocation[idx] -= request; s.need[idx] += request;
    cout << "[Banker] DENIED: would cause unsafe state\n";
    return false;
}

void printBankerState(const BankerState& s) {
    cout << "\n--- Banker State ---\nAvailable: " << s.available[0] << " units\n";
    cout << left << setw(20) << "Customer" << setw(10) << "Max" << setw(12) << "Allocated" << "Need\n";
    for (int i = 0; i < s.n; i++)
        cout << setw(20) << s.names[i] << setw(10) << s.maximum[i]
             << setw(12) << s.allocation[i] << s.need[i] << "\n";
}

void runBankersDemo() {
    cout << "\n========== Banker's Algorithm (Deadlock Avoidance) ==========\n";
    BankerState s;
    s.n         = 3;
    s.names     = {"Customer A (Loan)", "Customer B (Loan)", "Customer C (Loan)"};
    s.available  = {3};
    s.maximum    = {7, 5, 3};
    s.allocation = {0, 2, 3};
    s.need       = {7, 3, 0};
    printBankerState(s);
    requestLoan(s, 1, 2); // safe grant
    requestLoan(s, 0, 5); // denied: not enough available
    requestLoan(s, 0, 2); // denied: not enough available
    printBankerState(s);
}

// ─────────────────────────────────────────────────────────────────────────────
// module 4: ipc (message queue)
// ─────────────────────────────────────────────────────────────────────────────

struct Message {
    int customerId, amount, accountId;
    string customerName;
    TransactionType type;
};

class MessageQueue {
    queue<Message> q;
    mutex mtx;
    condition_variable cv;
    bool closed = false;
public:
    void send(const Message& msg) {
        lock_guard<mutex> lock(mtx);
        q.push(msg); cv.notify_one();
    }
    bool receive(Message& msg) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]{ return !q.empty() || closed; });
        if (q.empty()) return false;
        msg = q.front(); q.pop(); return true;
    }
    void close() { lock_guard<mutex> lock(mtx); closed = true; cv.notify_all(); }
};

void runIPCDemo() {
    cout << "\n========== IPC Message Queue Demo ==========\n";
    MessageQueue requestQ, responseQ;
    atomic<int> processed{0};
    int total = 5;
    unordered_map<int,double> accounts = {{123, 1000.0}, {456, 500.0}};
    mutex accMtx;

    thread server([&]() {
        Message msg;
        while (processed < total) {
            if (requestQ.receive(msg)) {
                string result;
                {
                    lock_guard<mutex> lock(accMtx);
                    if (msg.type == DEPOSIT) {
                        accounts[msg.accountId] += msg.amount;
                        result = "Success | " + msg.customerName + " deposited "
                               + to_string(msg.amount) + " | balance="
                               + to_string((int)accounts[msg.accountId]);
                    } else {
                        if (accounts[msg.accountId] >= msg.amount) {
                            accounts[msg.accountId] -= msg.amount;
                            result = "Success | " + msg.customerName + " withdrew "
                                   + to_string(msg.amount) + " | balance="
                                   + to_string((int)accounts[msg.accountId]);
                        } else {
                            result = "Failed | " + msg.customerName + " insufficient funds";
                        }
                    }
                }
                cout << "[Server] " << result << "\n";
                processed++;
                Message resp = msg; resp.customerName = result;
                responseQ.send(resp);
            }
        }
        responseQ.close();
    });

    vector<Message> requests = {
        {1, 500, 123, "Alice",    DEPOSIT},
        {2, 200, 123, "Bob",      WITHDRAWAL},
        {3, 300, 456, "Carol",    DEPOSIT},
        {4, 700, 456, "Dave",     WITHDRAWAL},
        {5, 100, 123, "Eve(VIP)", DEPOSIT},
    };

    vector<thread> producers;
    for (auto& req : requests) {
        producers.emplace_back([&requestQ, req]() {
            this_thread::sleep_for(chrono::milliseconds(20));
            cout << "[Customer " << req.customerId << "] Sending: "
                 << (req.type == DEPOSIT ? "Deposit " : "Withdraw ")
                 << req.amount << " to Account #" << req.accountId << "\n";
            requestQ.send(req);
        });
    }
    for (auto& p : producers) p.join();
    server.join();

    cout << "\n--- Response Log ---\n";
    Message resp;
    while (responseQ.receive(resp)) cout << "[Response] " << resp.customerName << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// module 5: memory management (fifo & lru)
// ─────────────────────────────────────────────────────────────────────────────

struct PageResult { int faults, hits; };

PageResult fifoReplace(const vector<int>& pages, int frames) {
    queue<int> frameQ;
    unordered_set<int> inMem;
    PageResult res = {0, 0};
    cout << "\n--- FIFO Page Replacement (frames=" << frames << ") ---\n";
    cout << left << setw(8) << "Page" << "Frames\n";
    for (int page : pages) {
        if (inMem.count(page)) {
            res.hits++;
            cout << setw(8) << page << "[HIT]\n";
        } else {
            res.faults++;
            if ((int)inMem.size() == frames) {
                int evict = frameQ.front(); frameQ.pop(); inMem.erase(evict);
                cout << setw(8) << page << "evict " << evict << " | ";
            } else { cout << setw(8) << page; }
            frameQ.push(page); inMem.insert(page);
            queue<int> tmp = frameQ;
            cout << "{ "; while (!tmp.empty()) { cout << tmp.front() << " "; tmp.pop(); }
            cout << "} [FAULT]\n";
        }
    }
    return res;
}

PageResult lruReplace(const vector<int>& pages, int frames) {
    list<int> lruList;
    unordered_map<int, list<int>::iterator> pageMap;
    PageResult res = {0, 0};
    cout << "\n--- LRU Page Replacement (frames=" << frames << ") ---\n";
    cout << left << setw(8) << "Page" << "Frames\n";
    for (int page : pages) {
        if (pageMap.count(page)) {
            lruList.erase(pageMap[page]);
            lruList.push_front(page);
            pageMap[page] = lruList.begin();
            res.hits++;
            cout << setw(8) << page << "[HIT]\n";
        } else {
            res.faults++;
            if ((int)lruList.size() == frames) {
                int evict = lruList.back(); lruList.pop_back(); pageMap.erase(evict);
                cout << setw(8) << page << "evict " << evict << " | ";
            } else { cout << setw(8) << page; }
            lruList.push_front(page); pageMap[page] = lruList.begin();
            cout << "{ "; for (int p : lruList) cout << p << " ";
            cout << "} [FAULT]\n";
        }
    }
    return res;
}

void runMemoryDemo() {
    cout << "\n========== Memory Management Demo ==========\n";
    vector<int> pageRef = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    int frames = 3;
    cout << "Page Reference String: ";
    for (int p : pageRef) cout << p << " ";
    cout << "\nFrames: " << frames << "\n";
    PageResult fifo = fifoReplace(pageRef, frames);
    PageResult lru  = lruReplace(pageRef, frames);
    int total = fifo.faults + fifo.hits;
    cout << "\n--- Memory Management Comparison ---\n";
    cout << left << setw(20) << "Algorithm" << setw(14) << "Page Faults"
         << setw(12) << "Page Hits" << "Hit Ratio\n";
    cout << fixed << setprecision(2);
    cout << setw(20) << "FIFO" << setw(14) << fifo.faults << setw(12) << fifo.hits
         << (double)fifo.hits/total*100 << "%\n";
    cout << setw(20) << "LRU"  << setw(14) << lru.faults  << setw(12) << lru.hits
         << (double)lru.hits/total*100  << "%\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    cout << "=================================================\n";
    cout << "   Concurrent Banking System - OS Semester Project\n";
    cout << "=================================================\n";

    vector<Customer> customers = {
        {1, 0, 4, REGULAR, 0,0,0,0, "Alice(Regular)"},
        {2, 1, 3, REGULAR, 0,0,0,0, "Bob(Regular)"},
        {3, 2, 5, PREMIUM, 0,0,0,0, "Carol(Premium)"},
        {4, 0, 2, VIP,     0,0,0,0, "Dave(VIP)"},
        {5, 1, 4, PREMIUM, 0,0,0,0, "Eve(Premium)"},
    };

    runFCFS(customers);
    runPriority(customers);
    runRoundRobin(customers, 2);
    runThreadedDemo(customers);
    runSynchronizationDemo();
    runBankersDemo();
    runIPCDemo();
    runMemoryDemo();

    cout << "\n=== All modules completed ===\n";
    return 0;
}
