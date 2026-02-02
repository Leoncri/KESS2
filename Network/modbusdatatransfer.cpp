#include "modbusdatatransfer.h"
#include "../debugoutput.h"

uint16_t ModbusDataTransfer::actTransferIndex = 0;
uint16_t ModbusDataTransfer::transfersUsed = 0;

ModbusDataTransfer::ModbusDataTransfer(int type, uint16_t startAddr, uint16_t length, void* payload) :
	type{ type }, startAddr{ startAddr }, length{ length }, ongoing{ false }
{
	// setup transfer index
	transferIndex = actTransferIndex;
	actTransferIndex++;
	transfersUsed++;

	// store payload pointer
	if (type & DATA_TRANSFER_TYPE_REGISTER)
	{
		registers = (uint16_t*)payload;
		coils = 0;
		OUTPUT_DEBUG_MESSAGE("Setting registers to " << &registers[0])
	}
	else
	{
		registers = 0;
		coils = (uint8_t*)payload;
	}
}

ModbusDataTransfer::~ModbusDataTransfer()
{
	// increase cleared transfer counter
	transfersUsed--;
}

bool ModbusDataTransfer::Clear()
{
	// reset counters to zero
	if (transfersUsed == 0)
	{
		actTransferIndex = 0;
		return true;
	}

	return false;
}