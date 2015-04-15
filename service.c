#include "rfos.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "myfs.c"

// RAID 1+0
//         0
//    -----+----
//    |        |
//    1        1
//   -+-      -+-
//  |   |    |   |
// [A] [A]  [B] [B]
//  0   1    2   3

// Free
// 0 is free, 1 is not
//
// TODO : (For RAID 10/01) Beware second disk - first 17 bytes is header (unusable)

// TODO TODO TODO : return EBUSY when not ready
guint getFileCounter();



char * diskFileName[4];
FILE * diskFile[4];
int diskCount;
guint64 diskSize;
int diskMode=1;
int reDistributeMode=0;
guint fileCounter;
GTree* fileMap;
GList* fileMapHole;

// TODO array of free file map hole in disk
// Keep address of free hole

// TODO
// TODO
// TODO : Need to change Addr data type to guint64
// TODO
// TODO
// TODO

static gboolean on_handle_get (
    RFOS *object,
    GDBusMethodInvocation *invocation,
    const gchar *key,
    const gchar *outpath) {

    /** Your Code for Get method here **/
    guint err = 0;
    /** End of Get method execution, returning values **/
    printf("GET: %s TO %s\n",key,outpath);
    myGet((gchar*)key,(gchar*)outpath);

    rfos_complete_get(object, invocation, err);
    return TRUE;
}

static gboolean on_handle_put (
    RFOS *object,
    GDBusMethodInvocation *invocation,
    const gchar *key,
    const gchar *src) {

    /** Your code for Put method here **/
    printf("PUT: %s FROM %s\n",key,src);

    guint err = myPut(key,src);
//    guint err = EAGAIN;
    /** End of Put method execution, returning values **/

    rfos_complete_put(object, invocation, err);
    printf("Finish\n");
    // TODO : for test
    g_tree_foreach(fileMap, (GTraverseFunc)iter_all, NULL);

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
    err=myRemove(key);

    rfos_complete_remove(object, invocation, err);
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

    rfos_complete_search(object, invocation, err);
    return TRUE;
}

static gboolean on_handle_stat (
    RFOS *object,
    GDBusMethodInvocation *invocation,
    const gchar *key) {
    /** Your Code for Get method here **/
    guint size=0;
    guint atime=0;
    guint err = 0;
    printf("STAT: %s\n",key);
    printf("FileCounter : %d\n",fileCounter);
    err = myStat(key,&size,&atime);
    /** End of Get method execution, returning values **/
    rfos_complete_stat(object, invocation,size,(guint64)atime,err);
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
    g_signal_connect (skeleton, "handle-remove", G_CALLBACK (on_handle_remove), NULL);
    g_signal_connect (skeleton, "handle-search", G_CALLBACK (on_handle_search), NULL);
    g_signal_connect (skeleton, "handle-stat", G_CALLBACK (on_handle_stat), NULL);

    /* Export the RFOS service on the connection as /kmitl/ce/os/RFOS object  */
    g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (skeleton),
        connection,
        "/kmitl/ce/os/RFOS",
        NULL);
}



int main (int argc,char* argv[])
{
  printf("Disk count : %d\n",argc-1);

  if(argc<3){
    printf("Error need 2+ disk\n");
    return 1;
  }else{
    diskMode=10;
  }

  // Check disk
  // TODO is this gonna take more than 3 min?
  diskCount=argc-1;
  checkDisk(argv);

  fileCounter=getFileCounter();
  printf("\n%u\n",fileCounter);

  // load file map into memory
  FMapLoad();

//  guint fcounter=0;
//  editFile("disk1.img",&fcounter,ADDR_FILE_COUNTER,4);

  // For test purpose
  g_tree_foreach(fileMap, (GTraverseFunc)iter_all, NULL);
  // TODO : Create thread to return EBUSY

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
