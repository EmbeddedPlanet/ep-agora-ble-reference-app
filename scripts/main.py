#!python
import asyncio
from bleak import BleakClient, BleakScanner
from bleak.backends.device import BLEDevice
from bleak.uuids import *
import argparse
import functools
import re
import signal
import logging
import math
import sys
import struct

# The state is what is printed by a simple text animation loading thingy (below)
state_lock = asyncio.Lock()
state = ''

uuid_map = {  # UUID: Name, struct format, scaling factor
    "00002a6e-0000-1000-8000-00805f9b34fb": ('Temperature', '<h', 0.01),   # int16_t
    '00002a6f-0000-1000-8000-00805f9b34fb': ('Humidity', '<H', 0.01),      # uint16_t
    '00002a6d-0000-1000-8000-00805f9b34fb': ('Pressure', '<I', 0.1),       # uint32_t
    '00001001-8dd4-4087-a16a-04a7c8e01734': ('CO2', '<f', 1.0),            # float
    '00002001-8dd4-4087-a16a-04a7c8e01734': ('bVOC', '<f', 1.0),           # float
    '00005001-8dd4-4087-a16a-04a7c8e01734': ('Gas Resistance', '<I', 1.0)  # uint32_t
}


class DataEntry:

    def __init__(self):

        self.values = {}
        for key, val in uuid_map.items():
            name, fmt, scaling = val
            self.values[name] = math.nan

    def is_complete(self) -> bool:
        for key, val in self.values.items():
            if math.isnan(val):
                return False

        return True

    def clear(self):
        for val in self.values:
            val = math.nan

    def to_string(self) -> str:
        result = ''
        for key, val in self.values.items():
            result += f'{val},'

        return result


def looks_like_mac(name: str) -> bool:
    """
    Checks if the given name looks like a MAC address or not
    :param name: Name to check
    :return: True if name looks like a MAC address, false otherwise
    """
    mac_regex = '^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$'
    if re.search(mac_regex, name):
        return True
    else:
        return False


async def scan_for_name(name):
    """
    Scans for a specifically-named BLE device nearby
    :param name: Name of device to scan for
    :return: BLEDevice handle to found device, None if not found
    """
    results = []
    devices = await BleakScanner.discover(timeout=5.0)
    for device in devices:
        if name == device.name:
            results.append(device)

    return results


async def connect_main(args):
    global state, state_lock

    # Logger for stdout output
    debug_logger = logging.getLogger('stdout')

    data_logger = logging.getLogger('stdout')
    # Logger for data
    if args.output_file:
        data_logger = logging.getLogger('data')
        data_handler = logging.FileHandler(filename=args.output_file)
        data_handler.setFormatter(logging.Formatter('%(created)f,%(message)s'))
        data_logger.addHandler(data_handler)
        data_logger.setLevel(logging.INFO)

    h = logging.StreamHandler(sys.stdout)
    if args.log_file:
        h = logging.FileHandler(filename=args.log_file)

    h.setFormatter(logging.Formatter('%(asctime)s.%(msecs)d | %(levelname)s | %(filename)s: %(message)s'))
    debug_logger.addHandler(h)
    debug_logger.setLevel(logging.DEBUG if args.verbose else logging.INFO)

    target_address = ''
    # Check if the device argument looks like a MAC address
    if looks_like_mac(args.device):
        target_address = args.device
        pass
    else:
        async with state_lock:
            state = 'Scanning'
        agoras = await scan_for_name('EP Agora')
        debug_logger.info(f'Found {len(agoras)} nearby Agoras over BLE')
        for i, agora in enumerate(agoras):
            debug_logger.info(f'[{i}] - {agora.address}')

        # TODO ask which one to connect to
        # For now just connect to the first one
        target_address = agoras[0].address

    debug_logger.info(f'Attempting to connect to {target_address}')
    # Set the state for display to user
    async with state_lock:
        state = 'Connecting'

    client = BleakClient(target_address)
    connected = await client.connect()

    debug_logger.info(f'Connected to {target_address}')

    # Discover characteristics we want to collect
    chars_to_collect = []

    for service in client.services:
        if service.uuid == '00000001-8dd4-4087-a16a-04a7c8e01734':
            for char in service.characteristics:
                if char.uuid == '00002a6e-0000-1000-8000-00805f9b34fb':  # Temperature
                    chars_to_collect.append(char)
                elif char.uuid == '00002a6d-0000-1000-8000-00805f9b34fb':  # Pressure
                    chars_to_collect.append(char)
                elif char.uuid == '00002a6f-0000-1000-8000-00805f9b34fb':  # Humidity
                    chars_to_collect.append(char)
                elif char.uuid == '00001001-8dd4-4087-a16a-04a7c8e01734':  # CO2
                    chars_to_collect.append(char)
                elif char.uuid == '00002001-8dd4-4087-a16a-04a7c8e01734':  # bVOC
                    chars_to_collect.append(char)
                elif char.uuid == '00005001-8dd4-4087-a16a-04a7c8e01734':  # Gas resistance
                    chars_to_collect.append(char)

    # Now enable notifications for the characteristics we want to read
    # TODO - notifications not working for more than 1 characteristic
    #    for char in chars_to_collect:
    #        await client.start_notify(char, notification_handler)

    # Clear the state (silence spinbar message)
    async with state_lock:
        state = 'Logging'

    cancelled = False
    while not cancelled:
        data_entry = DataEntry()
        try:
            # Read every characteristic and log the record
            for char in chars_to_collect:
                data = await client.read_gatt_char(char)
                name, fmt, scaling_factor = uuid_map[char.uuid]
                unpacked = struct.unpack(fmt, data)[0]
                unpacked *= scaling_factor
                data_entry.values[name] = unpacked
                if data_entry.is_complete():
                    data_logger.info(data_entry.to_string())
                    data_entry.clear()
            await asyncio.sleep(3.0)
        except asyncio.CancelledError as e:
            cancelled = True

    async with state_lock:
        state = 'Disconnecting'

    #for char in chars_to_collect:
    #    await client.stop_notify(char)

    await client.disconnect()


async def print_waiting(flag: asyncio.Event):
    """
    Prints a textual waiting animation until flag is set
    :param flag: Flag to wait until it is set
    :return: None
    """

    global state, state_lock

    idx = 0
    while not flag.is_set():
        try:
            async with state_lock:
                if state != '':
                    dots = ''
                    for i in range(0, idx % 4):
                        dots += '.'
                    print(f'\r{state}{dots}', end='')
                    idx += 1

            await asyncio.sleep(0.25)
        except asyncio.CancelledError as e:
            # Print state until the other coroutines exit
            pass


# TODO not sure why functools.partial passes in an extra argument??? Just ignore...
def on_done(evt: asyncio.Event, args):
    evt.set()


async def shutdown(signal, loop):
    """Cleanup tasks tied to the service's shutdown."""
    # logging.info(f"Received exit signal {signal.name}...")
    print(f'Received exit signal {signal.name}...')
    print(f'Number of total tasks: {len(asyncio.all_tasks())}')
    tasks = [t for t in asyncio.all_tasks() if t is not
             asyncio.current_task()]

    print(f'Cancelling {len(tasks)} outstanding tasks')
    [task.cancel() for task in tasks]
    # logging.info(f"Cancelling {len(tasks)} outstanding tasks")
    await asyncio.gather(*tasks, return_exceptions=True)
    # logging.info(f"Flushing metrics")
    loop.stop()


if __name__ == '__main__':

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter,
                                     description='Log data from EP Agora BLE reference app')

    parser.add_argument('-v', '--verbose', dest='verbose', help='Enable verbose debug output')
    parser.add_argument('-d', '--dev', dest='device', required=True,
                        help='BLE device address OR name to connect to (address in hex format, can be separated by '
                             'colons)')
    parser.add_argument('-o', '--output-file', dest='output_file',
                        help='File to log captured data to in csv format. Does not include debug messages.')
    parser.add_argument('--log-file', dest='log_file',
                        help='File to log debug information of this program to. '
                             'By default it is printed to the terminal')
    args = parser.parse_args()

    loop = asyncio.get_event_loop()

    # Gracefully shutdown with signal handlers (thanks roguelynn)
    #signals = (signal.SIGHUP, signal.SIGTERM, signal.SIGINT)
    #for s in signals:
    #    loop.add_signal_handler(
    #        s, lambda s=s: asyncio.create_task(shutdown(s, loop))
    #    )

    event_flag = asyncio.Event()

    try:
        scan_task = loop.create_task(connect_main(args))
        scan_task.add_done_callback(
            functools.partial(on_done, event_flag))
        loop.create_task(print_waiting(event_flag))
        loop.run_forever()

    finally:
        loop.close()
