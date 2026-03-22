#!/usr/bin/env python3
“””
Simple Relay Control - Minimal Enhancement Version
Same behavior as original with basic error handling
“””

from gpiozero import OutputDevice
from time import sleep

# Configuration

RELAY_PIN = 17

def main():
relay = None

```
try:
    # Initialize relay
    relay = OutputDevice(RELAY_PIN, active_high=True, initial_value=False)
    print("Relay initialized")
    
    # Sequence
    print("OFF - 1s")
    relay.off()
    sleep(1)
    
    print("ON - 5s")
    relay.on()
    sleep(5)
    
    print("OFF - 2s")
    relay.off()
    sleep(2)
    
    print("ON - 5s")
    relay.on()
    sleep(5)
    
    print("OFF - Final")
    relay.off()
    
    print("Sequence complete")
    
except Exception as e:
    print(f"Error: {e}")

finally:
    if relay:
        relay.close()
        print("Relay closed")
```

if **name** == “**main**”:
main()