#ifndef MYFILE_C
#define MYFILE_C

#include <stdio.h>
#include <glib.h>

extern guint fileCounter;
// This file is do work about file map,FCB

typedef struct FCB FCB;
struct FCB{
  gint64  accessTime; // g_date_time_to_unix()
  int size;     // End
};

struct FMAP{
  char* name;
  FCB* fptr;
};

guint8* readFileN(char * fileName,guint offset,guint size){
  FILE* fileptr;
  guint8* buffer;

  fileptr = fopen(fileName,"rb");  // Open file in binary
  fseek(fileptr,offset,SEEK_SET); // Seek file to the end

  buffer = (guint8*) malloc((size)*sizeof(guint8)); // Allocation memory for read file
  // TODO : Avoid allocate ahead (allocate only need)
  fread(buffer,size,1,fileptr); // Read file
  fclose(fileptr); // close file
  return buffer;
}

// TODO : Read entire file by stream (avoid allocate large memory"
char * readFile(char * fileName){
  FILE * fileptr;
  char * buffer;
  guint64 filelen;

  fileptr = fopen(fileName,"rb");  // Open file in binary
  fseek(fileptr,0,SEEK_END); // Seek file to the end
  filelen = ftell(fileptr); // Get number of current byte offset
  rewind(fileptr); // Junp to begin

  printf("filelen : %" G_GUINT64_FORMAT "\n",filelen);

  buffer = (char *) malloc((filelen+1)*sizeof(char)); // Allocation memory for read file
  // TODO : Avoid allocate ahead (allocate only need)
  fread(buffer,filelen,1,fileptr); // Read entire file // TODO what all this param?
  fclose(fileptr); // close file
  return buffer;
}

void editFile(char * filename,void * data, guint64 byteOffset,guint64 size){
  FILE * writeptr;
  writeptr = fopen(filename,"rb+");
  fseek ( writeptr, byteOffset, SEEK_SET );
  fwrite(data,size,1,writeptr);
  //fprintf(writeptr,"%s",data); // TODO didnt test
  fclose(writeptr);

}

void writeFile(char * filename,char * fileData){
  FILE * writeptr;
  writeptr = fopen(filename,"wb");
  fwrite(fileData,strlen(fileData),1,writeptr);
  fclose(writeptr);
}

#endif
