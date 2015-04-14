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
extern guint fileCounter;
extern GTree* fileMap;

extern const guint ADDR_FREE_SPACE_VECTOR;
extern const guint ADDR_FILE_COUNTER;
extern guint ADDR_FILE_MAP;
extern guint ADDR_DATA;
extern const guint KEY_SIZE;
extern const guint ATIME_SIZE;
extern const guint SIZE_SIZE;

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
  printf("Allocate freespace\n");
  guchar mark[realSize];
  int i;
  for(i=0;i<realSize;i++){
    mark[i]=0xFF;
  }
  diskWriteData(ADDR_FREE_SPACE_VECTOR+freeOffset,&mark,realSize);

}

void FreeSpaceUnmark(guint realSize,guint64 addrFile){
  printf("Free allocate space\n");
  // 0 = @21,...
  guint64 allocOffset=(addrFile-ADDR_DATA)/8;
  guint realBlockSize=realSize/8;
  printf("AllocOffset : %"G_GUINT64_FORMAT"\n",allocOffset);
  printf("BlockNumber : %d\n",realBlockSize);
  printf("RealAddrStart : %"G_GUINT64_FORMAT"\n",ADDR_FREE_SPACE_VECTOR+allocOffset);
  printf("RealAddrStop : %"G_GUINT64_FORMAT"\n",ADDR_FREE_SPACE_VECTOR+allocOffset+realBlockSize-1);

  guchar mark[realBlockSize];
  int i;
  for(i=0;i<realBlockSize;i++){
    mark[i]=0;
  }
  diskWriteData(ADDR_FREE_SPACE_VECTOR+allocOffset,&mark,realBlockSize);

}

// TODO : if fileCounter need to pass
void FMapAdd(const gchar *key,guint fptr){
  // Insert to map=8B+4B
  // key
  editFile(diskFileName[0],(void*)key,(guint64)ADDR_FILE_MAP+(fileCounter*12),8);
  // fptr
  editFile(diskFileName[0],&fptr,(guint64)ADDR_FILE_MAP+(fileCounter*12)
      +KEY_SIZE,4);

  FMAP* fm = (FMAP*) malloc(sizeof(FMAP));
  fm->fileNo=fileCounter;
  fm->fptr=fptr;  

  fileCounter+=1;
  g_tree_insert(fileMap, (char*)key, fm);

  //printf("Increase file counter\n");
  diskWriteData(ADDR_FILE_COUNTER,&fileCounter,SIZE_SIZE);

}

#endif
