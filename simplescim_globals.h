#ifndef SIMPLESCIM_GLOBALS_H
#define SIMPLESCIM_GLOBALS_H

#include <glib.h>

extern const char *simplescim_global_filename;
extern GHashTable *simplescim_global_vars;

void simplescim_globals_reset();

#endif
