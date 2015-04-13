#ifndef FREESPACE_C
#define FREESPACE_C

#include <stdio.h>
#include <glib.h>
#include "mydisk.c"

extern char * diskFileName[4];
extern FILE * diskFile[4];
extern int diskCount;
extern guint64 diskSize;
extern int diskMode;
extern const guint addrFreeSpaceVector; // 1/8 B
// Find first space with fit a specific size
gint64 FreeSpaceFind(guint64 size){
  // Calculate add Head too
  // name,atime,size,data
  guint realSize=8+4+4+size;
  guint64 fsSize=1000000;
  guchar* buffer=readFileN("disk1.img",addrFreeSpaceVector,fsSize);


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
  printf(">>>%"G_GUINT64_FORMAT"@%"G_GUINT64_FORMAT" (%"G_GUINT64_FORMAT")\n",count,start,size);
  return 0;
}

void FreeSpaceMark(){

}

void FreeSpaceUnmark(){

}

#endif
