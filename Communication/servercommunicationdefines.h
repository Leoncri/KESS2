#pragma once

/*
//  This file describes the communication defines for communicating with the server in general
//
*/

#include <stdint.h>
#include "genericcommunicationdefines.h"

#define SERVER_COMMAND_START_GRID       0x0001
#define SERVER_COMMAND_STOP_GRID        0x0002

#define SERVER_STATUS_GRID_LOADED       0x0001
#define SERVER_STATUS_GRID_STARTED      0x0002

typedef struct
{
    GenericCommandRespondHeader_t header;
    uint16_t usedConnections;
    uint16_t status;
    uint16_t serverLoad;
    uint16_t connectedDevices;
    uint16_t fileVersion;           // this increases every time a new file is loaded (NO actual file version)
    uint16_t rsvd1;
    uint32_t rsvd2;
} ServerStatusPacket_t;


#define SERVER_RESPOND_SUCCESS          0x0001
#define SERVER_STATUS_DATA              0x0010

#define SERVER_RESPOND_FAILURE          0x0100
#define SERVER_RESPOND_UNKNOWN_COMMAND  0x0101