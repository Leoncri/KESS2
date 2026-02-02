#include "adsinterface.h"

AdsInterfaceHandler* AdsInterface::interfaceHandler = NULL;
bool AdsInterface::portOpened = false;
int AdsInterface::numInterfaces = 0;

AdsInterface::AdsInterface()
{
	// check if port needs to be opened
	if (!portOpened)
	{
		// open ads port
#ifdef USE_ADS
		AdsPortOpen();
#endif
		portOpened = true;
	}

	// create a new interface handler if this is the first interface
	if (!interfaceHandler)
	{
		interfaceHandler = new AdsInterfaceHandler();
	}

	numInterfaces++;

	valid = false;
}

AdsInterface::~AdsInterface()
{
	// check if this is the last interface and clean up
	if (numInterfaces == 1)
	{
		if (interfaceHandler)
		{
			delete interfaceHandler;
			interfaceHandler = NULL;
		}

		if (portOpened)
		{
#ifdef USE_ADS
			AdsPortClose();
#endif
			portOpened = false;
		}
	}

	numInterfaces--;
}

bool AdsInterface::Initialize()
{
	// device specific, return
	return false;
}

void AdsInterface::Shutdown()
{
	// nothing to do here
	return;
}

bool AdsInterface::UpdateData()
{
	// nothing to do here
	return true;
}

void AdsInterface::OnBrokenConnectionCallback()
{
	// nothing to do here
	return;
}

bool AdsInterface::IsValid()
{
	return valid;
}

bool AdsInterface::StartInterfaces()
{
	// call the handler to start
	if (interfaceHandler)
	{
		return interfaceHandler->StartInterface();
	}
	return false;
}

void AdsInterface::StopInterfaces()
{
	// call the handler to stop
	if (interfaceHandler)
	{
		interfaceHandler->StopInterface();
	}
	return;
}