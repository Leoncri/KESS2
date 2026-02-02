#pragma once

class GridFile;

#include <string>
#include <list>
#include <mutex>

#include "gridserver.h"
#include "gridhandler.h"
#include "../Communication/gridfiledatatypes.h"
#include "../Communication/gridcommunicationdefines.h"

#include "../config.h"

class GridFile
{
public:
    // Constructor and Destructor
    GridFile();
    ~GridFile();

    // set server to load grid into
    void Initialize(GridServer* server, GridHandler* handler);
    void Shutdown();

    // callbacks for communication
    int NewCommand(char* buffer, int size);

    // update function callback
    bool Update();

    // check if file was updated
    bool WasUpdated();

private:
    // function to load and store the grid
    void ClearGrid();
    bool AppendConfig();
    bool LoadConfig();

    // function for getting nodes of points
    int GetNodeIdFromPoint(uint16_t pointId);

    // function for processing the recv buffer
    void ProcessRecvBuffer();

    // function for setting up the send buffer and getting its data
    void SendRespondPackage(uint16_t connection, uint32_t id, uint32_t respond);

    // function for sending back the grid configuration
    void SendGridFileLength(uint16_t connection, uint32_t id);
    void SendGridFileConfig(uint16_t connection, uint32_t id);

private:
    // Server to load the config into
    GridServer* gridServer;
    GridHandler* gridHandler;

    // Config data
    std::list<GridFileElementConfig_t*> gridFileConfig;
    int32_t version;
    int32_t subversion;
    int32_t numParts;

    // lock
    std::mutex recvBufferLock;
    std::mutex sendBufferLock;
    std::mutex configLock;
    bool recvBufferReady;

    // data buffers
    char recvBuffer[MAX_BUFFER_SIZE];
    char sendBuffer[MAX_BUFFER_SIZE];

    // update
    bool updated;
};