#pragma once

class ModbusInterfaceHandler;

#include "../config.h"
#include "networkinterface.h"
#include "modbusinterface.h"
#include <list>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>

#ifdef USE_WINDOWS
#include <WinSock2.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

class ModbusInterfaceHandler
{
public:
    // constructor and destructor
    ModbusInterfaceHandler();
    ~ModbusInterfaceHandler();

    // start and stop interface
    bool Initialize();
    void Shutdown();
    bool StartInterface();
    void StopInterface();

    // callback functions for threads
    void PeriodicSendCallback();
    void ReadCallback();

    // functions to register a new interface
    bool AddNewInterface(ModbusInterfaceData* ModbusInterface);
    void RemoveInterface(ModbusInterfaceData* modbusInterface);

private:
    // functions

private:
    // list to store interfaces
    std::list<ModbusInterfaceData*> modbusInterfaces;

    // thread variables
    std::thread sendThread;
    std::thread recvThread;

    bool sendThreadRunning;
    bool recvThreadRunning;
    bool stopRecvThread;
    bool stopSendThread;

    // lock
    std::mutex dataLock;
};

void SendThreadCallback(ModbusInterfaceHandler* handler);
void RecvThreadCallback(ModbusInterfaceHandler* handler);