#include <iostream>
#include <cstring> /* for strncpy */
#include <sstream>
#include <iomanip>

// Linux specific..
#include <unistd.h>
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

//Managment interface
#include <libfpgaregion.h>

using namespace std;

#define IP_LENGTH 4
#define HEX_NUM 6
#define PWM_NUM 2
#define BUFFER_SIZE (HEX_NUM + PWM_NUM)


static void ReconfigRequest();
static void ReconfigDone();
static FpgaRegion fpga("showip", ReconfigRequest, ReconfigDone);

//Flags for poor mans sync: Replace this with events.
static bool IsFpgaReady = false;
static bool IsFpgaInUse = false;

static const string char_device = "/dev/sevensegment";

static int GetIpAddr(uint8_t ip[IP_LENGTH], char const c_netIf[] = "eth0");


// Enable/Disable console output here
static void PrintToConsole(std::string const & out)
{
	//cout << out << endl;
}

int main(int argc, char *argv[])
{

	uint8_t ip[IP_LENGTH];
    char buffer[BUFFER_SIZE];

	fpga.Aquire();
	IsFpgaReady = true;
	PrintToConsole("FPGA Aquired. Starting now.");

	
    // endless loop to show ip
    for(int i = 0; i < IP_LENGTH; i++)
    {

	    IsFpgaInUse = true;
	    if (IsFpgaReady && (GetIpAddr(ip) == 0))
	    {
		    // Add leading zeros
		    std::stringstream ss;
		    ss << std::setw(3) << std::setfill('0') << (int)ip[IP_LENGTH - i - 1];
		    std::string ip_part = ss.str();

		    // string to write to char device
		    string to_write = to_string(i + 1) + "  " + ip_part + "ff";
			
		    // Write to display
		    int output_fd = open(char_device.c_str(), O_WRONLY);
		    if (output_fd == -1)
		    {
			    perror("open");
		    }
	        
		    PrintToConsole(to_write);
	        
		    write(output_fd, to_write.c_str(), BUFFER_SIZE);
		    close(output_fd);
	    }
	    else
	    {
		    PrintToConsole("Waiting for FPGA/Network..");
	    }
	    IsFpgaInUse = false;

	    
	    //Make this thing to an endless loop
        if(i == (IP_LENGTH - 1))
	    {
		    i = -1;
	    }

        usleep(1000000);
    }
	return 0;
}



static void ReconfigRequest() 
{
	IsFpgaReady = false;
	while (IsFpgaInUse)
	{
		usleep(1000);
	}
	fpga.Release();
	PrintToConsole("Released FPGA due to reconfig request.");
}

static void ReconfigDone() 
{	
	PrintToConsole("Aquiring FPGA againg after reconfig finished.");
	fpga.Aquire();
	IsFpgaReady = true;
}

static int GetIpAddr(uint8_t ip[IP_LENGTH], char const c_netIf[])
{
	int fd;
	struct ifreq ifr;

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
	ipAsInt = htonl(ipAsInt); 	// we want it as Big endian..

	ip[0] = (ipAsInt >> 0) & 0xFF;
	ip[1] = (ipAsInt >> 8) & 0xFF;
	ip[2] = (ipAsInt >> 16) & 0xFF;
	ip[3] = (ipAsInt >> 24) & 0xFF;

	return 0;
}