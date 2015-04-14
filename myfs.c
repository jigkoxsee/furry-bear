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
extern guint fileCounter;
extern GTree* fileMap;

const guint ADDR_FILE_COUNTER=17; // 4B
const guint ADDR_FREE_SPACE_VECTOR=21; // 1/8 B
const guint KEY_SIZE=8;
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

gboolean iter_all(gpointer key, gpointer value, gpointer data) {
  //printf("%d : %s, %d\n",key, key, *(guint*)value);
  printf("%s, %d\n", (char*)key, *(guint*)value);
  return FALSE;
}

void FMapLoad(){
  FILE* disk0;
  disk0 = fopen(diskFileName[0],"rb");
  fseek(disk0,ADDR_FILE_MAP,SEEK_SET);

  fileMap= g_tree_new((GCompareFunc)g_ascii_strcasecmp);
 
  gchar* key;
  guint* fptr;


  //Loop
  int i;
  printf("\n---%d\n",fileCounter);


  for (i = 0; i < fileCounter; ++i)
  {
    printf("File %d:",i);

    key  = (gchar*) malloc((KEY_SIZE+1)*sizeof(gchar));
    fptr = (guint*) malloc(sizeof(guint));
    fread(key,KEY_SIZE,1,disk0);
    key[8]=0;
    printf("%s\t",key);
    
    fread(fptr,SIZE_SIZE,1,disk0);
    printf("%x\t",*fptr);

    g_tree_insert(fileMap, key, fptr);
    /* code */
    printf("\n");
  }
  fclose(disk0);

  //g_tree_foreach(fileMap, (GTraverseFunc)iter_all, NULL);
    // Read each element
    // Insert to map
  printf("Load file map finish\n");
}

guint myPut(const gchar *key,const gchar *src){
  FILE *filePTR;
  filePTR = fopen(src,"rb");  // Open file in binary
  guint fileSize=getFileSize(filePTR);
  fclose(filePTR);
  printf(">> %d\n",fileSize);

  //guint fileCounter=getFileCounter();

  guint realSize=(4+4+fileSize)/8;
  // Find freeSpace
  printf("Find freespace %d\n",realSize);
  guint64 freeOffset=FreeSpaceFind(realSize);
  if(freeOffset==-1)
    return ENOSPC;
    // TODO return ENOSPC if no space enough
  guint64 addrFile=ADDR_DATA+freeOffset*8;

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

  // Inser map
  // check key size ==8
  // TODO : check duplicate
  printf("File Map add\n");
  if(strlen(key)==8){
    FMapAdd(fileCounter,key,addrFile);
  }else{
    return ENAMETOOLONG;
  }
  // Increse file counter
  printf("Increase file counter\n");
  fileCounter+=1;
  diskWriteData(ADDR_FILE_COUNTER,&fileCounter,SIZE_SIZE);

  // mark
  printf("Allocate freespace\n");
  guchar mark[realSize];
  int i;
  for(i=0;i<realSize;i++){
    mark[i]=0xFF;
  }
  diskWriteData(ADDR_FREE_SPACE_VECTOR+freeOffset,&mark,realSize);
  printf("Finish\n");
  return 0;
}


#endif
