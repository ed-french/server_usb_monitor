import serial
import datetime
import time

if __name__=="__main__":
    with serial.Serial("COM4") as ser:
        while True:
            ser.write(f"time:{datetime.datetime.now().isoformat()}\n")
            time.sleep(1)

            
