#include <sys/stat.h>
#include <glib.h>

typedef struct path *path;

// append gchar* components to path
path path_add(path self, ...);
path path_add1(path self, gchar* name);

// look up a base path
path path_lookup(GUserDirectory id);
path path_home(void);

path CWD(void);

const gchar* path_base(path);
gsize path_len(path);

void path_free(path);

bool left_is_older(path left, path right);
void path_check_terminate(path);

// should this return a path? or a GString*?
const char* path_add_ext(const char* name, const char* ext);
