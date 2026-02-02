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

#define BREAKER_RESPOND_SUCCESS             0x0001
#define BREAKER_RESPOND_DATA                0x0002

#define BREAKER_RESPOND_FAILURE             0x0100
#define BREAKER_RESPOND_LOCKED              0x0101
#define BREAKER_RESPOND_TIMEOUT             0x0102

typedef struct
{
    uint32_t status;
    int32_t voltageNode1;
    int32_t voltageNode2;
    int32_t current;
} BreakerStatus_t;

typedef struct
{
    GenericCommandRespondHeader_t* respondHeader;
    BreakerStatus_t status;
} BreakerRespondData_t;

#define BREAKER_MODBUS_DATA_BUFFER_SIZE     16
#define BREAKER_MODBUS_COMMAND_BUFFER_SIZE  4

#define BREAKER_MODBUS_START_OF_DATA        0x0100
#define BREAKER_MODBUS_DATA_LENGTH          0x0008

#define BREAKER_MODBUS_START_OF_COMMAND     0x0000
#define BREAKER_MODBUS_COMMAND_LENGTH       0x0002