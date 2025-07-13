import asyncio
from bleak import BleakClient, BleakScanner

DEVICE_NAME = "Azmuth"

# Corrected UUID from ESP32 byte array declaration
# WRITE_CHAR_UUID = "04030201-fceb-dac9-b8a7-f6e5d4c3b2a1"
WRITE_CHAR_UUID = "00000000-0000-0000-0000-0000000000A1"

async def main():
    print(f"Scanning for BLE devices named '{DEVICE_NAME}'...")
    devices = await BleakScanner.discover(timeout=5.0)

    target = None
    for d in devices:
        if d.name and DEVICE_NAME.lower() in d.name.lower():
            target = d
            print(f"Found: {d.name} ({d.address})")
            break

    if not target:
        print("Device not found.")
        return

    async with BleakClient(target.address) as client:
        print("Connected to ESP32 BLE device.")

        print("You can now send commands. Type 'exit' to quit.")
        while True:
            cmd = input("Enter command: ")
            if cmd.lower() in ["exit", "quit"]:
                break
            await client.write_gatt_char(WRITE_CHAR_UUID, cmd.encode(), response=False)
            print("Command sent.")

if __name__ == "__main__":
    asyncio.run(main())
