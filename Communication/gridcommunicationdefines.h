#pragma once

/*
//  This file describes the communication defines for communicating with the grid in general
//
*/

#include <stdint.h>
#include "genericcommunicationdefines.h"
#include "gridfiledatatypes.h"

// COMMANDS
#define GRID_COMMAND_BASE               0x1000
#define GRID_COMMAND_CONFIG             0x2000

// build up grid
// clear internal structure
#define GRID_COMMAND_CLEAR_ALL            (0x0001 | GRID_COMMAND_CONFIG)

// load grid topology file
#define GRID_COMMAND_LOAD_GRID_CONFIG     (0x0101 | GRID_COMMAND_CONFIG)
#define GRID_COMMAND_GET_GRID_CONFIG      (0x0102 | GRID_COMMAND_CONFIG)
#define GRID_COMMAND_GET_CONFIG_LENGTH    (0x0103 | GRID_COMMAND_CONFIG)

// setup grid
#define GRID_COMMAND_SETUP_GRID           (0x0201 | GRID_COMMAND_CONFIG)

typedef struct
{
    GenericCommandPacketHeader_t commandPacketHeader;
    int32_t version;            // version of the file
    int32_t subversion;         // subversion of the file
    int16_t numParts;           // number of parts when the configuration comes in multiple parts
    int16_t part;               // when the configuration comes in mutliple parts, this will define the part number
    uint32_t length;            // number of GridFileElementConfig_t structure in payload file
} GridCommandLoadGridFilePacketHeader_t;

typedef struct
{
    GridCommandLoadGridFilePacketHeader_t loadGridFileHeader;
    GridFileElementConfig_t elements[20];          // array of GridFileElementConfig_t structures (n of)
} GridCommandLoadGridFilePacket_t;

typedef struct
{
    GenericCommandRespondHeader_t commandRespondPacketHeader;
    uint32_t length;
    uint32_t rsvd[3];
} GridCommandGetConfigLengthPacket_t;

typedef struct
{
    GenericCommandRespondHeader_t commandRespondPacketHeader;
    uint32_t version;           // version of file
    uint32_t subversion;        // subversion
    uint16_t numParts;
    uint16_t part;
    uint32_t length;
} GridCommandGetGridFilePacketHeader_t;

typedef struct
{
    GridCommandGetGridFilePacketHeader_t getGridFileHeader;
    GridFileElementConfig_t elements[20];          // array of GridFieleElementConfig_t structures (n of)
} GridCommandGetGridFilePacket_t;


// respond messages
#define GRID_RESPOND_SUCCESS                0x0001
#define GRID_RESPOND_GET_CONFIG_DATA        0x0002
#define GRID_RESPOND_GET_CONFIG_LENGTH      0x0003

#define GRID_RESPOND_FAILURE                0x0100

#define GRID_RESPOND_UNKNOWN_COMMAND        0x0101
#define GRID_RESPOND_BUFFER_SIZE            0x0102
#define GRID_RESPOND_NOT_READY              0x0103
#define GRID_RESPOND_SETUP_ERROR            0x0104
#define GRID_RESPOND_GET_CONFIG_ERROR       0x0105