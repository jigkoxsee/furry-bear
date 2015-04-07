#include <stdio.h>
#include <glib.h>

gboolean iter_all(gpointer key, gpointer value, gpointer data) {
  //printf("%s, %s\n", key, value);
  printf("%d : %s, %s\n",key, key, value);
  return FALSE;
}

gboolean iter_start(gpointer key, gpointer value, gpointer data) {
  printf("%d : %s, %s\n",key, key, value);
  return g_str_has_prefix(key, "b") == 0; // TODO : Return prefix match
}

gboolean iter_stop(gpointer key, gpointer value, gpointer data) {
  printf("%s, %s\n", key, value);
  return g_str_has_prefix(key, "b") != 0; // TODO : Return prefix match
}


// user_data : pass "prefix key"
gint finder(gpointer key, gpointer user_data) {
  // TODO : if prefix match -> remember
  printf("F %d %s\n",key,(char*)key);
  return -g_ascii_strcasecmp(key,(char*)user_data);
}

int main(int argc, char** argv) {
  GTree* t = g_tree_new((GCompareFunc)g_ascii_strcasecmp);
  g_tree_insert(t, "d", "Detroit");
  g_tree_insert(t, "a", "Atlanta");
  g_tree_insert(t, "c", "Chicago");
  g_tree_insert(t, "bos", "Boston");
  g_tree_insert(t, "ba", "Bangkok");
  g_tree_insert(t, "ban", "Bangalor");

  printf("Iterating all nodes\n");
  g_tree_foreach(t, (GTraverseFunc)iter_all, NULL);
  printf("Iterating some of the nodes\n");

  g_tree_foreach(t, (GTraverseFunc)iter_start, NULL);
  //-------
  char * mykey="ba";
  gpointer value = g_tree_search(t, (GCompareFunc)finder, mykey);
  printf("Data :%s",value);

  //g_tree_foreach(value, (GTraverseFunc)iter_all, NULL);

  g_tree_destroy(t);
  return 0;
}
