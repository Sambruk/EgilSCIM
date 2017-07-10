#include "simplescim_globals.h"

#include <glib.h>

const char *simplescim_global_filename = NULL;
GHashTable *simplescim_global_vars = NULL;

void simplescim_globals_reset()
{
	if (simplescim_global_filename != NULL) {
		simplescim_global_filename = NULL;
	}

	if (simplescim_global_vars != NULL) {
		g_hash_table_destroy(simplescim_global_vars);
		simplescim_global_vars = NULL;
	}
}
