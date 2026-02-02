#pragma once

#include "modbusinterface.h"
#include "udpclientinterface.h"

class ModbusUDPInterface : public ModbusInterface
{
public:
    // constructor and destructor
    ModbusUDPInterface();
    ~ModbusUDPInterface();

    // init and shutdown
    bool Initialize(GridDevice* device, uint32_t ip, int port);
    void Shutdown();

    // recv function
    bool RecvData();

private:
    // UDP Interface
    UDPClientInterface* udpInterface;
};