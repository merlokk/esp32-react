#ifndef ESPCONFIG_H
#define ESPCONFIG_H

#include <string>
#include <vector>
#include <mutex>
#include "cJSON.h"

enum class ConnectionMode {
    None,
    AP,
    Client,
    ClientThenAP
};

struct WifiCredentials {
    std::string Name;
    std::string Password;
    void Clear() {
        Name = "";
        Password = "";
    }
};

class ESPConfig {
private:
    std::mutex lockPrivateResources;
    ESPConfig();

    std::string GetStrParam(cJSON *root, std::string name);
    int GetIntParam(cJSON *root, std::string name, int defaultv);
    bool GetBoolParam(cJSON *root, std::string name, bool defaultv);
public:
    static ESPConfig& getInstance() {
        static ESPConfig instance;
        return instance;
    }
    ESPConfig(ESPConfig const&) = delete;
    void operator=(ESPConfig const&) = delete;

    void Clear();
    void LoadDefaults();
    bool Load();
    bool Save();

    bool ResetToDefault();

    ConnectionMode Mode = ConnectionMode::None;
    std::string APName = "";
    std::string APPassword = "";
    uint8_t APChannel = 1;
    bool APHidden = false;
    std::vector<WifiCredentials> ClientCredentials;

    std::string WebLogin = "";
    std::string WebPassword = "";
    uint32_t WebPort = 80;

    bool AllowRestart = false;

    std::string GetModeStr();
};

#endif // ESPCONFIG_H
