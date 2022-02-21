#include "espcontrol.h"

#include "freertos/timers.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

#include "prjconfig.h"
//#include "espconfig.h"

static const char *TAG = "CONTROL";

void espcontrol_task(void *) {
    ESPControlTask &espcontrol = ESPControlTask::getInstance();

    ESP_LOGI(TAG, "ESP control task is started");

    while (true) {
        espcontrol.Execute();
    }
}

void espcontrol_init(void) {
    xTaskCreate(espcontrol_task, "espcontrol",  3000, NULL, ESP_CONTROL_TASK_PRIO,  NULL);
}

ESPControlTask::ESPControlTask() {
    eventCommand = xEventGroupCreate();
    msgSender.WakeUp = [this](){this->WakeUp();};
}

void ESPControlTask::WakeUp() {
    xEventGroupSetBits(eventCommand, eventESPControlWakeUp);
}

void ESPControlTask::Execute() {
    // if we have a message
    cmdESPControl cmd;
    if (msgSender.GetMessage(cmd)) {
        switch(cmd) {
        case ecmdRestart:
            if (false /*ESPConfig::getInstance().AllowRestart*/) {
                ESP_LOGW(TAG, "Restart ESP32 in 2 seconds....");
                RestartTime = xTaskGetTickCount() + pdMS_TO_TICKS(2000);
                msgSender.PutMessage(true);
            } else {
                ESP_LOGE(TAG, "Restart ESP32 is not allowed by config");
                msgSender.PutMessage(false);
            }
            break;
        default:
            msgSender.PutMessage(false);
        }
    }

    if (RestartTime && xTaskGetTickCount() > RestartTime) {
        esp_restart();
        RestartTime = 0;
    }

    xEventGroupWaitBits(eventCommand, eventESPControlWakeUp, pdTRUE, pdTRUE, pdMS_TO_TICKS(1000));
}

bool ESPControlTask::RestartESP() {
    bool res = false;
    if (!msgSender.SendMessage(ecmdRestart, res, 200))
        return false;

    return res;
}

