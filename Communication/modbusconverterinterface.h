#pragma once

#include "../Devices/griddeviceclass.h"
#include "../Communication/converterDataStructure.h"
#include "../Network/modbusTCPinterface.h"
#include "../config.h"

class ModbusConverterInterface
{
public:
	// constructor and destructor
	ModbusConverterInterface();
	~ModbusConverterInterface();

	// init and shutdown
	bool Initialize(GridDevice* device, uint32_t addr, converterDataStructure* converter2Server, converterDataStructure* server2Converter);
	void Shutdown();

	bool Connect();
	void OnDisconnect();

	// communication
	bool SetNewMode();

private:
	// private functions

private:
	// Device
	GridDevice* converter;

	// device address
	uint32_t ip;

	// data structures
	converterDataStructure* c2sData;
	converterDataStructure* s2cData;

	// tcp interface
	ModbusTCPInterface* modbusInterface;
	ModbusDataTransfer* periodicReadTransfer;
	ModbusDataTransfer* periodicWriteTransfer;
	ModbusDataTransfer* modeTransfer;


};