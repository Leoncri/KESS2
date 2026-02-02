#include "modbusconverterinterface.h"

ModbusConverterInterface::ModbusConverterInterface()
{
	ip = 0;
	c2sData = NULL;
	s2cData = NULL;
	modbusInterface = NULL;
	periodicReadTransfer = NULL;
	periodicWriteTransfer = NULL;
	modeTransfer = NULL;
	converter = NULL;
}

ModbusConverterInterface::~ModbusConverterInterface()
{
}

bool ModbusConverterInterface::Initialize(GridDevice* device, uint32_t addr, converterDataStructure* converter2Server, converterDataStructure* server2Converter)
{
	// variables
	bool result;

	// store parameters
	converter = device;
	ip = addr;
	c2sData = converter2Server;
	s2cData = server2Converter;

	// create modbus interface
	modbusInterface = new ModbusTCPInterface();
	if (!modbusInterface)
	{
		return false;
	}

	// create the transfers
	periodicReadTransfer = new ModbusDataTransfer(DATA_TRANSFER_TYPE_READ | DATA_TRANSFER_TYPE_REGISTER, 0, 52, (void*)c2sData);
	if (!periodicReadTransfer)
	{
		return false;
	}

	periodicWriteTransfer = new ModbusDataTransfer(DATA_TRANSFER_TYPE_WRITE | DATA_TRANSFER_TYPE_REGISTER, 24, 28, (void*)&s2cData->voltageControl_1);
	if (!periodicWriteTransfer)
	{
		return false;
	}

	modeTransfer = new ModbusDataTransfer(DATA_TRANSFER_TYPE_WRITE | DATA_TRANSFER_TYPE_REGISTER, 16, 4, (void*)&s2cData->converterCommands);
	if (!modeTransfer)
	{
		return false;
	}

	return true;
}

void ModbusConverterInterface::Shutdown()
{
	// stop and delete modbus communication
	if (modbusInterface)
	{
		modbusInterface->Shutdown();
		delete modbusInterface;
		modbusInterface = NULL;
	}

	// delete modbus transfers
	if (periodicReadTransfer)
	{
		delete periodicReadTransfer;
		periodicReadTransfer = NULL;
	}

	if (periodicWriteTransfer)
	{
		delete periodicWriteTransfer;
		periodicWriteTransfer = NULL;
	}

	if (modeTransfer)
	{
		delete modeTransfer;
		modeTransfer = NULL;
	}

	return;
}

bool ModbusConverterInterface::Connect()
{
	// reconnect device
	// variables
	bool result;

	result = modbusInterface->Initialize(converter, ip, MODBUS_PORT);
	if (!result)
	{
		modbusInterface->Shutdown();
		return false;
	}

	// add periodic transfer
	result = modbusInterface->AddPeriodicTransfer(periodicReadTransfer);
	if (!result)
	{
		modbusInterface->Shutdown();
		return false;
	}

	result = modbusInterface->AddPeriodicTransfer(periodicWriteTransfer);
	if (!result)
	{
		modbusInterface->Shutdown();
		return false;
	}

	return true;
}

void ModbusConverterInterface::OnDisconnect()
{
	// only shutdown modbus interface
	modbusInterface->Shutdown();
	return;
}

bool ModbusConverterInterface::SetNewMode()
{
	// do a single transfer to update the registers at the converter
	return modbusInterface->NewSingleTransfer(modeTransfer);
}