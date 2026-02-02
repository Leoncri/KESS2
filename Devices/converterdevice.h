#pragma once

class Converter;

#include "griddeviceclass.h"
#include "gridnodeclass.h"
#include "../Grid/gridhandler.h"
#include "../Auxillary/backgroundtask.h"
#include "../ADS/adsconverterinterface.h"
#include "../Communication/modbusconverterinterface.h"
#include "../Communication/convertercommunicationdefines.h"

#include <mutex>

#include "../config.h"

class Converter : public GridDevice
{
public:
	// contructor and destructor
	Converter();
	~Converter();

	// init and shutdown
	bool Initialize(GridHandler* handler, uint16_t id, GridNode* n1, GridNode* n2, uint32_t ip, uint16_t config);
	void Shutdown();

	// connection handling
	bool ConnectDevice();
	void OnConnectionFault();

	// get voltages
	float GetVoltage(int side);

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
	void UpdateConverterData(int mode);
	void UpdateConverterMode(int mode);

private:
	// input and output buffer for commands
	char recvBuffer[MAX_BUFFER_SIZE];
	char sendBuffer[MAX_BUFFER_SIZE];
	
	std::mutex recvBufferLock;
	std::mutex sendBufferLock;

	// communication
	AdsConverterInterface* adsInterface;
	ModbusConverterInterface* modbusInterface;
	bool useADS;
	bool recvBufferReady;

	// converter data
	converterDataStructure s2cData;
	converterDataStructure c2sData;
	
	// misc
	GridHandler* gridHandler;
	uint32_t deviceIP;
	uint16_t deviceConfig;
	GridNode* side1Node;
	GridNode* side2Node;
};