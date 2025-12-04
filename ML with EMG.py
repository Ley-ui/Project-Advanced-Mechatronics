import serial
import struct
import numpy as np
import tensorflow as tf
import time

# Load trained ML model
model = tf.keras.models.load_model("C:/Users/leyla/OneDrive - Universitetet i Agder/Biomedical Signal/My_Project_MAS509/MY_FINAL_ML_CV.keras")    # MY_FINAL_ML_CV

# Model expected input shape
input_shape = model.input_shape # (None, time_steps, channels)
time_steps = input_shape[1]
channels = input_shape[2]

class_names = {0: "Resting", 1: "Contracting"}

# Serial communication settings
SERIAL_PORT = 'COM4'
BAUD_RATE = 9600
PACKET_SIZE = 17        #Expected packet size in bytes
CHANNEL_INDEX = 0

# Serial connection to Arduino
ARDUINO_PORT = 'COM8'
ARDUINO_BAUD = 9600
arduino = serial.Serial(ARDUINO_PORT, ARDUINO_BAUD, timeout=0.1)
#time.sleep(2)

# Bluetooth setup
#BT_PORT = "COM7"
#BT_BAUD = 9600
#bt = serial.Serial(BT_PORT, BT_BAUD)
#time.sleep(2)
#print("Bluetooth connected !")

# Open serial connection
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
buffer = bytearray()    

# Sliding window buffer for ML input
window = []

#--------------- Extract EMG value from data packet-------------------
def parse_packet(packet):

    # Unpack header bytes (sync, version, counter)
    sync0, sync1, version, count = struct.unpack('BBBB', packet[0:4])

    # Check valid header
    if sync0 != 0xA5 or sync1 != 0x5A or version != 2:
        return None
    
    # Unpack channel value (6 channels)
    data = struct.unpack('>6H', packet[4:16])

    return data[CHANNEL_INDEX]

#-------------------------------------------------------------------------
def read_packet(ser, buffer):
    # Incrementally read serial bytes until a complete packet is assembled
    while True:
        b = ser.read(1) # read 1 byte
        if not b:
            break
        buffer += b
        while len(buffer) >= 2:         #at least 2 bytes --> check for sync header
            if buffer[0] == 0xA5 and buffer[1] == 0x5A:    
                if len(buffer) >= PACKET_SIZE:               #if we have enough bytes for a full packet
                    packet = buffer[:PACKET_SIZE]            #Extract 17 bytes
                    buffer[:] = buffer[PACKET_SIZE:]        #Remove from buffer
                    parsed = parse_packet(packet)           # Try to parse
                    if parsed:
                        return parsed                      
                    else:
                        if buffer:
                            buffer.pop(0)       #If parsing failed, discard 1 byte to resync
                        else:
                            break
                else:
                    break           #header found but not enough bytes yet
            else:
                #Not aligned --> discard first byte and retry
                if buffer:
                    buffer.pop(0)
                else:
                    break
    return None         #No complete packed yet

print("Live EMG prediction running...")
print("Press CTRL+C to stop\n")

try:
    while True:
        val = read_packet(ser, buffer)     #read new EMG sample

        if val is not None:
            window.append(val)  # add sample

            #If enough samples --> run ML model
            if len(window) == time_steps:

                # truncate high values
                median_val = np.median(window)
                threshold = median_val*1.5
                for i in range(1, len(window)):
                    if window[i] > threshold:
                        window[i] = window[i-1]

                #Convert to tensor input shape
                x_input = np.array(window).reshape(1, time_steps, channels)

                #print("Data sent to ML")
                #print(x_input)
                
                preds = model.predict(x_input, verbose=0)
                label = np.argmax(preds, axis=1)[0]
                movement = class_names[label]

                #print(f"\nMovement : {movement}")
                #time.sleep(1)
                print(f"Label envoyé à Arduino : {label}")

                #arduino.reset_input_buffer()
                arduino.write(f"{label}\n".encode())
                #time.sleep(1)
                
                #if arduino.in_waiting > 0:
                    #reception = arduino.readline().decode().strip()
                    #print("Aruino received : ", reception, "\n")
                    #time.sleep(1)

                # Slide window : remove first sample, keep latest 
                window = []

except KeyboardInterrupt:
    print("Stopped by user")
    ser.close()
    arduino.close()

