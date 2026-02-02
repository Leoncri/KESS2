#pragma once

class ModbusInterface;
struct ModbusInterfaceData;

#include "../config.h"
#include "modbusdatatransfer.h"
#include "../Devices/griddeviceclass.h"
#include "networkinterface.h"
#include "modbusinterfacehandler.h"
#include <stdint.h>
#include <list>
#include <mutex>

#define MODBUS_READ_REGISTER			0x04
#define MODBUS_WRITE_REGISTER			0x10
#define MODBUS_READ_COIL				0x01
#define MODBUS_WRITE_COIL				0x0F
#define MODBUS_ERROR_MASK				0x80
#define MODBUS_FUNCTION_MASK			0x7F

#define MAX_REGISTER_READ				123
#define MAX_REGISTER_WRITE				123
#define MAX_COIL_READ					2000
#define MAX_COIL_WRITE					2000

struct ModbusInterfaceData
{
    ModbusInterface* modbusInterface;
#ifdef USE_WINDOWS
    SOCKET socket;
#else
    int socket;
#endif
};

typedef struct
{
	uint16_t startAddr;
	uint16_t quantity;
} ModbusReadRegister_t;
#define MODBUS_READ_REGISTERS_PACKET_LENGTH					4

typedef struct
{
	uint8_t bytes;
	uint8_t data[254];
} ModbusReadRegisterResponse_t;
#define MODBUS_READ_REGISTERS_RESPOND_PACKET_BASE_LENGTH		1

typedef struct
{
	uint16_t startAddr;
	uint16_t quantity;
	uint8_t bytes;
	uint8_t data[254];
} ModbusWriteRegister_t;
#define MODBUS_WRITE_REGISTERS_PACKET_DATA_BASE_LENGTH		5

typedef struct
{
	uint16_t startAddr;
	uint16_t quantity;
} ModbusWriteRegisterResponse_t;
#define MODBUS_WRITE_REGISTERS_RESPOND_PACKET_LENGTH		4

typedef struct
{
	uint16_t startAddr;
	uint16_t quantity;
} ModbusReadCoil_t;
#define MODBUS_READ_COILS_PACKET_LENGTH						4

typedef struct
{
	uint8_t bytes;
	uint8_t data[255];
} ModbusReadCoilResponse_t;
#define MODBUS_READ_COILS_RESPOND_PACKET_BASE_LENGTH		1

typedef struct
{
	uint16_t startAddr;
	uint16_t quantity;
	uint8_t bytes;
	uint8_t data[255];
} ModbusWriteCoil_t;
#define MODBUS_WRITE_COILS_PACKET_BASE_LENGTH				5

typedef struct
{
	uint16_t startAddr;
	uint16_t quantity;
} ModbusWriteCoilResponse_t;
#define MODBUS_WRITE_COILS_RESPOND_PACKET_LENGTH			4

typedef struct
{
	uint16_t tId;		// transction id
	uint16_t pId;		// protocol id (always 0x0)
	uint16_t length;
	uint8_t unitId;
	uint8_t function;
} GenericModbusSendPacket_t;

typedef struct
{
	uint16_t tId;		// transction id
	uint16_t pId;		// protocol id (always 0x0)
	uint16_t length;
	uint8_t unitId;
	uint8_t function;
	ModbusReadRegister_t readRegister;
} ModbusSendPacketReadRegisters_t;

typedef struct
{
	uint16_t tId;		// transction id
	uint16_t pId;		// protocol id (always 0x0)
	uint16_t length;
	uint8_t unitId;
	uint8_t function;
	ModbusWriteRegister_t writeRegister;
} ModbusSendPacketWriteRegisters_t;

typedef struct
{
	uint16_t tId;		// transction id
	uint16_t pId;		// protocol id (always 0x0)
	uint16_t length;
	uint8_t unitId;
	uint8_t function;
	ModbusReadCoil_t readCoil;
} ModbusSendPacketReadCoils_t;

typedef struct
{
	uint16_t tId;		// transction id
	uint16_t pId;		// protocol id (always 0x0)
	uint16_t length;
	uint8_t unitId;
	uint8_t function;
	ModbusWriteCoil_t writeCoil;
} ModbusSendPacketWriteCoils_t;

typedef struct
{
	uint16_t tId;		// transaction id
	uint16_t pId;		// protocol id (always 0x0)
	uint16_t length;
	uint8_t unitId;
	uint8_t function;
} GenericModbusRecvPacket_t;

typedef struct
{
	uint16_t tId;		// transaction id
	uint16_t pId;		// protocol id (always 0x0)
	uint16_t length;
	uint8_t unitId;
	uint8_t function;
	ModbusReadRegisterResponse_t readRegister;
} ModbusRecvPacketReadRegisters_t;

typedef struct
{
	uint16_t tId;		// transaction id
	uint16_t pId;		// protocol id (always 0x0)
	uint16_t length;
	uint8_t unitId;
	uint8_t function;
	ModbusWriteRegisterResponse_t writeRegister;
} ModbusRecvPacketWriteRegisters_t;

typedef struct
{
	uint16_t tId;		// transaction id
	uint16_t pId;		// protocol id (always 0x0)
	uint16_t length;
	uint8_t unitId;
	uint8_t function;
	ModbusReadCoilResponse_t readCoil;
} ModbusRecvPacketReadCoils_t;

typedef struct
{
	uint16_t tId;		// transaction id
	uint16_t pId;		// protocol id (always 0x0)
	uint16_t length;
	uint8_t unitId;
	uint8_t function;
	ModbusWriteCoilResponse_t writeCoil;
} ModbusRecvPacketWriteCoils_t;

#define MODBUS_PACKET_BASE_LENGTH		8
#define MODBUS_PACKET_LENGTH_OFFSET		2

class ModbusInterface
{
public:
	// contructor and destructor
	ModbusInterface();
	~ModbusInterface();

	// Initialize and shutdown
	virtual bool Initialize(GridDevice* device, uint32_t ip, int port);
	virtual void Shutdown();
	bool IsValid();

	// start and stop the interfaces
	static bool StartInterfaceHandler();
	static void StopInterfaceHandler();
	static bool StartInterfaces();
	static void StopInterfaces();

	// Functions setting up data transfers
	bool AddPeriodicTransfer(ModbusDataTransfer* t);
	void RemovePeriodicTransfer(ModbusDataTransfer* t);
	bool NewSingleTransfer(ModbusDataTransfer* t);

	// Function for the ModbusInterfaceHandler to call
	void SendPeriodicData();
	virtual bool RecvData();
	void OnBrokenConnectionCallback();

	// Swap bytes function for converting between machine and modbus
	uint16_t mtom(uint16_t);

protected:
	// functions for creating and processing packets
	int CreateAndSendPacket(ModbusDataTransfer*);
	int GetTransferIndex();
	bool ProcessPacket(ModbusDataTransfer*, int dataSize);

protected:
	// lock for data
	std::mutex lock;

	// send and recv buffer
	char sendBuffer[MAX_BUFFER_SIZE];
	char recvBuffer[MAX_BUFFER_SIZE];

	// list for periodic data transfers and single data transfers
	std::list<ModbusDataTransfer*> periodicTransfers;
	int periodicDataTransfersOngoing;

	ModbusDataTransfer* singleDataTransfer;

	// Modbus interface handler
	NetworkInterface* networkInterface;
	static ModbusInterfaceHandler* interfaceHandler;
	static int interfaceCount;
	ModbusInterfaceData interfaceData;

	// Grid device
    GridDevice* gridDevice;

	// others
	bool valid;
};