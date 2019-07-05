/*
 * agora_components.cpp
 *
 *  Created on: Jul 5, 2019
 *      Author: becksteing
 */

#include "agora_components.h"

mbed::I2C sensor_i2c(PIN_NAME_SDA, PIN_NAME_SCL);

mbed::DigitalOut sensor_power_en(PIN_NAME_SENSOR_POWER_ENABLE, 0);
mbed::DigitalOut battery_mon_en(PIN_NAME_BATTERY_MONITOR_ENABLE, 0);
mbed::DigitalOut board_id_disable(PIN_NAME_BOARD_ID_DISABLE, 1);
mbed::DigitalOut board_led(LED1, 1);

mbed::AnalogIn battery_voltage_in(PIN_NAME_BATTERY);
mbed::AnalogIn board_id_in(PIN_NAME_BOARD_ID);

/** Sensors */
BME680 bme680(&sensor_i2c);
MAX44009 max44009(sensor_i2c);
Si7021 si7021(sensor_i2c);
VL53L0X vl53l0x((DevI2C*)&sensor_i2c, NC);
LSM9DS1 lsm9ds1(sensor_i2c);
ICM20602 icm20602(sensor_i2c);

