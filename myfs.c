#ifndef MYFS_C
#define MYFS_C

#include <stdio.h>
#include <glib.h>
#include "myfile.c"
#include "mydisk.c"
#include "freespace.c"

extern char * diskFileName[4];
extern FILE * diskFile[4];
extern int diskCount;
extern guint64 diskSize;
extern int diskMode;

const guint ADDR_FILE_COUNTER=17; // 4B
const guint ADDR_FREE_SPACE_VECTOR=21; // 1/8 B
const guint ATIME_SIZE=4;
const guint SIZE_SIZE=4;
guint ADDR_FILE_MAP=121000000; // 12B*N
// Name = 8B
// fPTR = 4B

// TODO : function to set this value
guint ADDR_DATA=200000000; // 16B+X
// Atime = 4B
// Size = 4B

guint BytesArrayToGuint(guchar* buffer){
  guint number;
  number=(guchar)buffer[0];
  number+=(guchar)buffer[1]<<8;
  number+=(guchar)buffer[2]<<16;
  number+=(guchar)buffer[3]<<24;
  return number;
}

guint getFileCounter(){
  guchar * buffer = readFileN(diskFileName[0],ADDR_FILE_COUNTER,4);
  return BytesArrayToGuint(buffer);
}

// TODO : Initiate disk in a first time of use
void formatDisk(int diskNo,guint64 diskSize){
  char numberOfDiskSize[17];
  sprintf(numberOfDiskSize,"%"G_GUINT64_FORMAT "",diskSize);
  int strSize=strlen(numberOfDiskSize);
  memmove(numberOfDiskSize+(16-strSize),numberOfDiskSize,strSize);
  memset(numberOfDiskSize,'0',16-strSize);
  numberOfDiskSize[16]='0'+diskNo;
  printf("----%s----",numberOfDiskSize);
  editFile(diskFileName[diskNo],numberOfDiskSize,0,17);
}


int getDiskSize(int i,char * fileName){
  guint64 filelen;
  diskFile[i] = fopen(fileName,"rb+");  // Open file in binary
  fseek(diskFile[i],0,SEEK_END); // Seek file to the end
  filelen = ftell(diskFile[i]); // Get number of current byte offset
  return filelen;
}

int checkFirstSection(int i, guint64 diskSize){
  guint64 filelen;
  char buffer[16]; // 16*4 = 64
  fseek ( diskFile[i], 0, SEEK_SET );
  fread(buffer,16,1,diskFile[i]); // Read entire file // TODO what all this param?
  filelen = atoi(buffer);
  printf("check File: %" G_GUINT64_FORMAT "\n",filelen);
  fclose(diskFile[i]);
  return filelen==diskSize;
}

void checkDisk(char* diskArg[]){
  int i=0;
  for(i=0;i<diskCount;i++){
    // TODO : Detect is disk in system, format disk
    // TODO : how to deal when disk order in wrong , Add disk order number to disk header
    // TODO : Detect missing,corrupt disk
    // TODO : Detect new disk
    // TODO (BONUS) : Redistribute file
    diskFileName[i]=diskArg[i+1];
    printf("D%d : %s\n",i,diskFileName[i]);
    // Check
    diskSize = getDiskSize(i,diskFileName[i]);
    printf("DiskSize : %" G_GUINT64_FORMAT "\n",diskSize);
    if (checkFirstSection(i,diskSize)){
      printf("\tOK!!\n");
    }else{
      printf("Initiate disk\n");
      formatDisk(i,diskSize);
    }
  }
}

gint getFileSize(FILE *ptr){
  guint filelen;
  fseek(ptr,0,SEEK_END); // Seek file to the end
  filelen = ftell(ptr); // Get number of current byte offset
  return filelen;
}

guint myPut(const gchar *key,const gchar *src){
  FILE *filePTR;
  filePTR = fopen(src,"rb");  // Open file in binary
  guint fileSize=getFileSize(filePTR);
  fclose(filePTR);
  printf(">> %d\n",fileSize);

  guint fileCounter=getFileCounter();

  guint realSize=(4+4+fileSize)/8;
  // Find freeSpace
  guint64 freeOffset=FreeSpaceFind(realSize);
  guint64 addrFile=ADDR_DATA+freeOffset*8;
  // TODO return ENOSPC if no space enough

  // Insert file data=atime,size,data
  guint timeUnix=(guint)g_date_time_to_unix(g_date_time_new_now_local());
  //printf("UNIX %"G_GUINT32_FORMAT"\n",timeUnix);
  diskWriteData(addrFile,&timeUnix,ATIME_SIZE);
  // size
  diskWriteData(addrFile+ATIME_SIZE,&fileSize,SIZE_SIZE);
  // data
  guchar* data=readFileN((char*)src,0,fileSize);
  diskWriteData(addrFile+ATIME_SIZE+SIZE_SIZE,(void*)data,fileSize);

  // Inser map
  // check key size ==8
  // TODO : check duplicate
  if(strlen(key)==8){
    FMapAdd(fileCounter,key,addrFile);
  }else{
    return ENAMETOOLONG;
  }
  // Increse file counter
  fileCounter+=1;
  diskWriteData(ADDR_FILE_COUNTER,&fileCounter,SIZE_SIZE);

  // mark
  guchar mark=0xFF;
  int i;
  for(i=0;i<realSize;i++){
    diskWriteData(ADDR_FREE_SPACE_VECTOR+freeOffset+i,&mark,1);
  }
  return 0;
}


#endif
