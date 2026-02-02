#ifndef _PGSCONVERTERCOMMUNICATION_
#define _PGSCONVERTERCOMMUNICATION_

#include <stdint.h>

typedef uint16_t uf16_t;    // 10 bits significand, 6 bit exponent, actual number as (significand) * 10 ^ (exponent)
#define CONVERTER_NEGATIVE_OFFSET   0x8000

// defines for status of converter
#define STATUS_OK                   0x1
#define STATUS_WARNING              0x2
#define STATUS_ERROR                0x4
#define STATUS_MISSING_LIGHTLOOP    0x8

// defines for warnings
#define WARNING_TEMP                0x1

// defines for errors
#define ERROR_TEMP                  0x01
#define ERROR_OVERVOLTAGE_1         0x10
#define ERROR_OVERVOLTAGE_2         0x20
#define ERROR_OVERCURRENT_1         0x40
#define ERROR_OVERCURRENT_2         0x80

// defines for modes
#define MODE_OFF                    1
#define MODE_IDLE                   2
#define MODE_VOLTAGE_CONTROL_1      3
#define MODE_VOLTAGE_CONTROL_2      4
#define MODE_DROOP_1                5
#define MODE_DROOP_2                6
#define MODE_POWER                  7
#define MODE_PRECHARGE_1            8
#define MODE_PRECHARGE_2            9
#define MODE_DISCHARGE_1            10
#define MODE_DISCHARGE_2            11

typedef struct
{
    uint16_t state;
    uint16_t warnings;
    uint16_t errors;
    uint16_t converterMode;
} StatusRegisterSet_t;

// defines for available modes
#define MODE_AV_VOLTAGE_CONTROL_1   0x004
#define MODE_AV_VOLTAGE_CONTROL_2   0x008
#define MODE_AV_DROOP_1             0x010
#define MODE_AV_DROOP_2             0x020
#define MODE_AV_POWER               0x040
#define MODE_AV_PRECHARGE_1         0x080
#define MODE_AV_PRECHARGE_2         0x100
#define MODE_AV_DISCHARGE_1         0x200
#define MODE_AV_DISCHARGE_2         0x400

typedef struct
{
    uint16_t avialableModes;
    uf16_t maxPower;
    uint16_t rsvd[2];
} StaticConverterDataRegisterSet_t;

// structs for commands
typedef struct
{
    uint16_t changeToMode;      // e.g. MODE_VOLTAGE_CONTROL_1
    uint16_t rsvd[3];
} CommandRegisterSet_t;

typedef struct
{
    uint16_t heartbeat;         // periodically toggled between 0 and 1
    uint16_t rsvd[3];
} HeartbeatRegisterSet_t;

// structs for measurements
typedef struct
{
    uint16_t voltageP_1;        // all in [V]
    uint16_t voltageM_1;
    uint16_t voltageP_2;
    uint16_t voltageM_2;
} VoltageMeasurementRegisterSet_t;

typedef struct
{
    int16_t currentP_1;        // all in [A]
    int16_t currentM_1;        // negative measurement
    int16_t currentP_2;
    int16_t currentM_2;
} CurrentMeasurementRegisterSet_t;

// structs for reference values
typedef struct
{
    uint16_t refVoltage;
    uint16_t rsvd[3];
} VoltageControlRegisterSet_t;

typedef struct
{
    uint16_t refVoltage;
    int16_t droopParameter;    // range to be defined yet
    uint16_t rsvd[2];
} DroopParameterRegisterSet_t;

typedef struct
{
    int16_t refPower;           // in 1 / 10.000 of Pnom
    uint16_t rsvd[3];
} PowerModeRegisterSet_t;

typedef struct
{
    uint16_t nextMode;          // e.g. MODE_VOLTAGE_CONTROL_1
    uint16_t rsvd[3];
} PrechargeRegisterSet_t;

// overall data structure to be used (overall 104 bytes)
typedef struct
{
    StatusRegisterSet_t                 converterStatus;
    StaticConverterDataRegisterSet_t    converterBaseData;
    VoltageMeasurementRegisterSet_t     voltageMeasurement;
    CurrentMeasurementRegisterSet_t     currentMeasurement;
    CommandRegisterSet_t                converterCommands;
    HeartbeatRegisterSet_t              heartbeat;
    VoltageControlRegisterSet_t         voltageControl_1;
    VoltageControlRegisterSet_t         voltageControl_2;
    DroopParameterRegisterSet_t         droopControl_1;
    DroopParameterRegisterSet_t         droopControl_2;
    PowerModeRegisterSet_t              powerControl;
    PrechargeRegisterSet_t              prechargeControl_1;
    PrechargeRegisterSet_t              prechargeControl_2;
} converterDataStructure;

#endif