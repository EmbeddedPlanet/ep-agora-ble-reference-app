/*
 * (C) 2019 Embedded Planet
 * Author: becksteing
 * Date: 6/19/19
 */

#include <stdio.h>

#include "platform/Callback.h"
#include "platform/NonCopyable.h"
#include "platform/mbed_wait_api.h"
#include "rtos/Thread.h"
#include "events/EventQueue.h"

#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble/GattClient.h"
#include "ble/GapAdvertisingParams.h"
#include "ble/GapAdvertisingData.h"
#include "ble/GattServer.h"
#include "BLEProcess.h"

/** Services */
#include "DeviceInformationService.h"
#include "BME680Service.h"
#include "Si7021Service.h"
#include "ICM20602Service.h"
#include "LSM9DS1Service.h"
#include "MAX44009Service.h"
#include "VL53L0XService.h"
#include "LEDService.h"
#include "BatteryVoltageService.h"

#include "agora_components.h"

#define POLL_INTERVAL_MS 5000 // Sensor polling interval in milliseconds

#define MAX_VBAT_VOLTAGE 3.3f

/** Device Information Strings */
const char manufacturers_name[]	= "Embedded Planet";
const char model_number[]		= "Agora BLE";
const char serial_number[]		= "123456789";
const char hardware_revision[]	= "1.0";
const char firmware_revision[]	= " ";
const char software_revision[]	= "1.0.0";

/** Standard Services */
DeviceInformationService* device_info_service;

/** Custom Services */
BME680Service bme680_service;
Si7021Service si7021_service;
ICM20602Service icm20602_service;
LSM9DS1Service lsm9ds1_service;
MAX44009Service max44009_service;
VL53L0XService vl53l0x_service;
LEDService led_service(true);
BatteryVoltageService battery_voltage_service;

void start_services(BLE& ble) {

	/** Start the standard services */
	device_info_service = new DeviceInformationService(ble, manufacturers_name, model_number, serial_number,
			hardware_revision, firmware_revision, software_revision);

	/** Start the custom services */
	bme680_service.start(ble);
	si7021_service.start(ble);
	icm20602_service.start(ble);
	lsm9ds1_service.start(ble);
	max44009_service.start(ble);
	vl53l0x_service.start(ble);
	led_service.start(ble);
	battery_voltage_service.start(ble);

}

void stop_services(void) {
	if(device_info_service != NULL) {
		delete device_info_service;
		device_info_service = NULL;
	}
}

void init_sensors(void) {

	// Enable sensor power domain
	sensor_power_en = 1;

	wait_ms(100);

	printf("Initializing sensors...\n");
	printf("\t BME680: ");

	if(bme680->init(&sensor_i2c)) {
		printf("OK\n");
	} else {
		printf("FAILED\n");
	}

	// No way to check this really...
	printf("\t MAX44009: OK\n");

	printf("\t Si7021: ");
	if(si7021.check() == 1) {
		printf("OK\n");
	} else {
		printf("FAILED\n");
	}

	printf("\t VL53L0X: ");
	if(vl53l0x.init_sensor(DEFAULT_DEVICE_ADDRESS) == 0) {
		printf("OK\n");
	} else {
		printf("FAILED\n");
	}

	printf("\t LSM9DS1: ");
	if(lsm9ds1.begin() != 0) {
		lsm9ds1.calibrate();
		printf("OK\n");
	} else {
		printf("FAILED\n");
	}

	printf("\t ICM20602: ");
	icm20602.init();
	if(icm20602.isOnline()) {
		printf("OK\n");
	} else {
		printf("FAILED\n");
	}

	// Attach LED to BLE service
	led_service.bind(&board_led);
	led_service.set_led_status(0);
}

void poll_sensors(void) {
	printf("Polling sensors...\n");

	/** Poll BME680 */
	// TODO - check if new data?
	float temperature 	= bme680->get_temperature();
	float pressure		= bme680->get_pressure();
	float humidity		= bme680->get_humidity();
	float gas_res		= bme680->get_gas_resistance();
	float co2_eq		= bme680->get_co2_equivalent();
	float breath_voc_eq	= bme680->get_breath_voc_equivalent();
	float iaq_score		= bme680->get_iaq_score();
	uint8_t iaq_acc		= bme680->get_iaq_accuracy();
	printf("BME680:\n");
	printf("\ttemperature: %.2f\n", temperature);
	printf("\tpressure: %.2f\n", pressure);
	printf("\thumidity: %.2f\n", humidity);
	printf("\tgas resistance: %.2f\n", gas_res);
	printf("\tco2 equivalent: %.2f\n", co2_eq);
	printf("\tbreath voc eq: %.2f\n", breath_voc_eq);
	printf("\tiaq score: %.2f\n", iaq_score);
	printf("\tiaq accuracy: %i\n", iaq_acc);
	temperature *= 10;	/** Scale up before converting to integer (preserves decimal component) */
	pressure *= 10;		/** Scale up before converting to integer (preserves decimal component) */
	humidity *= 10;		/** Scale up before converting to integer (preserves decimal component) */
	bme680_service.set_temp_c((int16_t) temperature);
	bme680_service.set_pressure((uint32_t) pressure);
	bme680_service.set_rel_humidity((uint16_t) humidity);
	bme680_service.set_gas_resistance((uint32_t) gas_res);
	bme680_service.set_estimated_co2(co2_eq);
	bme680_service.set_estimated_b_voc(breath_voc_eq);
	bme680_service.set_iaq_score((uint16_t) iaq_score);
	bme680_service.set_iaq_accuracy(iaq_acc);

	/** Poll MAX44009 */
	float als = (float) max44009.getLUXReading();
	printf("MAX44009:\n");
	printf("\tambient light reading: %.2f\n", als);
	max44009_service.set_als_reading(als);

	/** Poll Si7021 */
	si7021.measure();
	uint32_t humidity_si = si7021.get_humidity();
	uint32_t temp_si	 = si7021.get_temperature();
	humidity_si /= 100; // Divide by 100 to scale to 0.1 increments as specified by BLE
	temp_si /= 100;
	si7021_service.set_rel_humidity((uint16_t) humidity_si);
	si7021_service.set_temp_c((int16_t) temp_si);
	printf("Si7021:\n");
	printf("\ttemperature: %lu\n", temp_si);
	printf("\thumidity: %lu\n", humidity_si);

	/** Poll VL53L0X */
	uint32_t distance = 0;
	vl53l0x.get_distance(&distance);
	vl53l0x_service.set_distance((uint16_t) distance);
	printf("VL53L0X:\n");
	printf("\tdistance: %lu\n", distance);

	/** Poll LSM9DS1 */
	LSM9DS1Service::tri_axis_reading_t reading;
	lsm9ds1.readAccel();
	reading.x = lsm9ds1.calcAccel(lsm9ds1.ax);
	reading.y = lsm9ds1.calcAccel(lsm9ds1.ay);
	reading.z = lsm9ds1.calcAccel(lsm9ds1.az);
	printf("LSM9DS1:\n");
	printf("\taccel: (%0.2f, %0.2f, %0.2f)\n", reading.x, reading.y, reading.z);
	lsm9ds1_service.set_accel_reading(reading);

	lsm9ds1.readGyro();
	reading.x = lsm9ds1.calcGyro(lsm9ds1.gx);
	reading.y = lsm9ds1.calcGyro(lsm9ds1.gy);
	reading.z = lsm9ds1.calcGyro(lsm9ds1.gz);
	printf("\tgyro:  (%0.2f, %0.2f, %0.2f)\n", reading.x, reading.y, reading.z);
	lsm9ds1_service.set_gyro_reading(reading);

	lsm9ds1.readMag();
	reading.x = lsm9ds1.calcMag(lsm9ds1.mx);
	reading.y = lsm9ds1.calcMag(lsm9ds1.my);
	reading.z = lsm9ds1.calcMag(lsm9ds1.mz);
	printf("\tmag:   (%0.2f, %0.2f, %0.2f)\n", reading.x, reading.y, reading.z);
	lsm9ds1_service.set_mag_reading(reading);

	/** Poll ICM20602 */

	/** Check battery voltage */
	battery_mon_en = 1;
	wait_ms(10);
	float vbat = battery_voltage_in.read() * MAX_VBAT_VOLTAGE * 2.0f;
	printf("Battery Voltage:\n");
	printf("\tVbat: %.2f V\n", vbat);
	battery_voltage_service.set_voltage(vbat);
	battery_mon_en = 0;

	printf("\n");
}

void sensor_poll_main(void) {

	while(true) {
		rtos::ThisThread::sleep_for(POLL_INTERVAL_MS);
		poll_sensors();
	}

}

int main() {
    BLE &ble_interface = BLE::Instance();
    events::EventQueue event_queue;

    init_sensors();

    BLEProcess ble_process(event_queue, ble_interface);

    ble_process.on_init(mbed::callback(start_services));

    // bind the event queue to the ble interface, initialize the interface
    // and start advertising
    ble_process.start();

    // Spin off the sensor polling thread
    // need this to be separate from BLE processing since BLE requires higher priority processing
    rtos::Thread sensor_thread(osPriorityBelowNormal);
    sensor_thread.start(mbed::callback(sensor_poll_main));

    // Process the event queue.
    event_queue.dispatch_forever();

    return 0;
}
