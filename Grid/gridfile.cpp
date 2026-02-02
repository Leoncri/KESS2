#include "gridfile.h"
#include "../debugoutput.h"

GridFile::GridFile()
{
    // initialize variables
    recvBufferReady = false;
    version = -1;
}

GridFile::~GridFile()
{
}

void GridFile::Initialize(GridServer* server, GridHandler* handler)
{
    // debug
    OUTPUT_DEBUG_MESSAGE("Initializing grid file class...")

    gridServer = server;
    gridHandler = handler;

    // debug
    OUTPUT_DEBUG_MESSAGE("Initializing grid file class done")

    return;
}

void GridFile::Shutdown()
{
    // clear grid
    ClearGrid();

    // reset gridServer
    gridServer = 0;

    return;
}

int GridFile::NewCommand(char* buffer, int size)
{
    // variables

    // check size
    if (size > MAX_BUFFER_SIZE)
    {
        return GRID_RESPOND_BUFFER_SIZE;
    }

    // get access to the buffers
    std::unique_lock lk(recvBufferLock);

    // copy data
    if (recvBufferReady)
    {
        // send back failure
        return GRID_RESPOND_NOT_READY;
    }

    memcpy(recvBuffer, buffer, size);

    // set receive buffer to ready
    recvBufferReady = true;

    // notify grid server that this class needs to be updated
    gridServer->SubsystemProcess(SUBSYSTEM_GRIDFILE);

    return GRID_RESPOND_SUCCESS;
}

bool GridFile::Update()
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
    if (header->header.deviceType != DEVICE_TYPE_GRID || (header->command | GRID_COMMAND_CONFIG) == 0)
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

bool GridFile::WasUpdated()
{
    if (updated)
    {
        updated = false;
        return true;
    }

    return false;
}

void GridFile::ClearGrid()
{
    // clear the grid
    if (gridHandler)
    {
        gridHandler->ClearGrid();
    }

    // iterate over all list elements
    for (auto p : gridFileConfig)
    {
        // delete the config if valid
        if (p)
        {
            delete p;
        }
    }

    // clear the list
    gridFileConfig.clear();

    return;
}

bool GridFile::AppendConfig()
{
    // Variables
    GridCommandLoadGridFilePacket_t* packet = (GridCommandLoadGridFilePacket_t*)(recvBuffer);
    int numConfigs = packet->loadGridFileHeader.length;
    GridFileElementConfig_t* nextConfig;

    // check if version and subversion matches
    if (version == -1)
    {
        version = packet->loadGridFileHeader.version;
        subversion = packet->loadGridFileHeader.subversion;
        numParts = packet->loadGridFileHeader.numParts;
    }

    if (version != packet->loadGridFileHeader.version)
    {
        return false;
    }

    if (subversion != packet->loadGridFileHeader.subversion)
    {
        return false;
    }

    if (numParts < packet->loadGridFileHeader.part)
    {
        return false;
    }

    // iterate over configs
    for (auto i = 0; i < numConfigs; i++)
    {
        // create new element
        OUTPUT_DEBUG_MESSAGE("Appending new config element")
        GridFileElementConfig_t* e = new GridFileElementConfig_t;
        if (!e)
        {
            return false;
        }

        // copy data
        nextConfig = &packet->elements[i];
        memcpy(e, nextConfig, sizeof(GridFileElementConfig_t));

        // append to list
        gridFileConfig.push_back(e);
    }

    return true;
}

bool GridFile::LoadConfig()
{
    // variables
    bool bResult;

    // iterate over all config entries and append them to the grid
    for (auto c : gridFileConfig)
    {
        // check for type
        // nothing to do for points
        if (c->type == CONFIG_TYPE_POINT)
        {
            OUTPUT_DEBUG_MESSAGE("Creating new point in config")
            continue;
        }

        // nothing to do for segments
        if (c->type == CONFIG_TYPE_SEGMENT)
        {
            OUTPUT_DEBUG_MESSAGE("Creating new segment in config")
            continue;
        }

        // create nodes
        if (c->type == CONFIG_TYPE_NODE)
        {
            // cast element to node config
            OUTPUT_DEBUG_MESSAGE("Creating new node in config")
            NodeConfig_t* node = (NodeConfig_t*)c;
            bResult = gridHandler->CreateNode(node->id);
            if (!bResult)
            {
                return false;
            }

            continue;
        }

        // create converter
        if (c->type == CONFIG_TYPE_CONVERTER)
        {
            int n1, n2;

            // cast config to converter
            ConverterConfig_t* converter = (ConverterConfig_t*)c;
            OUTPUT_DEBUG_MESSAGE("Creating new converter in config; ID = " << converter->id)
            
            // get id of the nodes
            n1 = GetNodeIdFromPoint(converter->point1Id);
            if (n1 < 0)
            {
                return false;
            }

            n2 = GetNodeIdFromPoint(converter->point2Id);
            if (n2 < 0)
            {
                return false;
            }

            // create actual converter
            bResult = gridHandler->CreateConverter(converter->id, n1, n2, converter->ip, converter->config);
            if (!bResult)
            {
                return false;
            }

            continue;
        }

        // create sci break breaker
        if (c->type == CONFIG_TYPE_SCIBREAKBREAKER)
        {
            int n1, n2;

            // cast config to breaker
            OUTPUT_DEBUG_MESSAGE("Creating new SCiBreak breaker in config")
            BreakerConfig_t* breaker = (BreakerConfig_t*)c;

            // get id of nodes
            n1 = GetNodeIdFromPoint(breaker->point1Id);
            if (n1 < 0)
            {
                return false;
            }

            n2 = GetNodeIdFromPoint(breaker->point2Id);
            if (n2 < 0)
            {
                return false;
            }

            // create actual breaker
            bResult = gridHandler->CreateSciBreakBreaker(breaker->id, n1, n2, breaker->ip);
        }

        // create breaker
        if (c->type == CONFIG_TYPE_BREAKER)
        {
            int n1, n2;

            // cast config to breaker
            OUTPUT_DEBUG_MESSAGE("Creating new breaker in config")
            BreakerConfig_t* breaker = (BreakerConfig_t*)c;
            
            // get id of the nodes
            n1 = GetNodeIdFromPoint(breaker->point1Id);
            if (n1 < 0)
            {
                return false;
            }

            n2 = GetNodeIdFromPoint(breaker->point2Id);
            if (n2 < 0)
            {
                return false;
            }

            // create actual breaker
            bResult = gridHandler->CreateBreaker(breaker->id, n1, n2, breaker->ip);
            if (!bResult)
            {
                return false;
            }

            continue;
        }

        // create fen switchgear
        if (c->type == CONFIG_TYPE_FENSWITCHGEAR)
        {
            int n1, n2, n3, n4;

            // cast config to switchgear
            FENSwitchgearConfig_t* switchgear = (FENSwitchgearConfig_t*)c;
            OUTPUT_DEBUG_MESSAGE("Creating new switchgear in config; ID = " << switchgear->id)

            // get node id
            n1 = GetNodeIdFromPoint(switchgear->point1Id);
            if (n1 < 0)
            {
                OUTPUT_DEBUG_MESSAGE("Cannot find node with id " << n1)
                return false;
            }

            n2 = GetNodeIdFromPoint(switchgear->point2Id);
            if (n2 < 0)
            {
                OUTPUT_DEBUG_MESSAGE("Cannot find node with id " << n2)
                return false;
            }

            n3 = GetNodeIdFromPoint(switchgear->point3Id);
            if (n3 < 0)
            {
                OUTPUT_DEBUG_MESSAGE("Cannot find node with id " << n3)
                return false;
            }

            n4 = GetNodeIdFromPoint(switchgear->point4Id);
            if (n4 < 0)
            {
                OUTPUT_DEBUG_MESSAGE("Cannot find node with id " << n4)
                return false;
            }

            // create actual switchgear
            bResult = gridHandler->CreateFENSwitchgear(switchgear->id, n1, n2, n3, n4, switchgear->ip);
            if (!bResult)
            {
                return false;
            }

            continue;
        }

        if (c->type == CONFIG_TYPE_SOURCE)
        {
            int node;

            // cast config to source
            SourceConfig_t* source = (SourceConfig_t*)c;
            OUTPUT_DEBUG_MESSAGE("Creating a new source")

            // get node
            node = GetNodeIdFromPoint(source->pointId);

            // create new source
            // TODO
            continue;
        }
    }

    updated = true;
    gridHandler->SetLoaded();

    return true;
}

int GridFile::GetNodeIdFromPoint(uint16_t pointId)
{
    // iterate over all configs to find point
    for (auto c : gridFileConfig)
    {
        if (c->type == CONFIG_TYPE_POINT)
        {
            // cast config to point
            PointConfig_t* point = (PointConfig_t*)c;

            if (point->id == pointId)
            {
                return point->nodeId;
            }
        }
    }

    // no point with matching id found
    return -1;
}

void GridFile::ProcessRecvBuffer()
{
    // variables
    GenericCommandPacketHeader_t* header = (GenericCommandPacketHeader_t*)(recvBuffer);
    uint32_t command = header->command;
    bool bResult;

    // processs command
    switch(command)
    {
        case GRID_COMMAND_CLEAR_ALL:
        {
            // clear grid config
            OUTPUT_DEBUG_MESSAGE("Clearing grid...")
            ClearGrid();
            OUTPUT_DEBUG_MESSAGE("Grid cleared.")

            SendRespondPackage(header->header.connection, header->commandId, GRID_RESPOND_SUCCESS);

            break;
        }
        case GRID_COMMAND_LOAD_GRID_CONFIG:
        {
            // append to grid configuration
            OUTPUT_DEBUG_MESSAGE("Appending grid configuration to output....")
            bResult = AppendConfig();

            if (bResult)
            {
                SendRespondPackage(header->header.connection, header->commandId, GRID_RESPOND_SUCCESS);
                OUTPUT_DEBUG_MESSAGE("Grid configuration appended.")
            }
            else
            {
                SendRespondPackage(header->header.connection, header->commandId, GRID_RESPOND_SETUP_ERROR);
                OUTPUT_ERROR("Failure while appending grid configuration")
            }

            break;
        }
        case GRID_COMMAND_SETUP_GRID:
        {
            // try to load the config and setup the grid
            OUTPUT_DEBUG_MESSAGE("Loading in stored configuration...")
            bResult = LoadConfig();

            if (bResult)
            {
                SendRespondPackage(header->header.connection, header->commandId, GRID_RESPOND_SUCCESS);
                OUTPUT_DEBUG_MESSAGE("Grid configuration loaded.")
            }
            else
            {
                SendRespondPackage(header->header.connection, header->commandId, GRID_RESPOND_SETUP_ERROR);
                OUTPUT_DEBUG_MESSAGE("Cannot load in stored grid configuration.")
            }

            break;
        }
        case GRID_COMMAND_GET_CONFIG_LENGTH:
        {
            // send out length of grid configuration
            if (gridFileConfig.size() == 0)
            {
                SendRespondPackage(header->header.connection, header->commandId, 0);
                OUTPUT_DEBUG_MESSAGE("Trying to access non-existing grid configuration.")
            }
            else
            {
                SendGridFileLength(header->header.connection, header->commandId);
                OUTPUT_DEBUG_MESSAGE("Accessing grid configuration length by connection: " << header->header.connection)
            }

            break;
        }
        case GRID_COMMAND_GET_GRID_CONFIG:
        {
            // send out grid config
            if (gridFileConfig.size() == 0)
            {
                // nothing to send out
                SendRespondPackage(header->header.connection, header->commandId, GRID_RESPOND_GET_CONFIG_ERROR);
                OUTPUT_DEBUG_MESSAGE("Trying to access non-existing grid configuration.")
            }
            else
            {
                SendGridFileConfig(header->header.connection, header->commandId);
                OUTPUT_DEBUG_MESSAGE("Accessing grid configuration by connection: " << header->header.connection)
            }
            break;
        }
        default:
        {
            SendRespondPackage(header->header.connection, header->commandId, GRID_RESPOND_UNKNOWN_COMMAND);
            OUTPUT_DEBUG_MESSAGE("Unknown grid configuration command: " << command)
        }
    }

    return;
}

void GridFile::SendRespondPackage(uint16_t connection, uint32_t id, uint32_t respond)
{
    // lock send buffer first
    std::unique_lock lk(sendBufferLock);

    // clear buffer
    memset(sendBuffer, 0, sizeof(GenericCommandRespondHeader_t));

    // create package
    GenericCommandRespondHeader_t* r = (GenericCommandRespondHeader_t*)(sendBuffer);
    r->header.packetType = PACKET_TYPE_RESPOND;
    r->header.deviceType = DEVICE_TYPE_GRID;
    r->header.deviceId = 0;
    r->header.length = sizeof(GenericCommandRespondHeader_t);
    r->header.connection = connection;
    r->commandId = id;
    r->result = respond;

    // send data
    gridServer->SendData(sendBuffer, sizeof(GenericCommandRespondHeader_t));

    return;
}

void GridFile::SendGridFileLength(uint16_t connection, uint32_t id)
{
    // variables
    int numPackets;
    int elementsPerPacket;
    int elementsToSend;

     // lock send buffer and config
    std::unique_lock lk(sendBufferLock);
    std::unique_lock clk(configLock);

    // get number of elements
    elementsToSend = gridFileConfig.size();

    // calculate amount of packets that need to be send
    elementsPerPacket = (MAX_BUFFER_SIZE - sizeof(GridCommandGetGridFilePacketHeader_t)) / sizeof(GridFileElementConfig_t);
    numPackets = elementsToSend / elementsPerPacket;
    if (numPackets * elementsPerPacket < elementsToSend)
    {
        numPackets++;
    }

    // create respond packet
    // clear buffer
    memset(sendBuffer, 0, sizeof(GridCommandGetConfigLengthPacket_t));

    // create package
    GridCommandGetConfigLengthPacket_t* r = (GridCommandGetConfigLengthPacket_t*)(sendBuffer);
    r->commandRespondPacketHeader.header.packetType = PACKET_TYPE_RESPOND;
    r->commandRespondPacketHeader.header.deviceType = DEVICE_TYPE_GRID;
    r->commandRespondPacketHeader.header.deviceId = 0;
    r->commandRespondPacketHeader.header.length = sizeof(GridCommandGetConfigLengthPacket_t);
    r->commandRespondPacketHeader.header.connection = connection;
    r->commandRespondPacketHeader.commandId = id;
    r->commandRespondPacketHeader.result = GRID_RESPOND_GET_CONFIG_LENGTH;
    r->length = numPackets;

    // send data
    gridServer->SendData(sendBuffer, sizeof(GridCommandGetConfigLengthPacket_t));

    return;
}

void GridFile::SendGridFileConfig(uint16_t connection, uint32_t id)
{
    // variables
    int elementsToSend;
    int elementsPerPacket;
    int elementsInPacket;
    int numPackets;
    int packetLength;
    GridCommandGetGridFilePacket_t* packet = (GridCommandGetGridFilePacket_t*)sendBuffer;

    // lock send buffer and config
    std::unique_lock slk(sendBufferLock);
    std::unique_lock clk(configLock);

    // get number of elements
    elementsToSend = gridFileConfig.size();

    // check how many elements can fit into one packet
    elementsPerPacket = (MAX_BUFFER_SIZE - sizeof(GridCommandGetGridFilePacketHeader_t)) / sizeof(GridFileElementConfig_t);
    numPackets = elementsToSend / elementsPerPacket;
    if (numPackets * elementsPerPacket < elementsToSend)
    {
        numPackets++;
    }

    // fill header
    packet->getGridFileHeader.commandRespondPacketHeader.header.packetType = PACKET_TYPE_RESPOND;
    packet->getGridFileHeader.commandRespondPacketHeader.header.deviceType = DEVICE_TYPE_GRID;
    packet->getGridFileHeader.commandRespondPacketHeader.header.deviceId = 0;
    packet->getGridFileHeader.commandRespondPacketHeader.header.connection = connection;

    packet->getGridFileHeader.commandRespondPacketHeader.commandId = id;
    packet->getGridFileHeader.commandRespondPacketHeader.result = GRID_RESPOND_GET_CONFIG_DATA;

    packet->getGridFileHeader.version = version;
    packet->getGridFileHeader.subversion = subversion;
    packet->getGridFileHeader.numParts = numPackets;
    packet->getGridFileHeader.part = 0;
    
    // iterate over config elements and send out
    elementsInPacket = 0;
    for (auto elem : gridFileConfig)
    {
        // copy element into buffer
        memcpy(&packet->elements[elementsInPacket], elem, sizeof(GridFileElementConfig_t));

        // increase number of elements and check if packet needs to be send out
        elementsInPacket++;
        if (elementsInPacket >= elementsPerPacket)
        {
            // fill rest of packet and send out
            packet->getGridFileHeader.length = elementsPerPacket;
            packet->getGridFileHeader.part++;
            packetLength = sizeof(GridCommandGetGridFilePacketHeader_t) + sizeof(GridFileElementConfig_t) * elementsInPacket;
            if ((packetLength % 16) != 0)
            {
                // adjust to 16 bytes boundary
                packetLength += packetLength % 16;
            }
            packet->getGridFileHeader.commandRespondPacketHeader.header.length = packetLength;

            // send out
            gridServer->SendData(sendBuffer, packetLength);

            // reset element counter
            elementsInPacket = 0;
        }
    }

    // check if there is a incomplete packet that needs to be send out
    if (elementsInPacket != 0)
    {
        // fill rest of packet and send out
        packet->getGridFileHeader.length = elementsInPacket;
        packet->getGridFileHeader.part++;
        packetLength = sizeof(GridCommandGetGridFilePacketHeader_t) + sizeof(GridFileElementConfig_t) * elementsInPacket;
        if ((packetLength % 16) != 0)
        {
            // adjust to 16 bytes boundary
            packetLength += packetLength % 16;
        }
        packet->getGridFileHeader.commandRespondPacketHeader.header.length = packetLength;

        // send out
        gridServer->SendData(sendBuffer, packetLength);
    }

    // done, return
    return;
}