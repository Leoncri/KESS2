#include "gridserver.h"
#include "../debugoutput.h"

GridServer::GridServer()
{
    // initialize variables
    gridFile = 0;
    gridHandler = 0;
    gridStarted = false;
    eventHandler = 0;
    serverCommunication = 0;
    stopThreads = true;
    updateThreadRunning = false;
    periodicDataThreadRunning = false;
    backgroundThreadRunning = false;
    gridFileVersion = 1;
}

GridServer::~GridServer()
{
}

bool GridServer::Initialize()
{
    // Debug stuff
    OUTPUT_DEBUG_MESSAGE("Grid server initialize started")

    // variables
    bool result;

    // create communication class
    serverCommunication = new ServerCommunication();
    if (!serverCommunication)
    {
        return false;
    }

    // create event handler
    eventHandler = new EventHandler();
    if (!eventHandler)
    {
        return false;
    }

    // create grid file
    gridFile = new GridFile();
    if (!gridFile)
    {
        return false;
    }

    // create grid handler
    gridHandler = new GridHandler();
    if (!gridHandler)
    {
        return false;
    }

    // init server communication
    result = serverCommunication->Initialize(this);
    if (!result)
    {
        return false;
    }

    // init grid file
    gridFile->Initialize(this, gridHandler);

    // init grid handler
    gridHandler->Initialize(this);

    // Debug stuff
    OUTPUT_DEBUG_MESSAGE("Grid server initialize ended successfully")

    return true;
}

void GridServer::Shutdown()
{
    // delete every instance that was created
    // Event handler
    if (eventHandler)
    {
        delete eventHandler;
        eventHandler = 0;
    }

    // grid file
    if (gridFile)
    {
        gridFile->Shutdown();
        delete gridFile;
        gridFile = 0;
    }

    // grid handler
    if (gridHandler)
    {
        gridHandler->Shutdown();
        delete gridHandler;
        gridHandler = 0;
    }

    // grid communication
    if (serverCommunication)
    {
        serverCommunication->Shutdown();
        delete serverCommunication;
        serverCommunication = 0;
    }

    return;
}

bool GridServer::StartServer()
{
    // variables
    bool result;

    // debug
    OUTPUT_INFO("Starting server...")

    // start threads of communication server
    result = serverCommunication->StartThreads();
    if (!result)
    {
        OUTPUT_ERROR("Cannot start communication threads.")
        return false;
    }

    // start own update and periodic data thread
    result = StartThreads();
    if (!result)
    {
        OUTPUT_ERROR("Cannot start update or periodic data thread")
        return false;
    }

    // debug
    OUTPUT_INFO("Server started.")

    return true;
}

void GridServer::StopServer()
{
    // debug
    OUTPUT_INFO("Stopping server...")

    // stop own threads
    StopThreads();

    // stop communicaiton threads
    serverCommunication->StopThreads();

    // debug
    OUTPUT_INFO("Server stopped.")

    return;
}

bool GridServer::StartThreads()
{
    // check if one thread is already running
    if (updateThreadRunning || periodicDataThreadRunning || backgroundThreadRunning)
    {
        return false;
    }
    stopThreads = false;

    // Debug stuff
    OUTPUT_DEBUG_MESSAGE("Grid server is starting threads")

    // start update thread
    updateThread = std::thread(UpdateThreadCallbackFunction, this);

    // start periodic data thread
    periodicDataThread = std::thread(PeriodicDataThreadCallbackFunction, this);

    // start background thread
    backgroundThread = std::thread(BackgroundThreadCallbackFunction, this);

    return true;
}

void GridServer::StopThreads()
{
    // Debug stuff
    OUTPUT_DEBUG_MESSAGE("Grid server is stopping threads")

    // stop threads
    stopThreads = true;

    // use an event to get the update thread running
    eventHandler->SetEvent();

    // wait for threads to end
    updateThread.join();
    periodicDataThread.join();
    backgroundThread.join();

    // Debug stuff
    OUTPUT_DEBUG_MESSAGE("Grid server threads stopped")

    return;
}

void GridServer::NewCommand(char* buffer, int size)
{
    // variables
    GenericCommandPacketHeader_t* header = (GenericCommandPacketHeader_t*)buffer;
    uint8_t deviceType;

    // get device type and dispatch packet
    deviceType = header->header.deviceType;
    OUTPUT_DEBUG_MESSAGE("New Command incoming")

    // dispatch
    if (IsDeviceType(deviceType, {DEVICE_TYPE_SERVER}))
    {
        // process here
        ProcessCommand(buffer, size);
    }
    else if (IsDeviceType(deviceType, {DEVICE_TYPE_GRID}))
    {
        // command goes to grid file
        gridFile->NewCommand(buffer, size);
    }
    else if (IsDeviceType(deviceType, {DEVICE_TYPE_BREAKER, DEVICE_TYPE_CONVERTER, DEVICE_TYPE_FENSWITCHGEAR, DEVICE_TYPE_SCIBREAKBREAKER}))
    {
        // command goes to grid handler
        gridHandler->NewCommand(buffer, size);
    }

    return;
}

void GridServer::ConnectionClosed(uint16_t connection)
{
    // call grid handler
    gridHandler->ConnectionClosed(connection);
    
    return;
}

void GridServer::SendData(char* buffer, int size)
{
    // send out data
    serverCommunication->SendData(buffer, size);

    return;
}

void GridServer::SubsystemProcess(uint32_t subsystem)
{
    // variables

    // set event handler
    eventHandler->SetEvent();

    return;
}

void GridServer::UpdateThreadFunction()
{
    // Debug stuff
    OUTPUT_DEBUG_MESSAGE("Grid server update thread started")

    // variables

    // set thread to running
    updateThreadRunning = true;

    // thread loop
    while(!stopThreads)
    {
        // basically wait until event handler is notified
        eventHandler->WaitForEvent();

        // call update function for both grid file and grid handler
        OUTPUT_DEBUG_MESSAGE("Grid server update thread invoked.")
        gridFile->Update();
        gridHandler->Update();
    }

    // set thread to stopped
    updateThreadRunning = false;

    // Debug stuff
    OUTPUT_DEBUG_MESSAGE("Grid server update thread stopped")

    return;
}

void GridServer::PeriodicDataThreadFunction()
{
    // Debug stuff
    OUTPUT_DEBUG_MESSAGE("Grid server periodic data thread started")

    // variables
    int size;
    char* buffer;

    // set thread to running
    periodicDataThreadRunning = true;

    // thread loop
    while(!stopThreads)
    {
        // get time and calculate when to call next iteration
        auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(SERVER_PERIODIC_DATA_PERIOD_MS);

        // send server status to all connections
        SendServerStatus();

        // ask grid handler to send out new data
        gridHandler->SendPeriodicData();

        // wait for next iteration if needed
        if (x > std::chrono::steady_clock::now())
        {
            std::this_thread::sleep_until(x);
        }
    }

    // set thread to stopped
    periodicDataThreadRunning = false;

    // Debug stuff
    OUTPUT_DEBUG_MESSAGE("Grid server periodic data thread stopped")

    return;
}

void GridServer::BackgroundThreadFunction()
{
    // Debug stuff
    OUTPUT_DEBUG_MESSAGE("Background thread started")

    // variables
    auto stime = std::chrono::milliseconds(BACKGROUND_TASK_PERIOD_MS);

    // set thread to running
    backgroundThreadRunning = true;

    // thread loop
    while (!stopThreads)
    {
        // call background thread function
        BackgroundTask::TaskFunction();

        // sleep for a certain time
        std::this_thread::sleep_for(stime);
    }

    // thread stopped
    backgroundThreadRunning = false;

    // Debug
    OUTPUT_DEBUG_MESSAGE("Background thread stopped")

    return;
}

void GridServer::ProcessCommand(char* buffer, int size)
{
    // variables
    GenericCommandPacketHeader_t* packet = (GenericCommandPacketHeader_t*)buffer;
    uint32_t command;
    bool result;
    
    // get command
    command = packet->command;

    // process
    switch(command)
    {
        case SERVER_COMMAND_START_GRID:
        {
            // call function for starting grid
            result = StartGrid();
            if (result)
            {
                SendRespondPacket(packet->header.connection, packet->commandId, SERVER_RESPOND_SUCCESS);
            }
            else
            {
                SendRespondPacket(packet->header.connection, packet->commandId, SERVER_RESPOND_FAILURE);
            }
            break;
        }
        case SERVER_COMMAND_STOP_GRID:
        {
            // call function for stopping the grid
            StopGrid();
            SendRespondPacket(packet->header.connection, packet->commandId, SERVER_RESPOND_SUCCESS);
            break;
        }
        default:
        {
            OUTPUT_DEBUG_MESSAGE("Unknown command.")
            SendRespondPacket(packet->header.connection, packet->commandId, SERVER_RESPOND_UNKNOWN_COMMAND);
        }
    }

    return;
}

bool GridServer::StartGrid()
{
    // check if grid can be started (TODO)
    OUTPUT_DEBUG_MESSAGE("Starting grid.")
    gridHandler->StartGrid();
    return true;
}

void GridServer::StopGrid()
{
    // stop the grid
    gridHandler->StopGrid();
    return;
}

bool GridServer::IsDeviceType(uint8_t type, std::initializer_list<uint8_t> list)
{
    // iterate over all list elements and return true if one is hit
    for (auto elem : list)
    {
        if (elem == type)
        {
            return true;
        }
    }

    return false;
}

void GridServer::SendRespondPacket(uint16_t connection, uint32_t id, uint32_t respond)
{
    // variables
    GenericCommandRespondHeader_t* packet = (GenericCommandRespondHeader_t*)sendBuffer;

    // get lock
    std::unique_lock lk(sendBufferLock);

    // generate packet
    packet->header.packetType = PACKET_TYPE_RESPOND;
    packet->header.deviceType = DEVICE_TYPE_SERVER;
    packet->header.deviceId = 0;
    packet->header.connection = connection;
    packet->header.length = sizeof(GenericCommandRespondHeader_t);
    packet->commandId = id;
    packet->result = respond;

    // send packet
    serverCommunication->SendData(sendBuffer, sizeof(GenericCommandRespondHeader_t));

    return;
}

void GridServer::SendServerStatus()
{
    // variables
    ServerStatusPacket_t* packet = (ServerStatusPacket_t*)sendBuffer;
    uint16_t status = 0;

    // get lock
    std::unique_lock lk(sendBufferLock);

    // status
    if (gridHandler->IsGridLoaded())
    {
        status |= SERVER_STATUS_GRID_LOADED;
    }

    if (gridHandler->IsGridStarted())
    {
        status |= SERVER_STATUS_GRID_STARTED;
    }

    if (gridFile->WasUpdated())
    {
        gridFileVersion++;
        if (gridFileVersion > 10000) gridFileVersion = 1;
    }

    packet->header.header.packetType = PACKET_TYPE_RESPOND;
    packet->header.header.deviceType = DEVICE_TYPE_SERVER;
    packet->header.header.deviceId = 0;
    packet->header.header.connection = 0xFFFF;
    packet->header.header.length = sizeof(ServerStatusPacket_t);
    packet->header.commandId = 0;
    packet->header.result = SERVER_STATUS_DATA;
    packet->usedConnections = 0;
    packet->status = status;
    packet->serverLoad = 0;
    packet->connectedDevices = 0;
    packet->fileVersion = gridFileVersion;

    // send packet
    serverCommunication->SendData(sendBuffer, sizeof(ServerStatusPacket_t));

    return;
}

void UpdateThreadCallbackFunction(GridServer* server)
{
    // just call the update function of the server
    server->UpdateThreadFunction();

    return;
}

void PeriodicDataThreadCallbackFunction(GridServer* server)
{
    // just call the periodic data function of the server
    server->PeriodicDataThreadFunction();

    return;
}

void BackgroundThreadCallbackFunction(GridServer* server)
{
    // just call the backgroudn thread function of the server
    server->BackgroundThreadFunction();

    return;
}