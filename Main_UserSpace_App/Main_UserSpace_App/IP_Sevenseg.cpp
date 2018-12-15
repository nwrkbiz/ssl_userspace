#include <iostream>
#include <cstring> /* for strncpy */
#include <sstream>
#include <iomanip>

// Linux specific..
#include <unistd.h>	// "usleep", ...
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "IP_Sevenseg.h"

using namespace std;

// Init STATIC Members
FpgaRegion  *IP_Sevenseg::mFPGA = new FpgaRegion("showip", ReconfigRequest, ReconfigDone);
std::mutex	 IP_Sevenseg::mMutex_FPGA;
uint8_t		 IP_Sevenseg::mBrightness = 0;

IP_Sevenseg::~IP_Sevenseg()
{
	mThread.join();
	delete mFPGA; mFPGA = nullptr;
}

void IP_Sevenseg::Thread_Handle()
{
	static const	char char_device[]	= "/dev/sevensegment";
	static const	size_t cIP_Length	= 4;
	static const	size_t cHEX_Num		= 6;
	static const	size_t cPWM_Num		= 2;
	static const	size_t cReg_Size	= (cHEX_Num + cPWM_Num);
	static			uint8_t ip[cIP_Length];
	
	// endless loop to show ip
    for(int i = 0 ; i < cIP_Length ; i++)
	{		
		if (GetIpAddr(ip, cIP_Length) == 0)
		{
			// Add leading zeros
			std::stringstream ss, ss2;
			ss << std::setw(3) << std::setfill('0') << (int)ip[cIP_Length - i - 1];
			ss2 << hex << mBrightness;
			std::string ip_part = ss.str();
			string pwm_part = ss2.str();

			// string to write to char device
			
			string to_write = to_string(i + 1) + "  " + ip_part + pwm_part;
			
			// Before accessing CHAR DEVICE
			// Lock MUTEX and AQUIRE FPGA!
			mMutex_FPGA.lock();
			mFPGA->Aquire();
			
			// Open and WRITE CHAR DEVICE (Seven Segment Display)
			int output_fd = open(char_device, O_WRONLY);
			if (output_fd == -1)
			{
				perror("open");
				return;
			}

			write(output_fd, to_write.c_str(), cReg_Size);
			close(output_fd);
			
			// After WRITING and CLOSING the file --> RELEASE FPGA and unlock MUTEX
			mFPGA->Release();
			mMutex_FPGA.unlock();
		}
	    
		//Make this thing to an endless loop
		if(i == (cIP_Length - 1))
			i = -1;

		// sleep for 1 second (1000 * 1 ms)
		usleep(1000*1000);
	}
}

void IP_Sevenseg::ReconfigRequest()
{
	// Blocking LOCK function call on MUTEX
	mMutex_FPGA.lock();
	
	// RELEASE FPGA
	mFPGA->Release();
}

void IP_Sevenseg::ReconfigDone()
{
	// AQUIRE FGPA
	mFPGA->Aquire();
	
	// UNLOCK MUTEX --> let handler function work again
	mMutex_FPGA.unlock();
}

int IP_Sevenseg::GetIpAddr(uint8_t * ip, size_t const cLen, char const c_netIf[] /* = "eth0" */)
{
	static int fd;
	static struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd == -1)
	{
		return fd;
	}

	ifr.ifr_addr.sa_family = AF_INET;

	strncpy(ifr.ifr_name, c_netIf, IFNAMSIZ - 1);

	{
		int ret = ioctl(fd, SIOCGIFADDR, &ifr);
		close(fd);
		if (ret != 0)
		{
			return ret;
		}
	}

	uint32_t ipAsInt = (((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr).s_addr;
	ipAsInt = htonl(ipAsInt);  	// we want it as Big endian..

	ip[0] = (ipAsInt >> 0) & 0xFF;
	ip[1] = (ipAsInt >> 8) & 0xFF;
	ip[2] = (ipAsInt >> 16) & 0xFF;
	ip[3] = (ipAsInt >> 24) & 0xFF;

	return 0;
}


void IP_Sevenseg::Set_Brightness(uint8_t Brightness)
{
	mBrightness = Brightness;
}
