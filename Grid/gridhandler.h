#pragma once

class GridHandler;

#include "gridserver.h"
#include "../Devices/gridnodeclass.h"
#include "../Devices/fenswitchgeardevice.h"
#include "../Devices/scibreakbreakerdevice.h"
#include "../Devices/converterdevice.h"

#include <list>
#include <mutex>

#define SUBSYSTEM_PROCESS_CONVERTER         0x1
#define SUBSYSTEM_PROCESS_BREAKER           0x2
#define SUBSYSTEM_PROCESS_FENSWITCHGEAR     0x4
#define SUBSYSTEM_PROCESS_SCIBREAK          0x8

class GridHandler
{
public:
    // constructor and destructor
    GridHandler();
    ~GridHandler();

    // init and shutdown
    void Initialize(GridServer* server);
    void Shutdown();

    // new command and connection handling
    void NewCommand(char* buffer, int size);
    void ConnectionClosed(uint16_t connection);

    // Starting and stopping the grid
    bool StartGrid();
    void StopGrid();
    void ClearGrid();
    bool IsGridLoaded();
    bool IsGridStarted();
    void SetLoaded();

    // update function
    void Update();

    // send single data
    void SendData(char* buffer, int size);

    // periodic data access
    void SendPeriodicData();

    // function for asking for update function
    void SubsystemProcess(int mask);

    // create grid elements
    bool CreateNode(int id);
    bool CreateConverter(int id, int nodeID1, int nodeID2, uint32_t ip, uint16_t config);
    bool CreateBreaker(int id, int nodeID1, int nodeID2, uint32_t ip);
    bool CreateFENSwitchgear(int id, int nodeID1, int nodeID2, int nodeID3, int nodeID4, uint32_t ip);
    bool CreateSciBreakBreaker(int id, int nodeID1, int nodeID2, uint32_t ip);

private:
    // functions
    GridNode* GetNodeFromID(int nodeID);

private:
    // variables
    GridServer* gridServer;
    bool gridStarted;
    bool gridLoaded;

    // list of switchgears
    std::list<GridNode*> gridNodeList;
    std::list<FENSwitchgear*> gridSwitchgearList;
    std::list<Converter*> gridConverterList;
    std::list< SciBreakBreaker*> gridSciBreakBreakerList;

    // mask and lock for subsystem process
    int subsystemProcessMask;
    std::mutex subsystemProcessLock;
};