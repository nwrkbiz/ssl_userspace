#pragma once

#include <thread>
#include <mutex>

// for FpgaRegion
#include <libfpgaregion.h>

class IP_Sevenseg
{
public:
	// Make SINGLETON useable
	static IP_Sevenseg& instance()
	{
		static IP_Sevenseg INSTANCE;
		return INSTANCE;
	}
	
	// DTOR
	~IP_Sevenseg();
	
	// Ban these calls to make SINGLETON
	IP_Sevenseg(IP_Sevenseg const&)		= delete;
	void operator=(IP_Sevenseg const&)  = delete;
	
	void Set_Brightness(uint8_t Brightness);	// 0 to 255
private:
	
	// Private CTOR for SINGLETON
	IP_Sevenseg() : mThread(Thread_Handle) { mBrightness = 255; }

	// Thread
	std::thread mThread;
	static void Thread_Handle();
	
	// Function to get IP Adress from board
	static int GetIpAddr(uint8_t * ip, size_t const cLen, char const c_netIf[] = "eth0");
	
	// FPGA manager
	static void			ReconfigRequest();
	static void			ReconfigDone();
	static FpgaRegion  *mFPGA;
	static std::mutex	mMutex_FPGA;
	
	// Private Member
	static uint8_t mBrightness;
};

