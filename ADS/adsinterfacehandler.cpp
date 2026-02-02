#include "adsinterfacehandler.h"
#include "../debugoutput.h"

AdsInterfaceHandler::AdsInterfaceHandler()
{
	updateThreadRunning = false;
	stopUpdateThread = true;
}

AdsInterfaceHandler::~AdsInterfaceHandler()
{
}

bool AdsInterfaceHandler::StartInterface()
{
	// check if thread is already started
	OUTPUT_DEBUG_MESSAGE("Starting ADS interface handler")
	if (updateThreadRunning)
	{
		return false;
	}
	stopUpdateThread = false;

	// start thread
	updateThread = std::thread(UpdateThreadCallback, this);
	OUTPUT_DEBUG_MESSAGE("ADS interface handler started")

	return true;
}

void AdsInterfaceHandler::StopInterface()
{
	if (updateThreadRunning)
	{
		// stop update thread
		stopUpdateThread = true;

		// wait for update thread to end
		updateThread.join();
	}
	
	return;
}

void AdsInterfaceHandler::UpdateThread()
{
	// variables
	bool result;

	// set thread to running
	updateThreadRunning = true;

	while (!stopUpdateThread)
	{
		// update thread loop
		auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(ADS_PERIODIC_UPDATE_MS);

		// iterate over all valid devices
		for (auto e : deviceList)
		{
			if (e->IsValid())
			{
				OUTPUT_DEBUG_MESSAGE("Updating data for ADS interface")
				result = e->UpdateData();
				if (!result)
				{
					OUTPUT_DEBUG_MESSAGE("Failed to update data for ADS interface")
					e->OnBrokenConnectionCallback();
				}
			}
		}

		// wait until next round
		if (x > std::chrono::steady_clock::now())
		{
			std::this_thread::sleep_until(x);
		}
	}

	// stop thread
	updateThreadRunning = false;

	return;
}

bool AdsInterfaceHandler::AddNewInterface(AdsInterface* ads)
{
	// check if interface is already in list
	for (auto e : deviceList)
	{
		if (e == ads)
		{
			return false;
		}
	}

	deviceList.push_back(ads);

	return true;
}

void AdsInterfaceHandler::RemoveInterface(AdsInterface* ads)
{
	deviceList.remove(ads);
	return;
}

void UpdateThreadCallback(AdsInterfaceHandler* handler)
{
	// only call update thread function
	handler->UpdateThread();

	return;
}