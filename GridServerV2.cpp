// DCGridServer.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include "Network/servercommunicationclass.h"
#include "Grid/gridserver.h"

#include <thread>
#include <chrono>
#include <string>

#include "debugoutput.h"
#include "config.h"

int main()
{
    // variables
    GridServer* gridServer = 0;
    bool result;
    std::string input;
    std::string stopServerString("stop");

    // create grid server
    gridServer = new GridServer();
    if (!gridServer)
    {
        OUTPUT_ERROR("Cannot create grid server class.")
            return 1;
    }

    // initialize grid server
    result = gridServer->Initialize();
    if (!result)
    {
        // shutdown grid server to delete potential created objects
        gridServer->Shutdown();
        return 1;
    }

    // start server
    result = gridServer->StartServer();
    if (!result)
    {
        OUTPUT_ERROR("Cannot start server.")
    }

    // wait for user input
    do
    {
        std::cin >> input;
    } while (input != stopServerString);

    // stop server
    gridServer->StopServer();

    // shutdown server
    gridServer->Shutdown();

    // delete variables
    delete gridServer;
    gridServer = 0;

    return 0;
}