#include "rfos.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <error.h>

// 1+3
// |1|
// 2+4

// Free
// 0 is free, 1 is not
//
// TODO : (For RAID 10/01) Beware second disk - first 17 bytes is header (unusable)


char * diskFileName[4];
FILE * diskFile[4];
int diskCount;

typedef struct FCB FCB;
struct FCB{
  char* name;
  gint64  accessTime; // g_date_time_to_unix()
  int size;     // End
};

struct FMAP{
  char* name;
  FCB* fptr;
};

// TODO : Free check
// TODO : Free search (size)
// TODO : Free allocate


// TODO : MAP
//
// TODO : FCB
// TODO : 

static gboolean on_handle_get (
    RFOS *object,
    GDBusMethodInvocation *invocation,
    const gchar *key,
    const gchar *outpath) {

    /** Your Code for Get method here **/
    guint err = 0;
    /** End of Get method execution, returning values **/
    printf("GET: %s TO %s\n",key,outpath);

    rfos_complete_get(object, invocation, err);
    return TRUE;
}

static gboolean on_handle_put (
    RFOS *object,
    GDBusMethodInvocation *invocation,
    const gchar *key,
    const gchar *src) {

    /** Your code for Put method here **/
    guint err = 0;
    /** End of Put method execution, returning values **/
    printf("PUT: %s FROM %s\n",key,src);

    rfos_complete_put(object, invocation, err);

    return TRUE;
}

static gboolean on_handle_remove (
    RFOS *object,
    GDBusMethodInvocation *invocation,
    const gchar *key) {

    /** Your Code for Get method here **/
    guint err = 0;
    /** End of Get method execution, returning values **/
    printf("REMOVE: %s\n",key);

    rfos_complete_get(object, invocation, err);
    return TRUE;
}

static gboolean on_handle_search (
    RFOS *object,
    GDBusMethodInvocation *invocation,
    const gchar *key,
    const gchar *outpath) {

    /** Your Code for Get method here **/
    guint err = 0;
    /** End of Get method execution, returning values **/
    printf("SEARCH: %s\n",key);

    rfos_complete_get(object, invocation, err);
    return TRUE;
}

static gboolean on_handle_stat ( // TODO : how to return atime and size
    RFOS *object,
    GDBusMethodInvocation *invocation,
    const gchar *key) {

    /** Your Code for Get method here **/
    guint err = 0;
    /** End of Get method execution, returning values **/
    printf("STAT: %s\n",key);

    rfos_complete_get(object, invocation, err);
    return TRUE;
}
static void on_name_acquired (GDBusConnection *connection,
    const gchar *name,
    gpointer user_data)
{
    /* Create a new RFOS service object */
    RFOS *skeleton = rfos_skeleton_new ();
    /* Bind method invocation signals with the appropriate function calls */
    g_signal_connect (skeleton, "handle-get", G_CALLBACK (on_handle_get), NULL);
    g_signal_connect (skeleton, "handle-put", G_CALLBACK (on_handle_put), NULL);

    //TODO is this all Command? remove,search,stat
    
    g_signal_connect (skeleton, "handle-remove", G_CALLBACK (on_handle_remove), NULL);
    g_signal_connect (skeleton, "handle-search", G_CALLBACK (on_handle_search), NULL);
    g_signal_connect (skeleton, "handle-stat", G_CALLBACK (on_handle_stat), NULL);

    /* Export the RFOS service on the connection as /kmitl/ce/os/RFOS object  */
    g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (skeleton),
        connection,
        "/kmitl/ce/os/RFOS",
        NULL);
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

void editFile(char * filename,char * data, guint64 byteOffset,guint64 size){
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

int main (int argc,char* argv[])
{
  printf("Disk count : %d\n",argc-1);
  /*
  if(argc<3){
    printf("Error need 2+ disk\n");
    return 1;
  }
  */

  // Check disk
  // TODO is this gonna take more than 3 min
  diskCount=argc;
  checkDisk(argv);

  // TODO : For test purpose

  printf("RFOS Ready\n");
  //-------------------------------------------------------------------------
    /* Initialize daemon main loop */
    GMainLoop *loop = g_main_loop_new (NULL, FALSE);
    /* Attempt to register the daemon with 'kmitl.ce.os.RFOS' name on the bus */
    g_bus_own_name (G_BUS_TYPE_SESSION,
        "kmitl.ce.os.RFOS",
        G_BUS_NAME_OWNER_FLAGS_NONE,
        NULL,
        on_name_acquired,
        NULL,
        NULL,
        NULL);
    /* Start the main loop */
    g_main_loop_run (loop);
    return 0;
}
