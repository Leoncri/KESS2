#include "modbusUDPinterface.h"

ModbusUDPInterface::ModbusUDPInterface()
{
    // reset pointers
    udpInterface = 0;
    gridDevice = 0;
}

ModbusUDPInterface::~ModbusUDPInterface()
{
}

bool ModbusUDPInterface::Initialize(GridDevice* device, uint32_t ip, int port)
{
    // variables
    bool result;

    // store variables
    gridDevice = device;

    // create UDP interface
    udpInterface = new UDPClientInterface();
    if (!udpInterface)
    {
        return false;
    }

    // initialize interface
    result = udpInterface->Initialize(ip, port);
    if (!result)
    {
        return false;
    }

    // store as network interface
    networkInterface = udpInterface;

    // add to interface handler
    interfaceData.modbusInterface = this;
    interfaceData.socket = udpInterface->GetInterfaceSocket();
    interfaceHandler->AddNewInterface(&interfaceData);

    return true;
}

void ModbusUDPInterface::Shutdown()
{
    // remove from interface handler
    interfaceHandler->RemoveInterface(&interfaceData);

    // clear UDP interface
    if (udpInterface)
    {
        udpInterface->Shutdown();
        delete udpInterface;
        udpInterface = 0;
    }
    
    gridDevice = 0;
    networkInterface = 0;
}

bool ModbusUDPInterface::RecvData()
{
    // variables
    int transfersOngoing;
    bool bResult;
    int iResult;
    int transferIndex;

    // get lock
    std::unique_lock lk(lock);

    // check if there is anything to read
    transfersOngoing = periodicDataTransfersOngoing;
    if (singleDataTransfer)
    {
        transfersOngoing++;
    }

    // loop until socket is cleared or there are no additional transfers ongoing
    while (transfersOngoing != 0)
    {
        // check if there is something to read
        bResult = udpInterface->SocketReady();
        if (!bResult)
        {
            // nothing to read, return
            return true;
        }

        // read in packet
        iResult = udpInterface->RecvData(recvBuffer, MAX_BUFFER_SIZE);
        if (iResult < 0)
        {
            // something went wrong
            return false;
        }
        else if (iResult == 0)
        {
            // data came from wrong server, continue with next data
            continue;
        }
        
        // translate packet
        transferIndex = GetTransferIndex();

        // check in periodic transfers
        for (auto e : periodicTransfers)
        {
            if (e->transferIndex == transferIndex)
            {
                // found it
                e->ongoing = false;
                ProcessPacket(e, iResult);
                transfersOngoing--;
                periodicDataTransfersOngoing--;

                // check next round
                continue;
            }
        }

        // check if index matches with single transfer
        if (singleDataTransfer->transferIndex == transferIndex)
        {
            singleDataTransfer->ongoing = false;
            ProcessPacket(singleDataTransfer, iResult);
            singleDataTransfer = 0;
            transfersOngoing--;

            // check next round
            continue;
        }
    }

    return true;
}