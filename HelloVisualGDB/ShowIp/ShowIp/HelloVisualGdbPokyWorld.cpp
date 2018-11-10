#include <iostream>
#include <cstring> /* for strncpy */

// Linux specific..
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

using namespace std;



int GetIpAddr(uint8_t ip[4], char const c_netIf[] = "eth0")
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

	uint8_t ip[4];
	
	if (GetIpAddr(ip) != 0)
	{
		return -1;
	}
	
	cout << "IP-Address is " << (int)ip[3] << "." << (int)ip[2] << "." << (int)ip[1] << "." << (int)ip[0] << endl;
	
	
	return 0;
}