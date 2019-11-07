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

#include "drivers/DigitalButton.h"

#include "BME680_BSEC.h"
#include "MAX44009.h"
#include "Si7021.h"
#include "VL53L0X.h"
#include "LSM9DS1.h"
#include "icm20602_i2c.h"

/** I2C Component Addresses */
#define BME680_I2C_ADDR				(0x76 << 1)
#define MAX44009_I2C_ADDR			(0x4A << 1)
#define SI7021_I2C_ADDR				(0x40 << 1)
#define VL53L0X_I2C_ADDR			(0x52 << 0)
#define LSM9DS1_ACC_GYRO_I2C_ADDR	(0x6A << 1)
#define LSM9DS1_MAG_I2C_ADDR		(0x1C << 1)
#define ICM20602_I2C_ADDR			(0x68 << 1)

extern mbed::I2C sensor_i2c;

extern mbed::DigitalOut sensor_power_en;
extern mbed::DigitalOut battery_mon_en;
extern mbed::DigitalOut board_id_disable;
extern mbed::DigitalOut board_led;

extern mbed::AnalogIn battery_voltage_in;
extern mbed::AnalogIn board_id_in;

extern ep::DigitalButton push_button_in;

/** Sensors */
extern BME680_BSEC* bme680;
extern MAX44009 max44009;
extern Si7021 si7021;
extern VL53L0X vl53l0x;
extern LSM9DS1 lsm9ds1;
extern ICM20602 icm20602;


#endif /* AGORA_COMPONENTS_H_ */
