#pragma once

#include "modbusinterface.h"
#include "tcpclientinterface.h"

class ModbusTCPInterface : public ModbusInterface
{
public:
    // Constructor and destructor
    ModbusTCPInterface();
    ~ModbusTCPInterface();

    // init and shutdown
    bool Initialize(GridDevice* device, uint32_t ip, int port);
    void Shutdown();

    // recv function
    bool RecvData();

private:
    // TCP client interface
    TCPClientInterface* tcpInterface;
};