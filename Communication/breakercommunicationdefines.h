#pragma once

#include "genericcommunicationdefines.h"

#define BREAKER_COMMAND_RESET               0x0100

#define BREAKER_COMMAND_BREAKER             0x0200
#define BREAKER_COMMAND_OPEN_BREAKER        0x0001
#define BREAKER_COMMAND_CLOSE_BREAKER       0x0002

#define BREAKER_COMMAND_GET_DATA            0x0300

#define BREAKER_COMMAND_PERIODIC_DATA       0x0400
#define BREAKER_COMMAND_PERIODIC_DATA_ON    0x0001
#define BREAKER_COMMAND_PERIODIC_DATA_OFF   0x0002

#define BREAKER_COMMAND_UPDATE_DATA         0x0500

#define BREAKER_COMMAND_ON_OFF              0x0600
#define BREAKER_COMMAND_TURN_ON             0x0001
#define BREAKER_COMMAND_TURN_OFF            0x0002

#define BREAKER_RESPOND_SUCCESS             0x0001
#define BREAKER_RESPOND_DATA                0x0002

#define BREAKER_RESPOND_FAILURE             0x0100
#define BREAKER_RESPOND_LOCKED              0x0101
#define BREAKER_RESPOND_TIMEOUT             0x0102

#define BREAKER_RESPOND_UNKNOWN_COMMAND     0x0200

#define BREAKER_COMMAND_MASK                0xFF00
#define BREAKER_DATA_MASK                   0x00FF

typedef struct
{
    GenericCommandPacketHeader_t header;
    uint16_t data[4];
    uint32_t rsvd[2];
} BreakerUpdateData_t;

typedef struct
{
    uint32_t status;
    int16_t voltageTop;
    int16_t voltageBot;
    int16_t currentTop;
    int16_t currentBot;
    int16_t tripLevelTop;
    int16_t tripLevelBot;
} BreakerStatus_t;

typedef struct
{
    DeviceDataPacketHeader_t respondHeader;
    BreakerStatus_t status;
} BreakerDeviceData_t;

#define BREAKER_STATUS_ONLINE               0x1
#define BREAKER_STATUS_CLOSED_TOP           0x2
#define BREAKER_STATUS_CLOSED_BOT           0x4
#define BREAKER_FAULT                       0x8
#define BREAKER_TRIP                        0x10


#define BREAKER_MODBUS_DATA_BUFFER_SIZE     16
#define BREAKER_MODBUS_COMMAND_BUFFER_SIZE  4
/*
#define BREAKER_MODBUS_START_OF_DATA        0x0100
#define BREAKER_MODBUS_DATA_LENGTH          0x0008

#define BREAKER_MODBUS_START_OF_COMMAND     0x0000
#define BREAKER_MODBUS_COMMAND_LENGTH       0x0002

*/