#ifndef ESPBASE_H
#define ESPBASE_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class ESPBase {
private:

public:
    ESPBase();
    ~ESPBase();

};

template<typename T>
class ESPBaseThread {
public:
	ESPBaseThread(const char* name, UBaseType_t priority, uint32_t stackDepth);

	TaskHandle_t GetHandle();
    void Sleep(TickType_t timeToSleep_in_ms);

	void Execute();

private:
	static void task(void* params);

	TaskHandle_t taskHandle;
};


template<typename T>
ESPBaseThread<T>::ESPBaseThread(const char* name, UBaseType_t priority, uint32_t stackDepth) {
	xTaskCreate(task, name, stackDepth, this, priority, &this->taskHandle);
}

template<typename T>
TaskHandle_t ESPBaseThread<T>::GetHandle() {
	return this->taskHandle;
}

template<typename T>
void ESPBaseThread<T>::Sleep(TickType_t timeToSleep_in_ms) {
    vTaskDelay(pdMS_TO_TICKS(timeToSleep_in_ms));
}

template<typename T>
void ESPBaseThread<T>::Execute() {
	static_cast<T&>(*this).Execute();
}

template<typename T>
void ESPBaseThread<T>::task(void* params) {
	ESPBaseThread* p = static_cast<ESPBaseThread*>(params);
    while (true) {
	    p->Execute();
    }
}



#endif