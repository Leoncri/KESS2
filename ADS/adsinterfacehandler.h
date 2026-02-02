#pragma once

class AdsInterfaceHandler;

#include <list>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>

#include "../config.h"
#include "adsinterface.h"

class AdsInterfaceHandler
{
public:
	// Constructor and destructor
	AdsInterfaceHandler();
	~AdsInterfaceHandler();

	// init and shutdown
	//bool Initialize();
	//void Shutdown();

	// start and stop the interface
	bool StartInterface();
	void StopInterface();

	// thread callback functions
	void UpdateThread();

	// add and remove interfaces
	bool AddNewInterface(AdsInterface*);
	void RemoveInterface(AdsInterface*);

private:
	// private functions

private:
	// list of devices
	std::list<AdsInterface*> deviceList;

	// update thread
	std::thread updateThread;
	bool updateThreadRunning;
	bool stopUpdateThread;

	// lock some data
	std::mutex dataLock;

};

void UpdateThreadCallback(AdsInterfaceHandler*);