/* mbed Microcontroller Library
 * Copyright (c) 2017 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GATT_SERVER_EXAMPLE_BLE_PROCESS_H_
#define GATT_SERVER_EXAMPLE_BLE_PROCESS_H_

#include <stdint.h>
#include <stdio.h>

#include "events/EventQueue.h"
#include "platform/Callback.h"
#include "platform/NonCopyable.h"

#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble/GapAdvertisingParams.h"
#include "ble/GapAdvertisingData.h"
#include "ble/FunctionPointerWithContext.h"
#include "ble/gap/Types.h"
#include "ble/gap/Events.h"

/** Services */
#include "DeviceInformationService.h"
#include "BME680Service.h"

/**
 * Handle initialization adn shutdown of the BLE Instance.
 *
 * Setup advertising payload and manage advertising state.
 * Delegate to GattClientProcess once the connection is established.
 */
class BLEProcess : private mbed::NonCopyable<BLEProcess>,
				   public ble::Gap::EventHandler,
				   public SecurityManager::EventHandler {
public:
    /**
     * Construct a BLEProcess from an event queue and a ble interface.
     *
     * Call start() to initiate ble processing.
     */
    BLEProcess(events::EventQueue &event_queue, BLE &ble_interface) :
        event_queue(event_queue),
        ble_interface(ble_interface),
		connection_handle(),
		connected(false),
        post_init_cb(),
		sm_file_name(NULL)
		{
    }

    virtual ~BLEProcess()
    {
        stop();
    }

   /**
     * Subscription to the ble interface initialization event.
     *
     * @param[in] cb The callback object that will be called when the ble
     * interface is initialized.
     */
    void on_init(mbed::Callback<void(BLE&)> cb)
    {
        post_init_cb = cb;
    }

    /**
     * Initialize the ble interface, configure it and start advertising.
     * @param[in] sm_file File name of where to store security manager data (for persistent pairing, if supported)
     */
    bool start(const char* sm_file = NULL)
    {
        printf("Ble process started.\r\n");

        if (ble_interface.hasInitialized()) {
            printf("Error: the ble instance has already been initialized.\r\n");
            return false;
        }

        ble_interface.gap().setEventHandler(this);

        ble_interface.onEventsToProcess(
            makeFunctionPointer(this, &BLEProcess::schedule_ble_events)
        );

        sm_file_name = sm_file;

        ble_error_t error = ble_interface.init(
            this, &BLEProcess::when_init_complete
        );

        if (error) {
            printf("Error: %u returned by BLE::init.\r\n", error);
            return false;
        }

        return true;
    }

    /**
     * Disconnect from existing connection
     */
    void disconnect(void) {
    	if(connected) {
    		ble_error_t error = ble_interface.gap().disconnect(connection_handle,
    				ble::local_disconnection_reason_t(ble::local_disconnection_reason_t::USER_TERMINATION));
    		if(error) {
    			printf("ble: error when disconnecting - 0x%X\n", error);
    		} else {
    			printf("ble: disconnecting from central...\n");
    		}
    	}
    }

    /**
     * Close existing connections and stop the process.
     */
    void stop()
    {
        if (ble_interface.hasInitialized()) {
            ble_interface.shutdown();
            printf("Ble process stopped.");
        }
    }

    bool is_connected(void) {
    	return connected;
    }

private:

    /**
     * Schedule processing of events from the BLE middleware in the event queue.
     */
    void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *event)
    {
        event_queue.call(mbed::callback(&event->ble, &BLE::processEvents));
    }

    /**
     * Sets up adverting payload and start advertising.
     *
     * This function is invoked when the ble interface is initialized.
     */
    void when_init_complete(BLE::InitializationCompleteCallbackContext *event)
    {
        if (event->error) {
            printf("Error %u during the initialization\r\n", event->error);
            return;
        }
        printf("Ble instance initialized\r\n");


//        printf("ble: initializing the security manager\n");
//
//        // TODO - update these settings in the future
//        ble_error_t error = event->ble.securityManager().init(
//        		true,
//				false,
//				SecurityManager::IO_CAPS_NONE,
//				NULL,
//				false,
//				sm_file_name
//        		);
//
//
//        if(error) {
//        	printf("ble: error while initializing the security manager\n");
//        }
//
//        event->ble.securityManager().preserveBondingStateOnReset(true);
//        event->ble.securityManager().setSecurityManagerEventHandler(this);
//        event->ble.securityManager().setPairingRequestAuthorisation(true); // Accept any pairing request

        if (!set_advertising_parameters()) {
            return;
        }

        if (!set_advertising_data()) {
            return;
        }

        if (!start_advertising()) {
            return;
        }

        if (post_init_cb) {
            post_init_cb(ble_interface);
        }
    }

    /** Override Gap event handler */
    void onConnectionComplete(const ble::ConnectionCompleteEvent &event)
    {
    	connection_handle = event.getConnectionHandle();
        connected = true;
        printf("Connected.\r\n");
    }

    void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event)
    {
    	connected = false;
        printf("Disconnected.\r\n");
        start_advertising();
    }

    bool start_advertising(void)
    {
        Gap &gap = ble_interface.gap();

        /* Start advertising the set */
        ble_error_t error = gap.startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            printf("Error %u during gap.startAdvertising.\r\n", error);
            return false;
        } else {
            printf("Advertising started.\r\n");
            return true;
        }
    }

    bool set_advertising_parameters()
    {
        Gap &gap = ble_interface.gap();

        ble_error_t error = gap.setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            ble::AdvertisingParameters()
        );

        if (error) {
            printf("Gap::setAdvertisingParameters() failed with error %d", error);
            return false;
        }

        return true;
    }

    bool set_advertising_data()
    {
        Gap &gap = ble_interface.gap();

        /* Use the simple builder to construct the payload; it fails at runtime
         * if there is not enough space left in the buffer */
        ble_error_t error = gap.setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            ble::AdvertisingDataSimpleBuilder<ble::LEGACY_ADVERTISING_MAX_SIZE>()
                .setFlags()
                .setName("EP Agora")
				.setAppearance(ble::adv_data_appearance_t::GENERIC_TAG)
                .getAdvertisingData()
        );

        if (error) {
            printf("Gap::setAdvertisingPayload() failed with error %d", error);
            return false;
        }

        error = gap.setAdvertisingScanResponse(ble::LEGACY_ADVERTISING_HANDLE,
        		ble::AdvertisingDataSimpleBuilder<ble::LEGACY_ADVERTISING_MAX_SIZE>()
//				.setLocalService(BME680_SERVICE_UUID)
				.getAdvertisingData());

        if (error) {
            printf("Gap::setAdvertisingScanResponse() failed with error %d", error);
            return false;
        }


        error = gap.setDeviceName((const uint8_t*) "EP Agora");

        if (error) {
            printf("Gap::setDeviceName() failed with error %d", error);
            return false;
        }


        return true;

    }

    /** Override SecurityManagerEventHandler */
    void pairingRequest(ble::connection_handle_t connectionHandle)
    {
    	BLE& ble = BLE::Instance();
    	printf("ble: pairing requested\n");
    	ble.securityManager().acceptPairingRequest(connectionHandle);
    }

    /** Override SecurityManagerEventHandler */
    void linkEncryptionResult(
    		ble::connection_handle_t connectionHandle, ble::link_encryption_t result)
    {
    	printf("ble: link %s\n",
    			(result == ble::link_encryption_t::ENCRYPTED? "ENCRYPTED" : "not encrypted!"));


    }

    events::EventQueue &event_queue;
    BLE &ble_interface;
    ble::connection_handle_t connection_handle;
    bool connected;
    mbed::Callback<void(BLE&)> post_init_cb;
    const char* sm_file_name;

};

#endif /* GATT_SERVER_EXAMPLE_BLE_PROCESS_H_ */
