#pragma once

#include <DNSServer.h>
#include <WebServer.h>

class DomeController;

class WebController {
public:
    void setDomeController(DomeController* dome) { dome_ = dome; }
    void setup();
    void loop();

private:
    void handleHome();
    void handleUpdate();
    void handleUpdateUpload();
    void handleInfoApi();
    void handleDomeStatus();
    void handleDomeCalibrate();

    DNSServer dns_;
    WebServer server_{80};
    bool updateSuccess_ = false;
    DomeController* dome_ = nullptr;
};
