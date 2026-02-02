#pragma once

class AdsInterface;

#include <iostream>
#include <windows.h>
#include <conio.h>

#include "../config.h"
#include <stdint.h>
#include "adsinterfacehandler.h"
#include "C:\TwinCAT\AdsApi\TcAdsDll\Include\TcAdsDef.h"
#include "C:\TwinCAT\AdsApi\TcAdsDll\Include\TcAdsAPI.h"

class AdsInterface
{
public:
	AdsInterface();
	~AdsInterface();

	// init and shutdown
	virtual bool Initialize();
	virtual void Shutdown();

	bool IsValid();

	// update function
	virtual bool UpdateData();
	virtual void OnBrokenConnectionCallback();

	// function to start and stop the ads interface handler
	static bool StartInterfaces();
	static void StopInterfaces();

private:
	// private functions

protected:
	// common ads interface handler
	static AdsInterfaceHandler* interfaceHandler;
	static bool portOpened;
	static int numInterfaces;

	bool valid;
};