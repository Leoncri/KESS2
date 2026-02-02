#pragma once

class GridNode;

#include <list>
#include <algorithm>

#include "gridconverterclass.h"
#include "gridbreakerclass.h"

class GridNode
{
    public:
        // Constructor and destructor
        GridNode(int id);
        ~GridNode();

        int GetID();
        
        // Connect and disconnect to grid elements
        bool ConnectConverter(GridConverter*);
        bool ConnectBreaker(GridBreaker*);
        
        void DisconnectConverter(GridConverter*);
        void DisconnectBreaker(GridBreaker*);
        
        // Update node status (floating / connected), call only when breakers change status
        void Update();
        
        // Get node status
        bool IsFloating();
        void SpreadFloating(bool);
        GridNode* GetRefNode();
        
        // Get node voltage in V
        float GetVoltage();
    
    private:
        // array of converter connections
        std::list<GridConverter*> converterList;
        
        // array of breaker connections
        std::list<GridBreaker*> breakerList;
        
        // converter for getting voltage from
        GridConverter* refConverter;
        
        // node for getting voltage from
        GridNode* refNode;
        
        // breaker for getting voltage from
        GridBreaker* refBreaker;
        
        // node status
        bool isVisited;
        bool isFloating;
        int selfID;
};