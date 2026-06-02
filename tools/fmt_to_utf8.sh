#!/bin/bash

# 执行环境：linux
# 该脚本可以将同级目录以及子目录下的所有GB2312以及ANSI编码转为utf-8

# UTF-8是一种超集，它可以无损地表示US-ASCII编码。这意味着当一个文件使用US-ASCII编码保存时，你可以将其视为UTF-8编码的文件，而不会丢失任何信息。
# 如果文件的内容确实只包含 US-ASCII 字符，而没有使用其他字符集的字符，那么无论你使用何种方法进行转换，文件的字符集都将保持为 US-ASCII。

# charset=iso-8859-1
find . -type f \( -name "*.c" -o -name "*.h"  -o -name "*.md"  -o -name "*.txt"  -o -name "*.bat" \) -exec sh -c 'file -i "{}" | grep -q "iso-8859-1" && iconv -f GBK -t UTF-8 "{}" > "{}.tmp" && mv "{}.tmp" "{}" && echo Converted: "{}"' \; -print
#  charset=us-ascii，转换无效
find . -type f \( -name "*.c" -o -name "*.h" \) -exec sh -c 'file -i "{}" | grep -q "us-ascii" && iconv -f US-ASCII -t UTF-8 "{}" > "{}.tmp" && mv "{}.tmp" "{}" && echo Converted: "{}"' \; -print
find . -type f -name "*.tmp" -delete