#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <random>

using namespace std;

const int NUM_RESOURCES = 3;
const int NUM_CUSTOMERS = 5;
const int MAX_NEED = 10;

std::mutex mtx;
std::condition_variable cv;

std::vector<int> available(NUM_RESOURCES);
std::vector<std::vector<int>> maximum(NUM_CUSTOMERS, std::vector<int>(NUM_RESOURCES));
std::vector<std::vector<int>> allocation(NUM_CUSTOMERS, std::vector<int>(NUM_RESOURCES));
std::vector<std::vector<int>> need(NUM_CUSTOMERS, std::vector<int>(NUM_RESOURCES));

bool isSafeState() {
    std::vector<bool> finish(NUM_CUSTOMERS, false);
    std::vector<int> work(available);

    int completed = 0;
    while (completed < NUM_CUSTOMERS) {
        bool found = false;
        for (int i = 0; i < NUM_CUSTOMERS; ++i) {
            if (!finish[i]) {
                bool canAllocate = true;
                for (int j = 0; j < NUM_RESOURCES; ++j) {
                    if (need[i][j] > work[j]) {
                        canAllocate = false;
                        break;
                    }
                }
                if (canAllocate) {
                    for (int j = 0; j < NUM_RESOURCES; ++j) {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    found = true;
                    completed++;
                }
            }
        }
        if (!found) {
            return false; 
        }
    }
    return true; 
}

void requestResource(int customer) {
    std::unique_lock<std::mutex> lock(mtx);
    std::vector<int> request(NUM_RESOURCES);

    std::cout << "Customer " << customer << " is requesting resources: ";
    for (int i = 0; i < NUM_RESOURCES; ++i) {
        request[i] = std::rand() % (need[customer][i] + 1);
        std::cout << request[i] << " ";
    }
    std::cout << std::endl;

    bool grantRequest = true;

    for (int i = 0; i < NUM_RESOURCES; ++i) {
        if (request[i] > available[i] || request[i] > need[customer][i]) {
            grantRequest = false;
            break;
        }
    }

    if (grantRequest) {
        for (int i = 0; i < NUM_RESOURCES; ++i) {
            available[i] -= request[i];
            allocation[customer][i] += request[i];
            need[customer][i] -= request[i];
        }
        if (isSafeState()) {
            std::cout << "Request granted. System is in a safe state." << std::endl;
        } else {
            std::cout << "Request denied. Granting the request would lead to an unsafe state." << std::endl;
            // Rollback allocation
            for (int i = 0; i < NUM_RESOURCES; ++i) {
                available[i] += request[i];
                allocation[customer][i] -= request[i];
                need[customer][i] += request[i];
            }
        }
    } else {
        std::cout << "Request denied. Not enough resources available." << std::endl;
    }
    cv.notify_all();
}

void releaseResource(int customer) {
    std::unique_lock<std::mutex> lock(mtx);
    std::vector<int> release(NUM_RESOURCES);

    std::cout << "Customer " << customer << " is releasing resources: ";
    for (int i = 0; i < NUM_RESOURCES; ++i) {
        release[i] = std::rand() % (allocation[customer][i] + 1);
        std::cout << release[i] << " ";
    }
    std::cout << std::endl;

    for (int i = 0; i < NUM_RESOURCES; ++i) {
        if (release[i] > allocation[customer][i]) {
            std::cout << "Invalid release request." << std::endl;
            return;
        }
    }

    for (int i = 0; i < NUM_RESOURCES; ++i) {
        available[i] += release[i];
        allocation[customer][i] -= release[i];
        need[customer][i] += release[i];
    }

    cv.notify_all();
}

void customerThread(int customer, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        requestResource(customer);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        releaseResource(customer);
    }
}


int main() {
    
    for (int i = 0; i < NUM_RESOURCES; ++i) {
        available[i] = MAX_NEED;
        for (int j = 0; j < NUM_CUSTOMERS; ++j) {
            maximum[j][i] = std::rand() % (MAX_NEED + 1);
            allocation[j][i] = 0;
            need[j][i] = maximum[j][i];
        }
    }

    int numIterations = 10;

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_CUSTOMERS; ++i) {
        threads.emplace_back(customerThread, i, numIterations);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}
