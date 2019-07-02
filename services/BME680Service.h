/*
 * BME680Service.h
 *
 *  Created on: Jun 19, 2019
 *      Author: becksteing
 */

#ifndef SERVICES_BME680SERVICE_H_
#define SERVICES_BME680SERVICE_H_

#include "platform/mbed_debug.h"

#include "ble/BLE.h"
#include "ble/GattServer.h"
#include "ble/GattService.h"
#include "ble/GattCharacteristic.h"
#include "ble/GattAttribute.h"

#include "ble_constants.h"
#include "GattPresentationFormatDescriptor.h"

#define BME680_SERVICE_UUID UUID("00000001-8dd4-4087-a16a-04a7c8e01734")
#define BME680_EST_CO2_CHAR_UUID UUID("00001001-8dd4-4087-a16a-04a7c8e01734")
#define BME680_EST_BVOC_CHAR_UUID UUID("00002001-8dd4-4087-a16a-04a7c8e01734")
#define BME680_IAQ_SCORE_CHAR_UUID UUID("00003001-8dd4-4087-a16a-04a7c8e01734")
#define BME680_IAQ_ACCURACY_CHAR_UUID UUID("00004001-8dd4-4087-a16a-04a7c8e01734")
#define BME680_GAS_RESISTANCE_CHAR_UUID UUID("00005001-8dd4-4087-a16a-04a7c8e01734")

class BME680Service {

public:

	BME680Service() :
		estimated_co2_desc(GattCharacteristic::BLE_GATT_FORMAT_FLOAT32, BLE_GATT_UNIT_CONCENTRATION_PPM), estimated_co2_desc_ptr(&estimated_co2_desc),
		estimated_bVOC_desc(GattCharacteristic::BLE_GATT_FORMAT_FLOAT32, BLE_GATT_UNIT_CONCENTRATION_PPM), estimated_bVOC_desc_ptr(&estimated_bVOC_desc),
		iaq_score_desc(GattCharacteristic::BLE_GATT_FORMAT_UINT16), iaq_score_desc_ptr(&iaq_score_desc),
		iaq_accuracy_desc(GattCharacteristic::BLE_GATT_FORMAT_UINT8), iaq_accuracy_desc_ptr(&iaq_accuracy_desc),
		gas_resistance_desc(GattCharacteristic::BLE_GATT_FORMAT_UINT32, GattCharacteristic::BLE_GATT_UNIT_ELECTRIC_RESISTANCE_OHM), gas_resistance_desc_ptr(&gas_resistance_desc),
		temp_c_char(GattCharacteristic::UUID_TEMPERATURE_CHAR, &temp_c,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
		rel_humidity_char(GattCharacteristic::UUID_HUMIDITY_CHAR, &rel_humidity,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
		pressure_char(GattCharacteristic::UUID_PRESSURE_CHAR, &pressure,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
		estimated_co2_char(BME680_EST_CO2_CHAR_UUID, &estimated_co2,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&estimated_co2_desc_ptr), 1),
		estimated_bVOC_char(BME680_EST_BVOC_CHAR_UUID, &estimated_bVOC,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&estimated_bVOC_desc_ptr), 1),
		iaq_score_char(BME680_IAQ_SCORE_CHAR_UUID, &iaq_score,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&iaq_score_desc_ptr), 1),
		iaq_accuracy_char(BME680_IAQ_ACCURACY_CHAR_UUID, &iaq_accuracy,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&iaq_accuracy_desc_ptr), 1),
		gas_resistance_char(BME680_GAS_RESISTANCE_CHAR_UUID, &gas_resistance,
				GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY,
				(GattAttribute**)(&gas_resistance_desc_ptr), 1),
		temp_c(0), rel_humidity(0), pressure(0), estimated_co2(0.0f), estimated_bVOC(0.0f), iaq_score(0),
		iaq_accuracy(0), gas_resistance(0),
		bme680_service(
		/** UUID */					BME680_SERVICE_UUID,
		/** Characteristics */		characteristics,
		/** Num Characteristics */	sizeof(characteristics) /
									sizeof(characteristics[0])),
		server(NULL),
		started(false)
		{
			characteristics[0] = &temp_c_char;
			characteristics[1] = &rel_humidity_char;
			characteristics[2] = &pressure_char;
			characteristics[3] = &estimated_co2_char;
			characteristics[4] = &estimated_bVOC_char;
			characteristics[5] = &iaq_score_char;
			characteristics[6] = &iaq_accuracy_char;
			characteristics[7] = &gas_resistance_char;
		}

	void start(BLE &ble_interface)
	{
		// Can't start again!
		if(started) {
			return;
		}

		server = &ble_interface.gattServer();

		// Register the service
		ble_error_t err = server->addService(bme680_service);

		if(err) {
			debug("Error %u during bme680 service registration. \r\n", err);
			return;
		}

		debug("bme680 service registered\r\n");
		debug("service handle: %u\r\n", bme680_service.getHandle());
		started = true;

	}

//	ble_error_t set_current_tag_id(const unsigned char* tag_id) const {
//		return server->write(current_tag_id_char.getValueHandle(), tag_id, SENTRICON_TAG_ID_LENGTH);
//	}
//
//	ble_error_t get_current_tag_id(unsigned char* tag_id) {
//		uint16_t value_length = SENTRICON_TAG_ID_LENGTH;
//		return server->read(current_tag_id_char.getValueHandle(), tag_id, &value_length);
//	}


protected:



	/** Descriptors (and their pointers...) */
	GattPresentationFormatDescriptor estimated_co2_desc;
	GattPresentationFormatDescriptor* estimated_co2_desc_ptr;
	GattPresentationFormatDescriptor estimated_bVOC_desc;
	GattPresentationFormatDescriptor* estimated_bVOC_desc_ptr;
	GattPresentationFormatDescriptor iaq_score_desc;
	GattPresentationFormatDescriptor* iaq_score_desc_ptr;
	GattPresentationFormatDescriptor iaq_accuracy_desc;
	GattPresentationFormatDescriptor* iaq_accuracy_desc_ptr;
	GattPresentationFormatDescriptor gas_resistance_desc;
	GattPresentationFormatDescriptor* gas_resistance_desc_ptr;

	/** Characteristics */

	/** Standard Characteristics */
	ReadOnlyGattCharacteristic<int16_t> temp_c_char;
	ReadOnlyGattCharacteristic<uint16_t> rel_humidity_char;
	ReadOnlyGattCharacteristic<uint32_t> pressure_char;

	/** Custom Characteristics */
	ReadOnlyGattCharacteristic<float> estimated_co2_char;
	ReadOnlyGattCharacteristic<float> estimated_bVOC_char;
	ReadOnlyGattCharacteristic<uint16_t> iaq_score_char;
	ReadOnlyGattCharacteristic<uint8_t> iaq_accuracy_char;
	ReadOnlyGattCharacteristic<uint32_t> gas_resistance_char;

	/** Underlying data */
	int16_t temp_c;
	uint16_t rel_humidity;
	uint32_t pressure;
	float estimated_co2;
	float estimated_bVOC;
	uint16_t iaq_score;
	uint8_t iaq_accuracy;
	uint32_t gas_resistance;

	GattCharacteristic* characteristics[8];

	GattService bme680_service;

	GattServer* server;

	bool started;

};



#endif /* SERVICES_BME680SERVICE_H_ */
