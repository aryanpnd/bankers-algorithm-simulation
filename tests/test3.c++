#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <random>
#include <string>
#include <queue>

using namespace std;

const int NUM_RESOURCES = 3;
const int NUM_CUSTOMERS = 5;
const int MAX_NEED = 10;

mutex mtx;
condition_variable cv;

vector<int> available(NUM_RESOURCES);
vector<vector<int>> maximum(NUM_CUSTOMERS, vector<int>(NUM_RESOURCES));
vector<vector<int>> allocation(NUM_CUSTOMERS, vector<int>(NUM_RESOURCES));
vector<vector<int>> need(NUM_CUSTOMERS, vector<int>(NUM_RESOURCES));
queue<int> completedProcesses; // Queue to store completed processes


bool isSafeState()
{
    vector<bool> finish(NUM_CUSTOMERS, false);
    vector<int> work(available);

    int completed = 0;
    while (completed < NUM_CUSTOMERS)
    {
        bool found = false;
        for (int i = 0; i < NUM_CUSTOMERS; ++i)
        {
            if (!finish[i])
            {
                bool canAllocate = true;
                for (int j = 0; j < NUM_RESOURCES; ++j)
                {
                    if (need[i][j] > work[j])
                    {
                        canAllocate = false;
                        break;
                    }
                }
                if (canAllocate)
                {
                    for (int j = 0; j < NUM_RESOURCES; ++j)
                    {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    found = true;
                    completed++;
                }
            }
        }
        if (!found)
        {
            return false;
        }
    }
    return true;
}

void requestResource(int customer)
{
    unique_lock<mutex> lock(mtx);
    vector<int> request(NUM_RESOURCES);

    cout << "Customer " << customer << " is requesting resources: ";
    for (int i = 0; i < NUM_RESOURCES; ++i)
    {
        request[i] = rand() % (need[customer][i] + 1);
        cout << request[i] << " ";
    }
    cout << endl;

    bool grantRequest = true;

    for (int i = 0; i < NUM_RESOURCES; ++i)
    {
        if (request[i] > available[i] || request[i] > need[customer][i])
        {
            grantRequest = false;
            break;
        }
    }

    if (grantRequest)
    {
        for (int i = 0; i < NUM_RESOURCES; ++i)
        {
            available[i] -= request[i];
            allocation[customer][i] += request[i];
            need[customer][i] -= request[i];
        }
        cout << "Request granted. System is in a safe state." << endl;

        // Print available resources and resource allocation
        cout << "Available resources: ";
        for (int i = 0; i < NUM_RESOURCES; ++i)
        {
            cout << available[i] << " ";
        }
        cout << endl;
        cout << "Resource allocation: " << endl;
        for (int i = 0; i < NUM_CUSTOMERS; ++i)
        {
            cout << "Customer " << i << ": ";
            for (int j = 0; j < NUM_RESOURCES; ++j)
            {
                cout << allocation[i][j] << " ";
            }
            cout << endl;
        }
    }
    else
    {
        cout << "Request denied. Granting the request would lead to an unsafe state." << endl;
        // Rollback allocation
        for (int i = 0; i < NUM_RESOURCES; ++i)
        {
            available[i] += request[i];
            allocation[customer][i] -= request[i];
            need[customer][i] += request[i];
        }
    }
    cv.notify_all();
}

void releaseResource(int customer)
{
    unique_lock<mutex> lock(mtx);
    vector<int> release(NUM_RESOURCES);

    cout << "Customer " << customer << " is releasing resources: ";
    for (int i = 0; i < NUM_RESOURCES; ++i)
    {
        release[i] = rand() % (allocation[customer][i] + 1); // Release up to the currently allocated amount
        cout << release[i] << " ";
    }
    cout << endl;

    for (int i = 0; i < NUM_RESOURCES; ++i)
    {
        if (release[i] > allocation[customer][i])
        {
            cout << "Invalid release request." << endl;
            return;
        }
    }

    for (int i = 0; i < NUM_RESOURCES; ++i)
    {
        available[i] += release[i];
        allocation[customer][i] -= release[i];
        need[customer][i] += release[i];
    }

    // Print available resources and resource allocation
    cout << "Available resources: ";
    for (int i = 0; i < NUM_RESOURCES; ++i)
    {
        cout << available[i] << " ";
    }
    cout << endl;
    cout << "Resource allocation: " << endl;
    for (int i = 0; i < NUM_CUSTOMERS; ++i)
    {
        cout << "Customer " << i << ": ";
        for (int j = 0; j < NUM_RESOURCES; ++j)
        {
            cout << allocation[i][j] << " ";
        }
        cout << endl;
    }

    cv.notify_all();
}

void customerThread(int customer, int iterations)
{
    for (int i = 0; i < iterations; ++i) {
        requestResource(customer);
        this_thread::sleep_for(chrono::seconds(1));
        releaseResource(customer);
        this_thread::sleep_for(chrono::seconds(1));
    }

    // When a customer completes all iterations, add it to the completedProcesses queue
    completedProcesses.push(customer);
}

int main()
{
    int numIterations = 10;

    // Prompt the user to input available resources
    cout << "Enter the available resources:" << endl;
    for (int i = 0; i < NUM_RESOURCES; ++i) {
        cout << "Resource " << i << ": ";
        cin >> available[i];
    }

    // Prompt the user to input maximum needs for each customer
    for (int i = 0; i < NUM_CUSTOMERS; ++i) {
        cout << "Enter maximum needs for Customer " << i << ":" << endl;
        for (int j = 0; j < NUM_RESOURCES; ++j) {
            cout << "Resource " << j << ": ";
            cin >> maximum[i][j];
            allocation[i][j] = 0;
            need[i][j] = maximum[i][j];
        }
    }

    vector<thread> threads;
    for (int i = 0; i < NUM_CUSTOMERS; ++i) {
        threads.emplace_back(customerThread, i, numIterations);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Print the sequence of completed processes
    cout << "Sequence of completed processes: ";
    while (!completedProcesses.empty()) {
        cout << "Customer " << completedProcesses.front() << " -> ";
        completedProcesses.pop();
    }
    cout << endl;

    return 0;
}
