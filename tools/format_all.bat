@echo off
setlocal

rem 获取当前脚本的目录路径
set SCRIPT_DIR=%~dp0

rem 配置根路径
set ROOT_PATH=%SCRIPT_DIR%..

rem 配置 clang-format 路径
set CLANG_FORMAT_PATH=%ROOT_PATH%\tools\clang-format.exe

rem 配置 Python 编码转换脚本路径
set ENCODE_CONVERT_PATH=%ROOT_PATH%\tools\convertEncoding.py

echo ====================Starting encoding conversion===================

rem 定义需要转换编码的目录列表
set ENCODE_DIRS=src\app src\bsp src\lib\microlib

rem 统一编码转换为 utf-8，只对指定目录
for %%d in (%ENCODE_DIRS%) do (
    python "%ENCODE_CONVERT_PATH%" -d "%ROOT_PATH%\%%d" -s GB2312 -t utf-8
)

echo =====================Starting code formatting=====================

rem 执行循环处理，只对指定目录下的文件进行格式化
for /f "delims=" %%i in ('dir /s /b "%ROOT_PATH%\*.c" "%ROOT_PATH%\*.h" ^| findstr /i /c:"src\\app" /c:"src\\bsp" /c:"src\\lib\\microlib" ') do (
    echo Formatting file: %%i
    "%CLANG_FORMAT_PATH%" -style=file -i "%%i"
)
endlocal