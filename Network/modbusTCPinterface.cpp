#include "modbusTCPinterface.h"
#include "../debugoutput.h"

ModbusTCPInterface::ModbusTCPInterface()
{
    tcpInterface = 0;
    interfaceData.modbusInterface = this;
    interfaceHandler->AddNewInterface(&interfaceData);
}

ModbusTCPInterface::~ModbusTCPInterface()
{
    // remove interface from handler
    interfaceHandler->RemoveInterface(&interfaceData);
}

bool ModbusTCPInterface::Initialize(GridDevice* device, uint32_t ip, int port)
{
    // variables
    bool result;

    // store grid device
    gridDevice = device;

    // create TCP interface
    //OUTPUT_DEBUG_MESSAGE("Start initializing Modbus TCP interface.")
    //OUTPUT_DEBUG_MESSAGE("Creating a new TCP client interface.")
    tcpInterface = new TCPClientInterface();
    if (!tcpInterface)
    {
        return false;
    }

    // init TCP interface
    //OUTPUT_DEBUG_MESSAGE("Initializing the TCP client interface.")
    result = tcpInterface->Initialize(ip, port);
    if (!result)
    {
        return false;
    }

    // store as network interface
    networkInterface = tcpInterface;

    // add interface to handler
    interfaceData.socket = tcpInterface->GetInterfaceSocket();

    //OUTPUT_DEBUG_MESSAGE("New Modbus TCP Interface created.")

    valid = true;

    return true;
}

void ModbusTCPInterface::Shutdown()
{
    // delete tcp interface
    if (tcpInterface)
    {
        tcpInterface->Shutdown();
        delete tcpInterface;
        tcpInterface = 0;
    }

    // clear some pointers
    gridDevice = 0;
    networkInterface = 0;

    valid = false;

    return;
}

bool ModbusTCPInterface::RecvData()
{
    // variables
    int transfersOngoing;
    bool bResult;
    int iResult;
    GenericModbusRecvPacket_t* packetHeader;
    int payloadLength;
    int transferIndex;

    // check if interface is valid
    if (!valid)
    {
        return false;
    }

    // get lock
    std::unique_lock lk(lock);

    // check if there is anything to read
    transfersOngoing = periodicDataTransfersOngoing;
    if (singleDataTransfer)
    {
        transfersOngoing++;
    }

    // loop until socket is cleared or there are not more transfers ongoing
    do
    {
        // check if socket has something to read
        bResult = tcpInterface->SocketReady();
        if (!bResult)
        {
            // nothing to do, continue
            //OUTPUT_DEBUG_MESSAGE("Socket-Ready returned false")
            return true;
        }

        // read in packet header
        iResult = tcpInterface->RecvData(recvBuffer, MODBUS_PACKET_BASE_LENGTH);
        if (iResult != MODBUS_PACKET_BASE_LENGTH)
        {
            // assume a broken connection
            valid = false;
            return false;
        }

        // get length of packet
        packetHeader = (GenericModbusRecvPacket_t*)recvBuffer;
        payloadLength = mtom(packetHeader->length) - MODBUS_PACKET_LENGTH_OFFSET;

        // get rest of packet
        iResult = tcpInterface->RecvData(recvBuffer + MODBUS_PACKET_BASE_LENGTH, payloadLength);
        if (iResult != payloadLength)
        {
            // something went wrong, return
            valid = false;
            return false;
        }

        // now the packet is fully read in, process it
        // get transfer index
        transferIndex = GetTransferIndex();

        // check in periodic transfers
        for (auto e : periodicTransfers)
        {
            if (e->transferIndex == transferIndex)
            {
                // found it
                e->ongoing = false;
                ProcessPacket(e, payloadLength + MODBUS_PACKET_BASE_LENGTH);
                transfersOngoing--;
                periodicDataTransfersOngoing--;
            }
        }

        // check if index matches with single transfer
        if (singleDataTransfer)
        {
            if (singleDataTransfer->transferIndex == transferIndex)
            {
                singleDataTransfer->ongoing = false;
                ProcessPacket(singleDataTransfer, payloadLength + MODBUS_PACKET_BASE_LENGTH);
                singleDataTransfer = 0;
                transfersOngoing--;
            }
        } 
    } while (transfersOngoing > 0);

    return true;
}