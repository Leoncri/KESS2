#include "modbusinterface.h"
#include "../debugoutput.h"

ModbusInterfaceHandler* ModbusInterface::interfaceHandler = 0;
int ModbusInterface::interfaceCount = 0;

ModbusInterface::ModbusInterface()
{
	// initialize pointers
	networkInterface = 0;

	periodicDataTransfersOngoing = 0;

	singleDataTransfer = 0;

	// check if there is an interface handler available
	if (!interfaceHandler)
	{
		// create a new interface handler and start it
		interfaceHandler = new ModbusInterfaceHandler();
	}
	interfaceCount++;

	valid = true;
}

ModbusInterface::~ModbusInterface()
{
	// decrease interface count and delete interface handler if needed
	interfaceCount--;
	if (!interfaceCount)
	{	
		if (interfaceHandler)
		{
			interfaceHandler->Shutdown();
			delete interfaceHandler;
			interfaceHandler = 0;
		}
	}
}

bool ModbusInterface::Initialize(GridDevice* device, uint32_t ip, int port)
{
	// this is dependent on UDP or TCP
	return true;
}

void ModbusInterface::Shutdown()
{
	// this is depended on UDP or TCP
	return;
}

bool ModbusInterface::IsValid()
{
	return valid;
}

bool ModbusInterface::StartInterfaceHandler()
{
	// variables
	bool result;

	// state the interface handler
	if (interfaceHandler)
	{
		OUTPUT_DEBUG_MESSAGE("Starting the modbus interface handler")
		result = interfaceHandler->Initialize();
		return result;
	}

	return true;
}

void ModbusInterface::StopInterfaceHandler()
{
	// stop the modbus interface handler
	if (interfaceHandler)
	{
		interfaceHandler->Shutdown();
	}

	return;
}

bool ModbusInterface::StartInterfaces()
{
	// variables
	bool result;

	// start the interfaces by starting interface handler thread
	if (interfaceHandler)
	{
		OUTPUT_DEBUG_MESSAGE("Starting the threads for the modbus interface handler")
		result = interfaceHandler->StartInterface();
		return result;
	}

	return true;
}

void ModbusInterface::StopInterfaces()
{
	// stop the interfaces
	if (interfaceHandler)
	{
		OUTPUT_DEBUG_MESSAGE("Stopping the threads for the modbus interface handler")
		interfaceHandler->StopInterface();
	}

	return;
}

bool ModbusInterface::AddPeriodicTransfer(ModbusDataTransfer* t)
{
	// get lock
	OUTPUT_DEBUG_MESSAGE("Getting modbus interface lock to append periodic transfers")
	std::unique_lock lk(lock);
	OUTPUT_DEBUG_MESSAGE("Got the modbus interface lock for appending periodic transfers")

	// check if transfer is already in list
	for (auto e : periodicTransfers)
	{
		if (e == t)
		{
			// already in list
			return true;
		}
	}

	// append to list
	periodicTransfers.push_back(t);

	return true;
}

void ModbusInterface::RemovePeriodicTransfer(ModbusDataTransfer* t)
{
	// get lock
	OUTPUT_DEBUG_MESSAGE("Getting modbus interface lock to remove periodic transfers")
	std::unique_lock lk(lock);
	OUTPUT_DEBUG_MESSAGE("Got the modbus interface lock for removing periodic transfers")

	// remove from list
	periodicTransfers.remove(t);

	return;
}

bool ModbusInterface::NewSingleTransfer(ModbusDataTransfer* t)
{
	// check if there is already a transfer ongoing
	if (singleDataTransfer)
	{
		return false;
	}

	singleDataTransfer = t;

	return true;
}

void ModbusInterface::SendPeriodicData()
{
	// variables
	int result;

	// check if interface is valid
	if (!valid)
	{
		// nothing to do here
		return;
	}

	// get lock
	OUTPUT_DEBUG_MESSAGE("Trying to get lock for modbus interface")
	//std::unique_lock lk(lock);
	OUTPUT_DEBUG_MESSAGE("Got lock for modbus interface")

	// iterate over all transfers and send out if not ongoing
	for (auto e : periodicTransfers)
	{
		if (!e->ongoing)
		{
			result = CreateAndSendPacket(e);
			if (result > 0)
			{
				periodicDataTransfersOngoing++;
			}
		}
	}

	// send out single transfer if there is one
	if (singleDataTransfer)
	{
		if (!singleDataTransfer->ongoing)
		{
			CreateAndSendPacket(singleDataTransfer);
		}
	}

	return;
}

void ModbusInterface::OnBrokenConnectionCallback()
{
	// notify device
	valid = false;
	gridDevice->OnConnectionFault();
	return;
}

bool ModbusInterface::RecvData()
{
	// this is dependent on UDP/TCP
	return true;
}

int ModbusInterface::CreateAndSendPacket(ModbusDataTransfer* t)
{
	// variables
	int packetLength = -1;
	bool bResult;

	// change buffer to modbus packet
	GenericModbusSendPacket_t* packet = (GenericModbusSendPacket_t*)sendBuffer;

	// set id
	packet->tId = mtom(t->transferIndex);
	packet->pId = 0;
	packet->unitId = 0;

	// create packet depending on type
	switch (t->type)
	{
	case DATA_TRANSFER_TYPE_READ | DATA_TRANSFER_TYPE_REGISTER:
	{
		// Register read
		ModbusSendPacketReadRegisters_t* sendPacket = (ModbusSendPacketReadRegisters_t*)sendBuffer;
		if (t->length > MAX_REGISTER_READ || t->length == 0)
		{
			return -1;
		}
		sendPacket->length = mtom(MODBUS_READ_REGISTERS_PACKET_LENGTH + MODBUS_PACKET_LENGTH_OFFSET);
		sendPacket->function = MODBUS_READ_REGISTER;
		sendPacket->readRegister.startAddr = mtom(t->startAddr);
		sendPacket->readRegister.quantity = mtom(t->length);

		packetLength = MODBUS_PACKET_BASE_LENGTH + MODBUS_READ_REGISTERS_PACKET_LENGTH;

		break;
	}
	case DATA_TRANSFER_TYPE_WRITE | DATA_TRANSFER_TYPE_REGISTER:
	{
		// Register write
		ModbusSendPacketWriteRegisters_t* sendPacket = (ModbusSendPacketWriteRegisters_t*)sendBuffer;
		if (t->length > MAX_REGISTER_WRITE || t->length == 0)
		{
			return -1;
		}
		sendPacket->length = mtom(2 * t->length + MODBUS_WRITE_REGISTERS_PACKET_DATA_BASE_LENGTH + MODBUS_PACKET_LENGTH_OFFSET);
		sendPacket->function = MODBUS_WRITE_REGISTER;
		sendPacket->writeRegister.startAddr = mtom(t->startAddr);
		sendPacket->writeRegister.quantity = mtom(t->length);
		sendPacket->writeRegister.bytes = (uint8_t)(2 * t->length);
		
		// copy data
		for (int i = 0; i < t->length; i++)
		{
			sendPacket->writeRegister.data[2 * i] = (uint8_t)(t->registers[i] >> 8);
			sendPacket->writeRegister.data[2 * i + 1] = (uint8_t)(t->registers[i] & 0xFF);
		}

		packetLength = MODBUS_PACKET_BASE_LENGTH + MODBUS_WRITE_REGISTERS_PACKET_DATA_BASE_LENGTH + sendPacket->writeRegister.bytes;

		break;
	}
	case DATA_TRANSFER_TYPE_READ | DATA_TRANSFER_TYPE_COIL:
	{
		// Coil read
		ModbusSendPacketReadCoils_t* sendPacket = (ModbusSendPacketReadCoils_t*)sendBuffer;
		if (t->length > MAX_COIL_WRITE || t->length == 0)
		{
			return -1;
		}
		sendPacket->length = mtom(MODBUS_READ_COILS_PACKET_LENGTH + MODBUS_PACKET_LENGTH_OFFSET);
		sendPacket->function = MODBUS_READ_COIL;
		sendPacket->readCoil.startAddr = mtom(t->startAddr);
		sendPacket->readCoil.quantity = mtom(t->length);

		packetLength = MODBUS_PACKET_BASE_LENGTH + MODBUS_READ_COILS_PACKET_LENGTH;

		break;
	}
	case DATA_TRANSFER_TYPE_WRITE | DATA_TRANSFER_TYPE_COIL:
	{
		// coil write
		ModbusSendPacketWriteCoils_t* sendPacket = (ModbusSendPacketWriteCoils_t*)sendBuffer;
		if (t->length > MAX_COIL_WRITE || t->length == 0)
		{
			return -1;
		}
		sendPacket->function = MODBUS_WRITE_COIL;
		sendPacket->writeCoil.startAddr = mtom(t->startAddr);
		sendPacket->writeCoil.quantity = mtom(t->length);
		sendPacket->writeCoil.bytes = (t->length % 8) == 0 ? (t->length / 8) : (t->length / 8) + 1;
		sendPacket->length = mtom(sendPacket->writeCoil.bytes + MODBUS_WRITE_COILS_PACKET_BASE_LENGTH + MODBUS_PACKET_LENGTH_OFFSET);

		// copy data
		memcpy(sendPacket->writeCoil.data, t->coils, sendPacket->writeCoil.bytes);

		packetLength = MODBUS_PACKET_BASE_LENGTH + MODBUS_WRITE_COILS_PACKET_BASE_LENGTH + sendPacket->writeCoil.bytes;

		break;
	}
	default:
	{
		return -1;
	}
	}

	// when we get here, only point is to send the packet out now
	bResult = networkInterface->SendData(sendBuffer, packetLength);
	if (!bResult)
	{
		// something went wrong
		valid = false;
		return -1;
	}
	
	t->ongoing = true;

	return packetLength;
}

int ModbusInterface::GetTransferIndex()
{
	// assume packet is at position 0 in recv buffer
	GenericModbusRecvPacket_t* packet = (GenericModbusRecvPacket_t*)recvBuffer;

	return mtom(packet->tId);
}

bool ModbusInterface::ProcessPacket(ModbusDataTransfer* t, int dataSize)
{
	// variables

	// change type of buffer
	GenericModbusRecvPacket_t* packet = (GenericModbusRecvPacket_t*)recvBuffer;

	// check length
	int length = mtom(packet->length) + MODBUS_PACKET_BASE_LENGTH - MODBUS_PACKET_LENGTH_OFFSET;
	if (length != dataSize)
	{
		return false;
	}

	// check if error bit is set
	if (packet->function & MODBUS_ERROR_MASK)
	{
		// call grid device
		OUTPUT_DEBUG_MESSAGE("Getting broken modbus respond")
		gridDevice->CallbackDataTransferError(t);
		return false;
	}

	// access data according to function
	switch (packet->function)
	{
	case MODBUS_READ_REGISTER:
	{
		// check length
		OUTPUT_DEBUG_MESSAGE("Modbus read register respond")
		ModbusRecvPacketReadRegisters_t* recvPacket = (ModbusRecvPacketReadRegisters_t*)recvBuffer;
		if (t->length != recvPacket->readRegister.bytes / 2)
		{
			OUTPUT_DEBUG_MESSAGE("Broken read register respond")
			return false;
		}

		// copy data
		int dataLength = recvPacket->readRegister.bytes / 2;
		OUTPUT_DEBUG_MESSAGE(&t->registers[0]);
		for (int i = 0; i < dataLength; i++)
		{
			t->registers[i] = (uint16_t)(recvPacket->readRegister.data[i * 2]) << 8 | (uint16_t)(recvPacket->readRegister.data[i * 2 + 1]);
		}

		break;
	}
	case MODBUS_WRITE_REGISTER:
	{
		// check length
		OUTPUT_DEBUG_MESSAGE("Modbus write register respond")
		ModbusRecvPacketWriteRegisters_t* recvPacket = (ModbusRecvPacketWriteRegisters_t*)recvBuffer;
		if (t->length != mtom(recvPacket->writeRegister.quantity))
		{
			OUTPUT_DEBUG_MESSAGE("Broken write register respond")
			return false;
		}

		// check start addr
		if (t->startAddr != mtom(recvPacket->writeRegister.startAddr))
		{
			return false;
		}

		break;
	}
	case MODBUS_READ_COIL:
	{
		// check length
		ModbusRecvPacketReadCoils_t* recvPacket = (ModbusRecvPacketReadCoils_t*)recvBuffer;
		int expectedDataLength = (t->length % 8) == 0 ? (t->length / 8) : (t->length / 8) + 1;
		if (expectedDataLength != recvPacket->readCoil.bytes)
		{
			return false;
		}

		// copy data
		memcpy(t->coils, recvPacket->readCoil.data, recvPacket->readCoil.bytes);

		break;
	}
	case MODBUS_WRITE_COIL:
	{
		// check length
		ModbusRecvPacketWriteCoils_t* recvPacket = (ModbusRecvPacketWriteCoils_t*)recvBuffer;
		if (t->length != mtom(recvPacket->writeCoil.quantity))
		{
			return false;
		}

		// check start addr
		if (t->startAddr != mtom(recvPacket->writeCoil.startAddr))
		{
			return false;
		}

		break;
	}
	default:
		return false;
	}

	// call grid device
	gridDevice->CallbackDataTransfer(t);

	return true;
}

uint16_t ModbusInterface::mtom(uint16_t in)
{
	// swap bytes
	uint16_t out;

	out = (in >> 8) | ((in & 0xFF) << 8);

	return out;
}