#ifndef FREESPACE_C
#define FREESPACE_C


/*// Find first space with fit a specific size
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
*/
#endif
