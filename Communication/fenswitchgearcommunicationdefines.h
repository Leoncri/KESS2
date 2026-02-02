#pragma once

#include "genericcommunicationdefines.h"

#define     FENSWITCHGEAR_COMMAND_SET_SWITCH            0x0100
#define     FENSWITCHGEAR_COMMAND_RESET_SWITCH          0x0200

#define     FENSWITCHGEAR_COMMAND_GET_DATA              0x0300

#define     FENSWITCHGEAR_COMMAND_PERIODIC_DATA         0x0400
#define     FENSWICHTGEAR_COMMAND_PERIODIC_DATA_ON      0x0001
#define     FENSWICHTGEAR_COMMAND_PERIODIC_DATA_OFF     0x0002

#define     FENSWITCHGEAR_RESPOND_SUCCESS               0x0001
#define     FENSWITCHGEAR_RESPOND_DATA                  0x0002

#define     FENSWITCHGEAR_RESPOND_ERROR                 0x0100
#define     FENSWITCHGEAR_RESPOND_UNKNOWN_COMMAND       0x0101
#define     FENSWITCHGEAR_RESPOND_BUFFER_SIZE           0x0102
#define     FENSWITCHGEAR_RESPOND_NOT_READY             0x0103

#define     FENSWITCHGEAR_RESPOND_LOCKED                0x0111

#define     FENSWITCHGEAR_SWITCH_MASK                   0x001F

typedef struct
{
    DeviceDataPacketHeader_t header;
    uint8_t closedSwitches;
    uint8_t lockedSwitches;
    uint8_t hvOnLine;
    uint8_t status;
    uint16_t voltageP;
    uint16_t voltageM;
    uint16_t currents [4];
} FENSwitchgearDeviceData_t;

#define FENSWITCHGEAR_STATUS_ONLINE                     0x01