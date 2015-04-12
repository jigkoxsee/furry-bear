#ifndef FREESPACE_C
#define FREESPACE_C

#include <stdio.h>
#include <glib.h>

extern const guint addrFreeSpaceVector; // 1/8 B
// Find first space with fit a specific size
gint64 FreeSpaceFind(guint64 size){
  // Calculate add Head too
  // name,atime,size,data
  guint realSize=8+4+4+size;
  guchar *buffer=diskReadN(addrFreeSpaceVector);

  return 0;
}

void FreeSpaceMark(){

}
void FreeSpaceUnmark(){

}

#endif
