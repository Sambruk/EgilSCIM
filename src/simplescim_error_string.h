#ifndef SIMPLESCIM_ERROR_STRING_H
#define SIMPLESCIM_ERROR_STRING_H

/**
 * An error string that contains the latest error message
 * if such exists.
 */
extern char simplescim_error_string[1024];

/**
 * Prints an error about malloc to simplescim_error_string.
 */
void simplescim_error_string_malloc();

#endif
