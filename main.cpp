/*
 * (C) 2019 Embedded Planet
 * Author: becksteing
 * Date: 6/19/19
 */

#include <stdio.h>

#include "platform/Callback.h"
#include "platform/NonCopyable.h"
#include "platform/mbed_wait_api.h"
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
LEDService led_service;
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
	if(bme680.begin()) {
		printf("OK\n");
	} else {
		printf("FAILED\n");
	}

	// No way to check this really...
	printf("\t MAX44009: OK\n");

	printf("\t Si7021: ");
	if(si7021.check() == 0) {
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

    event_queue.call_every(POLL_INTERVAL_MS, mbed::callback(poll_sensors));

    // Process the event queue.
    event_queue.dispatch_forever();

    return 0;
}
