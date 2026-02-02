#pragma once

class SciBreakBreaker;

#include "griddeviceclass.h"
#include "gridnodeclass.h"
#include "gridbreakerclass.h"
#include "../Grid/gridhandler.h"
#include "../Auxillary/backgroundtask.h"
#include "../Communication/modbusconverterinterface.h"
#include "../Communication/breakercommunicationdefines.h"

#include <mutex>

#include "../config.h"

#define MODBUS_TURN_ON		0x0001
#define MODBUS_TURN_OFF		0x0002
#define MODBUS_CLOSE_TOP	0x0004
#define MODBUS_CLOSE_BOT	0x0008
#define MODBUS_OPEN_TOP		0x0010
#define MODBUS_OPEN_BOT		0x0020

class SciBreakBreaker : public GridDevice
{
public:
	// constructor and destructor
	SciBreakBreaker();
	~SciBreakBreaker();

	// init and shutdown
	bool Initialize(GridHandler* handler, uint16_t id, GridNode* n1, GridNode* n2, uint32_t ip, uint16_t port, uint16_t config);
	void Shutdown();

	// connection handling
	bool ConnectDevice();
	void OnConnectionFault();

	// data handling
	bool CallbackDataTransfer(ModbusDataTransfer* t);
	void ProcessPeriodicRecvData();

	// new command and periodic data
	bool NewCommand(char* buffer, int size);
	void SendPeriodicData();

	// update function
	bool Update();

private:
	// private functions
	void ProcessRecvBuffer();
	void SendRespondPacket(uint16_t connection, uint32_t id, uint32_t respond);
	void SendPeriodicDataPacket(uint16_t connection, uint32_t id);
	void SetBreakerState(bool closed);
	void TurnOnBreaker(bool on);
	void SetBreakerTripLevel(uint16_t level);

private:
	// private variables
	// input and output buffer for commands
	char recvBuffer[MAX_BUFFER_SIZE];
	char sendBuffer[MAX_BUFFER_SIZE];
	char periodicDataBuffer[sizeof(BreakerDeviceData_t)];

	std::mutex recvBufferLock;
	std::mutex sendBufferLock;

	// communication
	ModbusTCPInterface* modbusInterface;
	bool recvBufferReady;

	// breaker data
	uint16_t devicePort;
	ModbusDataTransfer* modbusPeriodicRead;
	ModbusDataTransfer* modbusCommand;
	ModbusDataTransfer* modbusSetTripLevel;
	// TODO

	// misc
	GridHandler* gridHandler;
	uint32_t deviceIP;
	uint16_t deviceConfig;
	GridNode* side1Node;
	GridNode* side2Node;

	// grid breaker
	GridBreaker* deviceBreaker;

	// modbus buffer
	uint16_t modbusPeriodicTransferBuffer[8];
	char modbusCommandBuffer[sizeof(uint16_t)];
	char modbusTripLevelBuffer[sizeof(uint16_t)];

	// device data
	uint32_t breakerStatusTop, breakerStatusBot;
	int voltageTop, voltageBot;
	int currentTop, currentBot;
	int tripLevelTop, tripLevelBot;
};