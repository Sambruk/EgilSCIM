#include "simplescim_error_string.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "simplescim_config_file.h"

/**
 * An error string that contains the latest error message
 * if such exists.
 */
char simplescim_error_string[1024];

/**
 * Prints an error about malloc to simplescim_error_string.
 */
void simplescim_error_string_malloc()
{
	sprintf(simplescim_error_string,
	        "%s:malloc: %s",
	        simplescim_config_file_name,
	        strerror(ENOMEM));
}
