/*
 * (C) 2019 Embedded Planet
 * Author: becksteing
 * Date: 6/19/19
 */

#include <stdio.h>

#include "platform/Callback.h"
#include "events/EventQueue.h"
#include "platform/NonCopyable.h"

#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble/GattClient.h"
#include "ble/GapAdvertisingParams.h"
#include "ble/GapAdvertisingData.h"
#include "ble/GattServer.h"
#include "BLEProcess.h"

/** Device Information Strings */
const char manufacturers_name[]	= "Embedded Planet";
const char model_number[]			= "Agora BLE";
const char serial_number[]		= "123456789";
const char hardware_revision[]	= "1.0";
const char software_revision[]	= "1.0.0";

/** Services */
#include "DeviceInformationService.h"
#include "BME680Service.h"
#include "Si7021Service.h"
#include "ICM20602Service.h"
#include "LSM9DS1Service.h"

/** Standard Services */
DeviceInformationService* deviceinfo_service;

/** Custom Services */
BME680Service bme680_service;
Si7021Service si7021_service;
ICM20602Service icm20602_service;
LSM9DS1Service lsm9ds1_service;

void start_services(BLE& ble) {

	/** Start the standard services */
	deviceinfo_service = new DeviceInformationService(ble,
			manufacturers_name, model_number, serial_number, hardware_revision, NULL, software_revision);

	/** Start the custom services */
	bme680_service.start(ble);
	si7021_service.start(ble);
	icm20602_service.start(ble);
	lsm9ds1_service.start(ble);


}

void stop_servies(void) {
	if(deviceinfo_service != NULL) {
		delete deviceinfo_service;
		deviceinfo_service = NULL;
	}
}


int main() {
    BLE &ble_interface = BLE::Instance();
    events::EventQueue event_queue;

    BLEProcess ble_process(event_queue, ble_interface);

    ble_process.on_init(mbed::callback(start_services));

    // bind the event queue to the ble interface, initialize the interface
    // and start advertising
    ble_process.start();

    // Process the event queue.
    event_queue.dispatch_forever();

    return 0;
}
