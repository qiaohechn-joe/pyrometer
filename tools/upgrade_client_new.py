#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Author       : Zhenghong Duan hw.d@qq.com
# Date         : 2024-11-30 00:12:29
# LastEditors  : Zhenghong Duan hw.d@qq.com
# LastEditTime : 2024-11-30 20:03:47
# FilePath     : \cpd\project\升级上位机\upgrade_client_new.py
# Description  : 
# 
# Copyright (c) 2024 by Zhenghong Duan email:hw.d@qq.com, All Rights Reserved. 


import argparse
import serial
import struct
import binascii
import time
import os
import re
import zlib

# python upgrade_client_new.py --port COM6 --baudrate 115200 --addr 01 --seg-size 1024 --file firmware.bin

def modbus_crc(data):
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if (crc & 0x0001):
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return struct.pack('<H', crc)

def int_to_hex_bytes(value, byteorder='big'):
    # 将整数转换为字节串
    bytes_value = value.to_bytes((value.bit_length() + 7) // 8, byteorder=byteorder)
    # 将字节串转换为十六进制字节串
    hex_bytes = bytes_value
    return hex_bytes

def extract_version_from_filepath(filepath):
    # 从路径中提取文件名
    filename = os.path.basename(filepath)

    # 使用正则匹配类似 _V1-0-4_ 的模式
    match = re.search(r'_V(\d+)-(\d+)-(\d+)_', filename)
    if not match:
        raise ValueError("Filename does not contain a valid version pattern like _V1-0-4_")

    # 提取三段版本号并转换为整数
    v1, v2, v3 = map(int, match.groups())

    # 每个版本号限制在 0~255 范围内（因为要用单字节表示）
    if not (0 <= v1 <= 255 and 0 <= v2 <= 255 and 0 <= v3 <= 255):
        raise ValueError("Version numbers must be between 0 and 255.")

    # 构造三字节的 bytes 对象
    version_bytes = bytes([v1, v2, v3])
    return version_bytes

def crc32(data):
    """
    计算非反射版本的 CRC32 校验码（使用多项式 0x04C11DB7）
    
    :param data: 输入数据 (bytes)
    :return: 4字节的CRC校验码(bytes, big-endian)
    """
    # 生成 CRC32 表（非反射方式）
    crc_table = []
    for i in range(256):
        crc = i << 24  # 高位对齐到最高位
        for _ in range(8):
            if crc & 0x80000000:
                crc = (crc << 1) ^ 0x04C11DB7
            else:
                crc <<= 1
        crc_table.append(crc & 0xFFFFFFFF)

    crc = 0xFFFFFFFF  # 初始值

    for byte in data:
        crc = ((crc << 8) & 0xFFFFFF00) ^ crc_table[(crc >> 24) ^ byte]

    crc ^= 0x00000000  # 输出异或值

    return crc.to_bytes(4, 'little')  # 返回大端序格式


def calculate_file_crc32(filepath):
    """
    计算指定文件的 CRC32 校验码（以太网标准 IEEE 802.3）

    :param filepath: 文件路径
    :return: 32位 CRC 校验码 (bytes, big-endian)
    """
    with open(filepath, 'rb') as f:
        data = f.read()

    return crc32(data)

# def calculate_file_crc32(filepath):
#     with open(filepath, 'rb') as f:
#         file_content = f.read()
#     # return binascii.crc32(file_content).to_bytes(4, 'big')
#     # 固定返回 0xE46E426C（大端格式）
#     return bytes([0xf9, 0xE8, 0x7F, 0xC1])

def send_upgrade_notify(ser, addr, filepath, seg_size, max_retries=3):
    print("Sending upgrade notify...")

    version = extract_version_from_filepath(filepath)
    file_size = os.path.getsize(filepath)
    print(f"File size: {file_size} bytes")
    seg_num = (file_size + seg_size - 1) // seg_size
    file_crc32 = calculate_file_crc32(filepath)

    # Step 1: 构造 data 部分（升级通知内容）
    data = (
        version +
        struct.pack('<I', file_size) +
        struct.pack('<H', seg_size) +
        struct.pack('<H', seg_num) +
        file_crc32
    )

    print(f"data (before CRC): {data.hex()}")

    # Step 2: 计算 data 的 CRC（即 CRC16_D），用于后面的数据区段
    data_crc = modbus_crc(data)

    # Step 3: 构造 header 部分（不包含 header_crc）
    header_part = b'\x23' + addr + b'\x32\x01\x0F\x00'

    print(f"header_part: {header_part.hex()}")

    # Step 4: 计算 header CRC（CRC16_H）
    header_crc = modbus_crc(header_part)

    # Step 5: 构造完整帧
    frame = header_part + header_crc + data + data_crc

    print(f"Total frame length: {len(frame)}")

    # Step 6: 发送帧，并带重试机制
    for attempt in range(1, max_retries + 1):
        print(f"\nAttempt {attempt} to send upgrade notify...")
        ser.write(frame)
        print(f"Sent upgrade notify: {frame.hex()}")

        # 等待响应（假设最大等待时间为 1 秒）
        response = ser.read(len(frame))
        print(f"Received response: {response.hex() if response else 'None'}")

        try:
            if not response:
                raise Exception("No response received")
            if response[:len(frame)] != frame[:len(frame)]:
                raise Exception("Response mismatch")
            print("Upgrade notify succeeded.")
            return  # 成功退出函数
        except Exception as e:
            print(f"Attempt {attempt} failed: {e}")
            if attempt < max_retries:
                time.sleep(1)  # 失败后稍等片刻再重试

    # 所有尝试都失败了
    raise Exception("Failed to send upgrade notify after maximum retries")

def parse_request_frame(request_frame, expected_addr, expected_index=None, start_byte=b'\x23'):
    """
    解析并校验请求帧，返回解析出的包序号。

    :param request_frame: 收到的完整请求帧
    :param expected_addr: 期望地址（用于校验）
    :param expected_index: 如果不为 None，则检查请求中的包序号是否匹配
    :param start_byte: 起始符
    :return: packet_index 请求的包序号
    :raises: Exception 如果任何校验失败
    """
    print(f"Parsing request frame: {binascii.hexlify(request_frame)}")


    # 包长校验
    if len(request_frame) != 18:
        raise Exception("Received frame len error")

    # 基础字段校验
    if request_frame[0:1] != start_byte:
        raise Exception("Invalid start byte.")
    if request_frame[1:3] != expected_addr:
        raise Exception(f"Address mismatch. Expected: {binascii.hexlify(expected_addr)}, Got: {binascii.hexlify(request_frame[1:3])}")
    if request_frame[3] != 0x33 or request_frame[4] != 0x01:
        raise Exception("Command or operation type mismatch.")

    # 提取头部长度和 CRC
    data_len = struct.unpack('<H', request_frame[5:7])[0]
    header_crc = request_frame[7:9]
    calculated_header_crc = modbus_crc(request_frame[:7])
    if calculated_header_crc != header_crc:
        raise Exception("Header CRC check failed.")

    # 提取数据内容 + 数据 CRC
    received_data = request_frame[9:-2]
    received_data_crc = request_frame[-2:]
    calculated_data_crc = modbus_crc(received_data)
    if calculated_data_crc != received_data_crc:
        raise Exception("Data CRC check failed.")

    packet_index = struct.unpack('<H', received_data[3:5])[0]
    print( received_data)
    print( received_data[4:6])

    # 校验包序号是否匹配预期
    if expected_index is not None and packet_index != expected_index:
        raise Exception(f"Packet index mismatch. Expected: {expected_index}, Got: {packet_index}")

    print(f"Request for packet index: {packet_index}")
    return packet_index

def send_program_data(ser, expected_addr, file_path, seg_size=1024, start_index=0, max_retries=3, timeout=2.0):
    """
    响应式发送固件数据：每次等设备请求一个包，然后发送。

    :param ser: 串口对象（如 serial.Serial）
    :param expected_addr: 设备地址（bytes，小端格式）
    :param file_path: 固件文件路径
    :param seg_size: 每包大小（字节）
    :param start_index: 开始发送的包索引（用于断点续传）
    :param max_retries: 每个包最大重试次数
    :param timeout: 每次请求等待超时时间（秒）
    """
    print(f"Opening firmware file: {file_path}")

    version = extract_version_from_filepath(file_path)

    with open(file_path, 'rb') as f:
        file_size = os.path.getsize(file_path)
        total_packets = (file_size + seg_size - 1) // seg_size
        print(f"Total file size: {file_size} bytes, Total packets: {total_packets}")

        current_index = start_index
        while current_index < total_packets:
            retry_count = 0
            success = False

            while retry_count <= max_retries and not success:
                print(f"\nWaiting for request for packet {current_index}...")

                # 清空串口缓冲区
                ser.reset_input_buffer()

                # 等待请求帧
                request_frame = bytearray()
                start_time = time.time()

                while time.time() - start_time < timeout:
                    if ser.in_waiting > 0:
                        request_frame += ser.read(ser.in_waiting)
                        if len(request_frame) >= 11:  # 至少起始符+地址+命令+操作+长度+头CRC
                            break
                    time.sleep(0.1)

                if not request_frame:
                    print(f"No request received for packet {current_index}.")
                    retry_count += 1
                    continue

                # 解析请求帧
                try:
                    packet_index = parse_request_frame(
                        request_frame,
                        expected_addr=expected_addr,
                        expected_index=current_index
                    )
                except Exception as e:
                    print(f"Request frame error: {e}")
                    retry_count += 1
                    time.sleep(1.0)
                    continue
                
                # 读取当前包数据
                f.seek(packet_index * seg_size)
                raw_data = f.read(seg_size)
                if not raw_data:
                    raise Exception(f"Failed to read packet {packet_index} from file.")

                # 填充不足部分为 0xFF
                if len(raw_data) < seg_size:
                    raw_data += b'\xFF' * (seg_size - len(raw_data))
                print("=============1===========")
                print(version)
                print(packet_index)
                print(seg_size)
                # 构造 payload：[version(1)] + [index(2)] + [total_length(2)] + [data]
                payload = version + struct.pack('HH', packet_index, seg_size) + raw_data
                data_crc = modbus_crc(payload)
                print("=============2===========")
                # 构造头部
                start_byte = b'\x23'
                header = start_byte + expected_addr + b'\x33\x01'
                length = struct.pack('<H', len(payload))
                header_part = header + length
                header_crc = modbus_crc(header_part)

                # 组装完整帧
                reply_frame = header_part + header_crc + payload + data_crc

                # 发送回复帧
                ser.write(reply_frame)
                print(f"Sent packet {current_index}: {binascii.hexlify(reply_frame)}")

                success = True

            if not success:
                raise Exception(f"Failed to send packet {current_index} after {max_retries} retries.")

            current_index += 1

    print("Firmware update completed successfully.")

def send_upgrade_complete_confirmation(ser, device_addr, file_path, seg_size, timeout=2.0):
    """
    向设备发送升级完成确认帧，并等待设备的响应。

    :param ser: 串口对象（如 serial.Serial）
    :param device_addr: 设备地址（bytes，小端格式）
    :param software_version: 软件版本号（int）
    :param total_packets: 数据包总数（int）
    :param timeout: 等待响应的超时时间（秒）
    """
    software_version = extract_version_from_filepath(file_path)
    file_size = os.path.getsize(file_path)
    total_packets = (file_size + seg_size - 1) // seg_size

    print("======================1")
    print(software_version)
    # 构造数据部分：软件版本号 + 数据包总数
    data = software_version + struct.pack('<H', total_packets)
    print(data)
    # 构造头部
    start_byte = b'\x23'
    command = 52
    operation_type = 0
    data_length = len(data)
    print("======================3")
    header = start_byte + device_addr + struct.pack('<BBH', command, operation_type, data_length)
    print("======================4")
    # 计算并添加 CRC 校验码
    header_crc = modbus_crc(header)
    data_crc = modbus_crc(data)
    
    # 组装完整帧
    frame = header + header_crc + data + data_crc
    
    print(f"Sending upgrade complete confirmation frame: {binascii.hexlify(frame)}")
    ser.write(frame)
    
    # 等待设备响应
    start_time = time.time()
    response_frame = bytearray()
    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            response_frame += ser.read(ser.in_waiting)
            if len(response_frame) >= 11:  # 至少起始符+地址+命令+操作+长度+头CRC
                break
        time.sleep(0.1)
    
    if not response_frame:
        raise Exception("No response received from the device.")
    
    print(f"Received response frame: {binascii.hexlify(response_frame)}")
    
    # 解析响应帧
    if response_frame[0] != 0x23 or response_frame[1:3] != device_addr:
        raise Exception("Invalid response frame.")
    
    command_response = response_frame[3]
    operation_type_response = response_frame[4]
    data_length_response = struct.unpack('<H', response_frame[5:7])[0]
    
    if command_response != 52 or operation_type_response != 10 or data_length_response != 0:
        raise Exception("Unexpected response frame.")
    
    print("Upgrade complete confirmation successful.")


# 设置命令行参数
def parse_args():
    parser = argparse.ArgumentParser(description='MCU Program Upgrade Client')
    parser.add_argument('--port', type=str, required=True, help='Serial port name')
    parser.add_argument('--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('--addr', type=str, required=True, help='Slave address in hex (e.g., 01)')
    parser.add_argument('--file', type=str, required=True, help='Path to the firmware file')
    parser.add_argument('--seg-size', type=int, default=1024, help='Segment size for each data transmission (default: 1024 bytes)')
    return parser.parse_args()

# 主函数
def main():
    args = parse_args()
    addr = int(args.addr, 16).to_bytes(2, 'little')

    try:
        ser = serial.Serial(args.port, args.baudrate, timeout=1, parity=serial.PARITY_NONE)
        print(f"Connected to {args.port} at {args.baudrate} baud")

        send_upgrade_notify(ser, addr, args.file, args.seg_size)
        send_program_data(ser, addr, args.file, args.seg_size)
        time.sleep(1)
        send_upgrade_complete_confirmation(ser, addr, args.file, args.seg_size)

        print("Upgrade process completed successfully.")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        ser.close()

if __name__ == '__main__':
    main()