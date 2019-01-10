#include <iostream>
#include <string>
#include <stdint.h>
#include <unistd.h>	// "usleep", ...
#include <thread>
#include <ctime>
#include <chrono>
#include <cassert>
#include <mutex>
#include "Sensor.h"
#include "KafkaProducer.h"
#include "HDC1000.h"
#include "MPU9250.h"
#include "APDS9301.h"
#include "IP_Sevenseg.h"

// for FpgaRegion
#include <libfpgaregion.h>

using namespace std;

// IRQ Handler
static void irq_handle();

// Thread Handler Function for Sevenseg Thread
static void Sevenseg_Handler(APDS9301 const * const apds);

// FPGA Region Definitions
static void ReconfigRequest();
static void ReconfigDone();
static FpgaRegion fpga("ssl_userspace_app", ReconfigRequest, ReconfigDone);

// HANDLE function for SENSORS
static bool Handle_Sensor(Sensor * const sens);

// Function to flush the FPGA FIFOs
static void Flush_FIFO(Sensor * const sens);

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
static const size_t APDS9301_Buffer_Length	= 11;

////////////////////////////////////////////////
// To handle the sensors
static HDC1000  * hdc  = nullptr;
static MPU9250  * mpu  = nullptr;
static APDS9301 * apds = nullptr;

////////////////////////////////////////////////
// Print message when data was successfully sent
//#define PRINT_SENT_MESSAGE

// TO enable IP display printing
//#define DISPLAY_IP

// Tolerances values
#define TOL_X	3000
#define TOL_Y	3000
#define TOL_Z	3000

////////////////////////////////////////////////
// MAIN FUNCTIONS
int main()
{
	// Create THREAD to handle incomming INTERRUPTs
	//auto irq_handle_thread = thread(irq_handle);
	
	thread *sevenseg_thread = nullptr;
	
	try
	{
		cout << "Started MAIN Userspace Appl.!" << endl;
		
		// CREATE Kafka Producer
		KafkaProducer producer(HOSTNAME, TOPIC, PORT);
		
		// Create Sensor Classes
		hdc  = new HDC1000("HDC1000", "/dev/hdc", HDC1000_Buffer_Length, producer);
		mpu  = new MPU9250("MPU9250", "/dev/mpu", MPU9250_Buffer_Length, producer);
		apds = new APDS9301("APDS9301", "/dev/apds", APDS9301_Buffer_Length, producer);
	
		if ((hdc == nullptr) || (mpu == nullptr) || (apds == nullptr))
		{
			cerr << "ERROR: allocating memory for sensor classes!" << endl;
			return -1;
		}
		
		// Configure Tolerances of MPU (for EVENT mode)
		MPU9250::ThreeAxis tolerances = {TOL_X, TOL_Y, TOL_Z};
		mpu->ConfigureTolerance(tolerances);
		
		// Create Thread that constantly sets the BRIGHTNESS of the SEVENSEG displays
		sevenseg_thread = new thread(Sevenseg_Handler, apds);
		
		// Prepare Interrupt Handler
		// ---- TODO ----

		// AQUIRE FPGA to read from CHAR DEVICE
		fpga.Aquire();
		
		int i = 0;
		
		// Flush FIFO to ensure we are reading NEW values
		Flush_FIFO(hdc);
		Flush_FIFO(mpu);
		Flush_FIFO(apds);
		
		// Read every Sensor as long as we dont receive the same timestamp twice
		while (true)
		{		
			// Lock FPGA for OUR use
			FPGAMutex.lock();
			
			// Set FPGA Ready for MPU to handle IRQ
			mpu->SetFPGAReady();
			
			// Handle all Sensors:
			// Read from one sensor as long as the same timestamp
			// is recieved!
			if(!Handle_Sensor(hdc))
				return -1;
			if (!Handle_Sensor(mpu))
				return -1;
			if (!Handle_Sensor(apds))
				return -1;
			
			// Set FPGA NOT Ready in MPU
			mpu->SetFPGANOTReady();
			
			// Release Lock for FPGA (can now be reconfigured)
			FPGAMutex.unlock();

			//cout << "Handled all Sensors " << i++ << endl;
			
			// sleep for 500 ms
			usleep(200*1000);
		}
	}
	catch (string const & exept)
	{
		cout << "EXCEPTION: " << exept << endl;
	}	
	
	// JOIN THREADS and FREE MEMORY
	//irq_handle_thread.join();
	sevenseg_thread->join();
	delete sevenseg_thread; sevenseg_thread = nullptr;
	delete hdc; hdc = nullptr;
	delete mpu; mpu = nullptr;
	delete apds; apds = nullptr;
	
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
	
	cout << "FPGA is now being RECONFIGURED (Main Userspace App)!" << endl;
}

void ReconfigDone()
{
	// Now that RECONFIGURING the FPGA is FINISHED,
	// let the MAIN thread do its work again!
	fpga.Aquire();
	FPGAMutex.unlock();
	cout << "FPGA was successfully RECONFIGURED (Main Userspace App)!" << endl;
}

bool Handle_Sensor(Sensor * const sens)
{
	assert(sens != nullptr);
	
	// Reset timestamps (prepare for next sensor!)
	uint32_t fpga_timestamp = sens->Get_Timestamp();
	static int64_t base = 0;
	static int64_t const current_ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch()).count();
	static bool first = true;	
	
	// READ next sensor
	if(!sens->Measure())
		return false;
	
	// If its the FIRST measurement --> calculate BASE time
	if (first)
	{
		base = current_ts - (int64_t)sens->Get_Timestamp();
		first = false;
	}
			
	// As long as we read a new timestamp --> Send to cloud!
	while(sens->Get_Timestamp() != fpga_timestamp)
	{
		fpga_timestamp = sens->Get_Timestamp();
		
		// Send all values from Sensor
		if(!sens->SendValues(base + fpga_timestamp))
			return false;
			
		// New MEASUREMENT
		if(!sens->Measure())
			return false;
	}
	
	return true;
}


void Sevenseg_Handler(APDS9301 const * const apds)
{
	// Create reference to this object to start thread
	// and display the IP adress of the board to the
	// SEVENSEG displays
#if defined(DISPLAY_IP)
	IP_Sevenseg & ip_displ = IP_Sevenseg::instance();
#endif
	// To calculate Lux value into 0-255 PWM value
	uint8_t pwm_value = 0;
	
	// ENDLESS: Set PWM (Brightness) of SEVENSEG displays
	while (true)
	{
		//pwm_value = apds->Get_Brightness() / 4;
		pwm_value = 255;
#if defined(DISPLAY_IP)
		ip_displ.Set_Brightness(pwm_value);
#endif
		usleep(200 * 1000);	// 200 ms
	}
}

void Flush_FIFO(Sensor * const sens)
{
	uint32_t fpga_timestamp = -1;
	uint32_t fifo_depth = 0;
	
	// New MEASUREMENT
	if(!sens->Measure())
		return;
	
	// As long as we read a new timestamp --> Send to cloud!
	while(sens->Get_Timestamp() != fpga_timestamp)
	{
		fpga_timestamp = sens->Get_Timestamp();
		fifo_depth++;
				
		// New MEASUREMENT
		if(!sens->Measure())
			return;
	}
	
	cout << "FIFO from " << sens->Get_SensorName() << " was flushed successfully! FIFO was " << fifo_depth << " steps deep!" << endl;
}
