#include "adsconverterinterface.h"
#include "../debugoutput.h"

AdsConverterInterface::AdsConverterInterface()
{
	c2sHandleCreated = false;
	s2cHandleCreated = false;
	newModeHandleCreated = false;
	c2sHandle = 0;
	s2cHandle = 0;
	newModeHandle = 0;
	deviceAddr = 0;
	newMode = 1;
}

AdsConverterInterface::~AdsConverterInterface()
{
	// remove device from handler list
	if (interfaceHandler)
	{
		interfaceHandler->RemoveInterface(this);
	}
}

bool AdsConverterInterface::Initialize(GridDevice* device, uint32_t addr, converterDataStructure* converter2Server, converterDataStructure* server2Converter)
{
	// variables
	LONG nErr;
	char varS2C[] = { "MAIN.outputParameterList" };
	char varC2S[] = { "MAIN.inputparameterList" };
	char varUpdate[] = { "MAIN.newMode" };

	// store parameters
	converter = device;
	deviceAddr = addr;
	c2sData = converter2Server;
	s2cData = server2Converter;
	
	// setup adsAddr
	adsAddr.port = 851;
	adsAddr.netId.b[0] = (addr >> 24) & 0xFF;
	adsAddr.netId.b[1] = (addr >> 16) & 0xFF;
	adsAddr.netId.b[2] = (addr >> 8) & 0xFF;
	adsAddr.netId.b[3] = addr & 0xFF;
	adsAddr.netId.b[4] = 1;
	adsAddr.netId.b[5] = 1;

	// try to get handles
	// start with input handle
#ifdef USE_ADS
	nErr = AdsSyncReadWriteReq(&adsAddr, ADSIGRP_SYM_HNDBYNAME, 0x0, sizeof(c2sHandle), &c2sHandle, sizeof(varC2S), varC2S);
	if (nErr)
	{
		return false;
	}
	c2sHandleCreated = true;

	nErr = AdsSyncReadWriteReq(&adsAddr, ADSIGRP_SYM_HNDBYNAME, 0x0, sizeof(s2cHandle), &s2cHandle, sizeof(varS2C), varS2C);
	if (nErr)
	{
		return false;
	}
	s2cHandleCreated = true;

	nErr = AdsSyncReadWriteReq(&adsAddr, ADSIGRP_SYM_HNDBYNAME, 0x0, sizeof(newModeHandle), &newModeHandle, sizeof(varUpdate), varUpdate);
	if (nErr)
	{
		return false;
	}
	newModeHandleCreated = true;

	// add device to handler list
	if (interfaceHandler)
	{
		interfaceHandler->AddNewInterface(this);
	}

	OUTPUT_DEBUG_MESSAGE("Successfully created ADS interface")
	valid = true;
#endif
	return true;
}

void AdsConverterInterface::Shutdown()
{
	// release all handles
#ifdef USE_ADS
	if (c2sHandleCreated)
	{
		AdsSyncWriteReq(&adsAddr, ADSIGRP_SYM_RELEASEHND, 0, sizeof(c2sHandle), &c2sHandle);
		c2sHandleCreated = false;
	}

	if (s2cHandleCreated)
	{
		AdsSyncWriteReq(&adsAddr, ADSIGRP_SYM_RELEASEHND, 0, sizeof(s2cHandle), &s2cHandle);
		s2cHandleCreated = false;
	}

	if (newModeHandleCreated)
	{
		AdsSyncWriteReq(&adsAddr, ADSIGRP_SYM_RELEASEHND, 0, sizeof(newModeHandle), &newModeHandle);
		newModeHandleCreated = false;
	}

	valid = false;
#endif
	return;
}

bool AdsConverterInterface::UpdateData()
{
	// variables
	LONG nErr;

	// get parameter lock
	std::unique_lock lk(parameterLock);

	// read from one array and update the other
#ifdef USE_ADS
	nErr = AdsSyncReadReq(&adsAddr, ADSIGRP_SYM_VALBYHND, c2sHandle, sizeof(converterDataStructure), c2sData);
	if (nErr)
	{
		valid = false;
		return false;
	}

	// write array
	nErr = AdsSyncWriteReq(&adsAddr, ADSIGRP_SYM_VALBYHND, s2cHandle, sizeof(converterDataStructure), s2cData);
	if (nErr)
	{
		valid = false;
		return false;
	}
#endif
	return true;
}

void AdsConverterInterface::OnBrokenConnectionCallback()
{
	// call device
	valid = false;
	converter->OnConnectionFault();
	return;
}

bool AdsConverterInterface::SetNewMode()
{
	// variables
	LONG nErr;

	// write paramters
#ifdef USE_ADS
	nErr = AdsSyncWriteReq(&adsAddr, ADSIGRP_SYM_VALBYHND, newModeHandle, sizeof(newMode), &newMode);
	if (nErr)
	{
		valid = false;
		return false;
	}
#endif
	return true;
}