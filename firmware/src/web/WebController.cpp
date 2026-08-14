#ifndef UNIT_TEST

#include "WebController.h"
#include "logger.h"
#include "web_pages.h"
#include "../dome/DomeController.h"
#include <Update.h>
#include <WiFi.h>
#include <ESPmDNS.h>

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.0.0"
#endif

void WebController::setup() {
    server_.on("/",       HTTP_GET,  [this]() { handleHome(); });
    server_.on("/update", HTTP_GET,  [this]() { handleUpdate(); });
    server_.on("/update", HTTP_POST,
        [this]() { handleUpdateUpload(); },
        [this]() {
            HTTPUpload& upload = server_.upload();
            if (upload.status == UPLOAD_FILE_START) {
                updateSuccess_ = false;
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                if (Update.end(true)) {
                    updateSuccess_ = true;
                    logAll("OTA update success, rebooting...");
                } else {
                    Update.printError(Serial);
                }
            }
        }
    );
    server_.on("/api/info", HTTP_GET, [this]() { handleInfoApi(); });
    server_.on("/api/dome/status", HTTP_GET, [this]() { handleDomeStatus(); });
    server_.on("/api/dome/calibrate", HTTP_POST, [this]() { handleDomeCalibrate(); });

    // DNS: resolve all queries to the AP IP so "chopper.body" (or any URL) works
    dns_.start(53, "*", WiFi.softAPIP());

    // mDNS: advertise "chopper-body.local" for devices that support it
    if (MDNS.begin("chopper-body")) {
        MDNS.addService("http", "tcp", 80);
    }

    server_.begin();
    logAll("Web server started on http://chopper.body");
}

void WebController::loop() {
    dns_.processNextRequest();
    server_.handleClient();
}

void WebController::handleHome() {
    server_.send(200, "text/html", WEB_PAGE_HOME);
}

void WebController::handleUpdate() {
    server_.send(200, "text/html", WEB_PAGE_UPDATE);
}

void WebController::handleUpdateUpload() {
    server_.send(200, "text/plain", updateSuccess_ ? "OK" : "FAIL");
    if (updateSuccess_) {
        delay(500);
        ESP.restart();
    }
}

void WebController::handleInfoApi() {
    String json;
    json.reserve(256);
    json += "{\"version\":\"";
    json += FIRMWARE_VERSION;
    json += "\",\"uptime\":";
    json += millis();
    json += ",\"heap\":";
    json += ESP.getFreeHeap();
    json += ",\"mac\":\"";
    json += WiFi.macAddress();
    json += "\",\"espnow\":true}";
    server_.send(200, "application/json", json);
}

void WebController::handleDomeStatus() {
    String json;
    json.reserve(128);
    json += "{\"calibrating\":";
    json += dome_ && dome_->isCalibrating() ? "true" : "false";
    json += ",\"gearRatio\":";
    json += dome_ ? String(dome_->getGearRatio(), 2) : "0.00";
    json += ",\"encoderCount\":";
    json += dome_ ? String(dome_->getEncoderCount()) : "0";
    json += ",\"rotations\":";
    json += dome_ ? String(dome_->getRotations(), 2) : "0.00";
    json += "}";
    server_.send(200, "application/json", json);
}

void WebController::handleDomeCalibrate() {
    if (dome_) {
        dome_->startCalibration();
        server_.send(200, "application/json", "{\"started\":true}");
    } else {
        server_.send(500, "application/json", "{\"started\":false}");
    }
}

#endif // UNIT_TEST
