#ifndef MYFS_C
#define MYFS_C

#include <stdio.h>
#include <glib.h>
#include "myfile.c"
#include "mydisk.c"
#include "freespace.c"

char * diskFileNameTmp[4];
extern gboolean newDisk[4];
extern char * diskFileName[4];
//extern FILE * diskFile[4];
extern int diskCount;
extern int diskMode;
extern guint fileCounter;
extern GTree* fileMap;
extern GList* fileMapHole;

guint64 DISK_SIZE;

const guint HEADER_SIZE=17; // disk size (16B) + mode (1B)

const guint64 ADDR_FILE_COUNTER=17; // 4B
const guint64 ADDR_FREE_SPACE_VECTOR=21; // 1/8 B
const guint KEY_SIZE=8;
const guint FPTR_SIZE=8;
const guint ATIME_SIZE=4;
const guint FILE_SIZE=4;
// TODO : Need function to calculate this.
guint64 ADDR_FILE_MAP=100000021; // 12B*N
// Name = 8B
// fPTR = 8B

// TODO : function to set this value
guint64 ADDR_DATA=200000000; // 16B+X
// Atime = 4B
// Size = 4B
guint64 FS_SIZE=100000000; // TODO
guint64 MAX_FILE=1000001; // TODO

//guint64 halfOffset;

struct SearchData{
  gchar* key;
  gboolean status;
  GList* result;
  guint size;
};
typedef struct SearchData SearchData;

//-----------
struct FMAP{
  //gchar* key;
  guint fileNo;
  guint64 fptr;
};
typedef struct FMAP FMAP;

guint8* FileReadN(char * fileName,guint64 offset,guint size){
  FILE* fileptr;
  guint8* buffer;

  fileptr = fopen(fileName,"rb");  // Open file in binary
  fseek(fileptr,offset,SEEK_SET); // Seek file to the end

  buffer = (guint8*) malloc((size)*sizeof(guint8)); // Allocation memory for read file
  // TODO : Avoid allocate ahead (allocate only need)
  fread(buffer,size,1,fileptr); // Read file
  fclose(fileptr); // close file
  return buffer;
}

// TODO : Read entire file by stream (avoid allocate large memory"
guint8 * FileRead(char * fileName){
  FILE * fileptr;
  guint8 * buffer;
  guint64 filelen;

  fileptr = fopen(fileName,"rb");  // Open file in binary
  fseek(fileptr,0,SEEK_END); // Seek file to the end
  filelen = ftell(fileptr); // Get number of current byte offset
  rewind(fileptr); // Junp to begin

  printf("filelen : %" G_GUINT64_FORMAT "\n",filelen);

  buffer = (guint8 *) malloc((filelen)*sizeof(guint8)); // Allocation memory for read file
  // TODO : Avoid (large) allocate ahead (allocate only need)
  fread(buffer,filelen,1,fileptr); // Read entire file // TODO what all this param?
  fclose(fileptr); // close file
  return buffer;
}

void editFile(char * filename,void * data, guint64 byteOffset,guint64 size){
  printf("Write @%"G_GUINT64_FORMAT"(%"G_GUINT64_FORMAT")\n",byteOffset,size);
  FILE * writeptr;
  writeptr = fopen(filename,"rb+");
  fseek ( writeptr, byteOffset, SEEK_SET );
  // TODO - test write
  fwrite(data,1,size,writeptr);
  fclose(writeptr);

}

void writeFile(char * filename,char * fileData){
  FILE * writeptr;
  writeptr = fopen(filename,"wb");
  fwrite(fileData,strlen(fileData),1,writeptr);
  fclose(writeptr);
}
//-----------

// Mirror
void mirrorWrite(int diskGroup,void* data,guint64 addr,guint size){

  if(diskGroup==0){ // first
    if(newDisk[0]==FALSE){
      editFile(diskFileName[0],data,addr,size);
    }else{
      printf("D0 is new\n");
    }

    if(newDisk[1]==FALSE){
      editFile(diskFileName[1],data,addr,size);
    }else{
      printf("D1 is new\n");
    }
  }
  if(diskGroup==1){ // second
    if(newDisk[2]==FALSE){
      editFile(diskFileName[2],data,addr,size);
    }else{
      printf("D2 is new\n");
    }

    if(newDisk[3]==FALSE){
      editFile(diskFileName[3],data,addr,size);
    }else{
      printf("D3 is new\n");
    }
  }
  
}

void diskWriteData(void* data,guint64 addr,guint size){
  // 4 disk mng

  // first
  if(addr+size<DISK_SIZE){
    mirrorWrite(0,data,addr,size);
  }
  // cross 2 disk
  else if(addr<DISK_SIZE){
    guint size0,size1;
    guint64 addr0,addr1;
    void *data0,*data1;
    // 0
    size0=DISK_SIZE-addr;
    addr0=addr;
    data0=data;
    mirrorWrite(0,data0,addr0,size0);

    // 1
    size1=size-size0;
    addr1=HEADER_SIZE;
    data1=data+size0;
    mirrorWrite(1,data1,addr1,size1);

  }

  // second
  else{
    addr=addr-DISK_SIZE+HEADER_SIZE;
    mirrorWrite(1,data,addr,size);
  }

}

guint8* diskRead(int diskNo,guint64 addr,guint size){
  FILE* fileptr;
  guint8* buffer;

  fileptr = fopen(diskFileName[diskNo],"rb");  // Open file in binary
  fseek(fileptr,addr,SEEK_SET); // Seek file to the end

  buffer = (guint8*) malloc((size)*sizeof(guint8)); // Allocation memory for read file
  // TODO : Avoid allocate ahead (allocate only need)
  fread(buffer,size,1,fileptr); // Read file
  fclose(fileptr); // close file
  return buffer; 
}

guint8* mirrorRead(int diskGroup,guint64 addr,guint size){
  // Implement strip read
  if(diskGroup==0){ // in first {0,1}
    if(newDisk[0]==FALSE){  // if disk 0 is not new (sync)
      return diskRead(0,addr,size);
    }else{
      return diskRead(1,addr,size);
    }
  }

  if(diskGroup==1){ // in second {2,3}
    if(newDisk[2]==FALSE){ // if disk 2 is not new (sync)
      return diskRead(2,addr,size);
    }else{
      return diskRead(3,addr,size);
    }
  }
  
  return NULL;
}

guint8* diskReadData(guint64 addr,guint size){
  // Check addr (cross 2 disk?)
  if(addr+size<DISK_SIZE)// in first
    // TODO : check new disk and dont read from theme
    return mirrorRead(0,addr,size);

  else if(addr<DISK_SIZE){ // cross 2 disk
    guint64 addr0,addr1;
    guint size0,size1;
    guint8 *data0,*data1;

    //0
    size0=DISK_SIZE-addr;
    addr0=addr;
    data0=mirrorRead(0,addr0,size0);
    //1
    size1=size-size0;
    addr1=HEADER_SIZE; // 0-15 size,16 mode
    data1=mirrorRead(1,addr1,size1);

    // merge 2 data
    guint8* data=(guint8*) malloc(size*sizeof(guint8));

    memmove(data,data0,size0);
    memmove(data+size0,data1,size1);

    return data;


  }else if(diskMode==10){ // in second and diskMode=10
    addr=addr-DISK_SIZE+HEADER_SIZE;
    return mirrorRead(1,addr,size);
  }
  return NULL;
}


//-----------

//-----------
// Find first space with fit a specific size
gint64 FreeSpaceFind(guint realSize){
  // Calculate add Head too
  // atime,size,data
  guchar* buffer=FileReadN(diskFileName[0],ADDR_FREE_SPACE_VECTOR,FS_SIZE);

  // Search
  guint64 count=0;
  guint64 start=0;
  guint64 i;
  for(i=0;i<FS_SIZE;i++){
    /*
    if(count==0){
      start=i;
    }
    */
    if(buffer[i]==0){
      count++;
      if(count==realSize){
        break;
      }
    }else{
      count=0;
      start=i;
      printf("FS find reset @"G_GUINT64_FORMAT"\n"+ADDR_FREE_SPACE_VECTOR);
    }
  }
  // no free space enough
  if(i==FS_SIZE){
    printf("Error Not enough space (%"G_GUINT64_FORMAT")",ADDR_FREE_SPACE_VECTOR+FS_SIZE-1);
    return -1;
  }

  printf(">>>%"G_GUINT64_FORMAT"@%"G_GUINT64_FORMAT" (%d)\n",
      count,start,realSize);
  return start;
}

void FreeSpaceMark(guint realSize,guint64 freeOffset){
  printf("Allocate freespace %u block\n",realSize);
  printf("FSAddrStart: %"G_GUINT64_FORMAT"\n",ADDR_FREE_SPACE_VECTOR+freeOffset);
  printf("FSAddrStop : %"G_GUINT64_FORMAT"\n",ADDR_FREE_SPACE_VECTOR+freeOffset+realSize-1);
  guint8* mark=(guint8*)malloc(realSize*sizeof(guint8*));
  guint i;
  for(i=0;i<realSize;i++){
    mark[i]=0xFF;
  }
  diskWriteData(mark,ADDR_FREE_SPACE_VECTOR+freeOffset,realSize);
  free(mark);
}

void FreeSpaceUnmark(guint realSize,guint64 addrFile){
  printf("Free allocate space\n");
  // 0 = @21,...
  guint64 allocOffset=(addrFile-ADDR_DATA)/8;
  guint realBlockSize=realSize/8;
  printf("AllocOffset : %"G_GUINT64_FORMAT"\n",allocOffset);
  printf("BlockNumber : %d\n",realBlockSize);
  printf("FSAddrStart : %"G_GUINT64_FORMAT"\n",ADDR_FREE_SPACE_VECTOR+allocOffset);
  printf("FSAddrStop : %"G_GUINT64_FORMAT"\n",ADDR_FREE_SPACE_VECTOR+allocOffset+realBlockSize-1);

  guchar mark[realBlockSize];
  int i;
  for(i=0;i<realBlockSize;i++){
    mark[i]=0;
  }
  diskWriteData(&mark,ADDR_FREE_SPACE_VECTOR+allocOffset,realBlockSize);

}

// TODO : if fileCounter need to pass
void FMapAdd(const gchar *key,guint64 fptr){
  // if it has hole insert in hole first
  GList* hole;
  guint64 fmAddr=fileCounter;
  hole=g_list_first(fileMapHole);
  if(hole){// Have hole
    fmAddr=*((guint64*)hole->data);
    fileMapHole=g_list_remove(fileMapHole,hole);
  }
  // Insert to map=8B+4B
  // key
  diskWriteData((void*)key,ADDR_FILE_MAP+(fmAddr*12),KEY_SIZE);
  // fptr
  diskWriteData(&fptr,ADDR_FILE_MAP+(fmAddr*12)+KEY_SIZE,FPTR_SIZE);

  FMAP* fm = (FMAP*) malloc(sizeof(FMAP));
  fm->fileNo=fmAddr;
  fm->fptr=fptr;
  gchar* newKey;
  newKey = (gchar*) malloc((KEY_SIZE+1)*sizeof(gchar));
  strncpy(newKey,key,8);
  newKey[8]=0;
  
  printf("NewKey : %s\n",newKey);

  fileCounter++;
  g_tree_insert(fileMap, newKey, fm);

  //printf("Increase file counter\n");
  diskWriteData(&fileCounter,ADDR_FILE_COUNTER,FILE_SIZE);

}

void FMapRemove(const gchar *key,guint fileNo){
  printf("FMapRemove \n");
  // in mem
  g_tree_remove(fileMap,key);
  fileCounter--;
  diskWriteData(&fileCounter,ADDR_FILE_COUNTER,FILE_SIZE);

  // in disk
  guint64 fileMapAddr=ADDR_FILE_MAP+(fileNo)*12;
  guint8* blank[KEY_SIZE+FILE_SIZE];
  int i;
  for (i = 0; i < (KEY_SIZE+FILE_SIZE); ++i)
  {
    blank[i]=0;
  }
  diskWriteData(blank,fileMapAddr,KEY_SIZE+FILE_SIZE);
  // TODO : add hole to fileMapHole
  guint64* fmHole=(guint64*) malloc(sizeof(guint64));
  *fmHole=fileNo;
  fileMapHole=g_list_prepend(fileMapHole,fmHole);

}

//-----------

guint BytesArrayToGuint(guchar* buffer){
  guint number;
  number=(guchar)buffer[0];
  number+=(guchar)buffer[1]<<8;
  number+=(guchar)buffer[2]<<16;
  number+=(guchar)buffer[3]<<24;
  return number;
}

// TODO : FN-Increease fileCounter

guint getFileCounter(){
  guchar * buffer = FileReadN(diskFileName[0],ADDR_FILE_COUNTER,4);
  return BytesArrayToGuint(buffer);
}

// Initiate disk in a first time of use
void formatDisk(FILE * diskFile,guint64 diskSize,int mode){
  char header[HEADER_SIZE];
  sprintf(header,"%"G_GUINT64_FORMAT "",diskSize);
  int strSize=strlen(header);
  memmove(header+(HEADER_SIZE-1-strSize),header,strSize);
  memset(header,'0',HEADER_SIZE-1-strSize);
  // change byte 17 to RAID mode 1, 0
  header[16]=mode;
  //printf("----%s----\n",header);
  fseek ( diskFile, 0, SEEK_SET );
  fwrite(header,1,HEADER_SIZE,diskFile);
}


guint64 getDiskSize(FILE* disk,char * fileName){
  guint64 filelen;
  fseek(disk,0,SEEK_END); // Seek file to the end
  filelen = ftell(disk); // Get number of current byte offset
  return filelen;
}

int checkFirstSection(FILE* diskFile, guint64 diskSize){
  guint64 filelen;
  char buffer[16]; // 16*4 = 64
  fseek ( diskFile, 0, SEEK_SET );
  fread(buffer,1,16,diskFile); // Read entire file // TODO what all this param?
  filelen = atoi(buffer);
  
  if(filelen==diskSize){
    fread(buffer,1,1,diskFile);
    return buffer[0];
  }
  return -1; // -1 - Didnt format yet
}

//--------UtilFN------

//--------UtilFN------

int mirrorNo=0;
int stripNo=2;

// 2 disk
int diskOrdering1(int* mode, char* _diskFileName){
  *mode=1;
  diskFileName[mirrorNo]=_diskFileName;
  return mirrorNo++;
}

int diskOrdering10(int* mode, char* _diskFileName){
  if((*mode)==1&&mirrorNo<=1){ // Mirror
    diskFileName[mirrorNo]=_diskFileName;
    return mirrorNo++; //return 0,1

  }else if((*mode)==0&&stripNo<=3){ // strip
    diskFileName[stripNo]=_diskFileName;
    return stripNo++; //return 2,3

  }else if((*mode)==-1){
  if(mirrorNo<=1){
        *mode=1;
        diskFileName[mirrorNo]=_diskFileName;
        return mirrorNo++;
      }else{
        *mode=0;
        diskFileName[stripNo]=_diskFileName;
        return stripNo++;
      }
  }else{ // if have disk in mirror/strip mode more than 2 disk
    // eg 1,1,1,0
    printf("Disk RAID problem\n");
    exit(1);
  }
}

void diskOrdering(FILE * diskFile[],guint64 diskSize,char* diskName[]){
  int i;
  for (i = 0; i < diskCount; ++i)
  {
    int mode,modeInFile,orderNum;
    modeInFile=checkFirstSection(diskFile[i],diskSize);
    printf("modeInFile %d\n", modeInFile);
    // if modeInFile== {2,3} mode 0/1 when data not sync(newdisk)
    if(modeInFile>1)
      modeInFile=-1; //{2,3} => -1
    // for now do nothing

    mode=modeInFile;
    printf("mode : %d\n",mode);

    if(diskMode==1)
      orderNum=diskOrdering1(&mode,diskName[i]);
    else if(diskMode==10)
      orderNum=diskOrdering10(&mode,diskName[i]);

    printf("DiskMode : %d\n",mode);
    printf("orderNo. : %d\n",orderNum);
    newDisk[orderNum]=FALSE;
    if (modeInFile<0){
      printf("Initiate new disk\n");
      // 3 for mode 1 newDisk,2 for mode 0 newDisk (not sync)
      // mode = {0,1} ==> mode+2 = {2,3}
      newDisk[orderNum]=TRUE;
      if(orderNum==1||orderNum==3){ //0,1 2,3
        if(newDisk[orderNum-1]==newDisk[orderNum]){ //0,1 2,3
          newDisk[orderNum-1]=FALSE;
          newDisk[orderNum]=FALSE;
        }else{
          printf("-------------OldNewFound\n");
          mode+=2;   // mode = {2,3}
        }
      }
      printf("NewDisk D%d mode %d (%d)\n", i,mode%2,mode);
      // TODO formatDisk
      formatDisk(diskFile[i],diskSize,mode);
    }
    fclose(diskFile[i]);
  }
}

void calculateDiskAddr(){
  //if(DISK_SIZE!=1073741824){ // 1G=1073741824
  { // 1G=1073741824
    // mode 1
    // mode 0
    printf("--------------------------\n");

    guint64 addr=0;
    guint64 maxDiskSize=DISK_SIZE;
    if(diskMode==10)
      maxDiskSize = DISK_SIZE*2;
    printf("maxDiskSize %"G_GUINT64_FORMAT"\n",maxDiskSize);
    guint64 maxHeaderBytes= maxDiskSize*0.2;// 20%
    printf("maxHeaderBytes %"G_GUINT64_FORMAT"\n",maxHeaderBytes);
    guint64 maxDataBytes= maxDiskSize-maxHeaderBytes;// 20%
    printf("maxDataBytes %"G_GUINT64_FORMAT"\n",maxDataBytes);

    // Header+Mode 17 Bytes
    addr+=HEADER_SIZE; // 17+

    // FileCounter 4 Bytes
    addr+=4;

    // Free Space management
    // addr = 0+17+4 = 21;
    printf("FreeSpaceMNG ADDR %"G_GUINT64_FORMAT"\n",ADDR_FREE_SPACE_VECTOR);
    FS_SIZE= maxDataBytes/8; //100000000; // TODO
    printf("FreeSpaceMNG Size %"G_GUINT64_FORMAT"\n",FS_SIZE);
    addr+=FS_SIZE;
    printf("FreeSpaceMNG End %"G_GUINT64_FORMAT"\n",addr);

    // 1 FileMap = 8B+8B
    ADDR_FILE_MAP = addr; //100000021; // 16B*N
    printf("FileMap ADDR %"G_GUINT64_FORMAT"\n",ADDR_FILE_MAP);

    MAX_FILE = (maxHeaderBytes-ADDR_FILE_MAP)/(KEY_SIZE+FPTR_SIZE) -1;
    printf("FileMap Max file %"G_GUINT64_FORMAT"\n",MAX_FILE);
    printf("FileMap End %"G_GUINT64_FORMAT"\n",MAX_FILE);

    ADDR_DATA=addr+MAX_FILE; // 4B+4B+X
    printf("Data ADDR %"G_GUINT64_FORMAT"\n",ADDR_DATA);
    printf("Data End %"G_GUINT64_FORMAT"\n",ADDR_DATA+maxDataBytes);

    printf("--------------------------\n");
  }
}

void checkDisk(char* diskArg[]){
  int i;
  guint64 diskSize[diskCount];
  FILE * diskFile[diskCount];

  // Get every disk size
  for(i=0;i<diskCount;++i){
    diskFileNameTmp[i]=diskArg[i+1];
    //printf("D%d : %s\n",i,diskFileNameTmp[i]);
    
    // [x] Detect is disk in system, format disk
    // [X] TODO : how to deal when disk order in wrong , Add disk order number to disk header
    // [] TODO : Detect new disk
    // [] TODO (BONUS) : Redistribute file
    // [] TODO : Detect missing,corrupt disk
    diskFile[i] = fopen(diskFileNameTmp[i],"rb+");  // Open file in binary   
    // Check
    
    diskSize[i] = getDiskSize(diskFile[i],diskFileNameTmp[i]);
    printf("D%d Size : %" G_GUINT64_FORMAT "\n",i,diskSize[i]);

    if(i>0){
      if(diskSize[i-1]!=diskSize[i]){
        printf("Error, Disk didnt have a same size.\n");
        exit(1);
      }
      // TODO : check is use same disk more than one(not necessary?)
      //printf("Error, Use same disk more than one.\n");      
    }
  }
  DISK_SIZE=diskSize[0];

  diskOrdering(diskFile,diskSize[0],diskFileNameTmp);

  // Calculate ADDR
  calculateDiskAddr();


}

gint getFileSize(FILE *ptr){
  guint filelen;
  fseek(ptr,0,SEEK_END); // Seek file to the end
  filelen = ftell(ptr); // Get number of current byte offset
  return filelen;
}

gboolean iter_all(gpointer key, gpointer value, gpointer data) {
  //printf("%d : %s, %d\n",key, key, *(guint*)value);
  printf("%d: %s, %"G_GUINT64_FORMAT"\n", ((FMAP*)value)->fileNo, (char*)key, ((FMAP*)value)->fptr);
  return FALSE;
}

void listPrint(gpointer data,gpointer user_data){
  printf("List : %"G_GUINT64_FORMAT"\n",*(guint64*)data);
}

void FMapLoad(){
  FILE* disk0;
  disk0 = fopen(diskFileName[0],"rb");
  fseek(disk0,ADDR_FILE_MAP,SEEK_SET);

  fileMap= g_tree_new((GCompareFunc)g_ascii_strcasecmp);
  fileMapHole=NULL;

  FMAP* fm;
  gchar* key;
  guint64* fmHole;

  guint i;
  printf("\n---%d\n",fileCounter);
  /*
  * TODO : Detect hole
  * when remove file it will be hole and loop below VV
  * will work incorrectly
  */
  guint fileCounterRange=fileCounter;
  for (i = 0; i < fileCounterRange; ++i)
  {
    printf("FM load - %d/%d\n",i,fileCounterRange);
    //printf("File %d:",i);
    fm  = (FMAP*) malloc(sizeof(FMAP));
    key = (gchar*) malloc((KEY_SIZE+1)*sizeof(gchar));
    //fptr = (guint*) malloc(sizeof(guint));
    fread(key,KEY_SIZE,1,disk0);
    key[8]=0;
    //printf("%s\t",key);
    fread(&(fm->fptr),FILE_SIZE,1,disk0);
    //printf("%d\t",fm->fptr);
    fm->fileNo=i;

    // if it's hole 
    if(key[0]==0){
      // Add to freeFileMapHole
      fmHole=(guint64*) malloc(sizeof(guint64));
      *fmHole=i;
      fileMapHole=g_list_prepend(fileMapHole,fmHole);
      printf("FM hole found\n");
      // increase max read range
      fileCounterRange++;
      continue;
    }
    g_tree_insert(fileMap, key, fm);
    /* code */
    //printf("\n");
  }
  fclose(disk0);
  printf("\n Found %u hole\n",fileCounterRange-fileCounter);
  fileMapHole=g_list_reverse(fileMapHole);
  g_list_foreach(fileMapHole,listPrint,NULL); // func(pointer data,pointer userdata)

  //g_tree_foreach(fileMap, (GTraverseFunc)iter_all, NULL);
  printf("Load file map finish\n");
}

// user_data : pass "prefix key"
gint finder(gpointer key, gpointer user_data) {
  // TODO : if prefix match -> remember
  //printf("F %d %s\n",key,(char*)key);
  return -g_ascii_strcasecmp(key,(char*)user_data);
}

void getFileMeta(guint addr,guint* size,guint *atime){
  guint8* buffer;
  if(atime!=NULL){
    buffer=FileReadN(diskFileName[0],addr,ATIME_SIZE);
    *atime=BytesArrayToGuint(buffer);
  }
  if(size!=NULL){
    buffer=FileReadN(diskFileName[0],addr+ATIME_SIZE,FILE_SIZE);
    *size=BytesArrayToGuint(buffer);
  }
}

/*
* return 0 - Key not existing
* return value - Address of existing file meta = atime+size+data
*/
FMAP* checkExistingKey(const gchar* key){
  gpointer value = g_tree_search(fileMap, (GCompareFunc)finder, key);
  return (FMAP*)value;
}

void putFileDataToDisk(guint fileSize,const char* src,guint64 addrFile){
  // Insert file data=atime,size,data
  printf("Insert Data\n");
  guint timeUnix=(guint)g_date_time_to_unix(g_date_time_new_now_local());
  //printf("UNIX %"G_GUINT32_FORMAT"\n",timeUnix);
  diskWriteData(&timeUnix,addrFile,ATIME_SIZE);
  // size
  diskWriteData(&fileSize,addrFile+ATIME_SIZE,FILE_SIZE);
  // data
  guchar* data;
  if(fileSize>(guint)268435456){ // 256MB = 268435456

    guint64 conAddr=addrFile+ATIME_SIZE+FILE_SIZE;
    guint sizePerRound=268435456;

    int i,iter=fileSize/(268435456);
    for (i = 0; i < iter; ++i)
    {
      data=FileReadN((char*)src,0,sizePerRound);
      diskWriteData((void*)data,conAddr,sizePerRound);
      free(data);
      conAddr+=sizePerRound;
    }
  }else{
    data=FileReadN((char*)src,0,fileSize);
    diskWriteData((void*)data,addrFile+ATIME_SIZE+FILE_SIZE,fileSize);
    free(data);
  }
}

guint replaceKeyWithNewDataSize(const gchar *key,guint realBlockSize,
    guint fileSize,const char* src,FMAP* fileMeta){
  guint64 addrFile;
  guint freeOffset;

  // Find freeSpace
  printf("Find freespace %d\n",realBlockSize);
  freeOffset=FreeSpaceFind(realBlockSize);
  if(freeOffset==-1)
    return ENOSPC;
  addrFile=ADDR_DATA+freeOffset*8;

  // Insert file data=atime,size,data
  putFileDataToDisk(fileSize,src,addrFile);

  // mark
  FreeSpaceMark(realBlockSize,freeOffset);

  // Change file map - fptr
  fileMeta->fptr=addrFile;
  g_tree_replace (fileMap,(gchar*)key,fileMeta);
  diskWriteData(&addrFile,(guint64)ADDR_FILE_MAP+(fileMeta->fileNo*12)
      +KEY_SIZE,4);

  return 0;
}

guint myPut(gchar *key,gchar *src){
  guint8 replaceMode=0;
  // check key size ==8
  printf("File Map add\n");
  if(strlen(key)!=8){
    return ENAMETOOLONG;
  }
  if(fileCounter==MAX_FILE){ // eg.1,000,000 (for 1GB)
    printf("Max file number allow (%"G_GUINT64_FORMAT")\n", MAX_FILE);
    return ENOSPC;
  }

  // Get file size
  FILE *filePTR;
  filePTR = fopen(src,"rb");  // Open file in binary
  guint fileSize=getFileSize(filePTR);
  fclose(filePTR);
  printf(">> %d\n",fileSize);
  guint realBlockSize=(ATIME_SIZE+FILE_SIZE+fileSize)/8;

  // check duplicate key
  guint64 addrFile=0;
  FMAP* existingFileMeta=checkExistingKey(key);
  if(existingFileMeta){
    printf("Duplicate Key\n");
    guint64 existingFileAddr=existingFileMeta->fptr;
    // read file meta
    guint existSize;
    getFileMeta(existingFileAddr,&existSize,NULL);
    printf("Existing addr : %"G_GUINT64_FORMAT"\n", existingFileAddr);
    printf("Existing size : %d\n", existSize); 
    printf("New size : %d\n", fileSize);//realBlockSize
    // if same size => replace data in same addr
    if(fileSize==existSize){
      replaceMode=1;
      addrFile=existingFileAddr;
    }else{
      // unmark old addr
      guint existRealSize=(ATIME_SIZE+FILE_SIZE+existSize);
      FreeSpaceUnmark(existRealSize,existingFileAddr);
      return replaceKeyWithNewDataSize(key,realBlockSize,fileSize,src,existingFileMeta);
    }   
  }

  // Find freeSpace
  guint64 freeOffset;
  if(replaceMode==0){
    printf("Find freespace %d\n",realBlockSize);
    freeOffset=FreeSpaceFind(realBlockSize);
    if(freeOffset==-1)
      return ENOSPC;
        // TODO return ENOSPC if no space enough
    addrFile=ADDR_DATA+freeOffset*8;
  }else{
    printf("Replace new file with same size\n");
  }

  // Insert file data=atime,size,data
  putFileDataToDisk(fileSize,src,addrFile);

  if(replaceMode==0){ // if replace dont add new file map
    // mark
    FreeSpaceMark(realBlockSize,freeOffset);
    // Insert map
    FMapAdd(key,addrFile);
  }
  return 0;
}

guint myStat(gchar* key,guint* size,guint *atime){
  //printf("\nStat Searching\n");
  gpointer value = g_tree_search(fileMap, (GCompareFunc)finder, key);
  //printf("\nData :%d\n",value);
  //printf("\nData :%d\n",*(guint*)value);
  if(value==0){
    // File not found
    return ENOENT;
  }

  getFileMeta(((FMAP*)value)->fptr,size,atime);

  return 0;
}

// TODO - not work properly
guint myRemove(gchar* key){
  // TODO - file is using => EAGAIN

  // Check key length
  if(strlen(key)!=8)
    return ENAMETOOLONG;

  FMAP* fileMeta=checkExistingKey(key);
  if(fileMeta==0)
    return ENOENT;
  else{
    guint fileSize;
    guint64 fileAddr=fileMeta->fptr;
    getFileMeta(fileAddr,&fileSize,NULL);
    printf("Size: %d\n",fileSize);
    guint realSize=fileSize+ATIME_SIZE+FILE_SIZE;

    // Remove file map
    FMapRemove(key,fileMeta->fileNo);
    
    // Unmark allocate space
    FreeSpaceUnmark(realSize,fileAddr);
  }
  return 0;
}

guint myGet(gchar *key,gchar *outpath){
  // Check key size
  if(strlen(key)!=8){
    return ENAMETOOLONG;
  }
  
  FMAP* existingFile=checkExistingKey(key);
  if(existingFile){
    guint64 fileAddr=existingFile->fptr;
    
    // Update Atime when this fn is call
    guint atime=(guint)g_date_time_to_unix(g_date_time_new_now_local());
    diskWriteData(&atime,fileAddr,ATIME_SIZE);

    guint fileSize;
    getFileMeta(fileAddr,&fileSize,&atime);

    // Write data out
    FILE* fileOut;
    fileOut = fopen(outpath,"wb");

    // if outpath is busy return EAGAIN
    // if fopen error return EAGAIN
    if(fileOut==NULL)
      return EAGAIN;

    // writing
    guint8* buffer;
    guint writedSize=0;

    if(fileSize>(guint)(268435456)){ // 256MB = 268435456

      guint64 conAddr=fileAddr+ATIME_SIZE+FILE_SIZE;
      guint sizePerRound=268435456;
      guint readSize=0;

      int i,iter=fileSize/(268435456);
      for (i = 0; i < iter; ++i)
      {
        if(fileSize-readSize<sizePerRound){
          sizePerRound=fileSize-readSize;
        }
        buffer=diskReadData(conAddr,sizePerRound);
        writedSize+=fwrite(buffer,1/*byte*/,sizePerRound,fileOut);
        free(buffer);
        conAddr+=sizePerRound;
        readSize+=sizePerRound;
      }
    }else{
      buffer=diskReadData(fileAddr+ATIME_SIZE+FILE_SIZE,fileSize);
      if(fwrite(buffer,1/*byte*/,fileSize,fileOut)!=fileSize){
        free(buffer);
        return ENOSPC;
      }
      free(buffer);
    }

    
    
    fclose(fileOut);
    // TODO : FN-DiskClose

    // TODO - return ENOSPC if "real" disk free space not enough
    // (work?) if fwrite return!=count return ENOSPC

  }else{
    // File (key) not found
    return ENOENT;
  }
  return 0;
}

gboolean iterAllPrefixMatch(gpointer key, gpointer value, gpointer data) {
  if(g_str_has_prefix ((const gchar *)key,((SearchData*)data)->key)){
    //printf("prefix match %s\n", (char*)key);
    ((SearchData*)data)->status=TRUE;
    ((SearchData*)data)->result=g_list_prepend(((SearchData*)data)->result,(char*)key);
    ((SearchData*)data)->size++;
    return FALSE;
  }
  if(((SearchData*)data)->status==FALSE)
    return FALSE;
  else
    return TRUE;
}

void listToStr(gpointer data,gpointer str){
  //printf("ms : %s \n",(char*)data);
  g_string_append(str,(char*)data);
  g_string_append(str,",");
}

guint mySearch(gchar* key,gchar* outpath){
  if(strlen(key)>8)
    return ENAMETOOLONG;

  /*
  * 1 - key
  * 2 - result GList
  */
  SearchData sData;
  sData.key=key;
  sData.status=FALSE;
  sData.result=NULL;
  sData.size=0;

  // Search
  g_tree_foreach(fileMap, (GTraverseFunc)iterAllPrefixMatch, &sData);
  sData.result=g_list_reverse(sData.result);

  // not found
  if(sData.size==0)
    return ENOENT;

  GString *resCSV=g_string_new("");
  g_list_foreach(sData.result,listToStr,resCSV);
  //resCSV->str[resCSV->len-1]=0; // change last comman
  g_string_truncate(resCSV,resCSV->len-1); // change last comman
  //printf("Result : %s\n",resCSV->str);
  

  // Write file in CSV format
  FILE* fileOut;
  fileOut=fopen(outpath,"wb");
  // if outpath is busy (fopen fail) return EAGAIN
  if(fileOut==NULL)
    return EAGAIN;
  if(fwrite(resCSV->str,1,resCSV->len,fileOut)!=resCSV->len)
    return ENOSPC;
  // if real disk not enough space return ENOSPC
  fclose(fileOut);
  return 0;

}

#endif
