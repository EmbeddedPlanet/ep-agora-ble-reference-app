/*
 * (C) 2019 Embedded Planet
 * Author: becksteing
 * Date: 6/19/19
 */

#include <stdio.h>

/** Mbed */
#include "drivers/DigitalOut.h"
#include "platform/Callback.h"
#include "platform/NonCopyable.h"
#include "platform/mbed_wait_api.h"
#include "rtos/Thread.h"
#include "events/EventQueue.h"
#include "events/Event.h"
#include "LittleFileSystem.h"
#include "BlockDevice.h"

/** BLE */
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

// Prints extra sensor polling information
#define DEBUG_SENSOR_POLLING 0

// Erases the block device during filesystem initialization
#define ERASE_BLOCK_DEVICE 0

#define POLL_INTERVAL_MS 5000 // Sensor polling interval in milliseconds

#define MAX_VBAT_VOLTAGE 3.3f

#define LED_BLINK_SLOW_MS 1000	// Slow blinking while BLE is disconnected
#define LED_BLINK_FAST_MS 250	// Faster blinking while BLE is connected

/** Device Information Strings */
const char manufacturers_name[]	= "Embedded Planet";
const char model_number[]		= "Agora BLE";
const char serial_number[]		= "123456789";
const char hardware_revision[]	= "1.1";
const char firmware_revision[]	= " ";
const char software_revision[]	= "0.0.5";

/** Hardware peripheral driver objects are declared in agora_components.h */

/** BLE Process */
BLEProcess* ble_process;

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

/** Event Queue */
events::EventQueue event_queue;

/** Blink LED Event */
void blink_led(void);
events::Event<void(void)> led_event(&event_queue, blink_led);

/** BlockDevice on which the filesystem is mounted */
BlockDevice* fsbd;

/** Pairing file location */
static const char pairing_file_name[] = "/fs/sm.dat";

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

	printf("Initializing sensors...\r\n");
	printf("\t BME680: ");

	if(bme680->init(&sensor_i2c)) {
		printf("OK\r\n");
	} else {
		printf("FAILED\r\n");
	}

	// No way to check this really...
	printf("\t MAX44009: OK\r\n");

	printf("\t Si7021: ");
	if(si7021.check() == 1) {
		printf("OK\r\n");
	} else {
		printf("FAILED\r\n");
	}

	printf("\t VL53L0X: ");
	if(vl53l0x.init_sensor(DEFAULT_DEVICE_ADDRESS) == 0) {
		printf("OK\r\n");
	} else {
		printf("FAILED\r\n");
	}

	printf("\t LSM9DS1: ");
	if(lsm9ds1.begin() != 0) {
		lsm9ds1.calibrate();
		printf("OK\r\n");
	} else {
		printf("FAILED\r\n");
	}

	printf("\t ICM20602: ");
	icm20602.init();
	if(icm20602.isOnline()) {
		printf("OK\r\n");
	} else {
		printf("FAILED\r\n");
	}

	// Attach LED to BLE service
	led_service.bind(&board_led);
	led_service.set_led_status(0);
}

void poll_sensors(void) {
#if DEBUG_SENSOR_POLLING
	printf("Polling sensors...\n");
#endif

	/** Poll BME680 */
	float temperature 	= bme680->get_temperature();
	float pressure		= bme680->get_pressure();
	float humidity		= bme680->get_humidity();
	float gas_res		= bme680->get_gas_resistance();
	float co2_eq		= bme680->get_co2_equivalent();
	float breath_voc_eq	= bme680->get_breath_voc_equivalent();
	float iaq_score		= bme680->get_iaq_score();
	uint8_t iaq_acc		= bme680->get_iaq_accuracy();

#if DEBUG_SENSOR_POLLING
	printf("BME680:\n");
	printf("\ttemperature: %.2f\n", temperature);
	printf("\tpressure: %.2f\n", pressure);
	printf("\thumidity: %.2f\n", humidity);
	printf("\tgas resistance: %.2f\n", gas_res);
	printf("\tco2 equivalent: %.2f\n", co2_eq);
	printf("\tbreath voc eq: %.2f\n", breath_voc_eq);
	printf("\tiaq score: %.2f\n", iaq_score);
	printf("\tiaq accuracy: %i\n", iaq_acc);
#endif

	temperature *= 100;	/** Scale up before converting to integer (preserves decimal component) */
	pressure *= 10;		/** Scale up before converting to integer (preserves decimal component) */
	humidity *= 100;		/** Scale up before converting to integer (preserves decimal component) */
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

#if DEBUG_SENSOR_POLLING
	printf("MAX44009:\n");
	printf("\tambient light reading: %.2f\n", als);
#endif

	max44009_service.set_als_reading(als);

	/** Poll Si7021 */
	si7021.measure();
	uint32_t humidity_si = si7021.get_humidity();
	uint32_t temp_si	 = si7021.get_temperature();
	humidity_si /= 10; // Divide by 10 to scale to 0.01 increments as specified by BLE
	temp_si /= 10;
	si7021_service.set_rel_humidity((uint16_t) humidity_si);
	si7021_service.set_temp_c((int16_t) temp_si);

#if DEBUG_SENSOR_POLLING
	printf("Si7021:\n");
	printf("\ttemperature: %lu\n", temp_si);
	printf("\thumidity: %lu\n", humidity_si);
#endif

	/** Poll VL53L0X */
	uint32_t distance = 0;
	vl53l0x.get_distance(&distance);
	if(distance == 0) {
		// 0 means the distance is too far
		// Set to infinity
		distance = 0xFFFF;
	}
	vl53l0x_service.set_distance((uint16_t) distance);

#if DEBUG_SENSOR_POLLING
	printf("VL53L0X:\n");
	printf("\tdistance: %lu\n", distance);
#endif

	/** Poll LSM9DS1 */
	LSM9DS1Service::tri_axis_reading_t reading;
	lsm9ds1.readAccel();
	reading.x = lsm9ds1.calcAccel(lsm9ds1.ax);
	reading.y = lsm9ds1.calcAccel(lsm9ds1.ay);
	reading.z = lsm9ds1.calcAccel(lsm9ds1.az);

#if DEBUG_SENSOR_POLLING
	printf("LSM9DS1:\n");
	printf("\taccel: (%0.2f, %0.2f, %0.2f)\n", reading.x, reading.y, reading.z);
#endif

	lsm9ds1_service.set_accel_reading(reading);

	lsm9ds1.readGyro();
	reading.x = lsm9ds1.calcGyro(lsm9ds1.gx);
	reading.y = lsm9ds1.calcGyro(lsm9ds1.gy);
	reading.z = lsm9ds1.calcGyro(lsm9ds1.gz);
	lsm9ds1_service.set_gyro_reading(reading);

	lsm9ds1.readMag();
	reading.x = lsm9ds1.calcMag(lsm9ds1.mx);
	reading.y = lsm9ds1.calcMag(lsm9ds1.my);
	reading.z = lsm9ds1.calcMag(lsm9ds1.mz);

#if DEBUG_SENSOR_POLLING
	printf("\tgyro:  (%0.2f, %0.2f, %0.2f)\n", reading.x, reading.y, reading.z);
	printf("\tmag:   (%0.2f, %0.2f, %0.2f)\n", reading.x, reading.y, reading.z);
#endif

	lsm9ds1_service.set_mag_reading(reading);

	/** Poll ICM20602 */
	// TODO - support for ICM20602
	//icm20602.getAccXvalue();


	/** Check battery voltage */
	battery_mon_en = 1;
	wait_ms(10);
	float vbat = battery_voltage_in.read() * MAX_VBAT_VOLTAGE * 2.0f;

#if DEBUG_SENSOR_POLLING
	printf("Battery Voltage:\n");
	printf("\tVbat: %.2f V\n", vbat);
#endif

	battery_voltage_service.set_voltage(vbat);
	battery_mon_en = 0;

#if DEBUG_SENSOR_POLLING
	printf("\n");
#endif

}

void start_advertising(void) {
	//TODO - clear the bonding credentials storage
	printf("ble: pairing button pressed\n");
	if(ble_process->is_connected()) {
		ble_process->disconnect();
		// Disconnect handler will start advertising
	}
}

/** Push button long press handler */
void pb_long_press_handler(ep::ButtonIn* btn) {
	(void) btn; // Ignore argument
	// Disconnect and start advertising procedure -- defer to thread context
	event_queue.call(mbed::callback(start_advertising));
}

void sensor_poll_main(void) {

	while(true) {
		rtos::ThisThread::sleep_for(POLL_INTERVAL_MS);
		poll_sensors();
	}

}

bool create_filesystem()
{

	printf("filesystem - initializing...\n");

    /* Get the default system block device */

	/** Slice it so we only use part of it for the filesystem */
	static SlicingBlockDevice sbd(BlockDevice::get_default_instance(),
			0, (64*1024));
	fsbd = &sbd;
    BlockDevice& bd = *fsbd;

    int err = bd.init();

    if (err) {
    	printf("filesystem: failed to initialize block device\r\n");
        return false;
    }

#if ERASE_BLOCK_DEVICE

    err = bd.erase(0, bd.size());
    if(err) {
    	printf("filesystem: could not erase block device\r\n");
    	return false;
    }
#endif

    static LittleFileSystem fs("fs");

    err = fs.mount(&bd);

    if (err) {
        /* Reformat if we can't mount the filesystem */
        printf("filesystem: no filesystem found, formatting...\r\n");

        err = fs.reformat(&bd);

        if (err) {
            return false;
        }
    }

    return true;
}

void blink_led(void) {
	board_led = !board_led; // Toggle board LED
}

void on_ble_connect(void) {
	// Update the period of the event
	led_event.cancel();
	led_event.period(LED_BLINK_FAST_MS);
	led_event.call();
}

void on_ble_disconnect(void) {
	// Update the period of the event
	led_event.cancel();
	led_event.period(LED_BLINK_SLOW_MS);
	led_event.call();
}

int main() {
	printf("agora: BLE application begin\r\n");
    BLE &ble_interface = BLE::Instance();

    /* if filesystem creation fails or there is no filesystem the security manager
     * will fallback to storing the security database in memory */
    if (!create_filesystem()) {
        printf("filesystem: initialization failed!\r\n");
    } else {
    	printf("filesystem: initialization succeeded!\r\n");
    }

    init_sensors();

    BLEProcess main_ble_process(event_queue, ble_interface);
    ble_process = &main_ble_process;

    ble_process->on_init(mbed::callback(start_services));
    ble_process->on_connect_event().attach(on_ble_connect);
    ble_process->on_disconnect_event().attach(on_ble_disconnect);

    // bind the event queue to the ble interface, initialize the interface
    // and start advertising
    ble_process->start(pairing_file_name);

    // Attach a long press callback to the backside reset button -- starts repairing
    push_button_in.attach_long_press_callback(mbed::callback(pb_long_press_handler));

    // Spin off the sensor polling thread
    // need this to be separate from BLE processing since BLE requires higher priority processing
    rtos::Thread sensor_thread(osPriorityBelowNormal);
    sensor_thread.start(mbed::callback(sensor_poll_main));

    // Until Bluetooth is connected, blink slowly
    led_event.period(LED_BLINK_SLOW_MS);
    led_event.call();

    // Process the event queue.
    event_queue.dispatch_forever();

    return 0;
}
