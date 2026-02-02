#include "gridbreakerclass.h"

GridBreaker::GridBreaker()
{
    // resest variables
    closed = false;

    connNode1 = 0;
    voltageNode1 = -1.0;

    connNode2 = 0;
    voltageNode2 = -1.0;
}

GridBreaker::~GridBreaker()
{
}

bool GridBreaker::Initialize(GridNode* node1, GridNode* node2)
{
    // variables
    bool result;

    // save nodes
    connNode1 = node1;
    connNode2 = node2;

    // connect breaker to nodes
    result = connNode1->ConnectBreaker(this);
    if (!result)
    {
        return false;
    }

    result = connNode2->ConnectBreaker(this);
    if (!result)
    {
        return false;
    }

    return true;
}

void GridBreaker::Shutdown()
{
    // disconnect breaker from nodes
    connNode1->DisconnectBreaker(this);
    connNode2->DisconnectBreaker(this);

    return;
}

void GridBreaker::SetBreakerClosed(bool state)
{
    // Get lock
    std::unique_lock lk(lockStatus);

    // check if state matches new one
    if (state != closed)
    {
        // store new state
        closed = state;
        needsUpdate = true;
        
    }

    closed = state;

    return;
}

bool GridBreaker::NeedsUpdate()
{
    // Get lock
    std::unique_lock lk(lockStatus);

    return needsUpdate;
}

void GridBreaker::Update()
{
    // Get lock
    std::unique_lock lk(lockStatus);

    // update nodes
    connNode1->Update();
    connNode2->Update();

    needsUpdate = false;

    return;
}

void GridBreaker::SetVoltage(int index, float voltage)
{
    // set voltage according to index
    if (index == 1)
    {
        voltageNode1 = voltage;
    }

    if (index == 2)
    {
        voltageNode2 = voltage;
    }

    return;
}

float GridBreaker::GetVoltage(GridNode* node)
{
    // return voltage for node
    if (node == connNode1)
    {
        return voltageNode1;
    }

    if (node == connNode2)
    {
        return voltageNode2;
    }

    return -1.0;
}

GridNode* GridBreaker::GetOtherNode(GridNode* node)
{
    // when breaker is open, return 0
    if (!closed)
    {
        return 0;
    }

    // check if breaker is connected
    if (node == connNode1)
    {
        return connNode2;
    }

    if (node == connNode2)
    {
        return connNode1;
    }

    return 0;
}