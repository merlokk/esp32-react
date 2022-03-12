#ifndef MAINFACTORY_H
#define MAINFACTORY_H

#include "espconfig.h"
#include "espcontrol.h"

class MainFactory {
private:
    ESPControlTask &espControlTask = ESPControlTask::getInstance();
    ESPConfig& espConfig = ESPConfig::getInstance();

    MainFactory();
public:
    static MainFactory& getInstance() {
        static MainFactory instance;
        return instance;
    }
    MainFactory(MainFactory const&) = delete;
    void operator=(MainFactory const&) = delete;

    ESPControlTask &GetESPControlTask() {return espControlTask;};
    ESPConfig &GetESPConfig() {return espConfig;};

};

#endif