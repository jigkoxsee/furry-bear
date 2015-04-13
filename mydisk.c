#ifndef MYDISK_C
#define MYDISK_C
#include <stdio.h>
#include <glib.h>
#include "myfs.c"
#include "myfile.c"

extern char * diskFileName[4];
extern FILE * diskFile[4];
extern int diskCount;
extern guint64 diskSize;
extern int diskMode;

guint64 halfOffset; // 1GB
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


#endif
