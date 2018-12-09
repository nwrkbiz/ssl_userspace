#include <iostream>
#include <string>
#include <stdint.h>
#include <unistd.h>
#include <thread>
#include <ctime>
#include <chrono>
#include <cassert>
#include <mutex>
#include "Sensor.h"
#include "KafkaProducer.h"
#include "HDC1000.h"
#include "MPU9250.h"
#include "ADPS9301.h"

// for FpgaRegion
#include <libfpgaregion.h>

using namespace std;

// IRQ Handler
static void irq_handle();

// FPGA Region Definitions
static void ReconfigRequest();
static void ReconfigDone();
static FpgaRegion fpga("ssl_userspace_app", ReconfigRequest, ReconfigDone);

// HANDLE function for SENSORS
static bool Handle_Sensor(Sensor * const sens, uint32_t Current_TS);

// Mutex to handle incomming FPGARegion Requests
static mutex FPGAMutex;

////////////////////////////////////////////////
// CONFIGURATION for KafkaProducer
string		const HOSTNAME	= "193.170.192.210";
uint16_t	const PORT		= 9092;
string		const TOPIC		= "telegraf";

////////////////////////////////////////////////
// LENGHTS of Read Buffers for Sensors
static const size_t HDC1000_Buffer_Length	= 12;
static const size_t MPU9250_Buffer_Length	= 37;
static const size_t ADPS9301_Buffer_Length	= 11;

////////////////////////////////////////////////
// To handle the sensors
static HDC1000  * hdc  = nullptr;
static MPU9250  * mpu  = nullptr;
static ADPS9301 * adps = nullptr;

////////////////////////////////////////////////
// MAIN FUNCTIONS
int main()
{
	// Create THREAD to handle incomming INTERRUPTs
	auto irq_handle_thread = thread(irq_handle);
	
	try
	{
		cout << "Started MAIN Userspace Appl.!" << endl;
		fpga.Aquire();
		
		// CREATE Kafka Producer
		KafkaProducer producer(HOSTNAME, TOPIC, PORT);
		
		// Create Sensor Classes
		hdc  = new HDC1000("HDC1000", "/dev/hdc", HDC1000_Buffer_Length, producer);
		mpu  = new MPU9250("MPU9250", "/dev/mpu", MPU9250_Buffer_Length, producer);
		adps = new ADPS9301("ADPS9301", "/dev/adps", ADPS9301_Buffer_Length, producer);
	
		if ((hdc == nullptr) || (mpu == nullptr) || (adps == nullptr))
		{
			cerr << "ERROR: allocating memory for sensor classes!" << endl;
			return -1;
		}
		
		// Prepare Interrupt Handler
		// ---- TODO ----

		// Read every Sensor as long as we dont receive the same timestamp twice
		while (true)
		{		
			// Lock FPGA for OUR use
			FPGAMutex.lock();
			
			// Fetch current Time of System (timestamp in milliseconds)
			uint32_t current_ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch()).count();
			
			// Handle all Sensors:
			// Read from one sensor as long as the same timestamp
			// is recieved!
			if(!Handle_Sensor(hdc, current_ts))
				return -1;
			if (!Handle_Sensor(mpu, current_ts))
				return -1;
			if (!Handle_Sensor(adps, current_ts))
				return -1;
			
			// Release Lock for FPGA (can now be reconfigured)
			FPGAMutex.unlock();
			
			// sleep for 500 ms
			usleep(500*1000);
		}
	}
	catch (string const & exept)
	{
		cout << "EXCEPTION: " << exept << endl;
	}	
	
	// JOIN THREADS and FREE MEMORY
	irq_handle_thread.join();
	delete hdc; hdc = nullptr;
	delete mpu; mpu = nullptr;
	delete adps; adps = nullptr;
	
	return 0;
}

void irq_handle()
{
	// TODO: handle IRQ
}

void ReconfigRequest()
{
	// Try to get the FPGA
	FPGAMutex.lock();
	
	// If we successfully aquired the FPGA --> RELEASE and then RECONFIGURE it!
	fpga.Release();
	
	cout << "FPGA is now being RECONFIGURED!" << endl;
}

void ReconfigDone()
{
	// Now that RECONFIGURING the FPGA is FINISHED,
	// let the MAIN thread do its work again!
	fpga.Aquire();
	FPGAMutex.unlock();
	cout << "FPGA was successfully RECONFIGURED!" << endl;
}

bool Handle_Sensor(Sensor * const sens, uint32_t Current_TS)
{
	assert(sens != nullptr);
	
	// Reset timestamps (prepare for next sensor!)
	uint32_t timestamp = 0;
			
	// READ next sensor
	if(!sens->Measure())
		return false;
			
	// As long as we read a new timestamp --> Send to cloud!
	while(sens->Get_Timestamp() != timestamp)
	{
		timestamp = sens->Get_Timestamp();
		
		// Send all values from Sensor
		if(!sens->SendValues(timestamp + Current_TS))
			return false;
				
		// New MEASUREMENT
		if(!sens->Measure())
			return false;
	}
	
	return true;
}
