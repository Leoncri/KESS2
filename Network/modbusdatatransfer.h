#pragma once

#include <stdint.h>

#define DATA_TRANSFER_TYPE_READ		0x1
#define DATA_TRANSFER_TYPE_WRITE	0x2
#define DATA_TRANSFER_TYPE_REGISTER	0x4
#define DATA_TRANSFER_TYPE_COIL		0x8

#define TRANSFER_INTERFACE_TCP		0x1
#define TRANSFER_INTERFACE_UDP		0x2

class ModbusDataTransfer
{
public:
	ModbusDataTransfer(int type, uint16_t startAddr, uint16_t length, void* payload);
	~ModbusDataTransfer();

	static bool Clear();

public:
	uint16_t transferIndex;
	int type;
	uint16_t startAddr;
	uint16_t length;
	uint8_t* coils;
	uint16_t* registers;
	
	bool ongoing;

private:
	static uint16_t actTransferIndex;
	static uint16_t transfersUsed;
};