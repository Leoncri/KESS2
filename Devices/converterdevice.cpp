#include "converterdevice.h"
#include "../debugoutput.h"

Converter::Converter()
{
	adsInterface = NULL;
	modbusInterface = NULL;
	useADS = false;
	gridHandler = NULL;
	deviceIP = 0;
	deviceConfig = 0;
	recvBufferReady = false;
}

Converter::~Converter()
{
}

bool Converter::Initialize(GridHandler* handler, uint16_t id, GridNode* n1, GridNode* n2, uint32_t ip, uint16_t config)
{
	// variables
	bool result;

	// store parameters
	gridHandler = handler;
	selfID = id;
	side1Node = n1;
	side2Node = n2;
	deviceIP = ip;
	deviceConfig = config;

	//OUTPUT_DEBUG_MESSAGE(&c2sData)
	//OUTPUT_DEBUG_MESSAGE(&s2cData)

	// check if ADS needs to be used
	if (config & CONVERTER_USE_ADS)
	{
		useADS = true;
	}

	// initialize interfaces
	if (useADS)
	{
		// create a new ads interface
		adsInterface = new AdsConverterInterface();
		if (!adsInterface)
		{
			return false;
		}
	}
	else
	{
		// create a new modbus interface
		OUTPUT_DEBUG_MESSAGE("Creating new Modbus Converter Interface...")
		modbusInterface = new ModbusConverterInterface();
		if (!modbusInterface)
		{
			OUTPUT_DEBUG_MESSAGE("Cannot create a Modbus Converter Interface")
			return false;
		}
		
		// initialize
		result = modbusInterface->Initialize(this, deviceIP, &c2sData, &s2cData);
		if (!result)
		{
			OUTPUT_DEBUG_MESSAGE("Cannot initialize a Modbus Converter Interface")
			return false;
		}
		OUTPUT_DEBUG_MESSAGE("Done")
	}

	// connect the device
	result = ConnectDevice();
	if (!result)
	{
		SetConnectionStatus(false);
		BackgroundTask::AddDeviceForAutomaticReconnect(this);
	}

	// set the content of c2sData to 0
	memset(&c2sData, 0, sizeof(c2sData));
	memset(&s2cData, 0, sizeof(s2cData));

	return true;
}

void Converter::Shutdown()
{
	// release all data
	if (adsInterface)
	{
		adsInterface->Shutdown();
		delete adsInterface;
		adsInterface = NULL;
	}

	if (modbusInterface)
	{
		modbusInterface->Shutdown();
		delete modbusInterface;
		modbusInterface = NULL;
	}

	return;
}

bool Converter::ConnectDevice()
{
	// variables
	bool result;

	if (online)
	{
		return true;
	}

	// connect the device
	if (useADS)
	{
		// initialize the interface
		result = adsInterface->Initialize(this, deviceIP, &c2sData, &s2cData);
		if (!result)
		{
			adsInterface->Shutdown();
			return false;
		}
	}
	else
	{
		// connect the interface
		result = modbusInterface->Connect();
		if (!result)
		{
			return false;
		}
	}

	SetConnectionStatus(true);
	return true;
}

void Converter::OnConnectionFault()
{
	// this function is called when there is a connection fault
	SetConnectionStatus(false);

	if (useADS)
	{
		adsInterface->Shutdown();
	}
	else
	{
		modbusInterface->OnDisconnect();
	}

	BackgroundTask::AddDeviceForAutomaticReconnect(this);

	return;
}

float Converter::GetVoltage(int side)
{
	if (side == 1)
	{
		return (float)(c2sData.voltageMeasurement.voltageM_1 + c2sData.voltageMeasurement.voltageP_1);
	}

	return (float)(c2sData.voltageMeasurement.voltageM_2 + c2sData.voltageMeasurement.voltageP_2);
}

bool Converter::NewCommand(char* buffer, int size)
{
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
	gridHandler->SubsystemProcess(SUBSYSTEM_PROCESS_CONVERTER);

	return true;
}

bool Converter::Update()
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
	if (header->header.deviceType != DEVICE_TYPE_CONVERTER || header->header.deviceId != selfID)
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

void Converter::SendPeriodicData()
{
	SendPeriodicDataPacket(periodicDataConnections, 0);
	return;
}

void Converter::ProcessRecvBuffer()
{
	// variables
	GenericCommandPacketHeader_t* header = (GenericCommandPacketHeader_t*)recvBuffer;
	uint32_t command = header->command;
	int mode;

	// process command
	if ((command & CONVERTER_COMMAND_MASK) == CONVERTER_COMMAND_SET_MODE)
	{
		// set a new mode
		OUTPUT_DEBUG_MESSAGE("New converter mode set")
		mode = command & CONVERTER_MODE_MASK;
		UpdateConverterMode(mode);
		SendRespondPacket(header->header.connection, header->commandId, CONVERTER_RESPOND_SUCCESS);
	}
	else if ((command & CONVERTER_COMMAND_MASK) == CONVERTER_COMMAND_UPDATE_DATA)
	{
		// get mode and update data
		OUTPUT_DEBUG_MESSAGE("Converter data updated")
		mode = command & CONVERTER_MODE_MASK;
		UpdateConverterData(mode);
		SendRespondPacket(header->header.connection, header->commandId, CONVERTER_RESPOND_SUCCESS);
	}
	else if ((command & CONVERTER_COMMAND_MASK) == CONVERTER_COMMAND_PERIODIC_DATA)
	{
		OUTPUT_DEBUG_MESSAGE("Converter live data turned on")
		// change periodic data settings
		if ((command & CONVERTER_DATA_MASK) == CONVERTER_COMMAND_PERIODIC_DATA_ON)
		{
			AddPeriodicDataConnection(header->header.connection);
		}
		else
		{
			RemovePeriodicDataConnection(header->header.connection);
		}
		SendRespondPacket(header->header.connection, header->commandId, CONVERTER_RESPOND_SUCCESS);
	}
	else
	{
		SendRespondPacket(header->header.connection, header->commandId, CONVERTER_RESPOND_UNKNOWN_COMMAND);
	}

	return;
}

void Converter::SendRespondPacket(uint16_t connection, uint32_t id, uint32_t respond)
{
	// lock send buffer first
	std::unique_lock lk(sendBufferLock);

	// clear buffer
	memset(sendBuffer, 0, sizeof(GenericCommandRespondHeader_t));

	// create package
	GenericCommandRespondHeader_t* r = (GenericCommandRespondHeader_t*)(sendBuffer);
	r->header.packetType = PACKET_TYPE_RESPOND;
	r->header.deviceType = DEVICE_TYPE_CONVERTER;
	r->header.deviceId = selfID;
	r->header.length = sizeof(GenericCommandRespondHeader_t);
	r->header.connection = connection;
	r->commandId = id;
	r->result = respond;

	// send data
	gridHandler->SendData(sendBuffer, sizeof(GenericCommandRespondHeader_t));

	return;
}

void Converter::SendPeriodicDataPacket(uint16_t connection, uint32_t id)
{
	// lock the send buffer
	std::unique_lock lk(sendBufferLock);

	// create packet
	ConverterDeviceData_t* p = (ConverterDeviceData_t*)(sendBuffer);

	//OUTPUT_DEBUG_MESSAGE("Converter Data")
	//OUTPUT_DEBUG_MESSAGE(&s2cData);
	//OUTPUT_DEBUG_MESSAGE(&c2sData);
	//OUTPUT_DEBUG_MESSAGE("Converter Data End")
	
	p->header.header.connection = connection;
	p->header.header.deviceId = selfID;
	p->header.header.deviceType = DEVICE_TYPE_CONVERTER;
	p->header.header.length = sizeof(ConverterDeviceData_t);
	p->header.header.packetType = PACKET_TYPE_DEVICEDATA;
	p->header.id = 0;
	p->status = 0;

	if (online)
	{
		p->status |= CONVERTER_STATUS_ONLINE;
	}
	
	// copy converter data
	memcpy(&p->data, &c2sData, sizeof(c2sData));

	// send out data
	gridHandler->SendData(sendBuffer, sizeof(ConverterDeviceData_t));
	//OUTPUT_DEBUG_MESSAGE("Sending periodic converter data with " << sizeof(ConverterDeviceData_t) << " bytes")
	return;
}

void Converter::UpdateConverterData(int mode)
{
	// variables
	ConverterUpdateData_t* p = (ConverterUpdateData_t*)recvBuffer;

	// copy data into outgoing structure
	switch (mode)
	{
	case MODE_VOLTAGE_CONTROL_1:
	{
		memcpy(&s2cData.voltageControl_1, &p->data, 8);
		break;
	}
	case MODE_VOLTAGE_CONTROL_2:
	{
		memcpy(&s2cData.voltageControl_2, &p->data, 8);
		break;
	}
	case MODE_DROOP_1:
	{
		memcpy(&s2cData.droopControl_1, &p->data, 8);
		break;
	}
	case MODE_DROOP_2:
	{
		memcpy(&s2cData.droopControl_2, &p->data, 8);
		break;
	}
	case MODE_POWER:
	{
		memcpy(&s2cData.powerControl, &p->data, 8);
		break;
	}
	case MODE_PRECHARGE_1:
	{
		memcpy(&s2cData.prechargeControl_1, &p->data, 8);
		break;
	}
	case MODE_PRECHARGE_2:
	{
		memcpy(&s2cData.prechargeControl_2, &p->data, 8);
		break;
	}
	}

	return;
}

void Converter::UpdateConverterMode(int mode)
{
	// set converter mode
	s2cData.converterCommands.changeToMode = mode;

	// notify interfaces
	if (useADS)
	{
		adsInterface->UpdateData();
		adsInterface->SetNewMode();
	}
	else
	{
		modbusInterface->SetNewMode();
	}

	return;
}