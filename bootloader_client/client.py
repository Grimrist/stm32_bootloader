import serial
import os
import time
import math
import binascii

cmd_list = {
    'WRITE': b'\x31',
    'ERASE': b'\x43',
    'GO': b'\x20'
}

ACK = b'\x79'
NACK = b'\x1F'

def send_command(ser, cmd):
    try:
        cmd_send = cmd_list[cmd]
    except KeyError:
        print("Invalid command %d" % cmd)
    ser.write(cmd_send)
    ser.write((~int.from_bytes(cmd_send, byteorder='little')).to_bytes(length=1, byteorder='little', signed=True))
    resp = ser.read(1)
    if resp == NACK:
        print("MCU replied NACK for %s command, cancelling" % cmd)
        exit()
    print("MCU replied ACK for %s command" % cmd)

ser = serial.Serial('/dev/ttyACM0', 115200)
max_read = 32768//2
file_name = input("File to send:")
bank = input("Bank to use:")
page_offset = 0
if (bank == '2'):
    page_offset = 256

file_size = os.stat(file_name).st_size
print("File size is", file_size)

### First, we need to erase the right amount of pages in flash memory
# Send pages, then send address
# Each page is 2 Kibibytes
send_command(ser, 'ERASE')

page_count = math.ceil(file_size / 2048)
ser.write(page_count.to_bytes(length=4, byteorder='little'))
print("Sent page count %d" % page_count)
initial_page = 40 + page_offset
ser.write(initial_page.to_bytes(length=4, byteorder='little'))
print("Sent initial page %d" % initial_page)
resp = ser.read(1)
if resp == NACK:
    print("MCU replied NACK during erase, cancelling")
    exit()
print("MCU replied ACK for erase, advancing to write")

### Write app to flash memory that was erased
write_count = 0
while write_count < file_size:
    send_command(ser, 'WRITE')
    write_addr = 0x08000000 + (2048 * initial_page) + write_count
    ser.write(write_addr.to_bytes(length=4, byteorder='little'))
    print("Sent write addr %x" % write_addr)
    resp = ser.read(1)
    if resp == NACK:
        print("MCU replied NACK during write addr, cancelling")
        exit()
    print("MCU replied ACK for write addr, sending data")
    read_count = min(file_size - write_count, max_read)
    print("Read count is", read_count)
    ser.write(read_count.to_bytes(length=4, byteorder='little'))
    packets = 0
    with open(file_name, "rb") as data:
        data.seek(write_count)
        pck = data.read(read_count)
        if not pck:
            break
        print("CRC digest:", hex(binascii.crc32(pck)))
        ser.write(binascii.crc32(pck).to_bytes(length=4, byteorder='little'))
        # pck_size = len(pck)
        # ser.write(pck_size.to_bytes(length=4, byteorder='little'))
        ser.write(pck)
        packets += 1

    write_count += read_count
    print("Write count is", write_count)
    resp = ser.read(1)
    if resp == NACK:
        print("MCU replied NACK during write data, cancelling on packet %d" % packets)
        exit()
    print("MCU replied ACK for write")
    time.sleep(0.3)

### Jump to application
send_command(ser, 'GO')
go_addr = 0x08000000 + (2048 * initial_page)
ser.write(go_addr.to_bytes(length=4, byteorder='little'))
print("Sent go addr %x" % go_addr)
resp = ser.read(1)
if resp == NACK:
    print("MCU replied NACK during app jump, cancelling")
    exit()
print("MCU replied ACK for jump")

print("Transfer complete, sent %d packets" % packets)


