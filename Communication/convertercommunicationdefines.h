#pragma once

#include "genericcommunicationdefines.h"
#include "converterDataStructure.h"

#define CONVERTER_COMMAND_SET_MODE              0x0100

#define CONVERTER_COMMAND_UPDATE_DATA           0x0200

#define CONVERTER_COMMAND_PERIODIC_DATA         0x0300
#define CONVERTER_COMMAND_PERIODIC_DATA_ON      0x0001
#define CONVERTER_COMMAND_PERIODIC_DATA_OFF     0x0002

#define CONVERTER_RESPOND_SUCCESS               0x0001
#define CONVERTER_RESPOND_DATA                  0x0002

#define CONVERTER_RESPOND_ERROR                 0x0100
#define CONVERTER_RESPOND_UNKNOWN_COMMAND       0x0101
#define CONVERTER_RESPOND_BUFFER_SIZE           0x0102
#define CONVERTER_RESPOND_NOT_READY             0x0103

#define CONVERTER_MODE_MASK                     0x00FF
#define CONVERTER_DATA_MASK                     0x00FF
#define CONVERTER_COMMAND_MASK                  0xFF00

#define CONVERTER_USE_ADS                       0x0001

typedef struct
{
    GenericCommandPacketHeader_t header;
    uint16_t data[4];
    uint32_t rsvd[2];
} ConverterUpdateData_t;

typedef struct
{
    DeviceDataPacketHeader_t header;
    uint32_t status;
    uint32_t rsvd;
    converterDataStructure data;
} ConverterDeviceData_t;

#define CONVERTER_STATUS_ONLINE                 0x01