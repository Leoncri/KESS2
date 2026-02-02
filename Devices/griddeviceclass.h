#pragma once

#include "../Network/modbusdatatransfer.h"

class GridDevice
{
    public:
        // Constructor and destructor
        GridDevice();
        ~GridDevice();

        // functions for device commands
        virtual bool NewCommand(char* buffer, int size);

        // function for handling connections
        virtual bool ConnectDevice();
        virtual void OnConnectionFault();
        void SetConnectionStatus(bool connected);
        
        // generate periodic data packet
        virtual int GeneratePeriodicData(char* buffer, int maxSize);

        // callback for modbus data transfer
        virtual bool CallbackDataTransfer(ModbusDataTransfer* t);
        
        // callback for faulty modbus data transfer
        virtual bool CallbackDataTransferError(ModbusDataTransfer* t);

        void AddPeriodicDataConnection(uint16_t mask);
        void RemovePeriodicDataConnection(uint16_t mask);

        // return id of device
        int GetID();
        
    protected:
        // variables for all devices
        int selfID;
        uint16_t periodicDataConnections;
        bool online;
};