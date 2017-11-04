/*
 * uploader.cc
 */
#include "uploader.hh"

using namespace std;

/*
 * uploader thread handler
 *
 * @param param - input structure
 *
 */
void* Uploader::thread_handler(void* param) {

	/* get input parameters */
	param_t* temp = (param_t*)param;
	int cloudIndex = temp->cloudIndex;
	Uploader* obj = temp->obj;
	free(temp);

	Item_t output; 
	/* initialize hash object */
	CryptoPrimitive* hashobj = new CryptoPrimitive(SHA256_TYPE);

	/* main loop for uploader, end when indicator recv.ed */
	while (true) {
		/* get object from ringbuffer */
		obj->ringBuffer_[cloudIndex]->Extract(&output);

		/* IF this is a file header object.. */
		if (output.type == FILE_HEADER) {

			/* copy object content into metabuffer */
			memcpy(obj->uploadMetaBuffer_[cloudIndex]+obj->metaWP_[cloudIndex],
					&(output.fileObj.file_header), obj->fileMDHeadSize_);

			/* head array point to new file header */
			obj->headerArray_[cloudIndex] = (fileShareMDHead_t*)(obj->uploadMetaBuffer_[cloudIndex]+obj->metaWP_[cloudIndex]);

			/* meta index update */
			obj->metaWP_[cloudIndex] += obj->fileMDHeadSize_;

			/* copy file full path name */
			memcpy(obj->uploadMetaBuffer_[cloudIndex]+obj->metaWP_[cloudIndex], output.fileObj.data, output.fileObj.file_header.fullNameSize);

			/* meta index update */
			obj->metaWP_[cloudIndex] += obj->headerArray_[cloudIndex]->fullNameSize;

		}
		else if (output.type == SHARE_OBJECT || output.type == SHARE_END) {
			/* IF this is share object */
			int shareSize = output.shareObj.share_header.shareSize;

			/* see if the container buffer can hold the coming share, if not then perform upload */
			if(shareSize + obj->containerWP_[cloudIndex] > UPLOAD_BUFFER_SIZE){
				obj->performUpload(cloudIndex);
				obj->updateHeader(cloudIndex);
			}

			/* generate SHA256 fingerprint */
			hashobj->generateHash((unsigned char*)output.shareObj.data, shareSize, output.shareObj.share_header.shareFP);

			/* copy share header into metabuffer */
			memcpy(obj->uploadMetaBuffer_[cloudIndex]+obj->metaWP_[cloudIndex], &(output.shareObj.share_header), obj->shareMDEntrySize_);
			obj->metaWP_[cloudIndex]+=obj->shareMDEntrySize_;

			/* copy share data into container buffer */
			memcpy(obj->uploadContainer_[cloudIndex]+obj->containerWP_[cloudIndex], output.shareObj.data, shareSize);
			obj->containerWP_[cloudIndex]+=shareSize;

			/* record share size */
			obj->shareSizeArray_[cloudIndex][obj->numOfShares_[cloudIndex]] = shareSize;
			obj->numOfShares_[cloudIndex]++;

			/* update file header pointer */
			obj->headerArray_[cloudIndex]->numOfComingSecrets += 1;
			obj->headerArray_[cloudIndex]->sizeOfComingSecrets += output.shareObj.share_header.secretSize;

			/* IF this is the last share object, perform upload and exit thread */
			if(output.type == SHARE_END){
				obj->performUpload(cloudIndex);
				delete hashobj;
				pthread_exit(NULL);
			}
		}
	}
}

/*
 * constructor
 *
 * @param p - input large prime number
 * @param total - input total number of clouds
 * @param subset - input number of clouds to be chosen
 *
 */
Uploader::Uploader(int total, int subset, int userID, Configuration* confObj) {
	
	total_ = total;
	subset_ = subset;

	/* initialization */
	ringBuffer_ = (RingBuffer<Item_t>**)malloc(sizeof(RingBuffer<Item_t>*)*total_);
	uploadMetaBuffer_ = (char **)malloc(sizeof(char*)*total_);
	uploadContainer_ = (char **)malloc(sizeof(char*)*total_);
	containerWP_ = (int *)malloc(sizeof(int)*total_);
	metaWP_ = (int *)malloc(sizeof(int)*total_);
	numOfShares_ = (int *)malloc(sizeof(int)*total_);
	socketArray_ = (Socket**)malloc(sizeof(Socket*)*total_);
	headerArray_ = (fileShareMDHead_t **)malloc(sizeof(fileShareMDHead_t*)*total_);
	shareSizeArray_ = (int **)malloc(sizeof(int *)*total_);

	for (int i = 0; i < total_; i++) {

		ringBuffer_[i] = new RingBuffer<Item_t>(UPLOAD_RB_SIZE, true, 1);
		shareSizeArray_[i] = (int*)malloc(sizeof(int)*UPLOAD_BUFFER_SIZE);
		uploadMetaBuffer_[i] = (char*)malloc(sizeof(char)*UPLOAD_BUFFER_SIZE);
		uploadContainer_[i] = (char*)malloc(sizeof(char)*UPLOAD_BUFFER_SIZE);
		containerWP_[i] = 0;
		metaWP_[i] = 0;
		numOfShares_[i] = 0;

		param_t* param = (param_t*)malloc(sizeof(param_t));      // thread's parameter
		param->cloudIndex = i;
		param->obj = this;
		pthread_create(&tid_[i],0,&thread_handler, (void*)param);


		/* set sockets */
		char *IP = (char*)confObj->getServerConf(i).serverIP.c_str();
		socketArray_[i] = new Socket(IP ,confObj->getServerConf(i).dataStorePort, userID);
		accuData_[i] = 0;
		accuUnique_[i] = 0;
	}

	fileMDHeadSize_ = sizeof(fileShareMDHead_t);
	shareMDEntrySize_ = sizeof(shareMDEntry_t);

}

/*
 * destructor
 */
Uploader::~Uploader(){

	for (int i = 0; i < total_; i++) {

		delete(ringBuffer_[i]);
		free(shareSizeArray_[i]);
		free(uploadMetaBuffer_[i]);
		free(uploadContainer_[i]);
		delete(socketArray_[i]);
	}

	free(ringBuffer_);
	free(shareSizeArray_);
	free(headerArray_);
	free(socketArray_);
	free(numOfShares_);
	free(metaWP_);
	free(containerWP_);
	free(uploadContainer_);
	free(uploadMetaBuffer_);
}

/*
 * Initiate upload
 *
 * @param cloudIndex - indicate targeting cloud
 * 
 */
int Uploader::performUpload(int cloudIndex) {

	socketArray_[cloudIndex]->sendMeta(uploadMetaBuffer_[cloudIndex],metaWP_[cloudIndex]);
	socketArray_[cloudIndex]->sendData(uploadContainer_[cloudIndex], containerWP_[cloudIndex]);
	return 0;
}


/*
 * procedure for update headers when upload finished
 * 
 * @param cloudIndex - indicating targeting cloud
 */
int Uploader::updateHeader(int cloudIndex) {

	/* get the file name size */
	int offset = headerArray_[cloudIndex]->fullNameSize;

	/* update header counts */
	headerArray_[cloudIndex]->numOfPastSecrets += headerArray_[cloudIndex]->numOfComingSecrets;
	headerArray_[cloudIndex]->sizeOfPastSecrets += headerArray_[cloudIndex]->sizeOfComingSecrets;

	/* reset coming counts */
	headerArray_[cloudIndex]->numOfComingSecrets = 0;
	headerArray_[cloudIndex]->sizeOfComingSecrets = 0;

	/* reset all index (means buffers are empty) */
	containerWP_[cloudIndex] = 0;
	metaWP_[cloudIndex] = 0;
	numOfShares_[cloudIndex] = 0;

	/* copy the header into metabuffer */
	memcpy(uploadMetaBuffer_[cloudIndex],headerArray_[cloudIndex],fileMDHeadSize_+offset);
	metaWP_[cloudIndex]+=fileMDHeadSize_+offset;

	return 1;
}

/*
 * interface for adding object to ringbuffer
 *
 * @param item - the object to be added
 * @param size - the size of the object
 * @param index - the buffer index 
 *
 */
int Uploader::add(Item_t* item, int size, int index) {

	ringBuffer_[index]->Insert(item, size);
	return 1;
}

/*
 * indicate the end of uploading a file
 * 
 * @return total - total amount of data that input to uploader
 * @return uniq - the amount of unique data that transferred in network
 *
 */
int Uploader::indicateEnd(long long* total, long long* uniq){

	for (int i = 0; i < total_; i++) {
		
		pthread_join(tid_[i],NULL);
		*total+=accuData_[i];
		*uniq+=accuUnique_[i];
	}
	return 1;
}

int Uploader::uploadStub(char* name, int namesize) {

	char fullname[256];
	sprintf(fullname, "%s.meta", name);
	FILE* fp = fopen(fullname, "r");
	fseek(fp, 0, SEEK_END);
	int length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char* stub = (char*)malloc(sizeof(char)*length);

	int ret = fread(stub, 1, length, fp);
	cout << "flag" << endl;
	if (ret == 0) {
		printf("fail to open stub file before upload\n");
		return 0;
	}
	fclose(fp);

	int indicator = STUB;
	
	socketArray_[0]->genericSend((char*)&indicator, sizeof(int));
	socketArray_[0]->genericSend((char*)&length, sizeof(int));
	// send hashed file name to server
	socketArray_[0]->genericSend((char*)&namesize, sizeof(int));
	socketArray_[0]->genericSend(name, namesize);
	socketArray_[0]->genericSend(stub, length);
	free(stub);
	return 1;
}

