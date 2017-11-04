# REED Polish & Bug Fix
- based on https://github.com/chintran27/Reed/ - last commit 

## Fixed bugs 

1. client main.cc line 63-66: 必须能够打开要下载的文件才可以下载(参数1为文件路径，在main中不判断操作类型就强制打开。
2. client main.cc  line 82: 按注释解读为从conf file中获取配置参数，实际上配置全部写在conf.hh中，导致修改配置后必须重新编译。
3. client main.cc  line 231: decoder 构造函数存在逻辑问题，不应在构造时直接指定stub file路径。
4. client main.cc  line 250: bw计算使用的size来自line 65，在下载时不存在导致core dump。
5. client decoder.cc line 103: 构造函数要求指定.meta文件位置，导致无法使用下载的stubfile还原要下载的文件。（对应decoder.hh line 87-90
6. client decoder.cc line 112-121: 未确定判断文件是否正常打开直接使用ftell函数，在文件未打开时导致core dump。
7. client download.cc line 235: downloadstub方法中下载好stub文件后未输出到文件直接清空了下载内容。
8. client makefile : cpabe临时文件未clean
9. server makefile : 需手动创建对应文件夹
10. client chunker.hh : defined chunker VAR_SIZE_TYPE, but not implement
11. client/socket -> genericDownload method err, could not recv a big file (bigger than buffer) 
12. client/socket -> downloadChunk method err, err sending download request  
13. server/dedupCore -> indic num type err, sending indic & data length err 
14. client/socket -> downloadChunk method get stub's data (errr recv)
15. client/download -> multithread err while download chunks with client/socket/downloadChunk



## Features need add

1. client/ssl -> not used in data send/recv
2. client/decoder -> stub file get from ".meta" file (not encrypted)
3. client -> rekeying function not add
4. server -> download and store keys by .cipher file
5. client -> multithread keymanager
6. client -> multithread key upload and download
7. client -> retrieve file by download cipher file (now: download key with chunks)


