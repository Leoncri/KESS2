#include "gridnodeclass.h"

GridNode::GridNode(int id)
{
    selfID = id;
    refConverter = 0;
    refBreaker = 0;
    refNode = 0;
}

GridNode::~GridNode()
{
}

int GridNode::GetID()
{
    return selfID;
}

bool GridNode::ConnectConverter(GridConverter* converter)
{
    // make sure converter is not already in list
    if(std::find(std::begin(converterList), std::end(converterList), converter) != std::end(converterList))
    {
        return false;
    }
    
    // store converter
    converterList.push_back(converter);
    
    // node is directly connected
    isFloating = false;
    
    // make converter reference converter if there is no
    if (refConverter == 0)
    {
        refConverter = converter;
    }
    
    return true;
}

bool GridNode::ConnectBreaker(GridBreaker* breaker)
{
    // make sure breaker is not already in list
    if(std::find(std::begin(breakerList), std::end(breakerList), breaker) != std::end(breakerList))
    {
        return false;
    }
    
    // store breaker
    breakerList.push_back(breaker);
    
    return true;
}

void GridNode::DisconnectConverter(GridConverter* converter)
{
    // Remove converter from list
    converterList.remove(converter);
    
    if (refConverter == converter)
    {
        refConverter = 0;
    }
    
    // Find new reference converter
    if(!converterList.empty())
    {
        refConverter = converterList.front();
    }
        
    return;
}

void GridNode:: DisconnectBreaker(GridBreaker* breaker)
{
    // Remove breaker from list
    breakerList.remove(breaker);
    
    if (refBreaker == breaker)
    {
        refBreaker = 0;
    }
    
    return;
}

void GridNode::Update()
{
    // Check if node is floating
    isFloating = IsFloating();
    
    // Update reference node
    refNode = GetRefNode();

    // update reference breaker (is usually called only once)
    if (!refBreaker)
    {
        for (auto b : breakerList)
        {
            if (b->GetVoltage(this) < -0.5)
            {
                refBreaker = b;
            }
        }
    }
    
    return;
}

bool GridNode::IsFloating()
{
    bool floating = true;
    GridNode* n;
    
    // node is never floating when it is connected to a converter
    if (refConverter)
    {
        return false;
    }
    
    // mark node as visited for upcoming loop
    if (isVisited)
    {
        return true;
    }
    
    isVisited = true;
    
    // loop over every breaker
    for (auto b: breakerList)
    {
        // get other node from every breaker
        n = b->GetOtherNode(this);
        if (n)
        {
            floating = floating && n->IsFloating();
        }
    }
    
    // remove visited tag
    isVisited = false;
    
    return floating;
}

GridNode* GridNode::GetRefNode()
{
    GridNode* n = NULL;
    GridNode* newRefNode;
    
    // check if node is connected directly to converter
    if (refConverter)
    {
        return this;
    }
    
    // mark node as visited for upcoming loop
    if (isVisited)
    {
        return 0;
    }
    
    isVisited = true;
    
    // loop over every breaker to find node with converter connection
    for (auto b: breakerList)
    {
        n = b->GetOtherNode(this);
        if (n)
        {
            newRefNode = n->GetRefNode();
            if (newRefNode)
            {
                // break if node was found
                break;
            }
        }
    }
    
    // remove visited tag
    isVisited = false;
    
    return n;
}
    

float GridNode::GetVoltage()
{
    // check if reference converter exists
    if (refConverter)
    {
        return refConverter->GetVoltage(this);
    }
    
    // last try, check for reference breaker
    if (refBreaker)
    {
        return refBreaker->GetVoltage(this);
    }

    // check if were already here
    if (isVisited)
    {
        return -1.0;
    }
    isVisited = true;

    // ask reference node for voltage
    if (refNode)
    {
        return refNode->GetVoltage();
    }

    // remove visited tag
    isVisited = false;
    
    return -1.0;
}