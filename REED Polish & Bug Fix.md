# REED Polish & Bug Fix
- based on https://github.com/chintran27/Reed/ - last commit 

## 已修复的bug

1. client main.cc line 63-66: 必须能够打开要下载的文件才可以下载(参数1为文件路径，在main中不判断操作类型就强制打开。
2. client main.cc  line 82: 按注释解读为从conf file中获取配置参数，实际上配置全部写在conf.hh中，导致修改配置后必须重新编译。
3. client main.cc  line 231: decoder 构造函数存在逻辑问题，不应在构造时直接指定stub file路径。
4. client main.cc  line 250: bw计算使用的size来自line 65，在下载时不存在导致core dump。
5. client decoder.cc line 103: 构造函数要求指定.meta文件位置，导致无法使用下载的stubfile还原要下载的文件。（对应decoder.hh line 87-90
6. client decoder.cc line 112-121: 未确定判断文件是否正常打开直接使用ftell函数，在文件未打开时导致core dump。
7. client download.cc line 235: downloadstub方法中下载好stub文件后未输出到文件直接清空了下载内容。
8. client makefile : cpabe临时文件未clean
9. server makefile : 需手动创建对应文件夹
10. client chunker.hh : 定义了VAR_SIZE_TYPE的chunking, 但没有具体实现。
11. client/socket ：genericDownload 写法问题，超过64KB SocketBuffer大小限制的文件无法下载。
12. client/socket ： downloadChunk 写法问题，发送的下载请求信息不正确，导致无法下载。
13. server/dedupCore ：  indicate 标志符宏定义与client端协定不一致，导致判断请求类型错误。
14. client/socket ： download Stub错误，没有保存stub文件就将stubbuffer清空，导致还原文件错误。


## 尚未添加的功能

1. client/ssl ： 实现代码存在问题，且系统没有使用该模块，数据传输全部使用明文。
2. client/decoder ：上传至keystore中的stub文件其实是meta文件（未加密的stub文件），同时下载stub文件后，没有相应的解密过程。
3. client ： rekeying有实现，但是无法正常使用，且与paper不完全一致。
4. server ： 将每个chunk的密钥与chunk合并存储，client上传的cipher文件（cpabe加密后的密钥文件）没有被使用。
5. client ：没有实现多线程key的上传下载功能，仅支持单一keystore。
6. client下载server中不存在的文件时，程序异常退出，没有相应处理。






