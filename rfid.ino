from gpiozero import OutputDevice
from time import sleep

relay = OutputDevice(17, active_high=True, initial_value=False)

try:
    relay.off()
    sleep(1)

    relay.on()
    sleep(5)

    relay.off()
    sleep(2)

    relay.on()
    sleep(5)

    relay.off()

finally:
    relay.close()