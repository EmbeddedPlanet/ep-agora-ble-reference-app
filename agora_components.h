/*
 * agora_components.h
 *
 *  Created on: Jul 3, 2019
 *      Author: becksteing
 */

#ifndef AGORA_COMPONENTS_H_
#define AGORA_COMPONENTS_H_

#include "PinNames.h"

#include "drivers/I2C.h"
#include "drivers/DigitalOut.h"
#include "drivers/AnalogIn.h"

#include "BME680.h"
#include "MAX44009.h"
#include "Si7021.h"
#include "VL53L0X.h"
#include "LSM9DS1.h"
#include "icm20602_i2c.h"

extern mbed::I2C sensor_i2c;

extern mbed::DigitalOut sensor_power_en;
extern mbed::DigitalOut battery_mon_en;
extern mbed::DigitalOut board_id_disable;
extern mbed::DigitalOut board_led;

extern mbed::AnalogIn battery_voltage_in;
extern mbed::AnalogIn board_id_in;

/** Sensors */
extern BME680 bme680;
extern MAX44009 max44009;
extern Si7021 si7021;
extern VL53L0X vl53l0x;
extern LSM9DS1 lsm9ds1;
extern ICM20602 icm20602;


#endif /* AGORA_COMPONENTS_H_ */
