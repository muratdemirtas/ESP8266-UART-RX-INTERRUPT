//Setup for porting Arduino classes.
#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

//Extract Espressif methods
extern "C" {
#include "user_interface.h"
#include "espconn.h" 
}

#include <stdarg.h>	

enum debugTypes {
	ERROR = 0x0001,
	BOOT = 0x0002,
	MESH_STATUS = 0x0004,
	CONNECTION = 0x0008,
	COMMUNICATION = 0x0020,
	OS = 0x0080,
	APP = 0x0040,
	SYNC = 0x0010,
	WIFI = 0x0200,
	MQTT_STATUS = 0x0400,
	SCAN = 0x0800
};

//Print debug messages using serial monitor.
void ICACHE_FLASH_ATTR printMsg(debugTypes types, bool newline, const char* format ...) {

		char message[300];
		///Macro for arguments
		va_list args; va_start(args, format);
		vsnprintf(message, sizeof(message), format, args);
		Serial.print(message);

		//if u want newline
		if (newline)
			Serial.println();

		va_end(args);

}

#define MY_TASK_PRIORITY 0
#define MY_QUEUE_SIZE    1

static os_event_t my_queue[MY_QUEUE_SIZE];
volatile uint32_t value = 0;
static os_timer_t task_start_timer;

//os_timer timeout callback
void run_task(void*) {
	system_os_post(MY_TASK_PRIORITY, 0, 0);
}

//thread entry point,no for while allowed(!)
void my_task(os_event_t *) {
	

	if (Serial.available() > 0)
	{
		char k = Serial.read();
		printMsg(BOOT, true, "Received: %c", k);
	}
	// start the task again in 50  msec
	os_timer_arm(&task_start_timer, 50 /*ms*/, 0 /*once*/);
}

void setup() {

	Serial.begin(9600);
	system_os_task(my_task,
		MY_TASK_PRIORITY, my_queue,
		MY_QUEUE_SIZE);

	os_timer_setfn(&task_start_timer, (os_timer_func_t*)&run_task, 0);
	run_task(0);
}

// the loop function runs over and over again until power down or reset
void loop() {
	delay(10000);
}
