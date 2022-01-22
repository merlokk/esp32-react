/* SPIFFS Image Generation on Build Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_netif.h"
#include "mdns.h"
#include "esp_event.h"
#include "esp_ota_ops.h"
#include "nvs_flash.h"
#include "mbedtls/md5.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lwip/apps/netbiosns.h"

#include "utils.h"
#include "espcontrol.h"

static const char *TAG = "main";

void sprintfs(bool printFS) {
    struct dirent *ep;

	size_t total = 0;
	size_t used = 0;
	esp_spiffs_info(NULL, &total, &used);
    ESP_LOGI(TAG, "Memory total: %zu used: %zu", total, used);
    
    if (printFS) {
        ESP_LOGI(TAG, "--- files ---");
        DIR* dir = opendir("/spiffs");
        if (dir) {
            while ((ep = readdir(dir)))
              ESP_LOGI(TAG, "  %s", ep->d_name);

            closedir(dir);
        } else {
            ESP_LOGW(TAG, "Can't open root directory");
        }
    }

	return;
}

static void PrintTaskList() {
    char ptrTaskList[1024] = {0};
    vTaskList(ptrTaskList);
    printf("Task\t\t State\t Prio\t Stack\t Num\t Core\n");
    printf("*****************************************************\n");
    printf("%s", ptrTaskList);
}

static bool initialise_mdns(void)
{
    auto err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mdns_init error: %d", err);
        return false;
    }
    err = mdns_hostname_set("esp32");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mdns_hostname_set error: %d", err);
        return false;
    }
    err = mdns_instance_name_set("esp32 react");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mdns_instance_name_set error: %d", err);
        return false;
    }

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}
    };

    mdns_service_add("www.esp32", "_http", "_tcp", 80, serviceTxtData,
         sizeof(serviceTxtData) / sizeof(serviceTxtData[0]));

    netbiosns_init();
    netbiosns_set_name("esp32");

    ESP_LOGI(TAG, "MDNS init OK.");
    return true;
}

bool HWInit() {
    bool result = true;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs_flash_init fail. trying to erase nvs....");
        ret = nvs_flash_erase();
        if (ret != ESP_OK)
          ESP_LOGE(TAG, "nvs_flash_erase fail");
        ret = nvs_flash_init();
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init fail");
        result = false;
    }

    if (esp_netif_init()) {
        ESP_LOGE(TAG, "esp_netif_init fail");
        result = false;
    }

    if (esp_event_loop_create_default()) {
        ESP_LOGE(TAG, "esp_event_loop_create_default fail");
        result = false;
    }

    initialise_mdns();

    return result;
}

void PrintESPInfo() {
    esp_chip_info_t chipinfo;
    esp_chip_info(&chipinfo);
    ESP_LOGI(TAG, "Chip model: %s cores: %d revision: %d features: %s%s%s%s",
             (chipinfo.model == 1)?"ESP32":"ESP32-S2",
             chipinfo.cores,
             chipinfo.revision,
             (CHIP_FEATURE_EMB_FLASH & chipinfo.features)?"EmbeddedFlash ":"",
             (CHIP_FEATURE_WIFI_BGN  & chipinfo.features)?"WiFi ":"",
             (CHIP_FEATURE_BLE       & chipinfo.features)?"BluetoothLE ":"",
             (CHIP_FEATURE_BT        & chipinfo.features)?"BluetoothClassic ":""
             );
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());
    const auto desc = esp_ota_get_app_description();
    ESP_LOGI(TAG, "Project name: %s", desc->project_name);
    ESP_LOGI(TAG, "Firmware version: %s date and time: %s %s", desc->version, desc->date, desc->time);
    switch(esp_reset_reason()) {
    case ESP_RST_UNKNOWN:   ESP_LOGI(TAG, "Reset reason can not be determined"); break;
    case ESP_RST_POWERON:   ESP_LOGI(TAG, "Reset due to power-on event"); break;
    case ESP_RST_EXT:       ESP_LOGI(TAG, "Reset by external pin (not applicable for ESP32)"); break;
    case ESP_RST_SW:        ESP_LOGI(TAG, "Software reset via esp_restart"); break;
    case ESP_RST_PANIC:     ESP_LOGI(TAG, "Software reset due to exception/panic"); break;
    case ESP_RST_INT_WDT:   ESP_LOGI(TAG, "Reset (software or hardware) due to interrupt watchdog"); break;
    case ESP_RST_TASK_WDT:  ESP_LOGI(TAG, "Reset due to task watchdog"); break;
    case ESP_RST_WDT:       ESP_LOGI(TAG, "Reset due to other watchdogs"); break;
    case ESP_RST_DEEPSLEEP: ESP_LOGI(TAG, "Reset after exiting deep sleep mode"); break;
    case ESP_RST_BROWNOUT:  ESP_LOGI(TAG, "Brownout reset (software or hardware)"); break;
    case ESP_RST_SDIO:      ESP_LOGI(TAG, "Reset over SDIO"); break;
    default:
        ESP_LOGI(TAG, "Reset reason unknown");
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "-------------------------------------");
    PrintESPInfo();
    ESP_LOGI(TAG, "-------------------------------------");
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = "storage",
      .max_files = 20,
      .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    // print contents of filesystem
    sprintfs(false);

    if (!HWInit()) {
        ESP_LOGE(TAG, "Hardware init error");
    }
    
    uint8_t derived_mac_addr[6] = {0};
    //Get MAC address for WiFi Station interface
    if (esp_read_mac(derived_mac_addr, ESP_MAC_WIFI_STA) == ESP_OK) {
        ESP_LOGI(TAG, "STA MAC address: %s", string_format_mac(derived_mac_addr).c_str());
    } else {
        ESP_LOGE(TAG, "Get MAC error");
    }

    // load config from file system...
    //auto &cfg = ESPConfig::getInstance();
    //ESP_LOGI(TAG, "Configured WiFi mode: %s", cfg.GetModeStr().c_str());

    vTaskDelay(pdMS_TO_TICKS(100));
    
    //xTaskCreatePinnedToCore(update_task,     "updater",  12000, NULL, UPDATE_TASK_PRIO,  NULL, tskNO_AFFINITY);
    //xTaskCreatePinnedToCore(wifi_task,       "wifi",     8192, NULL, WIFI_TASK_PRIO,     NULL, tskNO_AFFINITY);

    //xTaskCreatePinnedToCore(espcontrol_task, "espcntrl", 4096, NULL, ESPCONTROL_TASK_PRIO,  NULL, tskNO_AFFINITY);

    PrintTaskList();
}
