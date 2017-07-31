#ifndef SIMPLESCIM_CONFIG_FILE_PARSER_H
#define SIMPLESCIM_CONFIG_FILE_PARSER_H

/**
 * Parses the configuration file with its contents in
 * 'input'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_config_file_parser(const char *input);

#endif
