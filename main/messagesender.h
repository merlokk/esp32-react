#ifndef MESSAGESENDER_H
#define MESSAGESENDER_H

#include <mutex>
#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum MessageSenderState {
    mssNone,
    mssMsgReceived,
    mssProcessing,
    mssResultWaiting
};

template <class Tin, class Tout>
class MessageSender {
    Tin MsgIn;
    Tout MsgOut;

    MessageSenderState State;
    std::mutex lockState;
public:
    std::function<void(void)> WakeUp = nullptr;
    MessageSender() {
        State = mssNone;
    };

    // server side
    bool GetMessage(Tin &msg) {
        if (State != mssMsgReceived)
            return false;

        std::lock_guard<std::mutex> lock(lockState);
        msg = MsgIn;
        State = mssProcessing;
        return true;
    };

    bool PutMessage(Tout msg) {
        if (State != mssProcessing)
            return false;

        std::lock_guard<std::mutex> lock(lockState);
        MsgOut = msg;
        State = mssResultWaiting;
        return true;
    };

    bool HaveMessage() {
        return State == mssMsgReceived;
    }

    // client side
    bool SendMessage(Tin msgIn, Tout &msgOut, size_t timeoutMS) {
        if (State != mssNone) {
            return false;
        } else {
            std::lock_guard<std::mutex> lock(lockState);

            MsgIn = msgIn;
            State = mssMsgReceived;
        }

        if (WakeUp != nullptr)
            WakeUp();

        auto timer = xTaskGetTickCount();
        while(State != mssResultWaiting) {
            if (xTaskGetTickCount() - timer > pdMS_TO_TICKS(timeoutMS)) {
                std::lock_guard<std::mutex> lock(lockState);
                State = mssNone;
                return false;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        std::lock_guard<std::mutex> lock(lockState);
        msgOut = MsgOut;
        State = mssNone;
        return true;
    };
};

#endif // MESSAGESENDER_H
