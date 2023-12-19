import asyncio
import nats

async def run():
    nc = await nats.connect("https://nats.beyless.com")
    if nc.is_connected:
        print("Connected to NATS server!")
    else:
        print("Failed to connect.")
    await nc.close()

if __name__ == '__main__':
    asyncio.run(run())
