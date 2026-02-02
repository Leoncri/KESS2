#include "eventhandler.h"

EventHandler::EventHandler()
{
    events = 0;
}

EventHandler::~EventHandler()
{
}

void EventHandler::WaitForEvent()
{
    // get access to the events
    std::unique_lock lk(m);

    if (events)
    {
        events = false;
        return;
    }

    // wait
    cv.wait(lk, [&]{return events;});

    return;
}

void EventHandler::SetEvent()
{
    // get access to the events
    std::unique_lock lk(m);

    // set event
    events = true;

    // notify waiting thread
    lk.unlock();
    cv.notify_one();

    return;
}