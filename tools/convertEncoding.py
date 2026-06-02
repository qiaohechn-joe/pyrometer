'''
Author: jihan hw.d@qq.com
Date: 2024-05-16 13:48:16
LastEditors: jihan hw.d@qq.com
LastEditTime: 2024-06-03 15:49:06
FilePath: \CPDd:\repo\OR4000-qh\convertEncoding.py
Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
'''
import os
import sys
import argparse
import subprocess

# 定义你的依赖列表
required_dependencies = ['chardet']

def print_with_style(text, color=None, style=None):
    ANSI_COLOR_RED = "\033[91m"
    ANSI_COLOR_GREEN = "\033[92m"
    ANSI_COLOR_BOLD = "\033[1m"
    ANSI_COLOR_RESET = "\033[0m"

    style_code = ""
    if style == "bold":
        style_code = ANSI_COLOR_BOLD

    color_code = ""
    if color == "red":
        color_code = ANSI_COLOR_RED
    elif color == "green":
        color_code = ANSI_COLOR_GREEN

    print(f"{style_code}{color_code}{text}{ANSI_COLOR_RESET}")

def install_package(package_name):
    """尝试使用pip安装指定的包"""
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", package_name])
        print(f"Successfully installed {package_name}")
    except subprocess.CalledProcessError as e:
        print(f"Failed to install {package_name}: {e}")
        sys.exit(1)  # 安装失败则退出程序

def detect_encoding(file_path):
    with open(file_path, 'rb') as file:
        raw_data = file.read()
        result = chardet.detect(raw_data)
        encoding = result['encoding']
        confidence = result['confidence']
        return encoding, confidence

def convert_encoding(file_path, source_encoding, target_encoding):
    try:
        with open(file_path, 'r', encoding=source_encoding, errors='replace') as file:
            content = file.read()
        with open(file_path, 'w', encoding=target_encoding) as file:
            try:
                file.write(content)
                print(f"Converted {file_path} from {source_encoding} to {target_encoding}")
                return True
            except Exception as e:
                print_with_style(f"Error writing to {file_path}: {str(e)}", color="red", style="bold")
                # 写入过程中出现异常，恢复原始文件内容
                restore_original_content(file_path, content, source_encoding)
                return False
    except Exception as e:
        print(f"Error converting {file_path}: {str(e)}")
        return False

def restore_original_content(file_path, content, original_encoding):
    try:
        with open(file_path, 'w', encoding=original_encoding) as file:
            file.write(content)
        print_with_style(f"Restored original content for {file_path}", color="green", style="bold")
    except Exception as e:
        print_with_style(f"Error restoring original content for {file_path}: {str(e)}", color="red", style="bold")

def convert_directory(source_encoding, target_encoding, directory = '.'):
    converted_files = []
    non_converted_files = []
    if directory is None: # 默认当前目录
        directory = '.'
    for root, dirs, files in os.walk(directory):
        for file_name in files:
            file_path = os.path.join(root, file_name)
            file_extension = os.path.splitext(file_path)[1]
            if file_extension.lower() in ['.c', '.h', '.txt', '.md']:
                encoding, _ = detect_encoding(file_path)
                if encoding == source_encoding:
                    if convert_encoding(file_path, source_encoding, target_encoding):
                        converted_files.append(file_path)
                else:
                    non_converted_files.append((file_path, encoding))

    print_with_style("Conversion completed.", color="green", style="bold")
    print_with_style("Converted files:", color="green")
    for file_path in converted_files:
        print(file_path)
    print_with_style("Non-converted files:", color="green")
    for file_path, encoding in non_converted_files:
        print(f"{file_path} - Encoding: {encoding}")

# 创建命令行参数解析器
parser = argparse.ArgumentParser(description='Convert file encodings in a directory.')
parser.add_argument('-d', '--directory', type=str, help='Path to the directory containing files to be converted')
parser.add_argument('-s', '--source_encoding', type=str, required=True, help='The source encoding of the files (optional, default is auto-detect)')
parser.add_argument('-t', '--target_encoding', type=str, required=True, help='The target encoding to convert the files to')

# 解析命令行参数
args = parser.parse_args()

# 检查并安装依赖
for dependency in required_dependencies:
    try:
        import chardet
        __import__(dependency)
    except ImportError:
        print(f"{dependency} is not installed. Installing...")
        install_package(dependency)
# 调用转换函数
convert_directory(args.source_encoding, args.target_encoding, args.directory)