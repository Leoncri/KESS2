#include "modbusinterfacehandler.h"
#include "../debugoutput.h"

ModbusInterfaceHandler::ModbusInterfaceHandler()
{
    sendThreadRunning = false;
    recvThreadRunning = false;
    stopSendThread = true;
    stopRecvThread = true;
}

ModbusInterfaceHandler::~ModbusInterfaceHandler()
{
}

bool ModbusInterfaceHandler::Initialize()
{
    // check if threads are already running
    if (recvThreadRunning)
    {
        return false;
    }

    // start recv thread
    stopRecvThread = false;

    recvThread = std::thread(RecvThreadCallback, this);

    return true;
}

void ModbusInterfaceHandler::Shutdown()
{
    // check if send thread is still running
    if (sendThreadRunning)
    {
        // stop it
        StopInterface();
    }

    if (!recvThreadRunning)
    {
        // nothing to do here
        return;
    }

    // ask recv thread to stop
    stopRecvThread = true;

    // wait for thread to stop
    recvThread.join();

    return;
}

bool ModbusInterfaceHandler::StartInterface()
{
    // check if send therad is already running
    if (sendThreadRunning)
    {
        OUTPUT_DEBUG_MESSAGE("Modbus interface handler already running")
        return false;
    }

    // check if recv thread is already running
    if (!recvThreadRunning)
    {
        OUTPUT_DEBUG_MESSAGE("Modbus interface handler not started - receiving thread not running")
        return false;
    }

    // start send thread
    stopSendThread = false;

    sendThread = std::thread(SendThreadCallback, this);

    return true;
}

void ModbusInterfaceHandler::StopInterface()
{
    // ask thread to stop
    stopSendThread = true;

    // wait for thread to stop
    OUTPUT_DEBUG_MESSAGE("Waiting for the modbus interface handler to stop the interfaces...")
    sendThread.join();
    OUTPUT_DEBUG_MESSAGE("Done")

    return;
}

bool ModbusInterfaceHandler::AddNewInterface(ModbusInterfaceData* mid)
{
    // get lock
    std::unique_lock lk(dataLock);

    // first check if interface is already in list
    for (auto e : modbusInterfaces)
    {
        if (e == mid)
        {
            OUTPUT_DEBUG_MESSAGE("Interface already in list")
            return false;
        }
    }

    // append to list
    OUTPUT_DEBUG_MESSAGE("Adding interface to list")
    modbusInterfaces.push_back(mid);

    return true;
}

void ModbusInterfaceHandler::RemoveInterface(ModbusInterfaceData* mid)
{
    // get lock
    std::unique_lock lk(dataLock);

    // just remove interfaces
    OUTPUT_DEBUG_MESSAGE("Removing interface from list")
    modbusInterfaces.remove(mid);

    return;
}

void ModbusInterfaceHandler::PeriodicSendCallback()
{
    // variables

    // set thread to running
    sendThreadRunning = true;
    OUTPUT_DEBUG_MESSAGE("Modbus Interface Send Thread started")

    // thread main loop
    while (!stopSendThread)
    {
        // calculate time when function should be called next
		auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(MODBUS_PERIODIC_DATA_PERIOD_MS);

        //OUTPUT_DEBUG_MESSAGE("Modbus Interface Send Thread invoked")
        // send data
        for (auto e : modbusInterfaces)
        {
            //OUTPUT_DEBUG_MESSAGE("Sending modbus interface data")
            e->modbusInterface->SendPeriodicData();
            //OUTPUT_DEBUG_MESSAGE("Sending modbus interface data done")
        }

        // wait until next round
        if (x > std::chrono::steady_clock::now())
		{
			std::this_thread::sleep_until(x);
		}
    }

    // thread is stopped
    sendThreadRunning = false;
    OUTPUT_DEBUG_MESSAGE("Modbus Interface Send Thread stopped")

    return;
}

void ModbusInterfaceHandler::ReadCallback()
{
    // variables
    fd_set fds;
    fd_set error_fds;
#ifndef USE_WINDOWS
    int maxSocket;
#endif
    struct timeval tmv;
    bool result;

    // set thread to running
    recvThreadRunning = true;

    // thread main loop
    while (!stopRecvThread)
    {
        // check if there is an interface to handle
        if (modbusInterfaces.size() == 0)
        {
            //OUTPUT_DEBUG_MESSAGE("Nothing to do in interface handler")
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // clear fd_set structure
        FD_ZERO(&fds);
        FD_ZERO(&error_fds);
#ifndef USE_WINDOWS
        maxSocket = -1;
#endif

        // set sockets
        for (auto e : modbusInterfaces)
        {
            if(e->modbusInterface->IsValid())
            {
                FD_SET(e->socket, &fds);
                FD_SET(e->socket, &error_fds);
#ifndef USE_WINDOWS
                if (e->socket > maxSocket) maxSocket = e->socket;
#endif
            }
        }

        // generate timeout structure
        tmv.tv_sec = 0;
        tmv.tv_usec = 100000;   // 100 ms

        // call select
#ifdef USE_WINDOWS
        select(0, &fds, NULL, &error_fds, &tmv);
#else
        select(maxSocket + 1, &fds, NULL, NULL, &tmv);
#endif

        // check which socket is ready
        //OUTPUT_DEBUG_MESSAGE("Doing some stuff for " << modbusInterfaces.size() << " interfaces")
        for (auto e : modbusInterfaces)
        {
            if (!e->modbusInterface->IsValid())
            {
                continue;
            }

            if (FD_ISSET(e->socket, &fds))
            {
                //OUTPUT_DEBUG_MESSAGE("Something to do for a socket")
                result = e->modbusInterface->RecvData();
                if (!result)
                {
                    // store broken connection
                    OUTPUT_DEBUG_MESSAGE("Broken connection by recv function")
                    e->modbusInterface->OnBrokenConnectionCallback();
                }
            }

            if (FD_ISSET(e->socket, &error_fds))
            {
                // assume a broken connection as socket cannot be written
                OUTPUT_DEBUG_MESSAGE("Broken connection by error_fds")
                e->modbusInterface->OnBrokenConnectionCallback();
            }
        }
    }

    // thread is stopped
    recvThreadRunning = false;

    return;
}

void SendThreadCallback(ModbusInterfaceHandler* handler)
{
    // just call thread function
    handler->PeriodicSendCallback();

    return;
}

void RecvThreadCallback(ModbusInterfaceHandler* handler)
{
    // call recv thread function
    handler->ReadCallback();

    return;
}