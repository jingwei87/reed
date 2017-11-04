# README
## INTRODUCTION

REED, a rekeying-aware encrypted deduplication storage system. REED builds on a deterministic version of all-or-nothing transform (AONT), such that it enables secure and lightweight rekeying, while preserving the deduplication capability. 

We propose two REED encryption schemes that trade between performance and security, and extend REED for dynamic access control. 

We implement a REED prototype with various performance optimization techniques and demonstrate how we can exploit similarity to mitigate key generation overhead. 

## ATTENTION

This implemention is not the final version and still has some bugs. Meanwhile, some features like rekeying function will be added in future.
 
## REQUIREMENTS

REED is built on Ubuntu 16.04 LTS with g++ version 5.1.0.

This software requires the following libraries:

 * OpenSSL (https://www.openssl.org/source/openssl-1.0.2a.tar.gz)
 * boost C++ library (http://sourceforge.net/projects/boost/files/boost/1.58.0/boost_1_58_0.tar.gz)
 * GMP library (https://gmplib.org/)
 * LevelDB (https://github.com/google/leveldb/archive/master.zip)
 * CP-ABE toolkit and libbswabe library (http://acsc.cs.utexas.edu/cpabe/) 
 * PBC library (https://crypto.stanford.edu/pbc/)

The LevelDB is packed in /server/lib/.


## INSTALLATION

### Base package installation
For linux user you can install the LevelDB dependency, OpenSSL and Boost by the following:

 * sudo apt-get install libssl-dev libboost-all-dev libsnappy-dev 

REED client also needs following packages to support CP-ABE toolkit:

 * sudo apt-get install flex bison libgmp3-dev libglib2.0-dev


 
### Dependency Installation

 Dependency:

 * Install gmp-6.1.2
 * Install pbc-0.5.14
 * Install libbswabe-0.9
 * Install cpabe-0.11

All the four packages are packed in /dependency. You need to using `tar` to decompression those packages.
For example : 

```
tar -xpf /dependency/gmp-6.1.2.tar.gz
```

**Pay attention : you need to install the four package as following order**

#### Step 1 : install gmp

For most computer(system), this package could  install by apt-get.
If you need to install by yourself, do the following steps:

```
cd /dependency/gmp-6.1.2
./configure
make
make check
sudo make install
```

> Using sudo need your passwd

#### Step 2 : install pbc

This package must install by yourself. 
The steps:

```
cd /dependency/pbc-0.5.14
./configure
make
sudo make install
```

#### Step 3 : install libbswabe

This package must install by yourself. 
The steps:

```
cd /dependency/libbswabe-0.9
./configure
sudo make
sudo make install
```


#### Step 4 : install cpabe

This package must install by yourself. 
The steps:

```
cd /dependency/cpabe-0.11
./configure
sudo make
```

After this step, you will get the error message like that : 

```
/usr/bin/ld: /usr/local/lib/libpbc.so: undefined reference to symbol '__gmpz_init'
/usr/local/lib/libgmp.so: error adding symbols: DSO missing from command line
collect2: error: ld returned 1 exit status
Makefile:34: recipe for target 'cpabe-setup' failed
make: *** [cpabe-setup] Error 1
```

So, you need to modify the makefile by yourself:

```
vim Makefile
```

In the makefile, you need to find the `LDFLAGS` :

```
LDFLAGS = -O3 -Wall \
    -lglib-2.0  \
    -Wl,-rpath /usr/local/lib -lgmp \
    -Wl,-rpath /usr/local/lib -lpbc \
    -lbswabe \
    -lcrypto -lcrypto \
```

And, then add `-lgmp` after `LDFLAGS` like that:

```
LDFLAGS = -O3 -Wall \
    -lglib-2.0  \
    -Wl,-rpath /usr/local/lib -lgmp \
    -Wl,-rpath /usr/local/lib -lpbc \
    -lbswabe \
    -lcrypto -lcrypto \
    -lgmp 
```

using `sudo make` again, and get new error message like that: 

```
policy_lang.y: In function ‘yyparse’:
policy_lang.y:67:38: error: expected ‘;’ before ‘}’ token
Makefile:50: recipe for target 'policy_lang.o' failed
make: *** [policy_lang.o] Error 1
```

You need to modify `policy_lang.y` file at line 67. All you need to do is add `;` before `}`. After modify, the line should look like that:

```
result: policy { final_policy = $1 ;}
```

At last :

```
sudo make 
sudo make install
```



## CONFIGURATION

### Configure the storage server (can be specified as keystore or datastore or both in conf.hh)


For data store and key store server:

* There will be three directories under /server/
    - "DedupDB" for levelDB logs
    - "RecipeFiles" for temp recipe files
    - "ShareContainers" for share local cache

* Start a server by "./SERVER [port] [port]"

  	Example: you have run keyStore server and dataStore server with "./SERVER [port_1] [port_2]" on machines (port_1 -> dataStore port; port_2 -> keyStore port):

  	
```
./SERVER 8000 8001
```
	

### Configure the client
  
  - Specify the number of storage nodes in /client/client.conf, line 1 (**for current version, we only support one storage node**)

  - Specify the key manager IPs, ports in /client/client.conf, line 2
  
  - Specify the store servers IPs and Ports in /client/client.conf, line 3 ~ (2 + numOfStorageServer) 


this conf file looks like that:
  
  ```
  1
127.0.0.1 8080
127.0.0.1 8000 8001
  ```
	
It means 

* one store server 
* one keymanager at 127.0.0.1 port 8080 
* one store server at 127.0.0.1 dataport 8000 & keyport 8001
	
### Start Keymanager
	
You have run 1 key manager with "./KEYMANAGER [port]" on machine:

```
./KEYMANAGER 8080
```

## MAKE


 * To make a client, on the client machine:
  - Go to /client/, type "make" to get the executable CLIENT program
  
 * To make a server for storage node:
  - Go to /server/lib/leveldb/, type "make" to make levelDB
  - Back to /server/, type "make" to get the executable SERVER program

 * To make a key manager, on the key manager machine:
  - Go to /keymanager/, type "make" to get the executable KEYMANAGER program

## USAGE EXAMPLE

 * After successful make

	usage: ./CLIENT [filename] [userID] [action] [secutiyType]

	- [filename]: full path of the file;
	- [userID]: user ID of current client;
	- [action]: [-u] upload; [-d] download;
	- [securityType]: [HIGH] AES-256 & SHA-256; [LOW] AES-128 & SHA-1


 * To upload a file "test", assuming from user "0" using enhanced scheme

	./CLIENT test 0 -u HIGH

 * To download a file "test", assuming from user "1" using baseline scheme

	./CLIENT test 1 -d LOW

## MAINTAINER

 * Origin maintainer

	- Chuan QIN, the Chinese University of Hong Kong, chintran27@gmail.com

* Current maintainer

    - Yanjing Ren, UESTC, tinoryj@gmail.com
    - Jingang Ma, UESTC, demon64523@gmail.com





