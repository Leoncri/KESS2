#pragma once

#include <list>
#include <mutex>

#include "../Devices/griddeviceclass.h"

class BackgroundTask
{
public:
    BackgroundTask();
    ~BackgroundTask();

    // main task function to be called in background thread
    static void TaskFunction();

    // function for adding a device to the automatic reconnect list
    static void AddDeviceForAutomaticReconnect(GridDevice* device);

private:
    // function for handling specific tasks
    static void HandleAutomaticReconnect();

public:
    // variables
    inline static std::mutex dataLock;

    inline static std::list<GridDevice*> reconnectionDeviceList;
};