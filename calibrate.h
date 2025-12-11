//calibrate.h
#pragma once
#include <Arduino.h>
#include "data.h"
#include "config.h"

void setupCalibrationRoutes();
void startCalibration();
void updateBaselineDriftCorrection();