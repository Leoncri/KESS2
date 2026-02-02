#include "networkinterface.h"
#include "../debugoutput.h"

#ifdef USE_WINDOWS
bool NetworkInterface::winsockInit = false;
int NetworkInterface::numInterfaces = 0;
#endif

NetworkInterface::NetworkInterface()
{
#ifdef USE_WINDOWS
    if (!winsockInit)
    {
        OUTPUT_INFO("Windows socket system need to be initialized...")
        // initiliaze the winsock api
        WSADATA wsa;
        auto result = WSAStartup(MAKEWORD(2,0), &wsa);
        if (!result)
        {
            OUTPUT_INFO("Windows socket system initialied")
            winsockInit = true;
        }
    }
    numInterfaces++;
    
#endif
}

NetworkInterface::~NetworkInterface()
{
#ifdef USE_WINDOWS
    // shutdown winsock api
    if (numInterfaces == 1 && winsockInit)
    {
        WSACleanup();
        winsockInit = false;
    }
    numInterfaces--;
#endif
}

bool NetworkInterface::SendData(char* buffer, int size)
{
    return true;
}

int NetworkInterface::RecvData(char* buffer, int size)
{
    return -1;
}

#ifdef USE_WINDOWS
SOCKET NetworkInterface::GetInterfaceSocket()
#else
int NetworkInterface::GetInterfaceSocket()
#endif
{
    return interfaceSocket;
}