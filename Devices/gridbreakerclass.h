#pragma once

class GridBreaker;

// this is a class for building up the grid structure only. Safety features or any other functionality is handles by overlay classes
#include <mutex>

#include "gridnodeclass.h"

class GridBreaker
{
    public:
        // Contructor and destructor
        GridBreaker();
        ~GridBreaker();
        
        // Init and shutdown
        bool Initialize(GridNode* node1, GridNode* node2);
        void Shutdown();
        
        // Breaker open or closed
        void SetBreakerClosed(bool closed);
        bool NeedsUpdate();
        void Update();
        
        // Set and get voltage for node
        void SetVoltage(int index, float voltage);
        float GetVoltage(GridNode* node);
        
        // Get node of other side (only when breaker is closed, otherwhise return 0)
        GridNode* GetOtherNode(GridNode*);
    
    private:
        // Breaker status
        bool closed;
        bool needsUpdate;
        float voltageNode1;
        float voltageNode2;
        std::mutex lockStatus;
        
        // connected nodes
        GridNode* connNode1;
        GridNode* connNode2;
};
