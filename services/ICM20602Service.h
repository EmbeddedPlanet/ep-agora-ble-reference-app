/*
 * ICM20602Service.h
 *
 *  Created on: Jun 19, 2019
 *      Author: becksteing
 */

#ifndef SERVICES_ICM20602SERVICE_H_
#define SERVICES_ICM20602SERVICE_H_

#include "platform/mbed_debug.h"
#include "platform/mbed_toolchain.h"

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/GattService.h"
#include "ble/GattCharacteristic.h"
#include "ble/GattAttribute.h"

#include "ble_constants.h"
#include "GattPresentationFormatDescriptor.h"

#define ICM20602_SERVICE_UUID UUID("00000003-8dd4-4087-a16a-04a7c8e01734")
#define ICM20602_ACCEL_XYZ_CHAR_UUID UUID("00001003-8dd4-4087-a16a-04a7c8e01734")
#define ICM20602_GYRO_XYZ_CHAR_UUID UUID("00002003-8dd4-4087-a16a-04a7c8e01734")

class ICM20602Service {

private:

	 typedef MBED_PACKED(struct) tri_axis_reading {
		int16_t x;
		int16_t y;
		int16_t z;
	} tri_axis_reading_t;

public:

	ICM20602Service() :
		accel_desc(GattCharacteristic::BLE_GATT_FORMAT_STRUCT,
				GattCharacteristic::BLE_GATT_UNIT_ACCELERATION_METRES_PER_SECOND_SQUARED), accel_desc_ptr(&accel_desc),
		gyro_desc(GattCharacteristic::BLE_GATT_FORMAT_STRUCT,
				GattCharacteristic::BLE_GATT_UNIT_ANGULAR_VELOCITY_RADIAN_PER_SECOND), gyro_desc_ptr(&gyro_desc),
		accel_char(ICM20602_ACCEL_XYZ_CHAR_UUID, &accel_reading, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NONE,
				(GattAttribute**)(&accel_desc_ptr), 1),
		gyro_char(ICM20602_GYRO_XYZ_CHAR_UUID, &gyro_reading, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NONE,
				(GattAttribute**)(&gyro_desc_ptr), 1),
		ICM20602_service(
		/** UUID */					ICM20602_SERVICE_UUID,
		/** Characteristics */		characteristics,
		/** Num Characteristics */	sizeof(characteristics) /
									sizeof(characteristics[0])),
		server(NULL),
		started(false)
		{
			characteristics[0] = &accel_char;
			characteristics[1] = &gyro_char;
		}

	void start(BLE &ble_interface)
	{
		// Can't start again!
		if(started) {
			return;
		}

		server = &ble_interface.gattServer();

		// Register the service
		ble_error_t err = server->addService(ICM20602_service);

		if(err) {
			debug("Error %u during ICM20602 service registration. \r\n", err);
			return;
		}

		debug("ICM20602 service registered\r\n");
		debug("service handle: %u\r\n", ICM20602_service.getHandle());
		started = true;

	}

protected:

	/** Descriptors (and their pointers...) */
	GattPresentationFormatDescriptor accel_desc;
	GattPresentationFormatDescriptor* accel_desc_ptr;
	GattPresentationFormatDescriptor gyro_desc;
	GattPresentationFormatDescriptor* gyro_desc_ptr;

	/** Characteristics */

	/** Standard Characteristics */
	ReadOnlyGattCharacteristic<tri_axis_reading_t> accel_char;
	ReadOnlyGattCharacteristic<tri_axis_reading_t> gyro_char;

	/** Underlying data */
	tri_axis_reading_t accel_reading;
	tri_axis_reading_t gyro_reading;

	GattCharacteristic* characteristics[2];

	GattService ICM20602_service;

	GattServer* server;

	bool started;

};


#endif /* SERVICES_ICM20602SERVICE_H_ */
