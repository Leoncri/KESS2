#pragma once

// outcomment the next line when using Linux (not tested yet)
#define USE_WINDOWS

// set DEBUG to 1 to enable debug outputs in terminal
#define DEBUG   1

// DO NOT CHANGE
#define MAX_SERVER_COMMUNICATION_SLOTS  16

// Config defines for server communication
#define MAX_BUFFER_SIZE                 1024
#define SERVER_NUM_COMM_SOCKETS         4
#define SERVER_PORT                     51234
#define SERVER_PERIODIC_DATA_PERIOD_MS  1000

// config defines for tcp interface
#define TCP_MAX_READ_ITERATIONS         10

// config for converters
#define CONVERTER_MAX_DROOP_PARAMS      4

// defines for modbus communication
#define MODBUS_PORT                     502
#define MODBUS_PERIODIC_DATA_PERIOD_MS  1000

// background tasks defines
#define BACKGROUND_TASK_PERIOD_MS       1000

// define for ADS communication
#define ADS_PERIODIC_UPDATE_MS			500
#define USE_ADS