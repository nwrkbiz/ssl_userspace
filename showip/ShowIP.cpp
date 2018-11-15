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

using namespace std;

#define IP_LENGHT 4
#define HEX_NUM 6
#define PWM_NUM 2
#define BUFFER_SIZE (HEX_NUM + PWM_NUM)

const string char_device = "/dev/sevensegment";

int GetIpAddr(uint8_t ip[IP_LENGHT], char const c_netIf[] = "eth0")
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
	ipAsInt = htonl(ipAsInt);	// we want it as Big endian..

	ip[0] = (ipAsInt >> 0) & 0xFF;
	ip[1] = (ipAsInt >> 8) & 0xFF;
	ip[2] = (ipAsInt >> 16) & 0xFF;
	ip[3] = (ipAsInt >> 24) & 0xFF;

	return 0;
}


int main(int argc, char *argv[])
{

	uint8_t ip[IP_LENGHT];
    char buffer[BUFFER_SIZE];

    // endless loop to show ip
    for(int i = 0; i < IP_LENGHT; i++)
    {

    	if (GetIpAddr(ip) == 0)
        {
            // Add leading zeros
            std::stringstream ss;
            ss << std::setw(3) << std::setfill('0') << (int)ip[IP_LENGHT-i-1];
            std::string ip_part = ss.str();

            // string to write to char device
            string to_write = to_string(i+1) + "  " + ip_part + "ff";

            // Write to display
            int output_fd = open(char_device.c_str(), O_WRONLY);
            if(output_fd == -1){
                perror("open");
            }
            write(output_fd, to_write.c_str(), BUFFER_SIZE);
            close(output_fd);
        }
        if(i == (IP_LENGHT-1))
           i = -1;

        usleep(1000000);
    }
	return 0;
}
