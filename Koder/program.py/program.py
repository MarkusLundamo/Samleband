import serial
import sys
import time
from time import sleep
from builtins import bytes
import threading
import RPi.GPIO as GPIO

ser = serial.Serial('/dev/ttyAMA0', baudrate=9600, timeout=1)

stegmotor_pinner = [24, 27, 22, 23]

seq = [[1,0,0,1],
    [1,0,0,0],
    [1,1,0,0],
    [0,1,0,0],
    [0,1,1,0],
    [0,0,1,0],
    [0,0,1,1],
    [0,0,0,1]
]

def sett_steg(w1, w2, w3, w4):
    GPIO.output(stegmotor_pinner[0], w1)
    GPIO.output(stegmotor_pinner[1], w2)
    GPIO.output(stegmotor_pinner[2], w3)
    GPIO.output(stegmotor_pinner[3], w4)

GPIO.setmode(GPIO.BCM)
for pin in stegmotor_pinner:
    GPIO.setup(pin, GPIO.OUT)

def stegmotor(steps, delay):
    for i in range(steps):
        for j in range(8):
            sett_steg(*seq[j])
            time.sleep(delay/1000)
    
def node_threader():
    global band_tilstand, ball_antall
    band_tilstand = 2
    while True:  
        line = sys.stdin.readline().strip()
        if line == "Start":
            band_tilstand = 1
        elif line == "Stop":
            band_tilstand = 2
        elif line == "To":
            ball_antall = 2
            print("To")
        elif line == "En":
            ball_antall = 1
            print("En")
        else:
            print(line)

def uart_threader():
    global ser, band_tilstand, ball_antall
    while True:
        str1 = ser.readline().decode('ascii', errors='ignore')
        if str1.strip():
            sleep(1)
            band_tilstand = 2
            if ball_antall == 2:
                ser.write(bytes("a\n", 'ascii'))
            elif ball_antall == 1:
                ser.write(bytes("1\n", 'ascii'))
            sleep(2)
            band_tilstand = 1

band_tilstand = 2
while True:
    node_thread = threading.Thread(target=node_threader)
    uart_thread = threading.Thread(target=uart_threader)
    uart_thread.start()
    node_thread.start()
    while True:
        while band_tilstand == 1:
            #print("Run motor")
            stegmotor(100, 1)
        while band_tilstand == 2:
            #print("Stop motor")
            sleep(1)
    


