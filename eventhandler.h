#pragma once

#include <mutex>
#include <condition_variable>
#include <stdint.h>

class EventHandler
{
public:
    // constructor and destructor
    EventHandler();
    ~EventHandler();

    // Wait for event
    void WaitForEvent();

    // Set event
    void SetEvent();

private:
    // mutex and stuff
    std::mutex m;
    std::condition_variable cv;
    bool events;
};