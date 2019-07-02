/*
 * BatteryVoltageService.h
 *
 *  Created on: Jun 19, 2019
 *      Author: becksteing
 */

#ifndef SERVICES_BATTERY_VOLTAGE_SERVICE_H_
#define SERVICES_BATTERY_VOLTAGE_SERVICE_H_

#include "platform/mbed_debug.h"
#include "platform/mbed_toolchain.h"

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/GattService.h"
#include "ble/GattCharacteristic.h"
#include "ble/GattAttribute.h"

#include "ble_constants.h"
#include "GattPresentationFormatDescriptor.h"

#define BATTERY_VOLTAGE_SERVICE_UUID UUID("00000009-8dd4-4087-a16a-04a7c8e01734")
#define BATTERY_VOLTAGE_CHAR_UUID UUID("00001009-8dd4-4087-a16a-04a7c8e01734")

class BatteryVoltageService {

public:

	BatteryVoltageService() :
		battery_voltage_desc(GattCharacteristic::BLE_GATT_FORMAT_FLOAT32,
				GattCharacteristic::BLE_GATT_UNIT_ELECTRIC_POTENTIAL_DIFFERENCE_VOLT), battery_voltage_desc_ptr(&battery_voltage_desc),
		battery_voltage_char(BATTERY_VOLTAGE_CHAR_UUID, &voltage,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&battery_voltage_desc_ptr), 1),
		voltage(0.0f),
		battery_voltage_service(
		/** UUID */					BATTERY_VOLTAGE_SERVICE_UUID,
		/** Characteristics */		characteristics,
		/** Num Characteristics */	sizeof(characteristics) /
									sizeof(characteristics[0])),
		server(NULL),
		started(false)
		{
			characteristics[0] = &battery_voltage_char;
		}

	void start(BLE &ble_interface)
	{
		// Can't start again!
		if(started) {
			return;
		}

		server = &ble_interface.gattServer();

		// Register the service
		ble_error_t err = server->addService(battery_voltage_service);

		if(err) {
			debug("Error %u during BatteryVoltage service registration. \r\n", err);
			return;
		}

		debug("BatteryVoltage service registered\r\n");
		debug("service handle: %u\r\n", battery_voltage_service.getHandle());
		started = true;

	}

protected:

	/** Descriptors (and their pointers...) */
	GattPresentationFormatDescriptor battery_voltage_desc;
	GattPresentationFormatDescriptor* battery_voltage_desc_ptr;

	/** Characteristics */

	/** Standard Characteristics */
	ReadOnlyGattCharacteristic<float> battery_voltage_char;

	/** Underlying data */
	float voltage;

	GattCharacteristic* characteristics[1];

	GattService battery_voltage_service;

	GattServer* server;

	bool started;

};


#endif /* SERVICES_BATTERYVOLTAGE_SERVICE_H_ */
