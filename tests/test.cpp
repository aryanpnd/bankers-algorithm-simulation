#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

const int MAX_RESOURCES = 5;
const int MAX_PROCESSES = 5;

int available[MAX_RESOURCES];
int allocation[MAX_PROCESSES][MAX_RESOURCES];
int max_need[MAX_PROCESSES][MAX_RESOURCES];
int need[MAX_PROCESSES][MAX_RESOURCES];
bool finished[MAX_PROCESSES];

mutex mtx;
condition_variable cv;

bool isSafeState(int process, int request[]) {
    for (int i = 0; i < MAX_RESOURCES; ++i) {
        if (request[i] > need[process][i] || request[i] > available[i])
            return false;
    }
    return true;
}

void requestResources(int process, int request[]) {
    unique_lock<mutex> lock(mtx);
    if (isSafeState(process, request)) {
        for (int i = 0; i < MAX_RESOURCES; ++i) {
            available[i] -= request[i];
            allocation[process][i] += request[i];
            need[process][i] -= request[i];
        }
        cout << "Thread " << process << " requested resources and got them." << endl;
    } else {
        cout << "Thread " << process << " requested resources but is denied (unsafe state)." << endl;
    }
}

void releaseResources(int process) {
    unique_lock<mutex> lock(mtx);
    for (int i = 0; i < MAX_RESOURCES; ++i) {
        available[i] += allocation[process][i];
        allocation[process][i] = 0;
        need[process][i] = max_need[process][i];
    }
    cout << "Thread " << process << " released resources." << endl;
    cv.notify_all();
}

void threadFunction(int process) {
    int request[MAX_RESOURCES];

    // Simulate random resource requests and releases for each thread
    for (int i = 0; i < 3; ++i) {
        // Generate a random request
        for (int j = 0; j < MAX_RESOURCES; ++j) {
            request[j] = rand() % (max_need[process][j] + 1);
        }

        requestResources(process, request);

        // Sleep to simulate work
        this_thread::sleep_for(chrono::milliseconds(500));

        releaseResources(process);
    }
}

int main() {
    // Initialize available, allocation, max_need, and need matrices

    for (int i = 0; i < MAX_PROCESSES; ++i) {
        finished[i] = false;
    }

    vector<thread> threads;

    for (int i = 0; i < MAX_PROCESSES; ++i) {
        threads.emplace_back(thread(threadFunction, i));
    }

    for (int i = 0; i < MAX_PROCESSES; ++i) {
        threads[i].join();
    }

    return 0;
}
