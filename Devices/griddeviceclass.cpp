#include "griddeviceclass.h"
#include "../debugoutput.h"

GridDevice::GridDevice()
{
    selfID = -1;
    periodicDataConnections = 0;
}

GridDevice::~GridDevice()
{
}

bool GridDevice::ConnectDevice()
{
    return false;
}

void GridDevice::OnConnectionFault()
{
    return;
}

bool GridDevice::NewCommand(char* buffer, int size)
{
    return false;
}

void GridDevice::SetConnectionStatus(bool connected)
{
    online = connected;
    return;
}

int GridDevice::GeneratePeriodicData(char* buffer, int maxSize)
{
    return 0;
}

bool GridDevice::CallbackDataTransfer(ModbusDataTransfer* t)
{
    return true;
}

bool GridDevice::CallbackDataTransferError(ModbusDataTransfer* t)
{
    return true;
}

void GridDevice::AddPeriodicDataConnection(uint16_t mask)
{
    OUTPUT_DEBUG_MESSAGE("Live data turned on")
    periodicDataConnections |= mask;
    return;
}

void GridDevice::RemovePeriodicDataConnection(uint16_t mask)
{
    OUTPUT_DEBUG_MESSAGE("Live data turned off")
    periodicDataConnections &= ~mask;
    return;
}

int GridDevice::GetID()
{
    // return ID of device
    return selfID;
}