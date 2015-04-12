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

const guint addrFileCounter=17; // 4B
const guint addrFreeSpaceVector=21; // 1/8 B
guint addrFileMap; // 12B*N
// Name = 8B
// fPTR = 4B
guint addrData; // 16B+X
// Name = 8B
// Atime = 4B
// Size = 4B

guint BytesArrayToGuint(char* buffer){
  guint number;
  number=(guchar)buffer[0];
  number+=(guchar)buffer[1]<<8;
  number+=(guchar)buffer[2]<<16;
  number+=(guchar)buffer[3]<<24;
  return number;
}

guint getFileCounter(){
  char * buffer = readFileN(diskFileName[0],addrFileCounter,4);
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
  guint64 diskSize;
  for(i=0;i<diskCount-1;i++){
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
  printf(">> %d\n",fileSize);
  // Find freeSpace
  FreeSpaceFind(fileSize);
  // Insert to map
  // Insert data to disk = allocate + write
  return 0;
}


#endif
