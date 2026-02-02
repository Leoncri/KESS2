#pragma once

class GridConverter;

#include <mutex>

#include "gridnodeclass.h"

class GridConverter
{
    public:
        // Contructor and destructor
        GridConverter();
        ~GridConverter();
        
        // Init and Shutdown functions
        bool Initialize(GridNode* node1, GridNode* node2);
        void Shutdown();
        
        // Set and get voltage for node
        void SetVoltage(int index, float voltage);
        float GetVoltage(GridNode* node);

    private:
        // converter data
        GridNode* connNode1;
        GridNode* connNode2;
        float voltageNode1;
        float voltageNode2;
};