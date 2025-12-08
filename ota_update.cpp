//ota_update.h

#include "ota_update.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "config.h"

void setupOTA() {
  ArduinoOTA.setHostname(appConfig.deviceId);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) type = "sketch";
    else type = "filesystem";
    addLog(("OTA Start: " + type).c_str());
  });

  ArduinoOTA.onEnd([]() {
    addLog("OTA End");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char buf[50];
    sprintf(buf, "OTA Progress: %u%%", (progress / (total / 100)));
    addLog(buf);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    char buf[100];
    sprintf(buf, "OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) strcat(buf, "Auth Failed");
    else if (error == OTA_BEGIN_ERROR) strcat(buf, "Begin Failed");
    else if (error == OTA_CONNECT_ERROR) strcat(buf, "Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) strcat(buf, "Receive Failed");
    else if (error == OTA_END_ERROR) strcat(buf, "End Failed");
    addLog(buf);
  });

  ArduinoOTA.begin();
  addLog("OTA Ready");
}
