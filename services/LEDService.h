/*
 * LEDService.h
 *
 *  Created on: Jun 19, 2019
 *      Author: becksteing
 */

#ifndef SERVICES_LED_SERVICE_H_
#define SERVICES_LED_SERVICE_H_

#include "platform/mbed_debug.h"
#include "platform/mbed_toolchain.h"

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/GattService.h"
#include "ble/GattCharacteristic.h"
#include "ble/GattAttribute.h"

#include "ble_constants.h"
#include "GattPresentationFormatDescriptor.h"

#define LED_SERVICE_UUID UUID("00000008-8dd4-4087-a16a-04a7c8e01734")
#define LED_STATUS_CHAR_UUID UUID("00001008-8dd4-4087-a16a-04a7c8e01734")

class LEDService {

public:

	LEDService() :
		led_status_desc(GattCharacteristic::BLE_GATT_FORMAT_BOOLEAN), led_status_desc_ptr(&led_status_desc),
		led_status_char(LED_STATUS_CHAR_UUID, &led_status,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&led_status_desc_ptr), 1),
		led_status(0),
		led_service(
		/** UUID */					LED_SERVICE_UUID,
		/** Characteristics */		characteristics,
		/** Num Characteristics */	sizeof(characteristics) /
									sizeof(characteristics[0])),
		server(NULL),
		started(false)
		{
			characteristics[0] = &led_status_char;
		}

	void start(BLE &ble_interface)
	{
		// Can't start again!
		if(started) {
			return;
		}

		server = &ble_interface.gattServer();

		// Register the service
		ble_error_t err = server->addService(led_service);

		if(err) {
			debug("Error %u during LED service registration. \r\n", err);
			return;
		}

		debug("LED service registered\r\n");
		debug("service handle: %u\r\n", led_service.getHandle());
		started = true;

	}

protected:

	/** Descriptors (and their pointers...) */
	GattPresentationFormatDescriptor led_status_desc;
	GattPresentationFormatDescriptor* led_status_desc_ptr;

	/** Characteristics */

	/** Standard Characteristics */
	ReadWriteGattCharacteristic<bool> led_status_char;

	/** Underlying data */
	bool led_status;

	GattCharacteristic* characteristics[1];

	GattService led_service;

	GattServer* server;

	bool started;

};


#endif /* SERVICES_LED_SERVICE_H_ */
