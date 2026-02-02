#include "fenswitchgeardevice.h"
#include "../debugoutput.h"

static SensorCorrectionTable_t sw201Table = { 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
static SensorCorrectionTable_t sw202Table = { 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
static SensorCorrectionTable_t initTable = { 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };

FENSwitchgear::FENSwitchgear()
{
    // clear all variables
    modbusInterface = 0;
    modbusPeriodicRead = 0;
    modbusWriteCommand = 0;
    deviceNode = 0;
    deviceSwitches = 0;
    recvBufferReady = false;
    online = false;
}

FENSwitchgear::~FENSwitchgear()
{
}

bool FENSwitchgear::Initialize(GridHandler* handler, uint16_t id, GridNode* n1, GridNode* n2, GridNode* n3, GridNode* n4, uint32_t ip, uint16_t port, uint16_t config)
{
    // store variables
    gridHandler = handler;
    deviceIP = ip;
    devicePort = port;
    deviceConfig = config;
    selfID = id;

    // variables
    bool bResult;

    // start init with creating some classes
    OUTPUT_DEBUG_MESSAGE("Creaating 4 new breakers for a switchgear")
    deviceSwitches = new GridBreaker[4];
    if (!deviceSwitches)
    {
        return false;
    }

    OUTPUT_DEBUG_MESSAGE("Creating a new Modbus TCP interface")
    modbusInterface = new ModbusTCPInterface();
    if (!modbusInterface)
    {
        return false;
    }

    modbusPeriodicRead = new ModbusDataTransfer(DATA_TRANSFER_TYPE_READ | DATA_TRANSFER_TYPE_REGISTER, 0, 9, (void*)&modbusPeriodicTransferBuffer);
    if (!modbusPeriodicRead)
    {
        return false;
    }

    modbusWriteCommand = new ModbusDataTransfer(DATA_TRANSFER_TYPE_WRITE | DATA_TRANSFER_TYPE_COIL, 0, 5, (void*)&modbusSingleTransferBuffer);
    if (!modbusWriteCommand)
    {
        return false;
    }

    deviceNode = new GridNode(-1);
    if (!deviceNode)
    {
        return false;
    }

    // setup sensor correction table
    uint8_t ipLast = ip & 0xFF;
    if (ipLast == 201)
    {
        sensorCorrection = &sw201Table;
    }
    else if (ipLast == 202)
    {
        sensorCorrection = &sw202Table;
    }
    else
    {
        sensorCorrection = &initTable;
    }

    // try to connect the device
    bResult = ConnectDevice();
    if (!bResult)
    {
        SetConnectionStatus(false);
        OUTPUT_DEBUG_MESSAGE("Device offline")

        // append to background task for reconnect
        BackgroundTask::AddDeviceForAutomaticReconnect(this);
    }

    // setup switches
    deviceSwitches[0].Initialize(n1, deviceNode);
    deviceSwitches[1].Initialize(n2, deviceNode);
    deviceSwitches[2].Initialize(n3, deviceNode);
    deviceSwitches[3].Initialize(n4, deviceNode);

    // set voltage to 0 for now
    deviceSwitches[0].SetVoltage(1, 0.0);

    OUTPUT_DEBUG_MESSAGE("New switchgear initialized.")

    return true;
}

void FENSwitchgear::Shutdown()
{
    // delete switches
    if (deviceSwitches)
    {
        deviceSwitches[0].Shutdown();
        deviceSwitches[1].Shutdown();
        deviceSwitches[2].Shutdown();
        deviceSwitches[3].Shutdown();

        delete[] deviceSwitches;
        deviceSwitches = NULL;
    }

    // delete internal node
    if (deviceNode)
    {
        delete deviceNode;
        deviceNode = NULL;
    }

    // stop and delete modbus communication
    if (modbusInterface)
    {
        modbusInterface->Shutdown();
        delete modbusInterface;
        modbusInterface = 0;
    }

    // delete modbus data transfers
    if (modbusPeriodicRead)
    {
        delete modbusPeriodicRead;
        modbusPeriodicRead = 0;
    }

    if (modbusWriteCommand)
    {
        delete modbusWriteCommand;
        modbusWriteCommand = 0;
    }

    return;
}

bool FENSwitchgear::ConnectDevice()
{
    // variables
    bool bResult;

    // check if device is already connected
    if (online)
    {
        return true;
    }

    // setup
    bResult = modbusInterface->Initialize(this, deviceIP, devicePort);
    if (!bResult)
    {
        modbusInterface->Shutdown();
        return false;
    }
    
    bResult = modbusInterface->AddPeriodicTransfer(modbusPeriodicRead);
    if (!bResult)
    {
        modbusInterface->Shutdown();
        return false;
    }
    
    SetConnectionStatus(true);
    return true;

}

void FENSwitchgear::OnConnectionFault()
{
    // this function is called when there is a connection fault
    OUTPUT_DEBUG_MESSAGE("Device going offline")
    SetConnectionStatus(false);

    // shutdown the interface
    if (modbusInterface)
    {
        modbusInterface->Shutdown();
    }

    // append device for reconnection
    BackgroundTask::AddDeviceForAutomaticReconnect(this);

    return;
}

bool FENSwitchgear::CallbackDataTransfer(ModbusDataTransfer* t)
{
    if (t == modbusPeriodicRead)
    {
        // process periodic recv data
        ProcessPeriodicRecvData();
    }
    return true;
}

float FENSwitchgear::GetSwitchgearVoltage()
{
    return (float)(deviceVoltageP + deviceVoltageM);
}

float FENSwitchgear::GetPortCurrent(int port)
{
    if (port < 0 || port >= 4)
    {
        return -1.0;
    }
    return (float)(deviceCurrents[port]);
}

bool FENSwitchgear::NewCommand(char* buffer, int size)
{
    // variables

    // check size
    if (size > MAX_BUFFER_SIZE)
    {
        return false;
    }

    // get access to the buffers
    std::unique_lock lk(recvBufferLock);

    // copy data
    if (recvBufferReady)
    {
        // send back failure
        return false;
    }

    memcpy(recvBuffer, buffer, size);

    // set receive buffer to ready
    recvBufferReady = true;

    // notify grid server that this class needs to be updated
    gridHandler->SubsystemProcess(SUBSYSTEM_PROCESS_FENSWITCHGEAR);

    return true;
}

bool FENSwitchgear::Update()
{
    // this funciton will check the command data and call ProcessRecvBuffer

    // get lock
    std::unique_lock lk(recvBufferLock);
    
    // check if new data has arrived
    if (!recvBufferReady)
    {
        // nothing to do here
        return true;
    }

    // transform data into readable packets
    GenericCommandPacketHeader_t* header = (GenericCommandPacketHeader_t*)(recvBuffer);

    // check if this packet is for grid file
    if (header->header.deviceType != DEVICE_TYPE_FENSWITCHGEAR || header->header.deviceId != selfID || header->header.packetType != PACKET_TYPE_COMMAND)
    {
        // nothing to do here
        recvBufferReady = false;

        return true;
    }

    // process specific commands
    ProcessRecvBuffer();

    recvBufferReady = false;

    return true;
}

void FENSwitchgear::SendPeriodicData()
{
    // calculate the actual data and send back
    SendPeriodicDataPacket(periodicDataConnections, 0);
    return;
}

void FENSwitchgear::ProcessRecvBuffer()
{
    // variables
    GenericCommandPacketHeader_t* header = (GenericCommandPacketHeader_t*)recvBuffer;
    uint32_t command = header->command;

    // process command
    if (command & FENSWITCHGEAR_COMMAND_SET_SWITCH)
    {
        // Try to close a switch
        SetSwitchStatus(command & FENSWITCHGEAR_SWITCH_MASK, true);
        SendRespondPacket(header->header.connection, header->commandId, FENSWITCHGEAR_RESPOND_SUCCESS);
    }
    else if (command & FENSWITCHGEAR_COMMAND_RESET_SWITCH)
    {
        // Try to open a switch
        SetSwitchStatus(command & FENSWITCHGEAR_SWITCH_MASK, false);
        SendRespondPacket(header->header.connection, header->commandId, FENSWITCHGEAR_RESPOND_SUCCESS);
    }
    else if (command & FENSWITCHGEAR_COMMAND_GET_DATA)
    {
        // Get data from the switchgear
        SendPeriodicDataPacket(header->header.connection, header->commandId);
    }
    else if (command & FENSWITCHGEAR_COMMAND_PERIODIC_DATA)
    {
        // Add the connection to the periodic data receivers
        if (command & FENSWICHTGEAR_COMMAND_PERIODIC_DATA_ON)
        {
            AddPeriodicDataConnection(header->header.connection);
        }
        else
        {
            RemovePeriodicDataConnection(header->header.connection);
        }
        SendRespondPacket(header->header.connection, header->commandId, FENSWITCHGEAR_RESPOND_SUCCESS);
    }
        
    return;
}

void FENSwitchgear::SendRespondPacket(uint16_t connection, uint32_t id, uint32_t respond)
{
    // lock send buffer first
    std::unique_lock lk(sendBufferLock);

    // clear buffer
    memset(sendBuffer, 0, sizeof(GenericCommandRespondHeader_t));

    // create package
    GenericCommandRespondHeader_t* r = (GenericCommandRespondHeader_t*)(sendBuffer);
    r->header.packetType = PACKET_TYPE_RESPOND;
    r->header.deviceType = DEVICE_TYPE_FENSWITCHGEAR;
    r->header.deviceId = 0;
    r->header.length = sizeof(GenericCommandRespondHeader_t);
    r->header.connection = connection;
    r->commandId = id;
    r->result = respond;

    // send data
    gridHandler->SendData(sendBuffer, sizeof(GenericCommandRespondHeader_t));

    return;
}

void FENSwitchgear::SendPeriodicDataPacket(uint16_t connection, uint32_t id)
{
    // variables
    FENSwitchgearDeviceData_t* packet = (FENSwitchgearDeviceData_t*)periodicDataBuffer;
    uint8_t switchState = 0;
    uint8_t switchLock = 0;

    // build up status values
    for (int i = 0; i < 5; i++)
    {
        if (deviceSwitchStatus[i])
        {
            switchState |= 0x1 << i;
        }
        if (deviceSwitchLocked[i])
        {
            switchLock |= 0x1 << i;
        }
    }

    // setup packet
    packet->header.header.connection = connection;
    packet->header.header.deviceId = selfID;
    packet->header.header.deviceType = DEVICE_TYPE_FENSWITCHGEAR;
    packet->header.header.length = sizeof(FENSwitchgearDeviceData_t);
    packet->header.header.packetType = PACKET_TYPE_DEVICEDATA;
    packet->header.id = id;
    packet->closedSwitches = switchState;
    packet->lockedSwitches = switchLock;
    packet->hvOnLine = 0;

    packet->status = 0;
    if (online)
    {
        packet->status |= FENSWITCHGEAR_STATUS_ONLINE;
    }    

    packet->voltageP = deviceVoltageP;
    packet->voltageM = deviceVoltageM;
    for (int i = 0; i < 4; i++)
    {
        packet->currents[i] = (uint16_t)(deviceCurrents[i] + WAGO_CURRENT_OFFSET);
    }

    gridHandler->SendData(periodicDataBuffer, sizeof(FENSwitchgearDeviceData_t));

    return;
}

void FENSwitchgear::SetSwitchStatus(uint32_t mask, bool closed)
{
    // variables
    bool changed = false;
    bool result;

    //OUTPUT_DEBUG_MESSAGE("Changing switch status to " << mask)

    // check if device is connected
    if (!online)
    {
        return;
    }

    // iterate over all switches and check if mask bit is set
    modbusSingleTransferBuffer[0] = 0;
    for (int i = 0; i < 5; i++)
    {
        if (mask & (0x1 << i))
        {
            if (!deviceSwitchLocked[i])
            {
                if (closed)
                {
                    modbusSingleTransferBuffer[0] |= 0x1 << i;
                }
                else
                {
                    modbusSingleTransferBuffer[0] &= ~(0x1 << i);
                }
                changed = true;
            }
        }
        else
        {
            if (deviceSwitchStatus[i])
            {
                modbusSingleTransferBuffer[0] |= 0x1 << i;
            }
        }
    }

    if (!changed)
    {
        return;
    }

    // send single data transfer
    result = modbusInterface->NewSingleTransfer(modbusWriteCommand);
    if (!result)
    {
        OUTPUT_DEBUG_MESSAGE("Cannot add a new single transfer")
    }

    return;
}

void FENSwitchgear::ProcessPeriodicRecvData()
{
    // variables
    float currentRaw[6];

    // get raw values
    for (auto i = 0; i < 6; i++)
    {
        currentRaw[i] = (((float)(modbusPeriodicTransferBuffer[i]) * WAGO_CURRENT_SCALING / WAGO_AN_DATA_RANGE - WAGO_CURRENT_OFFSET) - sensorCorrection->current_Offset[i]) * sensorCorrection->current_Gain[i];
    }
    //OUTPUT_DEBUG_MESSAGE("Voltage P: " << modbusPeriodicTransferBuffer[6])
    deviceVoltageP = (((float)(modbusPeriodicTransferBuffer[6]) * WAGO_VOLTAGE_SCALING / WAGO_AN_DATA_RANGE) + sensorCorrection->voltageP_Offset) * sensorCorrection->voltageP_Gain;
    deviceVoltageM = (((float)(modbusPeriodicTransferBuffer[7]) * WAGO_VOLTAGE_SCALING / WAGO_AN_DATA_RANGE) + sensorCorrection->voltageM_Offset) * sensorCorrection->voltageM_Gain;

    // get switch status
    //OUTPUT_DEBUG_MESSAGE("Switch status: " << modbusPeriodicTransferBuffer[8])
    for (auto i = 0; i < 5; i++)
    {
        if (modbusPeriodicTransferBuffer[8] & 0x1 << i)
        {
            deviceSwitchStatus[i] = true;
        }
        else
        {
            deviceSwitchStatus[i] = false;
        }
    }

    // calculate actual switch currents
    if (deviceSwitchStatus[0])
    {
        deviceCurrents[0] = (int)(currentRaw[0] + currentRaw[1]) / 2;
    }
    else
    {
        deviceCurrents[0] = 0;
    }

    if (deviceSwitchStatus[1])
    {
        deviceCurrents[1] = (int)((currentRaw[2] + currentRaw[3] - currentRaw[0] - currentRaw[1]) / 2);
    }
    else
    {
        deviceCurrents[1] = 0;
    }

    if (deviceSwitchStatus[2])
    {
        deviceCurrents[2] = (int)((currentRaw[4] + currentRaw[5] - currentRaw[2] - currentRaw[3]) / 2) - deviceCurrents[1];
    }
    else
    {
        deviceCurrents[2] = 0;
    }
    
    if (deviceSwitchStatus[3])
    {
        deviceCurrents[3] = -(int)(currentRaw[4] + currentRaw[5]) / 2;
    }
    else
    {
        deviceCurrents[3] = 0;
    }

    // lock switches
    for (auto i = 0; i < 5; i++)
    {
        if (deviceCurrents[i] > SWITCH_LOCK_THRESHOLD)
        {
            deviceSwitchLocked[i] = true;
        }
        else
        {
            deviceSwitchLocked[i] = false;
        }
    }
    
    return;
}