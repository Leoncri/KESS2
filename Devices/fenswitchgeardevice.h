#pragma once

class FENSwitchgear;

#include "griddeviceclass.h"
#include "gridnodeclass.h"
#include "gridbreakerclass.h"
#include "../Grid/gridhandler.h"
#include "../Communication/fenswitchgearcommunicationdefines.h"
#include "../Network/modbusTCPinterface.h"
#include "../Auxillary/backgroundtask.h"

#include <mutex>

#include "../config.h"

#define WAGO_AN_DATA_MASK       0x7FFC
#define WAGO_AN_DATA_RANGE      32768.0
#define WAGO_CURRENT_SCALING    2500.0
#define WAGO_CURRENT_OFFSET     1250.0
#define WAGO_VOLTAGE_SCALING    3000.0

#define SWITCH_LOCK_THRESHOLD   100

typedef struct
{
    float voltageP_Offset;
    float voltageP_Gain;
    float voltageM_Offset;
    float voltageM_Gain;
    float current_Offset[6];
    float current_Gain[6];
} SensorCorrectionTable_t;

class FENSwitchgear : public GridDevice
{
public:
    // constructor and destructor
    FENSwitchgear();
    ~FENSwitchgear();

    // init and shutdown switchgear
    bool Initialize(GridHandler* handler, uint16_t id,  GridNode* n1, GridNode* n2, GridNode* n3, GridNode* n4, uint32_t ip, uint16_t port, uint16_t config);
    void Shutdown();

    // connection handling
    bool ConnectDevice();
    void OnConnectionFault();

    // data handling
    bool CallbackDataTransfer(ModbusDataTransfer* t);

    // get voltages and currents
    float GetSwitchgearVoltage();
    float GetPortCurrent(int port);

    // new command and periodic data
    bool NewCommand(char* buffer, int size);
    void SendPeriodicData();

    // update function
    bool Update();

private:
    // processes the recv buffer
    void ProcessRecvBuffer();

    // return packet functions for commands
    void SendRespondPacket(uint16_t connection, uint32_t id, uint32_t respond);
    void SendPeriodicDataPacket(uint16_t connection, uint32_t id);

    // change switch status
    void SetSwitchStatus(uint32_t mask, bool closed);

    // process the data inside the periodic recv buffer
    void ProcessPeriodicRecvData();

private:
    // input and output buffer for commands
    char recvBuffer[MAX_BUFFER_SIZE];
    char sendBuffer[MAX_BUFFER_SIZE];
    char periodicDataBuffer[sizeof(FENSwitchgearDeviceData_t)];
    bool recvBufferReady;

    std::mutex recvBufferLock;
    std::mutex sendBufferLock;

    // switchgear communication
    ModbusTCPInterface* modbusInterface;
    ModbusDataTransfer* modbusPeriodicRead;
    ModbusDataTransfer* modbusWriteCommand;
    uint16_t modbusPeriodicTransferBuffer[9];
    uint8_t modbusSingleTransferBuffer[1];
    SensorCorrectionTable_t* sensorCorrection;

    // switchgear data
    int deviceCurrents[4];
    int deviceVoltageP;
    int deviceVoltageM;
    GridNode* deviceNode;
    bool deviceSwitchStatus[5];
    bool deviceSwitchLocked[5];

    // breaker devices
    GridBreaker* deviceSwitches;

    // misc
    GridHandler* gridHandler;
    uint32_t deviceIP;
    uint16_t devicePort;
    uint16_t deviceConfig;
};