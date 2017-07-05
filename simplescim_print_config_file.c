#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "simplescim_config_file_parser.h"

static void print_key_val(gpointer key,
                          gpointer value,
                          gpointer user_data __attribute__((unused)))
{
	printf("\n");
	printf("%s:\n", (char *)key);
	printf("==========\n");
	printf("%s\n", (char *)value);
	printf("==========\n");
}

int main(int argc, char *argv[])
{
	int i;
	size_t j;
	GHashTable *vars;

	for (i = 1; i < argc; ++i) {
		if (i > 1) {
			printf("\n");
		}

		for (j = 0; j < strlen(argv[i]) + 4; ++j) {
			printf("*");
		}

		printf("\n* %s *\n", argv[i]);

		for (j = 0; j < strlen(argv[i]) + 4; ++j) {
			printf("*");
		}

		printf("\n");
		vars = simplescim_parse_config_file(argv[i]);

		if (vars == NULL) {
			exit(EXIT_FAILURE);
		}

		g_hash_table_foreach(vars, print_key_val, NULL);
		g_hash_table_destroy(vars);
	}

	return 0;
}
