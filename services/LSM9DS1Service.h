/*
 * LSM9DS1Service.h
 *
 *  Created on: Jun 19, 2019
 *      Author: becksteing
 */

#ifndef SERVICES_LSM9DS1SERVICE_H_
#define SERVICES_LSM9DS1SERVICE_H_

#include "platform/mbed_debug.h"
#include "platform/mbed_toolchain.h"

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/GattService.h"
#include "ble/GattCharacteristic.h"
#include "ble/GattAttribute.h"

#include "ble_constants.h"
#include "GattPresentationFormatDescriptor.h"

#define LSM9DS1_SERVICE_UUID UUID("00000004-8dd4-4087-a16a-04a7c8e01734")
#define LSM9DS1_ACCEL_XYZ_CHAR_UUID UUID("00001003-8dd4-4087-a16a-04a7c8e01734")
#define LSM9DS1_GYRO_XYZ_CHAR_UUID UUID("00002003-8dd4-4087-a16a-04a7c8e01734")
#define LSM9DS1_MAG_XYZ_CHAR_UUID UUID("00003004-8dd4-4087-a16a-04a7c8e01734")

class LSM9DS1Service {

public:

	 typedef MBED_PACKED(struct) tri_axis_reading {
		float x;
		float y;
		float z;
	} tri_axis_reading_t;

public:

	LSM9DS1Service() :
		accel_desc(GattCharacteristic::BLE_GATT_FORMAT_STRUCT,
				GattCharacteristic::BLE_GATT_UNIT_ACCELERATION_METRES_PER_SECOND_SQUARED), accel_desc_ptr(&accel_desc),
		gyro_desc(GattCharacteristic::BLE_GATT_FORMAT_STRUCT,
				GattCharacteristic::BLE_GATT_UNIT_ANGULAR_VELOCITY_RADIAN_PER_SECOND), gyro_desc_ptr(&gyro_desc),
		mag_desc(GattCharacteristic::BLE_GATT_FORMAT_STRUCT,
				GattCharacteristic::BLE_GATT_UNIT_MAGNETIC_FLUX_DENSITY_TESLA), mag_desc_ptr(&mag_desc),
		accel_char(LSM9DS1_ACCEL_XYZ_CHAR_UUID, &accel_reading, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NONE,
				(GattAttribute**)(&accel_desc_ptr), 1),
		gyro_char(LSM9DS1_GYRO_XYZ_CHAR_UUID, &gyro_reading, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NONE,
				(GattAttribute**)(&gyro_desc_ptr), 1),
		mag_char(LSM9DS1_MAG_XYZ_CHAR_UUID, &mag_reading, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NONE,
				(GattAttribute**)(&mag_desc_ptr), 1),
		LSM9DS1_service(
		/** UUID */					LSM9DS1_SERVICE_UUID,
		/** Characteristics */		characteristics,
		/** Num Characteristics */	sizeof(characteristics) /
									sizeof(characteristics[0])),
		server(NULL),
		started(false)
		{
			characteristics[0] = &accel_char;
			characteristics[1] = &gyro_char;
			characteristics[2] = &mag_char;
		}

	void start(BLE &ble_interface)
	{
		// Can't start again!
		if(started) {
			return;
		}

		server = &ble_interface.gattServer();

		// Register the service
		ble_error_t err = server->addService(LSM9DS1_service);

		if(err) {
			debug("Error %u during LSM9DS1 service registration. \r\n", err);
			return;
		}

		debug("LSM9DS1 service registered\r\n");
		debug("service handle: %u\r\n", LSM9DS1_service.getHandle());
		started = true;

	}

	const tri_axis_reading_t& get_accel_reading() const {
		uint16_t len = sizeof(accel_reading);
		server->read(accel_char.getValueHandle(), (uint8_t*) &accel_reading, &len);
		return accel_reading;
	}

	void set_accel_reading(const tri_axis_reading_t& accel_reading) {
		this->accel_reading = accel_reading;
		server->write(accel_char.getValueHandle(), (uint8_t*) &this->accel_reading,
				sizeof(this->accel_reading));
	}

	const tri_axis_reading_t& get_gyro_reading() const {
		uint16_t len = sizeof(gyro_reading);
		server->read(gyro_char.getValueHandle(), (uint8_t*) &gyro_reading, &len);
		return gyro_reading;
	}

	void set_gyro_reading(const tri_axis_reading_t& gyro_reading) {
		this->gyro_reading = gyro_reading;
		server->write(gyro_char.getValueHandle(), (uint8_t*) &this->gyro_reading,
				sizeof(this->gyro_reading));
	}

	const tri_axis_reading_t& get_mag_reading() const {
		uint16_t len = sizeof(mag_reading);
		server->read(mag_char.getValueHandle(), (uint8_t*) &mag_reading, &len);
		return mag_reading;
	}

	void set_mag_reading(const tri_axis_reading_t& mag_reading) {
		this->mag_reading = mag_reading;
		server->write(mag_char.getValueHandle(), (uint8_t*) &this->mag_reading,
				sizeof(this->mag_reading));
	}

protected:

	/** Descriptors (and their pointers...) */
	GattPresentationFormatDescriptor accel_desc;
	GattPresentationFormatDescriptor* accel_desc_ptr;
	GattPresentationFormatDescriptor gyro_desc;
	GattPresentationFormatDescriptor* gyro_desc_ptr;
	GattPresentationFormatDescriptor mag_desc;
	GattPresentationFormatDescriptor* mag_desc_ptr;

	/** Characteristics */

	/** Standard Characteristics */
	ReadOnlyGattCharacteristic<tri_axis_reading_t> accel_char;
	ReadOnlyGattCharacteristic<tri_axis_reading_t> gyro_char;
	ReadOnlyGattCharacteristic<tri_axis_reading_t> mag_char;

	/** Underlying data */
	tri_axis_reading_t accel_reading;
	tri_axis_reading_t gyro_reading;
	tri_axis_reading_t mag_reading;

	GattCharacteristic* characteristics[3];

	GattService LSM9DS1_service;

	GattServer* server;

	bool started;

};


#endif /* SERVICES_LSM9DS1SERVICE_H_ */
