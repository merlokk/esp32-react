#include "espconfig.h"

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "espfswrapper.h"
#include "utils.h"

static const char *TAG = "CFG";

ESPConfig::ESPConfig() {
    if (!Load())
        LoadDefaults();
}

void ESPConfig::Clear() {
    Mode = ConnectionMode::None;
    APName = "";
    APPassword = "";
    APChannel = 1;
    APHidden = false;
    ClientCredentials.clear();

    AllowRestart = false;
}

void ESPConfig::LoadDefaults() {
    Clear();

    Mode = ConnectionMode::AP;
    APName = "ESP32React";
    APPassword = "er123456";
    AllowRestart = true;
}

bool ESPConfig::Load() {
    LockGuard lock(lockPrivateResources);
    Clear();

    if (!file_exist("/spiffs/config.json"))
        return false;

    char buf[1024] = {0};
    FILE* f = fopen("/spiffs/config.json", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open config.json");
        return false;
    }

    size_t read = fread((void*) buf, 1, sizeof(buf), f);
    ESP_LOGI(TAG, "Read config %zu bytes", read);
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        const char * err = cJSON_GetErrorPtr();
        ESP_LOGE(TAG, "Failed to parse config.json. error: %s", (err)?err:"n/a");
        return false;
    }

    cJSON *wifi = cJSON_GetObjectItem(root, "Wifi");
    if (!wifi) {
        ESP_LOGW(TAG, "Can't find a 'Wifi' section");
    } else {
        auto apmode = GetStrParam(wifi, "Mode");
        Mode = ConnectionMode::None;
        if (apmode == "AP")
            Mode = ConnectionMode::AP;
        if (apmode == "Client")
            Mode = ConnectionMode::Client;
        if (apmode == "ClientThenAP")
            Mode = ConnectionMode::ClientThenAP;

        APName = GetStrParam(wifi, "APName");
        APPassword = GetStrParam(wifi, "APPassword");
        APChannel = GetIntParam(wifi, "APChannel", 1);
        APHidden = GetBoolParam(wifi, "APHidden", false);

        cJSON *clienc = cJSON_GetObjectItem(wifi, "ClientCredentials");
        if (clienc && cJSON_IsArray(clienc)) {
            int asize = cJSON_GetArraySize(clienc);
            for (int i = 0; i < asize; i++) {
                cJSON * elm = cJSON_GetArrayItem(clienc, i);
                if (elm) {
                    WifiCredentials cred;
                    cred.Name = GetStrParam(elm, "Name");
                    cred.Password = GetStrParam(elm, "Password");

                    ClientCredentials.push_back(cred);
                } else {
                    ESP_LOGE(TAG, "Credentials array element is empty");
                }
            }

        }
    }

    cJSON *web = cJSON_GetObjectItem(root, "WEB");
    if (web) {
        WebLogin = GetStrParam(web, "Login");
        WebPassword = GetStrParam(web, "Password");
        WebPort = GetIntParam(web, "Port", 80);
    }

    AllowRestart = GetBoolParam(root, "AllowRestart", false);

    cJSON_Delete(root);

    ESP_LOGI(TAG, "Load config OK");
    return true;
}

bool ESPConfig::Save() {
    cJSON *root = cJSON_CreateObject();

    cJSON_Delete(root);
    return true;
}

bool ESPConfig::ResetToDefault() {
    if (!file_exist("/spiffs/defaultconfig.json")) {
        ESP_LOGE(TAG, "Default config file not exist.");
        return false;
    } else {
        ESP_LOGW(TAG, "Reset config to default");
        LockGuard lock(lockPrivateResources);

        if (remove("/spiffs/config.json") != ESP_OK)
            return false;

        if (file_copy("/spiffs/config.json", "/spiffs/defaultconfig.json") < 0)
            return false;
    }

    // coz there is another lock_guard
    return Load();
}

std::string ESPConfig::GetModeStr() {
    switch (Mode) {
    case ConnectionMode::AP:
        return "AP";
    case ConnectionMode::Client:
        return "Client";
    case ConnectionMode::ClientThenAP:
        return "Client then AP";
    default:
        return "Unknown";
    }

}

std::string ESPConfig::GetStrParam(cJSON *root, std::string name) {
    cJSON *obj = cJSON_GetObjectItem(root, name.c_str());
    if (!obj) {
        ESP_LOGE(TAG, "Can't find object for %s", name.c_str());
        return "";
    }
    if (!cJSON_IsString(obj)) {
        ESP_LOGE(TAG, "Object %s not a string", name.c_str());
        return "";
    }
    return obj->valuestring;
}

int ESPConfig::GetIntParam(cJSON *root, std::string name, int defaultv) {
    cJSON *obj = cJSON_GetObjectItem(root, name.c_str());
    if (!obj) {
        ESP_LOGE(TAG, "Can't find object for %s", name.c_str());
        return defaultv;
    }
    if (!cJSON_IsNumber(obj)) {
        ESP_LOGE(TAG, "Object %s not a integer", name.c_str());
        return defaultv;
    }
    return obj->valueint;
}

bool ESPConfig::GetBoolParam(cJSON *root, std::string name, bool defaultv) {
    cJSON *obj = cJSON_GetObjectItem(root, name.c_str());
    if (!obj) {
        ESP_LOGE(TAG, "Can't find object for %s", name.c_str());
        return defaultv;
    }
    if (!cJSON_IsBool(obj)) {
        ESP_LOGE(TAG, "Object %s not a boolean", name.c_str());
        return defaultv;
    }
    return cJSON_IsTrue(obj);
}
