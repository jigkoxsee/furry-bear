#ifndef MYDISK_C
#define MYDISK_C
#include <stdio.h>
#include <glib.h>
#include "myfs.c"
#include "myfile.c"


extern int diskMode;

 // 1GB
//
// D0,D1 = addr = 200M+
// D2,D3 = addr = 17+

// MODE 1 => 0
// MODE 10 => 0,2
/*
guchar* diskReadN(guint addr,guint64 size){
  halfOffset=diskSize;

  printf("DiskCount :%d\n",diskCount);
  guchar* buffer;
  // Open

  // Read
  if(diskMode==10){ // TODO
    if(addr<=halfOffset){
      // d[0]
      // if(diskCount>3){
      //    d[2]
    }else{
      // d[1]
    }
  }else{
  }
  return buffer;
}
*/

// TODO return value
// 1 success
// 0 fail
/*
void diskWriteData(guint64 addr,void* data,guint size){
  // TODO 4 disk mng
  editFile(diskFileName[0],data,addr,size);
}*/

#endif
