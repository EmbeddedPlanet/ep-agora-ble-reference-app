/*
 * MAX44009Service.h
 *
 *  Created on: Jun 19, 2019
 *      Author: becksteing
 */

#ifndef SERVICES_MAX44009SERVICE_H_
#define SERVICES_MAX44009SERVICE_H_

#include "platform/mbed_debug.h"
#include "platform/mbed_toolchain.h"

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/GattService.h"
#include "ble/GattCharacteristic.h"
#include "ble/GattAttribute.h"

#include "ble_constants.h"
#include "GattPresentationFormatDescriptor.h"

#define MAX44009_SERVICE_UUID UUID("00000005-8dd4-4087-a16a-04a7c8e01734")
#define MAX44009_AMBIENT_LIGHT_CHAR_UUID UUID("00001005-8dd4-4087-a16a-04a7c8e01734")

class MAX44009Service {

public:

	MAX44009Service() :
		als_desc(GattCharacteristic::BLE_GATT_FORMAT_FLOAT32,
				GattCharacteristic::BLE_GATT_UNIT_ILLUMINANCE_LUX), als_desc_ptr(&als_desc),
		als_char(MAX44009_AMBIENT_LIGHT_CHAR_UUID, &als_reading,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&als_desc_ptr), 1),
		als_reading(0),
		MAX44009_service(
		/** UUID */					MAX44009_SERVICE_UUID,
		/** Characteristics */		characteristics,
		/** Num Characteristics */	sizeof(characteristics) /
									sizeof(characteristics[0])),
		server(NULL),
		started(false)
		{
			characteristics[0] = &als_char;
		}

	void start(BLE &ble_interface)
	{
		// Can't start again!
		if(started) {
			return;
		}

		server = &ble_interface.gattServer();

		// Register the service
		ble_error_t err = server->addService(MAX44009_service);

		if(err) {
			debug("Error %u during MAX44009 service registration. \r\n", err);
			return;
		}

		debug("MAX44009 service registered\r\n");
		debug("service handle: %u\r\n", MAX44009_service.getHandle());
		started = true;

	}

protected:

	/** Descriptors (and their pointers...) */
	GattPresentationFormatDescriptor als_desc;
	GattPresentationFormatDescriptor* als_desc_ptr;

	/** Characteristics */

	/** Standard Characteristics */
	ReadOnlyGattCharacteristic<float> als_char;

	/** Underlying data */
	float als_reading;

	GattCharacteristic* characteristics[1];

	GattService MAX44009_service;

	GattServer* server;

	bool started;

};


#endif /* SERVICES_MAX44009SERVICE_H_ */
