#include "gridhandler.h"
#include "../debugoutput.h"

GridHandler::GridHandler()
{
    gridServer = 0;
    gridStarted = false;
    gridLoaded = false;
    subsystemProcessMask = 0;
}

GridHandler::~GridHandler()
{
}

void GridHandler::Initialize(GridServer* server)
{
    // debug
    OUTPUT_DEBUG_MESSAGE("Initializing grid handler class...")

    // store grid server
    gridServer = server;

    // debug
    OUTPUT_DEBUG_MESSAGE("Initializing grid handler class done")

    return;
}

void GridHandler::Shutdown()
{
    // stop grid if not done
    if (gridStarted)
    {
        StopGrid();
    }

    // clear the grid
    ClearGrid();

    // reset grid server
    gridServer = 0;

    return;
}

void GridHandler::NewCommand(char* buffer, int size)
{
    // variables
    GenericPacketHeader_t* header = (GenericPacketHeader_t*)buffer;
    uint8_t deviceType;
    uint16_t deviceId;

    // extract device type and id
    deviceType = header->deviceType;
    deviceId = header->deviceId;

    switch (deviceType)
    {
        case DEVICE_TYPE_FENSWITCHGEAR:
        {
            // iterate over all switchgears
            for (auto e : gridSwitchgearList)
            {
                if (e->GetID() == deviceId)
                {
                    // forward command to switchgear
                    OUTPUT_DEBUG_MESSAGE("Forwarding command to switchgear")
                    e->NewCommand(buffer, size);
                }
            }
            break;
        }

        case DEVICE_TYPE_CONVERTER:
        {
            // iterate over all converters
            for (auto e : gridConverterList)
            {
                if (e->GetID() == deviceId)
                {
                    // forward command to converter
                    OUTPUT_DEBUG_MESSAGE("Forwarding command to converter")
                        e->NewCommand(buffer, size);
                }
            }
            break;
        }

        case DEVICE_TYPE_SCIBREAKBREAKER:
        {
            // iterate over all breakers
            for (auto e : gridSciBreakBreakerList)
            {
                if (e->GetID() == deviceId)
                {
                    // forward command to breaker
                    OUTPUT_DEBUG_MESSAGE("Forwarding command to breaker")
                        e->NewCommand(buffer, size);
                }
            }
            break;
        }

        default:
        {
            break;
        }
    }

    return;
}

void GridHandler::ConnectionClosed(uint16_t connection)
{
    // iterate over all devices and clear live data bit
    for (auto e : gridSwitchgearList)
    {   
        e->RemovePeriodicDataConnection(connection);
    }

    for (auto e : gridConverterList)
    {
        e->RemovePeriodicDataConnection(connection);
    }

    for (auto e : gridSciBreakBreakerList)
    {
        e->RemovePeriodicDataConnection(connection);
    }

    return;
}

bool GridHandler::StartGrid()
{
    // variables
    bool result;

    // check if grid is already started
    if (gridStarted)
    {
        OUTPUT_DEBUG_MESSAGE("Grid already started.")
        return false;
    }

    gridStarted = true;

    // start the modbus interface handler
    result = ModbusInterface::StartInterfaces();
    if (!result)
    {
        OUTPUT_DEBUG_MESSAGE("Cannot start modbus interfaces.")
        return false;
    }

    result = AdsInterface::StartInterfaces();
    if (!result)
    {
        OUTPUT_DEBUG_MESSAGE("Cannot start ADS interfaces")
        return false;
    }

    OUTPUT_DEBUG_MESSAGE("Grid started.")

    return true;
}

void GridHandler::StopGrid()
{
    // stop the modbus interfaces
    ModbusInterface::StopInterfaces();
    AdsInterface::StopInterfaces();

    gridStarted = false;

    OUTPUT_DEBUG_MESSAGE("Grid stopped.")

    return;
}

void GridHandler::ClearGrid()
{
    // variables

    // only clear the grid when it is not started
    if (gridStarted)
    {
        return;
    }

    // stop interface handler completely
    ModbusInterface::StopInterfaceHandler();

    // iterate over all lists and clear the devices
    for (auto e : gridSciBreakBreakerList)
    {
        // shutdown device
        e->Shutdown();
        delete e;
    }
    gridSciBreakBreakerList.clear();

    for (auto e : gridSwitchgearList)
    {
        // shutdown the device
        e->Shutdown();
        delete e;
    }
    gridSwitchgearList.clear();

    for (auto e : gridConverterList)
    {
        // shutdown and delete device
        e->Shutdown();
        delete e;
    }
    gridConverterList.clear();

    for (auto e : gridNodeList)
    {
        // close node
        delete e;
    }
    gridNodeList.clear();

    gridLoaded = false;

    return;
}

void GridHandler::SetLoaded()
{
    // variables
    bool result;

    // grid is loaded
    gridLoaded = true;

    // start the modbus interface handler
    result = ModbusInterface::StartInterfaceHandler();
    if (!result)
    {
        OUTPUT_DEBUG_MESSAGE("Cannot start modbus interface handler")
    }

    return;
}

bool GridHandler::IsGridLoaded()
{
    return gridLoaded;
}

bool GridHandler::IsGridStarted()
{
    return gridStarted;
}

void GridHandler::Update()
{
    // variables
    int mask;

    // check if something needs updating
    if (!subsystemProcessMask)
    {
        return;
    }

    // clone the mask
    {
        std::unique_lock lk(subsystemProcessLock);

        mask = subsystemProcessMask;
        subsystemProcessMask = 0;
    }

    // check which mask bit is set
    if (mask & SUBSYSTEM_PROCESS_FENSWITCHGEAR)
    {
        // update all switchgears
        for (auto e : gridSwitchgearList)
        {
            e->Update();
        }
    }

    if (mask & SUBSYSTEM_PROCESS_CONVERTER)
    {
        // update all converters
        for (auto e : gridConverterList)
        {
            e->Update();
        }
    }

    if (mask & SUBSYSTEM_PROCESS_SCIBREAK)
    {
        // update all breakers
        for (auto e : gridSciBreakBreakerList)
        {
            e->Update();
        }
    }
    return;
}

void GridHandler::SendData(char* buffer, int size)
{
    // forward to server
    gridServer->SendData(buffer, size);
    return;
}

void GridHandler::SendPeriodicData()
{
    // this iterates over all devices and send out data
    for (auto e : gridSwitchgearList)
    {
        e->SendPeriodicData();
    }

    for (auto e : gridConverterList)
    {
        e->SendPeriodicData();
    }

    for (auto e : gridSciBreakBreakerList)
    {
        e->SendPeriodicData();
    }

    return;
}

void GridHandler::SubsystemProcess(int mask)
{
    // get the lock for the mask
    std::unique_lock lk(subsystemProcessLock);

    // alter the mask
    subsystemProcessMask |= mask;

    // notify the grid server
    gridServer->SubsystemProcess(SUBSYSTEM_GRIDHANDLER);

    return;
}

bool GridHandler::CreateNode(int id)
{
    // create a new node
    // variables
    GridNode* node;

    // create new
    node = new GridNode(id);
    if (!node)
    {
        return false;
    }

    // store into list
    gridNodeList.push_back(node);

    OUTPUT_DEBUG_MESSAGE("Creating new node.")
    return true;
}

bool GridHandler::CreateBreaker(int id, int nodeID1, int nodeID2, uint32_t ip)
{
    // return for now
    OUTPUT_DEBUG_MESSAGE("Creating new breaker.")
    return true;
}

bool GridHandler::CreateSciBreakBreaker(int id, int nodeID1, int nodeID2, uint32_t ip)
{
    // create new sci break breaker
    // variables
    SciBreakBreaker* breakerDevice;
    GridNode* n1;
    GridNode* n2;
    bool result;

    // Get nodes
    n1 = GetNodeFromID(nodeID1);
    if (!n1)
    {
        return false;
    }

    n2 = GetNodeFromID(nodeID2);
    if (!n2)
    {
        return false;
    }

    // create new
    breakerDevice = new SciBreakBreaker();
    if (!breakerDevice)
    {
        return false;
    }

    result = breakerDevice->Initialize(this, id, n1, n2, ip, 502, 0);
    if (!result)
    {
        delete breakerDevice;
        return false;
    }

    gridSciBreakBreakerList.push_back(breakerDevice);
    OUTPUT_DEBUG_MESSAGE("New sci break breaker created")

}

bool GridHandler::CreateConverter(int id, int nodeID1, int nodeID2, uint32_t ip, uint16_t config)
{
    // create a new converter
    // variables
    Converter* converterDevice;
    GridNode* n1;
    GridNode* n2;
    bool result;

    // Get nodes
    n1 = GetNodeFromID(nodeID1);
    if (!n1)
    {
        return false;
    }

    n2 = GetNodeFromID(nodeID2);
    if (!n2)
    {
        return false;
    }

    // create new
    converterDevice = new Converter();
    if (!converterDevice)
    {
        return false;
    }

    result = converterDevice->Initialize(this, id, n1, n2, ip, config);
    if (!result)
    {
        delete converterDevice;
        return false;
    }

    // append to list
    gridConverterList.push_back(converterDevice);
    OUTPUT_DEBUG_MESSAGE("New converter created")

    return true;
}

bool GridHandler::CreateFENSwitchgear(int id, int nodeID1, int nodeID2, int nodeID3, int nodeID4, uint32_t ip)
{
    // create a new switchgear
    // variables
    FENSwitchgear* switchgear;
    bool result;
    GridNode* n1;
    GridNode* n2;
    GridNode* n3;
    GridNode* n4;

    // Get nodes
    n1 = GetNodeFromID(nodeID1);
    if (!n1)
    {
        return false;
    }

    n2 = GetNodeFromID(nodeID2);
    if (!n2)
    {
        return false;
    }

    n3 = GetNodeFromID(nodeID3);
    if (!n3)
    {
        return false;
    }

    n4 = GetNodeFromID(nodeID4);
    if (!n4)
    {
        return false;
    }

    // create new
    switchgear = new FENSwitchgear();
    if (!switchgear)
    {
        return false;
    }

    // initialize
    result = switchgear->Initialize(this, id, n1, n2, n3, n4, ip, 502, 0);
    if (!result)
    {
        delete switchgear;
        return false;
    }

    // append to list
    gridSwitchgearList.push_back(switchgear);

    OUTPUT_DEBUG_MESSAGE("Creating new switchgear.")
    return true;
}

GridNode* GridHandler::GetNodeFromID(int id)
{
    for (auto e : gridNodeList)
    {
        if (e->GetID() == id)
        {
            return e;
        }
    }
    return 0;
}