import asyncio
from bleak import BleakClient, BleakScanner

DEVICE_NAME = "Azmuth"
WRITE_CHAR_UUID = "00000000-0000-0000-0000-0000000000A1"  # Replace with your actual UUID

COMMANDS_HELP = """
Available Commands:
  volup         - Volume Up
  voldown       - Volume Down
  mute          - Mute
  play          - Play / Pause
  next          - Next Track
  prev          - Previous Track
  stop          - Stop Playback
  move x y      - Move mouse by x and y (e.g., move 10 0)
  click         - Left click
  exit / quit   - Exit the program
"""

async def main():
    print(f"Scanning for BLE devices named '{DEVICE_NAME}'...")
    devices = await BleakScanner.discover(timeout=5.0)

    target = None
    for d in devices:
        if d.name and DEVICE_NAME.lower() in d.name.lower():
            target = d
            print(f"Found device: {d.name} ({d.address})")
            break

    if not target:
        print("Device not found.")
        return

    async with BleakClient(target.address) as client:
        print("Connected to ESP32 BLE device.")
        print(COMMANDS_HELP)

        while True:
            cmd = input("Enter command: ").strip()
            if cmd.lower() in ["exit", "quit"]:
                print("Exiting.")
                break

            try:
                await client.write_gatt_char(WRITE_CHAR_UUID, cmd.encode(), response=False)
                print("Command sent.")
            except Exception as e:
                print(f"Failed to send command: {e}")

if __name__ == "__main__":
    asyncio.run(main())
