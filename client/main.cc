#include <bits/stdc++.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/time.h>

#include "chunker.hh"
#include "encoder.hh"
#include "decoder.hh"
#include "uploader.hh"
#include "downloader.hh"
#include "CryptoPrimitive.hh"
#include "exchange.hh"
#include "conf.hh"

using namespace std;

Chunker* chunkerObj;
Decoder* decoderObj;
Encoder* encoderObj;
Uploader* uploaderObj;
CryptoPrimitive* cryptoObj;
Downloader* downloaderObj;
Configuration* confObj;
KeyEx* keyObj;
/*
	function : get start time 
	input & output : t(double)
*/
void timerStart(double *t) {

	struct timeval tv;
	gettimeofday(&tv, NULL);
	*t = (double)tv.tv_sec+(double)tv.tv_usec*1e-6;
}

/*
	function : get time split 
	input & output : t(double)
*/
double timerSplit(const double *t) {

	struct timeval tv;
	double cur_t;
	gettimeofday(&tv, NULL);
	cur_t = (double)tv.tv_sec + (double)tv.tv_usec*1e-6;
	return (cur_t - *t);
}

/*
	function : output usage massage
	input : NULL
	output : kill program
*/
void usage() {

	cout<<"usage: ./CLIENT [filename] [userID] [action] [secutiyType]"<<endl;
	cout<<setw(4)<<"- [filename]: full path of the file;"<<endl;
	cout<<setw(4)<<"- [userID]: use ID of current client;"<<endl;
	cout<<setw(4)<<"- [action]: [-u] upload; [-d] download; [-r] rekeying;"<<endl;
	cout<<setw(4)<<"- [securityType]: [HIGH] AES-256 & SHA-256; [LOW] AES-128 & SHA-1"<<endl;
	exit(1);
}
int main(int argc, char *argv[]) {
	/* argument check */
	if (argc != 5) {
		
		usage();
	}
	/* get options */
	int userID = atoi(argv[2]);
	char* opt = argv[3];
	char* securesetting = argv[4];

	/* read file test*/
	fstream inputFile;
	inputFile.open(argv[1], ios::in | ios::binary);
	long readInFileSize = 0;
	unsigned char * buffer;
	if (inputFile.is_open()) {

		inputFile.seekg(0, ios::end);
		readInFileSize = inputFile.tellg();
		inputFile.clear();
		inputFile.seekg(0, ios::beg);
		inputFile.close();
		
	}
	int *chunkEndIndexList;
	int numOfChunks;
	int n, *kShareIDList;
	/* initialize openssl locks */
	if (!CryptoPrimitive::opensslLockSetup()) {

		printf("fail to set up OpenSSL locks\n");

		return 0;
	}

	confObj = new Configuration();
	/* load from config file 
	 *
	 * Get the total number of data stores
	 *
	 * */
	n = confObj->getN(); //n -> data stores number

	/* initialize buffers 
	 *
	 * Based on CDStore settings: https://github.com/chintran27/CDStore
	 *
	 * Buffer Size: 1GB
	 * Chunk Index List Size: 1024KB
	 * Secret Buffer Size for each secret object: 16KB
	 * Share Buffer Size for aggregated cipher block: numOfStore*16KB
	 *
	 * 
	 */

	int bufferSize = 1024*1024*1024;
	int chunkEndIndexListSize = 1024*1024;
	int secretBufferSize = 16*1024;
	int shareBufferSize = n*16*1024;

	unsigned char *secretBuffer, *shareBuffer;
	unsigned char tmp[secretBufferSize];
	memset(tmp,0,secretBufferSize);
	long zero = 0;
	buffer = (unsigned char*) malloc (sizeof(unsigned char)*bufferSize);
	chunkEndIndexList = (int*)malloc(sizeof(int)*chunkEndIndexListSize);
	secretBuffer = (unsigned char*)malloc(sizeof(unsigned char) * secretBufferSize);
	shareBuffer = (unsigned char*)malloc(sizeof(unsigned char) * shareBufferSize);

	/* initialize share ID list */
	kShareIDList = (int*)malloc(sizeof(int)*n);
	for (int i = 0; i < n; i++) {
		
		kShareIDList[i] = i;
	}
	/* full file name process */
	int namesize = 0;
	while(argv[1][namesize] != '\0'){
		namesize++;
	}
	namesize++;

	/* parse secure parameters */
	int securetype;
	if(strncmp(securesetting,"HIGH", 4) == 0) {
		
		securetype = HIGH_SEC_PAIR_TYPE;
	}
	else {

		if (strncmp(securesetting,"LOW", 4) == 0) {
			
			securetype = HIGH_SEC_PAIR_TYPE;
		}
		else {
			
			cerr<<"Securetype setting error!"<<endl;
			exit(1);
		}
	}

	/* upload procedure */
	if (strncmp(opt,"-u",2) == 0 || strncmp(opt, "-a", 2) == 0) {

		/* object initialization */
		uploaderObj = new Uploader(n,n,userID, confObj);
		encoderObj = new Encoder(n, securetype, uploaderObj);
		keyObj = new KeyEx(encoderObj, securetype, confObj->getkmIP(), confObj->getkmPort(), confObj->getServerConf(0), CHARA_MIN_HASH,VAR_SEG);
		keyObj->readKeyFile("./keys/public.pem");
		keyObj->newFile(userID, argv[1], namesize);

		chunkerObj = new Chunker(VAR_SIZE_TYPE);
		double timer,split,bw, timer2, split2;
		double total_t = 0;
		timerStart(&timer2);

		/* init the file header */
		Encoder::Secret_Item_t header;
		header.type = 1;
		memcpy(header.file_header.data, argv[1], namesize);
		header.file_header.fullNameSize = namesize;
		header.file_header.fileSize = readInFileSize;

		/* add the file header to encoder */
		encoderObj->add(&header);

		/* main loop for adding chunks */
		long total = 0;
		int totalChunks = 0;
		inputFile.open(argv[1], ios::in);

		while (total < readInFileSize) {
			
			timerStart(&timer);

			/* read in a batch of data in buffer */
			inputFile.read((char *)buffer,bufferSize);
			int ret = inputFile.gcount();
			//int ret = fread(buffer,1,bufferSize,fin);

			/* perform chunking on the data */
			chunkerObj->chunking(buffer,ret,chunkEndIndexList,&numOfChunks);
			split = timerSplit(&timer);
			total_t += split;

			int count = 0;
			int preEnd = -1;
			encoderObj->setTotalChunk(numOfChunks);
			/* adding chunks */
			while (count < numOfChunks) {

				/* create structure */
				KeyEx::Chunk_t input;
				input.chunkID = totalChunks;
				input.chunkSize = chunkEndIndexList[count] - preEnd;
				memcpy(input.data, buffer+preEnd+1, input.chunkSize);

				/* zero counting */
				if(memcmp(buffer+preEnd+1, tmp, input.chunkSize) == 0){
					zero += input.chunkSize;
				}

				/* set end indicator */
				input.end = 0;
				if(ret+total == readInFileSize && count+1 == numOfChunks){
					input.end = 1;
				}

				/* add chunk to key client */
				keyObj->add(&input);  			

				/* increase counter */
				totalChunks++;
				preEnd = chunkEndIndexList[count];
				count++;
			}
			total+=ret;
		}
		//cout<<"min flag"<<endl;
		long long tt, unique;
		tt = 0;
		unique = 0;
		uploaderObj->indicateEnd(&tt, &unique);
		cout << "flag0" << endl;
		// encrypt stub file
		encoderObj->encStub(argv[1], keyObj->current_key);


		// upload stub file to server
		cout << "flag1" << endl;
		uploaderObj->uploadStub(argv[1],namesize);
		cout << "flag2" << endl;

		//encoderObj->indicateEnd();
		split2 = timerSplit(&timer2);

		bw = readInFileSize/1024/1024/(split2-total_t);
		printf("%lf\t%lf\t%lld\t%lld\t%ld\n",bw,(split2-total_t), tt, unique, zero);
		delete uploaderObj;
		delete chunkerObj;
		delete encoderObj;
		inputFile.close();
		char cmd_1[256];
		// sprintf(cmd_1, "rm -rf %s.stub", argv[1]);
		char cmd_2[256];
		sprintf(cmd_2, "rm -rf %s.meta", argv[1]);
		system(cmd_1);
		system(cmd_2);
	}


	/* download procedure */
	if (strncmp(opt,"-d",2) == 0 || strncmp(opt, "-a", 2) == 0){
		/* init objects */
		decoderObj = new Decoder(n, securetype);
		downloaderObj = new Downloader(n,n,userID,decoderObj,confObj);
		downloaderObj->downloadStub(argv[1],namesize);
		keyObj = new KeyEx(encoderObj, securetype, confObj->getkmIP(), confObj->getkmPort(), confObj->getServerConf(0), CHARA_MIN_HASH,VAR_SEG);
		keyObj ->cpabeKeygen(userID);
		keyObj->downloadFile(userID, argv[1], namesize);
		double timer; //bw;
		string downloadPath(argv[1]);
		downloadPath += ".d";
		FILE * fw = fopen(downloadPath.c_str(),"wb");
		timerStart(&timer);
		/* download stub first */

		cout<<"stub "<<endl;
		decoderObj->init(argv[1]);
		decoderObj->setFilePointer(fw);
		decoderObj->setShareIDList(kShareIDList);
		cout<<"start"<<endl;

		/* start download procedure */
		downloaderObj->downloadFile(argv[1], namesize, n);
		/* see if download finished */
		decoderObj->indicateEnd();
		cout<<"over"<<endl;

		fclose(fw);
		delete downloaderObj;
		delete decoderObj;
		char cmd[256];
		sprintf(cmd, "rm -rf %s.stub.d", argv[1]);
		system(cmd);
	}
	cout<<"temp file clean up, download end"<<endl;


	if (strncmp(opt,"-r",2) == 0){
		decoderObj = new Decoder(n, securetype);
		downloaderObj = new Downloader(n,n,userID,decoderObj,confObj);
		downloaderObj->downloadStub(argv[1],namesize);
		keyObj = new KeyEx(encoderObj, securetype, confObj->getkmIP(), confObj->getkmPort(), confObj->getServerConf(0), CHARA_MIN_HASH,VAR_SEG);
		keyObj ->cpabeKeygen(userID);
		keyObj->downloadFile(userID, argv[1], namesize);
		cout << "Please Input The PolicyPath" <<endl;
		char *PolicyPath =  (char*)malloc(sizeof(char)*256);
		scanf("%s",PolicyPath);
		cout << "this is a flag for debug rekeying" <<endl;
		double timer,split,bw;
		timerStart(&timer);
		uploaderObj = new Uploader(n,n,userID,confObj);
		encoderObj = new Encoder(n, securetype, uploaderObj);
		
		keyObj->readKeyFile("./keys/public.pem");
		
		keyObj->updateFileByPolicy(userID, argv[1], namesize, PolicyPath);
		uploaderObj->uploadStub(argv[1],namesize);
		split = timerSplit(&timer);
		bw = readInFileSize/1024/1024/split;
		printf("%lf\t%lf\n", bw, split);
	}
	free(buffer);
	free(chunkEndIndexList);
	free(secretBuffer);
	free(shareBuffer);
	free(kShareIDList);
	CryptoPrimitive::opensslLockCleanup();
	inputFile.close();

	char cmd[256];
	sprintf(cmd, "rm -rf temp_cpabe.cpabe");
	system(cmd);	
	return 0;	
}
