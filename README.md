# REED: Rekeying-aware Encrypted Deduplication Storage 

## Introduction

REED is an encrypted deduplication storage system with rekeying enabled.
Specifically, it can replace an existing key with a new key so as to protect
against key compromise and enable dynamic access control. REED builds on a
deterministic version of all-or-nothing transform (AONT) for secure and
lightweight rekeying, while preserving the deduplication capability. It also
exploits similarity to mitigate key generation overhead. We implement a REED
prototype with various performance optimization techniques. 

### Publications

- Jingwei Li, Chuan Qin, Patrick P. C. Lee, and Jin Li. 
**Rekeying for Encrypted Deduplication Storage.**
Proceedings of the 46th Annual IEEE/IFIP International Conference on Dependable Systems and Networks (DSN 2016) (Regular paper), Toulouse, France, June 2016.

- Chuan Qin, Jingwei Li, and Patrick P. C. Lee.
**The Design and Implementation of a Rekeying-aware Encrypted Deduplication Storage System.**
ACM Transactions on Storage (TOS), 13(1), 9:1-9:30, March 2017.


## Installation

### Dependencies

REED is built on Ubuntu 16.04 LTS with a g++ version of 5.1.0. It depends on the following libraries:

- OpenSSL (https://www.openssl.org/source/openssl-1.0.2a.tar.gz)
- Boost C++ library (http://sourceforge.net/projects/boost/files/boost/1.58.0/boost_1_58_0.tar.gz)
- GMP library (https://gmplib.org/)
- LevelDB (https://github.com/google/leveldb/archive/master.zip)
- CP-ABE toolkit and libbswabe (http://acsc.cs.utexas.edu/cpabe/) 
- PBC library (https://crypto.stanford.edu/pbc/)

We pack CP-ABE toolkit (version 0.11), GMP library (version 6.1.2), libbswabe (version 0.9) and PBC library (version 0.5.14) in `dependency/`. LevelDB (version 1.15.0) is also provided in `server/lib/`.

### Instructions

**Step 1:** Run the following commands to install OpenSSL and Boost C++ library.  
```
$ sudo apt-get install libssl-dev libboost-all-dev
```

**Step 2:** The GMP library depends on m4 that can be installed via the following command: 
```
$ sudo apt-get install m4
```

Then, compile our provided GMP source to make install. 
```
$ tar -xvf dependency/gmp-6.1.2.tar && cd gmp-6.1.2/ 
$ ./configure
$ make
$ sudo make install
$ cd ../ && rm -rf gmp-6.1.2/
```

Optionally, run `make check` (before removing the extracted GMP directory) to check the correctness of the GMP library.

**Step 3:** The PBC library depends on flex and bison, both of which can be installed via the following command:   
```
$ sudo apt-get install flex bison
```

Then, compile our provided PBC source to make install.
```
$ tar -xvf dependency/pbc-0.5.14.tar.gz && cd pbc-0.5.14/ 
$ ./configure
$ make
$ sudo make install
$ cd ../ && rm -rf pbc-0.5.14/
```

---

**Step 4:** The libbswabe depends on libglib2.0-dev which can be installed via the following command:
```
sudo apt-get install libglib2.0-dev
```

Then, compile our provided libbswabe source to make install.
```
$ tar -xvf dependency/libbswabe-0.9.tar.gz && cd libbswabe-0.9/ 
$ ./configure
$ make
$ sudo make install
$ cd ../ && rm -rf libbswabe-0.9/
```

**Step 5:** Configure the cpabe package for installation. 
```
$ tar -xvf dependency/cpabe-0.11.tar.gz && cd cpabe-0.11/ 
$ ./configure
```

Informed by the solutions to [make error with libgmp](https://stackoverflow.com/questions/10354053/ciphertext-policy-attribute-based-encryption-toolkit-make-error-with-libgmp) and [error in linking gmp](https://stackoverflow.com/questions/17373306/error-in-linking-gmp-while-compiling-cpabe-package-from-its-source-code), you need to make a few changes on the configuration files. 

First, add a line `-lgmp` into the definition of `LDFLAGS` (Line 14) in the Makefile. This makes the `LDFLAGS` like:    
```
...
LDFLAGS = -O3 -Wall \
    -lglib-2.0  \
    -Wl,-rpath /usr/local/lib -lgmp \
    -Wl,-rpath /usr/local/lib -lpbc \
    -lbswabe \
    -lcrypto -lcrypto \  # remember to add `\` here
    -lgmp # newly added line
...
```

Second, add the missed semicolon in the Line 67 of the file policy_lang.y.
```
...
result: policy { final_policy = $1; } # add the last (missed) semicolon
...
```

Finally, make and install the library.
```
$ make
$ sudo make install
$ cd ../ && rm -rf cpabe-0.11/
```

**Step 6:** The LevelDB depends on libsnappy-dev, which can be installed via the following command.
```
$ sudo apt-get install libsnappy-dev
```

Then, compile and make the LevelDB that is located at `server/lib/`. 
```
$ cd server/lib/leveldb-1.15.0/ && make
```

## REED Configurations

### Server

We include both datastore (for storing file related data and metadata) and keystore (for storing key information) in REED server. Compile server via the following command: 
```
$ cd server/ && make
```

Then, start a REED server by the following command. Here `port_1` and `port_2` direct to the datastore and keystore ports, respectively.  
```
./SERVER [port_1] [port_2]
```

### Key Manager

We have a key manager for key generation. Run the following commands to maintain key management service on the `port` of a machine.  

```
$ cd keymanager/ && make
./KEYMANAGER [port]
```

### Client

Edit the configuration file `client/client.conf` to set the server and the key manager information (note the information should be consistent with the IP address and port configured for the server and key manager).

- Line 1 specifies the number of servers in usage; currently we only support one server that combines the datastore and keystore. 
- Line 2 specifies the IP address and port of the configured key manager.
- Line 3 specifies the IP address and port of the configured server (including both the datastore and keystore, see above).

Compile and generate an executable program for client.

```
$ cd client/ && make
```

## Usage Example

After successful configurations, we can use REED client for a typical command:

```
useage:
	upload file: 
	./CLIENT -u [filename] [policy] [secutiyType]
    download file: 
	./CLIENT -d [filename] [privateKeyFileName] [secutiyType]
	rekeying file: 
	./CLIENT -r [filename] [oldPrivateKeyFileName] [policy] [secutiyType]
	keygen: 
	./CLIENT -k [attribute] [privateKeyFileName]

	 [filename]: full path of the file;
	 [policy]: like 'id = 1 or id = 2', provide the policy for CA-ABE encrytion;
	 [attribute]: like 'id = 1', provide the attribute for CA-ABE secret key generation;
	 [securityType]: [HIGH] AES-256 & SHA-256; [LOW] AES-128 & SHA-1
	 [privateKeyFileName]: get the private key by keygen function

```

in this version you could type policy by numbers to rekey your file and you can download it with your secret key.

### keyGen Function

You need to run keyGen before start using this system. And your should remember your secret key! 

For example: 
```
./CLIENT -k 'id =1' sk_1
```
You could get your sk_1 accroding to attribute 'id = 1'.

**This means that you could download the file which upload by policy 'id = 1' or 'id = 2'(incloud your attribute)**



### Upload Fuction

You need to make sure and remember your policy and secret key name. The secret key name will be asked when you download the file.

For example: 
```
./CLIENT -u /home/Documents/main.cc 'id = 1 or id = 2' HIGH
```
You can upload "/home/Documents/main.cc" with advanced encryption.
(This means that with the secret key genertde by 'id = 1' or 'id = 2' could download it)


### Download Function

You can start download your file as following example: 

```
./CLIENT -d /home/Documents/main.cc privateKeyFileName HIGH
```

you should type in your secret key, then you can get back your file in the same path.

**pay attention: you need to use the same securityType when upload file and download it**

### Rekeying Function

You can start rekeying your file as following example: 

```
./CLIENT -r /home/Documents/main.cc oldPrivateKeyFileName 'id = 1 or id = 3' HIGH
```

After that you could allow the secret key owner of 'id = 1' or 'id =3' could download the file in future.

**Pay attention: in this version, you can just us a number as your policy**


## Maintainers

 * Origin maintainer:

	- Chuan QIN, the Chinese University of Hong Kong, chintran27@gmail.com

* Current maintainers:

    - Yanjing Ren, UESTC, tinoryj@gmail.com
    - Jingang Ma, UESTC, demon64523@gmail.com

