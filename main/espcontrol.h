#ifndef ESPCONTROL_H
#define ESPCONTROL_H

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "messagesender.h"

void espcontrol_task(void *);

enum cmdESPControl {
    ecmdNone,
    ecmdRestart,
    ecmdResetConfig
};

static const EventBits_t eventESPControlWakeUp = (1 << 0);

class ESPControlTask {
    EventGroupHandle_t eventCommand;
    MessageSender<cmdESPControl, bool> msgSender;

    size_t RestartTime = 0;
    size_t ShutdownTime = 0;

    ESPControlTask();
public:
    static ESPControlTask& getInstance() {
        static ESPControlTask instance;
        return instance;
    }
    ESPControlTask(ESPControlTask const&) = delete;
    void operator=(ESPControlTask const&) = delete;

    void WakeUp();
    void Execute();

    bool RestartESP();
};

#endif // ESPCONTROL_H
