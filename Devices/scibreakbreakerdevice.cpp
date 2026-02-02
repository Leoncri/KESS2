#include "scibreakbreakerdevice.h"
#include "../debugoutput.h"

SciBreakBreaker::SciBreakBreaker()
{
	modbusInterface = NULL;
	gridHandler = NULL;
	side1Node = NULL;
	side2Node = NULL;
	modbusPeriodicRead = NULL;
	deviceBreaker = NULL;
}

SciBreakBreaker::~SciBreakBreaker()
{
}

bool SciBreakBreaker::Initialize(GridHandler* handler, uint16_t id, GridNode* n1, GridNode* n2, uint32_t ip, uint16_t port, uint16_t config)
{
	// variables
	bool result;

	// store parameters
	gridHandler = handler;
	selfID = id;
	side1Node = n1;
	side2Node = n2;
	deviceIP = ip;
	devicePort = port;
	deviceConfig = config;

	// create the actual breaker device
	deviceBreaker = new GridBreaker;
	if (!deviceBreaker)
	{
		return false;
	}

	// create modbus interface
	modbusInterface = new ModbusTCPInterface;
	if (!modbusInterface)
	{
		return false;
	}

	// TODO
	modbusPeriodicRead = new ModbusDataTransfer(DATA_TRANSFER_TYPE_READ | DATA_TRANSFER_TYPE_REGISTER, 0, 8, (void*)&modbusPeriodicTransferBuffer);
	if (!modbusPeriodicRead)
	{
		return false;
	}

	modbusCommand = new ModbusDataTransfer(DATA_TRANSFER_TYPE_WRITE | DATA_TRANSFER_TYPE_REGISTER, 9, 1, (void*)&modbusCommandBuffer);
	if (!modbusCommand)
	{
		return false;
	}

	modbusSetTripLevel = new ModbusDataTransfer(DATA_TRANSFER_TYPE_WRITE | DATA_TRANSFER_TYPE_REGISTER, 10, 1, (void*)&modbusCommandBuffer);
	if (!modbusSetTripLevel)
	{
		return false;
	}

	result = ConnectDevice();
	if (!result)
	{
		SetConnectionStatus(false);
		OUTPUT_DEBUG_MESSAGE("Device Offline")

		// append to background thread
		BackgroundTask::AddDeviceForAutomaticReconnect(this);
	}

	// setup actual breaker
	deviceBreaker->Initialize(side1Node, side2Node);
	deviceBreaker->SetVoltage(1, 0.0);

	OUTPUT_DEBUG_MESSAGE("New SciBreak breaker initialized")

	return true;
}

void SciBreakBreaker::Shutdown()
{
	// delete the switch
	if (deviceBreaker)
	{
		deviceBreaker->Shutdown();
		delete deviceBreaker;
		deviceBreaker = NULL;
	}

	// stop and delte modbus communication
	if (modbusInterface)
	{
		modbusInterface->Shutdown();
		delete modbusInterface;
		modbusInterface = NULL;
	}

	if (modbusPeriodicRead)
	{
		delete modbusPeriodicRead;
		modbusPeriodicRead = NULL;
	}

	return;
}

bool SciBreakBreaker::ConnectDevice()
{
	// variables
	bool result;

	// check if already connected
	if (online)
	{
		return true;
	}

	// setup
	result = modbusInterface->Initialize(this, deviceIP, devicePort);
	if (!result)
	{
		return false;
	}

	result = modbusInterface->AddPeriodicTransfer(modbusPeriodicRead);
	if (!result)
	{
		return false;
	}

	SetConnectionStatus(true);
	return true;
}

void SciBreakBreaker::OnConnectionFault()
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

bool SciBreakBreaker::CallbackDataTransfer(ModbusDataTransfer* t)
{
	// check if the transfer is our periodic read
	if (t == modbusPeriodicRead)
	{
		// process data
		ProcessPeriodicRecvData();
	}
	return true;
}

bool SciBreakBreaker::NewCommand(char* buffer, int size)
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
	gridHandler->SubsystemProcess(SUBSYSTEM_PROCESS_SCIBREAK);

	return true;
}

bool SciBreakBreaker::Update()
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
	if (header->header.deviceType != DEVICE_TYPE_SCIBREAKBREAKER || header->header.deviceId != selfID || header->header.packetType != PACKET_TYPE_COMMAND)
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

void SciBreakBreaker::SendPeriodicData()
{
	// calculate the actual data and send back
	SendPeriodicDataPacket(periodicDataConnections, 0);
	return;
}

void SciBreakBreaker::ProcessRecvBuffer()
{
	// variables
	GenericCommandPacketHeader_t* header = (GenericCommandPacketHeader_t*)recvBuffer;
	uint32_t command = header->command;

	// process command
	if ((command & BREAKER_COMMAND_MASK) == BREAKER_COMMAND_ON_OFF)
	{
		// turn on or off breaker
		if (command & BREAKER_COMMAND_TURN_ON)
		{
			TurnOnBreaker(true);
		}
		else
		{
			TurnOnBreaker(false);
		}

		SendRespondPacket(header->header.connection, header->commandId, BREAKER_RESPOND_SUCCESS);
	}
	else if ((command & BREAKER_COMMAND_MASK) == BREAKER_COMMAND_BREAKER)
	{
		// open or close breaker
		OUTPUT_DEBUG_MESSAGE("Breaker action")

		if ((command & BREAKER_DATA_MASK) == BREAKER_COMMAND_CLOSE_BREAKER)
		{
			SetBreakerState(true);
		}
		else if ((command & BREAKER_DATA_MASK) == BREAKER_COMMAND_OPEN_BREAKER)
		{
			SetBreakerState(false);
		}
		else
		{
			SendRespondPacket(header->header.connection, header->commandId, BREAKER_RESPOND_UNKNOWN_COMMAND);
		}

		SendRespondPacket(header->header.connection, header->commandId, BREAKER_RESPOND_SUCCESS);
	}
	else if ((command & BREAKER_COMMAND_MASK) == BREAKER_COMMAND_PERIODIC_DATA)
	{
		// Add the connection to the periodic data receivers
		if ((command & BREAKER_DATA_MASK) == BREAKER_COMMAND_PERIODIC_DATA_ON)
		{
			OUTPUT_DEBUG_MESSAGE("Periodic data ON command for breaker")
			AddPeriodicDataConnection(header->header.connection);
		}
		else
		{
			OUTPUT_DEBUG_MESSAGE("Periodic data OFF command for breaker")
			RemovePeriodicDataConnection(header->header.connection);
		}
		SendRespondPacket(header->header.connection, header->commandId, BREAKER_RESPOND_SUCCESS);
	}
	else if ((command & BREAKER_COMMAND_MASK) == BREAKER_COMMAND_UPDATE_DATA)
	{
		OUTPUT_DEBUG_MESSAGE("Setting new trip level for breaker")
		// cast packet to BreakerUpdateData_t
		BreakerUpdateData_t* updateData = (BreakerUpdateData_t*)recvBuffer;

		// set new data
		SetBreakerTripLevel(updateData->data[0]);

		SendRespondPacket(header->header.connection, header->commandId, BREAKER_RESPOND_SUCCESS);
	}

	return;
}

void SciBreakBreaker::SendRespondPacket(uint16_t connection, uint32_t id, uint32_t respond)
{
	// lock send buffer first
	std::unique_lock lk(sendBufferLock);

	// clear buffer
	memset(sendBuffer, 0, sizeof(GenericCommandRespondHeader_t));

	// create package
	GenericCommandRespondHeader_t* r = (GenericCommandRespondHeader_t*)(sendBuffer);
	r->header.packetType = PACKET_TYPE_RESPOND;
	r->header.deviceType = DEVICE_TYPE_SCIBREAKBREAKER;
	r->header.deviceId = 0;
	r->header.length = sizeof(GenericCommandRespondHeader_t);
	r->header.connection = connection;
	r->commandId = id;
	r->result = respond;

	// send data
	gridHandler->SendData(sendBuffer, sizeof(GenericCommandRespondHeader_t));

	return;
}

void SciBreakBreaker::SendPeriodicDataPacket(uint16_t connection, uint32_t id)
{
	// variables
	BreakerDeviceData_t* packet = (BreakerDeviceData_t*)periodicDataBuffer;

	// fill in header
	packet->respondHeader.header.connection = connection;
	packet->respondHeader.header.deviceId = selfID;
	packet->respondHeader.header.deviceType = DEVICE_TYPE_SCIBREAKBREAKER;
	packet->respondHeader.header.length = sizeof(BreakerDeviceData_t);
	packet->respondHeader.header.packetType = PACKET_TYPE_DEVICEDATA;
	packet->respondHeader.id = id;

	// data
	packet->status.voltageTop = voltageTop;
	packet->status.voltageBot = voltageBot;
	packet->status.currentTop = currentTop;
	packet->status.currentBot = currentBot;
	packet->status.tripLevelTop = tripLevelTop;
	packet->status.tripLevelBot = tripLevelBot;

	// set online bit
	if (online)
	{
		packet->status.status = BREAKER_STATUS_ONLINE;
	}
	else
	{
		packet->status.status = 0;
	}

	// put in status from both breakers
	packet->status.status |= breakerStatusTop << 8;
	packet->status.status |= breakerStatusBot << 16;
	
	// send packet
	//OUTPUT_DEBUG_MESSAGE("Sending periodic breaker data")
	gridHandler->SendData(periodicDataBuffer, sizeof(BreakerDeviceData_t));

	return;
}

void SciBreakBreaker::ProcessPeriodicRecvData()
{
	//
	OUTPUT_DEBUG_MESSAGE("Processing modbus data for breaker")
	
	// store incoming data
	breakerStatusTop = modbusPeriodicTransferBuffer[0];
	voltageTop = modbusPeriodicTransferBuffer[1];
	currentTop = modbusPeriodicTransferBuffer[2];
	tripLevelTop = modbusPeriodicTransferBuffer[3];

	breakerStatusBot = modbusPeriodicTransferBuffer[4];
	voltageBot = modbusPeriodicTransferBuffer[5];
	currentBot = modbusPeriodicTransferBuffer[6];
	tripLevelBot = modbusPeriodicTransferBuffer[7];

	OUTPUT_DEBUG_MESSAGE("voltage top" << voltageTop)
	OUTPUT_DEBUG_MESSAGE("voltage bot" << voltageBot)

	return;
}

void SciBreakBreaker::TurnOnBreaker(bool on)
{
	// variables

	// update modbus output buffer
	if (on)
	{
		modbusCommandBuffer[0] = MODBUS_TURN_ON;
	}
	else
	{
		modbusCommandBuffer[0] = MODBUS_TURN_OFF;
	}
	
	modbusCommandBuffer[1] = 0;

	// single write to device
	modbusInterface->NewSingleTransfer(modbusCommand);

	return;
}

void SciBreakBreaker::SetBreakerState(bool closed)
{
	// variables

	// update modbus output buffer
	if (closed)
	{
		modbusCommandBuffer[0] = MODBUS_CLOSE_TOP | MODBUS_CLOSE_BOT;
		modbusCommandBuffer[1] = 0;
	}
	else
	{
		modbusCommandBuffer[0] = MODBUS_OPEN_TOP | MODBUS_OPEN_BOT;
		modbusCommandBuffer[1] = 0;
	}

	// single write to device
	modbusInterface->NewSingleTransfer(modbusCommand);

	return;
}

void SciBreakBreaker::SetBreakerTripLevel(uint16_t level)
{
	// variables

	// fill buffer
	modbusTripLevelBuffer[0] = level & 0xFF;
	modbusTripLevelBuffer[1] = (level >> 8) & 0xFF;

	// single write to device
	modbusInterface->NewSingleTransfer(modbusSetTripLevel);

	return;
}