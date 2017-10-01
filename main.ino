//Setup for porting Arduino classes.
#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "commonHeaders.h"

#include <stdarg.h>	


//Define debug types for serial debugging.
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

typedef enum
{
	UART_ERROR_OCCURRED,
	UART_NOERROR_OCCURRED,
	UART_RECEIVING_DATA,
	UART_RECEIVED_DATA,
	UART_RECEIVE_TIMEOUT
}uart_events_t;

uart_events_t UART_EVENT_FLAG = UART_NOERROR_OCCURRED;

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
#define UART_RX_BUFFER_SIZE 1024
uint8_t UART_RX_BUFFER[UART_RX_BUFFER_SIZE];
long int UART_RX_RECEIVE_COUNT = 0;


#define UART_RX_TASK_PRIORITY 1
#define UART_TASK_PRIORITY 0

#define UART_RX_QUEUE_SIZE 6
#define UART_TASK_QUEUE_SIZE 6
#define UART_RX_ESCAPE_CHARACTER '\r'

static os_timer_t UART_RX_TASK_Timer;
static os_timer_t UART_RX_TimeoutTimer;
static os_timer_t UART_TASK_Timer;


static os_event_t UART_RX_QUEUE[UART_RX_QUEUE_SIZE];
static os_event_t UART_TASK_QUEUE[UART_TASK_QUEUE_SIZE];

long int UART_RX_CYCLE_IN_MS = 5;
long int UART_TASK_CYCLE_IN_MS = 2;
long int UART_RX_TIMEOUT_CYCLE_IN_MS = 500;

volatile uint32_t value = 0;


//os_timer timeout callback
void Temp_Run_UART_RX_Task(void*) {
	system_os_post(UART_RX_TASK_PRIORITY, 0, 0);
}

//os_timer timeout callback
void Temp_Run_UART_Task(void*) {
	system_os_post(UART_TASK_PRIORITY, 0, 0);
}

void Temp_Run_UART_Timeout_Task(void*)
{
	Serial.println("UART RX Timeout,clear buffer");
	UART_RX_RECEIVE_COUNT = 0;
	os_memset(UART_RX_BUFFER, 0, UART_RX_BUFFER_SIZE);
	UART_EVENT_FLAG = UART_RECEIVE_TIMEOUT;

}

//thread entry point
void UART_RX_TASK_Handler(os_event_t *) {
	
	if (Serial.available() > 0)
	{
		char Temp_Char = Serial.read();

		if (Temp_Char == UART_RX_ESCAPE_CHARACTER)
		{
			UART_EVENT_FLAG = UART_RECEIVED_DATA;
			UART_RX_RECEIVE_COUNT = 0;
			ets_timer_disarm(&UART_RX_TimeoutTimer);
		}

		else
		{
			UART_EVENT_FLAG = UART_RECEIVING_DATA;
			UART_RX_BUFFER[UART_RX_RECEIVE_COUNT] = Temp_Char;
			UART_RX_RECEIVE_COUNT++;
			os_timer_arm(&UART_RX_TimeoutTimer, UART_RX_TIMEOUT_CYCLE_IN_MS, 0);
		}
		
	}

	os_timer_arm(&UART_RX_TASK_Timer, UART_RX_CYCLE_IN_MS /*ms*/, 0 /*once*/);
}

void UART_TASK_Handler(os_event_t *) 
{
	

	if (UART_EVENT_FLAG == UART_RECEIVED_DATA)
	{
		UART_EVENT_FLAG = UART_NOERROR_OCCURRED;
		printMsg(OS, true, "alinan %s", (char*)UART_RX_BUFFER);
		os_memset(UART_RX_BUFFER, 0, UART_RX_BUFFER_SIZE);
	}
		

	os_timer_arm(&UART_TASK_Timer, UART_TASK_CYCLE_IN_MS /*ms*/, 0 /*once*/);
}

void setup() {

	Serial.begin(9600);

	//thread olusturma
	system_os_task(UART_RX_TASK_Handler,
		UART_RX_TASK_PRIORITY, UART_RX_QUEUE,
		UART_RX_QUEUE_SIZE);

	system_os_task(UART_TASK_Handler,
		UART_TASK_PRIORITY, UART_TASK_QUEUE,
		UART_TASK_QUEUE_SIZE);

	//timer olusturma
	os_timer_setfn(&UART_RX_TASK_Timer, 
			(os_timer_func_t*)&Temp_Run_UART_RX_Task, 0);

	os_timer_setfn(&UART_TASK_Timer,
		(os_timer_func_t*)&Temp_Run_UART_Task, 0);

	os_timer_setfn(&UART_RX_TimeoutTimer,
		(os_timer_func_t*)&Temp_Run_UART_Timeout_Task, 0);

	//taski baslatma
	Temp_Run_UART_RX_Task(0);
	Temp_Run_UART_Task(0);

}

// the loop function runs over and over again until power down or reset
void loop() {
	delay(10000);
}
