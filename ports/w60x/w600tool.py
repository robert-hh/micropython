#!/usr/bin/env python3

import serial
import os
import sys
import time
import struct
import argparse
import platform


import pyprind
from xmodem import XMODEM1k

__version__ = "0.2"

CMD_SET_BAUD = 0x31
CMD_ERASE = 0x32  # ROM boot only
CMD_SET_SEC = 0x33  # ROM boot only
CMD_GET_SEC = 0x34  # ROM boot only
CMD_SET_GAIN = 0x35
CMD_GET_GAIN = 0x36
CMD_SET_MAC = 0x37
CMD_GET_MAC = 0x38
CMD_GET_QFID = 0x3C  # ROM boot only
CMD_ERASE_SECBOOT = 0x3F


# CRC-16/CCITT-FALSE
def crc16(data: bytearray):
    crc = 0xFFFF
    for i in range(0, len(data)):
        crc ^= data[i] << 8
        for j in range(0, 8):
            if (crc & 0x8000) > 0:
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1
    return crc & 0xFFFF


def putc(c):
    sys.stdout.write(c)
    sys.stdout.flush()


def error_exit(msg):
    print("Error:", msg)
    sys.exit(1)


serial_device = None


def deviceHardReset():
    serial_device.setRTS(True)
    time.sleep(0.1)
    serial_device.setRTS(False)


def deviceWaitBoot(timeout=3):
    serial_device.timeout = 0.01
    serial_device.flushInput()
    started = time.time()
    buff = b""
    while time.time() - started < timeout:
        serial_device.write(b"\x1B")
        buff = buff + serial_device.read(1)
        buff = buff[-16:]  # Remember last 16 chars
        if buff.endswith(b"CCCC"):
            return True
    return False


def sendCommand(cmd):
    cmd = struct.pack("<BHH", 0x21, len(cmd) + 2, crc16(cmd)) + cmd
    serial_device.flushInput()
    serial_device.write(cmd)
    serial_device.flush()
    # print('<<< ', cmd.hex())


def deviceSetBaud(baud):
    prev_baud = serial_device.baudrate

    def serialSetBaud(value):
        if serial_device.baudrate == value:
            return
        serial_device.close()
        serial_device.baudrate = value
        serial_device.open()
        time.sleep(0.1)

    for retry in range(3):
        serialSetBaud(prev_baud)
        sendCommand(struct.pack("<II", CMD_SET_BAUD, baud))
        serialSetBaud(baud)
        if deviceWaitBoot():
            return True

    serialSetBaud(prev_baud)
    return False


def deviceEraseImage():
    serial_device.timeout = 1
    sendCommand(struct.pack("<I", CMD_ERASE))
    return deviceWaitBoot(5)


def deviceEraseSecboot():
    serial_device.timeout = 1
    sendCommand(struct.pack("<I", CMD_ERASE_SECBOOT))
    deviceWaitBoot(15)
    return deviceIsInRomBoot()


def deviceIsInRomBoot():
    return deviceGetFlashID() is not None


def deviceSetMAC(mac):
    serial_device.timeout = 1
    sendCommand(struct.pack("<I", CMD_SET_MAC) + mac)


def deviceGetMAC():
    serial_device.timeout = 1
    sendCommand(struct.pack("<I", CMD_GET_MAC))
    result = serial_device.read_until(b"\n")
    result = result.decode("ascii").upper().strip()
    if result.startswith("MAC:"):
        return result[4:]
    return None


def deviceGetFlashID():
    serial_device.timeout = 1
    sendCommand(struct.pack("<I", CMD_GET_QFID))
    result = serial_device.read_until(b"\n")
    result = result.decode("ascii").upper().strip()
    if result.startswith("FID:"):
        return result[4:]
    return None


def deviceUploadFile(fn):
    serial_device.timeout = 1
    statinfo_bin = os.stat(fn)
    bar = pyprind.ProgBar(statinfo_bin.st_size, bar_char="═")

    def ser_write(data, timeout=1):
        bar.update(1024)
        return serial_device.write(data)

    def ser_read(size, timeout=1):
        return serial_device.read(size)

    stream = open(fn, "rb+")
    time.sleep(0.2)
    modem = XMODEM1k(ser_read, ser_write)
    serial_device.flushInput()
    if modem.send(stream):
        time.sleep(1)
        reply = serial_device.read_until(b"run user code...")
        reply = reply.decode("ascii").strip()
        return reply

    return None


import serial.tools.list_ports


def getDefaultPort():
    comlist = serial.tools.list_ports.comports()
    if comlist:
        return comlist[0].device

    if platform.system() == "Windows":
        return "COM1"
    else:
        return "/dev/ttyUSB0"


if __name__ == "__main__":
    supportedBauds = [115200, 460800, 921600, 1000000, 2000000]

    parser = argparse.ArgumentParser(prog="w600tool")
    parser.add_argument("-p", "--port", default=getDefaultPort())
    parser.add_argument(
        "-b", "--baud", default=115200, type=int, choices=supportedBauds, metavar="BAUD"
    )
    parser.add_argument("--get-mac", action="store_true")
    parser.add_argument("--set-mac", metavar="MAC")
    parser.add_argument("--erase", "-e", action="store_true")
    parser.add_argument("--upload", "-u", metavar="FILE")
    parser.add_argument("--upload-baud", default=1000000, type=int, choices=supportedBauds)
    parser.add_argument("--version", action="version", version="%(prog)s " + __version__)
    args = parser.parse_args()

    print("Opening device:", args.port)
    serial_device = serial.Serial(args.port, args.baud, timeout=1)

    deviceHardReset()
    if not deviceWaitBoot():
        print("Push reset button to enter bootloader...")
        if not deviceWaitBoot(15):
            error_exit("Bootloader not responding")

    if args.set_mac:
        mac = args.set_mac.replace(":", "").replace(" ", "").upper()
        print("Setting MAC:", mac)
        deviceSetMAC(bytearray.fromhex(mac))

    if args.get_mac:
        mac = deviceGetMAC()
        print("MAC:", mac)

    if args.erase:
        print("Erasing secboot")
        if not deviceEraseSecboot():
            error_exit("Erasing secboot failed")

        print("Erasing image")
        deviceEraseImage()
        deviceWaitBoot(5)

    if args.upload:
        if not os.path.exists(args.upload):
            error_exit("The specified file does not exist")

        _, ext = os.path.splitext(args.upload)
        ext = ext.lower()

        if ext == ".fls" and not args.erase:
            print("Erasing secboot")
            if not deviceEraseSecboot():
                error_exit("Erasing secboot failed => Try entering ROM boot manually")
        elif ext == ".img" and deviceIsInRomBoot():
            error_exit("ROM bootloader only accepts FLS files")

        if args.upload_baud != serial_device.baudrate:
            if deviceSetBaud(args.upload_baud):
                print("Switched speed to", serial_device.baudrate)
            else:
                print("Warning: Cannot switch speed")
                if not deviceWaitBoot(5):
                    error_exit(
                        "Could not recover from speed switch failure => Try again, or set upload-baud to 115200"
                    )

        print("Uploading", args.upload)
        reply = deviceUploadFile(args.upload)

        print("Reset board to run user code...")
