#pragma once

#include <stdint.h>

#define CONFIG_TYPE_NODE            1
#define CONFIG_TYPE_POINT           2
#define CONFIG_TYPE_SEGMENT         3
#define CONFIG_TYPE_CONVERTER       4
#define CONFIG_TYPE_BREAKER         5
#define CONFIG_TYPE_FENSWITCHGEAR   6
#define CONFIG_TYPE_SOURCE          7
#define CONFIG_TYPE_SCIBREAKBREAKER 8

typedef struct NodeConfig
{
    uint16_t type;
    uint16_t id;
    uint16_t nodeType;
    uint16_t rsvd;
} NodeConfig_t;

typedef struct PointConfig
{
    uint16_t type;
    uint16_t id;
    uint16_t nodeId;
    uint16_t rsvd;
    uint16_t posX;
    uint16_t posY;
} PointConfig_t;

typedef struct SegmentConfig
{
    uint16_t type;
    uint16_t id;        // not actually used
    uint16_t point1Id;
    uint16_t point2Id;
} SegmentConfig_t;

typedef struct ConverterConfig
{
    uint16_t type;
    uint16_t id;
    uint16_t point1Id;
    uint16_t point2Id;
    uint16_t config;
    uint16_t port;
    uint32_t ip;
    uint16_t posX;
    uint16_t posY;
    uint16_t rotation;
    uint16_t rsvd;
    uint8_t name[24];
} ConverterConfig_t;

typedef struct BreakerConfig
{
    uint16_t type;
    uint16_t id;
    uint16_t point1Id;
    uint16_t point2Id;
    uint16_t config;
    uint16_t port;
    uint32_t ip;
    uint16_t posX;
    uint16_t posY;
    uint16_t rotation;
    uint16_t rsvd;
    uint8_t name[24];
} BreakerConfig_t;

typedef struct FENSwitchgearConfig
{
    uint16_t type;
    uint16_t id;
    uint16_t point1Id;
    uint16_t point2Id;
    uint16_t point3Id;
    uint16_t point4Id;
    uint16_t port;
    uint16_t config;
    uint32_t ip;
    uint16_t posX;
    uint16_t posY;
    uint16_t rotation;
    uint16_t rsvd;
    uint8_t name[20];
} FENSwitchgearConfig_t;

typedef struct SourceConfig
{
    uint16_t type;
    uint16_t id;
    uint16_t pointId;
    uint16_t posX;
    uint16_t posY;
    uint16_t rotation;
    uint8_t name[36];
} SourceConfig_t;

typedef struct GridFileElementConfig
{
    uint16_t type;
    uint16_t elementData[23];
} GridFileElementConfig_t;