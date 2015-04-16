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
//extern guint64 diskSize;
extern int diskMode;
extern guint fileCounter;
extern GTree* fileMap;
extern GList* fileMapHole;



const guint ADDR_FILE_COUNTER=17; // 4B
const guint ADDR_FREE_SPACE_VECTOR=21; // 1/8 B
const guint KEY_SIZE=8;
const guint ATIME_SIZE=4;
const guint SIZE_SIZE=4;
// TODO : Need function to calculate this.
guint ADDR_FILE_MAP=121000000; // 12B*N
// Name = 8B
// fPTR = 4B

// TODO : function to set this value
guint ADDR_DATA=200000000; // 16B+X
// Atime = 4B
// Size = 4B

guint64 halfOffset;

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
  guint fptr;
};
typedef struct FMAP FMAP;

guint8* readFileN(char * fileName,guint offset,guint size){
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
char * readFile(char * fileName){
  FILE * fileptr;
  char * buffer;
  guint64 filelen;

  fileptr = fopen(fileName,"rb");  // Open file in binary
  fseek(fileptr,0,SEEK_END); // Seek file to the end
  filelen = ftell(fileptr); // Get number of current byte offset
  rewind(fileptr); // Junp to begin

  printf("filelen : %" G_GUINT64_FORMAT "\n",filelen);

  buffer = (char *) malloc((filelen+1)*sizeof(char)); // Allocation memory for read file
  // TODO : Avoid allocate ahead (allocate only need)
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
  //fwrite(data,size,1,writeptr);
  fwrite(data,1,size,writeptr);
  //fprintf(writeptr,"%s",data); // TODO didnt test
  fclose(writeptr);

}

void writeFile(char * filename,char * fileData){
  FILE * writeptr;
  writeptr = fopen(filename,"wb");
  fwrite(fileData,strlen(fileData),1,writeptr);
  fclose(writeptr);
}
//-----------
void diskWriteData(guint64 addr,void* data,guint size){
  // TODO 4 disk mng
  editFile(diskFileName[0],data,addr,size);
}
//-----------

//-----------
// Find first space with fit a specific size
gint64 FreeSpaceFind(guint realSize){
  // Calculate add Head too
  // atime,size,data
  guint64 fsSize=100000000; // TODO
  guchar* buffer=readFileN(diskFileName[0],ADDR_FREE_SPACE_VECTOR,fsSize);

  // Search
  guint64 count=0;
  guint64 start;
  guint64 i;
  for(i=0;i<fsSize;i++){
    if(count==0){
      start=i;
    }
    if(count==realSize){
      break;
    }
    if(buffer[i]==0){
      count++;
    }else{
      count=0;
    }
    // no free space enough
    if(i==fsSize-1){
      printf("Error Not enough space");
      return -1;
    }
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
  diskWriteData(ADDR_FREE_SPACE_VECTOR+freeOffset,mark,realSize);
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
  diskWriteData(ADDR_FREE_SPACE_VECTOR+allocOffset,&mark,realBlockSize);

}

// TODO : if fileCounter need to pass
void FMapAdd(const gchar *key,guint fptr){
  // TODO : if it has hole insert in hole first
  GList* hole;
  guint64 fmAddr=fileCounter;
  hole=g_list_first(fileMapHole);
  if(hole){// Have hole
    fmAddr=*((guint64*)hole->data);
  }
  // Insert to map=8B+4B
  // key
  editFile(diskFileName[0],(void*)key,(guint64)ADDR_FILE_MAP+(fmAddr*12),8);
  // fptr
  editFile(diskFileName[0],&fptr,(guint64)ADDR_FILE_MAP+(fmAddr*12)
      +KEY_SIZE,4);

  FMAP* fm = (FMAP*) malloc(sizeof(FMAP));
  fm->fileNo=fmAddr;
  fm->fptr=fptr;
  gchar* newKey;
  newKey = (gchar*) malloc((KEY_SIZE+1)*sizeof(gchar));
  strncpy(newKey,key,8);
  newKey[8]=0;
  
  printf("NK : %s\n",newKey);

  fileCounter++;
  g_tree_insert(fileMap, newKey, fm);

  //printf("Increase file counter\n");
  diskWriteData(ADDR_FILE_COUNTER,&fileCounter,SIZE_SIZE);

}

void FMapRemove(const gchar *key,guint fileNo){
  printf("FMapRemove \n");
  // in mem
  g_tree_remove(fileMap,key);
  fileCounter--;
  editFile(diskFileName[0],&fileCounter,ADDR_FILE_COUNTER,SIZE_SIZE);

  // in disk
  // TODO ?? - Compact file map
  guint64 fileMapAddr=ADDR_FILE_MAP+(fileNo)*12;
  guint8* blank[KEY_SIZE+SIZE_SIZE];
  int i;
  for (i = 0; i < (KEY_SIZE+SIZE_SIZE); ++i)
  {
    blank[i]=0;
  }
  editFile(diskFileName[0],blank,fileMapAddr,KEY_SIZE+SIZE_SIZE);

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
  guchar * buffer = readFileN(diskFileName[0],ADDR_FILE_COUNTER,4);
  return BytesArrayToGuint(buffer);
}

// TODO : Initiate disk in a first time of use
void formatDisk(FILE * diskFile,guint64 diskSize,int* mode){
  char header[17];
  sprintf(header,"%"G_GUINT64_FORMAT "",diskSize);
  int strSize=strlen(header);
  memmove(header+(16-strSize),header,strSize);
  memset(header,'0',16-strSize);
  // TODO change byte 17 to RAID mode 1, 0
  header[16]=(*mode);
  printf("----%s----\n",header);
  // TODO : for test purpose
  //editFile(diskName,header,0,17);
  fseek ( diskFile, 0, SEEK_SET );
  fwrite(header,1,17,diskFile);
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


int mirrorNo=0;
int stripNo=2;

// 2 disk
int diskOrdering1(int* mode, char* _diskFileName){
  *mode=1;
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
        return mirrorNo++;
      }else{
        *mode=0;
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
      newDisk[orderNum]=TRUE;
      formatDisk(diskFile[i],diskSize,&mode);
    }
    fclose(diskFile[i]);
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

  diskOrdering(diskFile,diskSize[0],diskFileNameTmp);
}

gint getFileSize(FILE *ptr){
  guint filelen;
  fseek(ptr,0,SEEK_END); // Seek file to the end
  filelen = ftell(ptr); // Get number of current byte offset
  return filelen;
}

gboolean iter_all(gpointer key, gpointer value, gpointer data) {
  //printf("%d : %s, %d\n",key, key, *(guint*)value);
  printf("%d: %s, %d\n", ((FMAP*)value)->fileNo, (char*)key, ((FMAP*)value)->fptr);
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
    fread(&(fm->fptr),SIZE_SIZE,1,disk0);
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
    buffer=readFileN(diskFileName[0],addr,ATIME_SIZE);
    *atime=BytesArrayToGuint(buffer);
  }
  if(size!=NULL){
    buffer=readFileN(diskFileName[0],addr+ATIME_SIZE,SIZE_SIZE);
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
  diskWriteData(addrFile,&timeUnix,ATIME_SIZE);
  // size
  diskWriteData(addrFile+ATIME_SIZE,&fileSize,SIZE_SIZE);
  // data
  guchar* data=readFileN((char*)src,0,fileSize);
  diskWriteData(addrFile+ATIME_SIZE+SIZE_SIZE,(void*)data,fileSize);
  free(data);
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
  editFile(diskFileName[0],&addrFile,(guint64)ADDR_FILE_MAP+(fileMeta->fileNo*12)
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

  // Get file size
  FILE *filePTR;
  filePTR = fopen(src,"rb");  // Open file in binary
  guint fileSize=getFileSize(filePTR);
  fclose(filePTR);
  printf(">> %d\n",fileSize);
  guint realBlockSize=(ATIME_SIZE+SIZE_SIZE+fileSize)/8;

  // check duplicate key
  guint addrFile=0;
  FMAP* existingFileMeta=checkExistingKey(key);
  if(existingFileMeta){
    printf("Duplicate Key\n");
    guint existingFileAddr=existingFileMeta->fptr;
    // read file meta
    guint existSize;
    getFileMeta(existingFileAddr,&existSize,NULL);
    printf("Existing addr : %d\n", existingFileAddr);
    printf("Existing size : %d\n", existSize); 
    printf("New size : %d\n", fileSize);//realBlockSize
    // if same size => replace data in same addr
    if(fileSize==existSize){
      replaceMode=1;
      addrFile=existingFileAddr;
    }else{
      // unmark old addr
      guint existRealSize=(ATIME_SIZE+SIZE_SIZE+existSize);
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

  // mark
  FreeSpaceMark(realBlockSize,freeOffset);

  // Insert map
  if(replaceMode==0) // if replace dont add new file map
    FMapAdd(key,addrFile);
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
    guint realSize=fileSize+ATIME_SIZE+SIZE_SIZE;

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
    diskWriteData(fileAddr,&atime,ATIME_SIZE);

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
    // TODO : FN-DiskPrepare
    // TODO : FN-DiskReadN
    guint8* buffer=readFileN(diskFileName[0],fileAddr+ATIME_SIZE+SIZE_SIZE,fileSize);
    // TODO : what fwrite do if data size is shorter than count(param3)
    
    if(fwrite(buffer,1/*byte*/,fileSize,fileOut)!=fileSize)
      return ENOSPC;
    
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
  resCSV->str[resCSV->len-1]=0xA; // change last comman
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
