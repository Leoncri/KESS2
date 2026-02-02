#include "backgroundtask.h"
#include "../debugoutput.h"

bool ReconnectDevice(GridDevice* device)
{
    //OUTPUT_DEBUG_MESSAGE("Trying to reconnect device")
    return device->ConnectDevice();
}

BackgroundTask::BackgroundTask()
{
}

BackgroundTask::~BackgroundTask()
{
}

void BackgroundTask::TaskFunction()
{
    // do something
    HandleAutomaticReconnect();

    return;
}

void BackgroundTask::AddDeviceForAutomaticReconnect(GridDevice* device)
{
    // add device to list
    std::unique_lock lk(dataLock);

    for (auto e : reconnectionDeviceList)
    {
        if (e == device)
        {
            // already in list
            return;
        }
    }

    reconnectionDeviceList.push_back(device);

    return;
}

void BackgroundTask::HandleAutomaticReconnect()
{
    // variables
    //bool result;

    // iterate over all devices and call connect
    std::unique_lock lk(dataLock);

    reconnectionDeviceList.remove_if(ReconnectDevice);

    return;
}