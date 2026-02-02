#pragma once

#include "adsinterface.h"
#include "../Communication/converterDataStructure.h"
#include "../Devices/griddeviceclass.h"

#include <Windows.h>
#include <mutex>

#include "C:\TwinCAT\AdsApi\TcAdsDll\Include\TcAdsDef.h"
#include "C:\TwinCAT\AdsApi\TcAdsDll\Include\TcAdsAPI.h"

#include "../config.h"

class AdsConverterInterface : public AdsInterface
{
public:
	// construct and destructor
	AdsConverterInterface();
	~AdsConverterInterface();

	// init and shutdown
	bool Initialize(GridDevice* device, uint32_t addr, converterDataStructure* converter2Server, converterDataStructure* server2Converter);
	void Shutdown();

	// communication
	bool UpdateData();
	void OnBrokenConnectionCallback();

	// setup newly updated parameters
	bool SetNewMode();

private:
	// private functions

private:
	// device
	GridDevice* converter;

	// device addr
	uint32_t deviceAddr;
	AmsAddr adsAddr;

	// data structures
	converterDataStructure* c2sData;
	converterDataStructure* s2cData;

	// variable handles
	ULONG c2sHandle, s2cHandle, newModeHandle;
	bool c2sHandleCreated, s2cHandleCreated, newModeHandleCreated;

	// updated paramters
	uint16_t newMode;
	std::mutex parameterLock;
};