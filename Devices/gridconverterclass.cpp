#include "gridconverterclass.h"

GridConverter::GridConverter()
{
    // reset pointers
    connNode1 = 0;
    connNode2 = 0;
}

GridConverter::~GridConverter()
{
}

bool GridConverter::Initialize(GridNode* node1, GridNode* node2)
{
    // store variables
    connNode1 = node1;
    connNode2 = node2;

    return true;
}

void GridConverter::Shutdown()
{
    // reset pointers
    connNode1 = 0;
    connNode2 = 0;

    return;
}

void GridConverter::SetVoltage(int index, float voltage)
{
    // store voltage
    switch(index)
    {
        case 0:
            voltageNode1 = voltage;
            break;
        case 1:
            voltageNode2 = voltage;
            break;
        default:
            break;
    }

    return;
}

float GridConverter::GetVoltage(GridNode* node)
{
    if (node == connNode1)
    {
        return voltageNode1;
    }
    else if (node == connNode2)
    {
        return voltageNode2;
    }

    return -1.0;
}