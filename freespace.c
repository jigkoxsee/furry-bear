#ifndef FREESPACE_C
#define FREESPACE_C

#include <stdio.h>
#include <glib.h>
#include "mydisk.c"
#include "myfs.c"

extern char * diskFileName[4];
extern FILE * diskFile[4];
extern int diskCount;
extern guint64 diskSize;
extern int diskMode;
extern const guint ADDR_FREE_SPACE_VECTOR; // 1/8 B
extern const guint ADDR_FILE_COUNTER; // 4B
extern guint ADDR_FILE_MAP; // 12B*N
extern guint ADDR_DATA; // 16B+X

const guint FNAME_SIZE=8;


// Find first space with fit a specific size
gint64 FreeSpaceFind(guint64 realSize){
  // Calculate add Head too
  // atime,size,data
  guint64 fsSize=1000000; // TODO
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
  }
  printf(">>>%"G_GUINT64_FORMAT"@%"G_GUINT64_FORMAT" (%"G_GUINT64_FORMAT")\n",
      count,start,realSize);
  return start;
}

void FreeSpaceMark(){

}

void FreeSpaceUnmark(){

}

void FMapAdd(guint64 fileCounter,const gchar *key,guint64 fptr){
  // Insert to map=8B+4B
  // key
  editFile(diskFileName[0],(void*)key,(guint64)ADDR_FILE_MAP+(fileCounter*12),8);
  // fptr
  editFile(diskFileName[0],&fptr,(guint64)ADDR_FILE_MAP+(fileCounter*12)
      +FNAME_SIZE,4);
}

#endif
