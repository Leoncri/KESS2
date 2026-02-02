#pragma once

// generic data types for server communication

#include <stdint.h>

/*
typedef struct GenericPacketDescriptor
{
    GenericPacketDescriptor* next;
    GenericPacketDescriptor* prev;
    uint32_t length;
    char data[];
} GenericPacketDescriptor_t;
*/
#define PACKET_TYPE_ISALIVE         0x00
#define PACKET_TYPE_COMMAND         0x01
#define PACKET_TYPE_RESPOND         0x81
#define PACKET_TYPE_ERROR           0x82
#define PACKET_TYPE_DEVICEDATA      0x11

#define DEVICE_TYPE_NONE                0x00
#define DEVICE_TYPE_SERVER              0x01
#define DEVICE_TYPE_GRID                0x02
#define DEVICE_TYPE_CONVERTER           0x41
#define DEVICE_TYPE_BREAKER             0x42
#define DEVICE_TYPE_FENSWITCHGEAR       0xC1
#define DEVICE_TYPE_SCIBREAKBREAKER     0xC2

#define ERROR_TYPE_UNSUPPORTED      0x01

typedef struct
{
    uint8_t packetType;
    uint8_t deviceType;
    uint16_t deviceId;
    uint16_t length;            // All packets have length of n * 16 bytes
    uint16_t connection;        // the interface the packet came from, is filled by the server communication class during readout
} GenericPacketHeader_t;

typedef struct
{
    GenericPacketHeader_t header;
    uint32_t command;           // command specified in *communicationdefines.h
    uint32_t commandId;         // random command id, will be send back for response packet
} GenericCommandPacketHeader_t;

typedef struct
{
    GenericPacketHeader_t header;
    uint32_t result;            // result of the command
    uint32_t commandId;         // command id responding to
} GenericCommandRespondHeader_t;

typedef struct
{
    GenericPacketHeader_t header;
    uint32_t rsvd0;
    uint32_t rsvd1;
} IsAlivePacket_t;

typedef struct
{
    GenericPacketHeader_t header;
    uint32_t error;
    uint32_t rsvd0;
} ErrorPacket_t;

typedef struct
{
    GenericPacketHeader_t header;
    uint32_t id;                    // upcounting
    uint32_t rsvd;
} DeviceDataPacketHeader_t;