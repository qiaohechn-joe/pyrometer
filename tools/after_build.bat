@echo off
setlocal

rem 获取脚本自身的路径
set "script_path=%~dp0"

rem 检查是否提供了boot bin目录和输出目录参数
if "%~1"=="" (
    echo 错误：未指定输入boot_bin目录。
    echo 使用方法：%0 输入目录 boot_bin目录
    exit /b 1
)

if "%~2"=="" (
    echo 错误：未指定app_bin目录。
    echo 使用方法：%0 输出目录 app_bin目录
    exit /b 1
)

if "%~3"=="" (
    echo 错误：未指定ZIP文件名。
    echo 使用方法：%0 输出ZIP文件名
    exit /b 1
)

rem 设置输出目录为外部参数传入的第一个值
set "boot_bin_dir=%~1"

rem 设置boot bin目录为外部参数传入的第二个值
set "app_bin_dir=%~2"

rem 设置ZIP文件名为外部参数传入的第三个值
set "zip_file_name=%~3"

set "output_dir=%app_bin_dir%/../"
set "zip_file_path=%output_dir%\%zip_file_name%.zip"

echo 脚本路径：%script_path%
echo boot bin文件路径：%boot_bin_dir%
echo app  bin文件路径：%app_bin_dir%
echo zip file name：%zip_file_name%

rem 调用同级目录下的 bintobin.exe
%script_path%bintobin.exe -vf %script_path%..\src\app\app_cfg.h -b %boot_bin_dir% -a %app_bin_dir% -o %app_bin_dir% -boot-addr 8000000 -app-addr 0x8010000

rem 将 boot*.bin 文件复制到指定的输出目录中
rem copy "%boot_bin_dir%\boot*.bin" "%app_bin_dir%\"
xcopy "%script_path%..\doc\release\*" "%app_bin_dir%\..\" /E /I /Y

echo 正在创建ZIP文件：%zip_file_path%
powershell.exe -Command "Compress-Archive -Path '%output_dir%*' -DestinationPath '%output_dir%\%zip_file_name%.zip' -Force -Verbose"

if %ERRORLEVEL% == 0 (
    echo ZIP文件创建成功：%zip_file_path%
) else (
    echo ZIP文件创建失败。
)

endlocal