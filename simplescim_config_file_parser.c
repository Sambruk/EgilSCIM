#include "simplescim_config_file_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>

static struct {
	const char *name;
	char *inp;
	const char *cur;
	size_t line;
	size_t col;
	GHashTable *vars;
} global;

static void global_reset()
{
	global.name = NULL;
	global.inp = NULL;
	global.cur = NULL;
	global.line = 0;
	global.col = 0;
	global.vars = NULL;
}

static int global_read_inp()
{
	int fd;
	struct stat sb;
	char *inp;
	size_t inp_len;
	ssize_t nread;
	unsigned char tmp;

	/* Open file */
	fd = open(global.name, O_RDONLY);

	if (fd == -1) {
		/* Could not open file */
		perror(global.name);
		return -1;
	}

	/* Stat file for type/mode and size */
	if (fstat(fd, &sb) == -1) {
		/* Could not stat file */
		perror(global.name);
		close(fd);
		return -1;
	}

	/* Ensure that file is regular */
	if (!S_ISREG(sb.st_mode)) {
		/* File is not regular */
		fprintf(stderr,
		        "%s: Must be a regular file\n",
		        global.name);
		close(fd);
		return -1;
	}

	/* Fetch the reported file size */
	inp_len = sb.st_size;

	/* Allocate string for file contents */
	inp = malloc(inp_len + 1);

	if (inp == NULL) {
		/* Could not allocate string */
		perror(global.name);
		close(fd);
		return -1;
	}

	/*
	  Read file to string.

	  If 'nread' is equal to 'inp_len', the actual file size is
	  at least as large as the reported file size.

	  If 'nread' is less than 'inp_len', the actual file size is
	  smaller than the reported file size.

	  If 'nread' is -1, an error occurred.
	*/

	nread = read(fd, inp, inp_len);

	if (nread == -1) {
		/* An error occurred */
		perror(global.name);
		free(inp);
		close(fd);
		return -1;
	}

	if ((size_t)nread < inp_len) {
		/* The actual file size is smaller than the reported
		   file size */
		fprintf(stderr,
"%s: File size reported as %lu B but could only read %ld B\n",
		        global.name,
		        inp_len,
		        nread);
		free(inp);
		close(fd);
		return -1;
	}

	/*
	  Ensure that the actual file size is not larger than the
	  reported file size by reading one more byte from the file.

	  If 'nread' is 0, no more bytes could be read and the actual
	  file size is equal to the reported file size.

	  If 'nread' is 1, more bytes could be read and the actual
	  file size is larger than the reported file size.

	  If 'nread' is -1, an error occurred.
	*/

	nread = read(fd, &tmp, 1);

	if (nread == -1) {
		/* An error occurred */
		perror(global.name);
		free(inp);
		close(fd);
		return -1;
	}

	if (nread == 1) {
		/* The actual file size is larger than the reported
		   file size */
		fprintf(stderr,
"%s: File size reported as %lu B but actual file size is larger\n",
		        global.name,
		        inp_len);
		free(inp);
		close(fd);
		return -1;
	}

	/* Close file */
	close(fd);

	/* Terminate string */
	inp[inp_len] = '\0';

	/* Store string globally */
	global.inp = inp;

	return 0;
}

static void print_error(const char *str)
{
	fprintf(stderr,
	        "%s:%lu:%lu: syntax error: %s\n",
	        global.name,
	        global.line,
	        global.col,
	        str);
}

static void print_error_expected(const char *str)
{
	fprintf(stderr,
	        "%s:%lu:%lu: syntax error: Expected %s, found ",
	        global.name,
	        global.line,
	        global.col,
	        str);

	if (isgraph(*global.cur)) {
		fprintf(stderr, "'%c'\n", *global.cur);
	} else {
		fprintf(stderr, "0x%02X\n", *global.cur);
	}
}

static int is_varid(char c)
{
	static unsigned char lookup_table[0x100] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 00-0F */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10-1F */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, /* 20-2F */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 30-3F */
 		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 40-4F */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 50-5F */
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 60-6F */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 70-7F */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 80-8F */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 90-9F */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* A0-AF */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* B0-BF */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* C0-CF */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* D0-DF */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* E0-EF */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  /* F0-FF */
	};

	return (int)lookup_table[(unsigned char)c];
}

/* <ws> ::= ' ' | '\t' */
static void rule_skip_ws()
{
	while (*global.cur == ' ' || *global.cur == '\t') {
		++global.cur;
		++global.col;
	}
}

/* <varid> ::= [-_a-zA-Z0-9]+ */
static int rule_varid(char **var)
{
	size_t var_len = 0;
	char *tmp;

	/* Determine variable name length */
	while (is_varid(global.cur[var_len])) {
		++var_len;
	}

	if (var_len == 0) {
		print_error("Variable name cannot be empty");
		return -1;
	}

	/* Allocate and copy variable name string */
	tmp = malloc(var_len + 1);

	if (tmp == NULL) {
		print_error(strerror(errno));
		return -1;
	}

	memcpy(tmp, global.cur, var_len);
	tmp[var_len] = '\0';
	*var = tmp;

	global.cur += var_len;
	global.col += var_len;

	return 0;
}

/* <value> ::= '<?' [^('?>')]* '?>' <ws>* | [^('#'|'\n')]* */
static int rule_value(char **val)
{
	size_t val_len = 0;
	char *tmp;

	/* Multi line value or single line value */
	if (global.cur[0] == '<' && global.cur[1] == '?') {
		size_t tmp_line, tmp_col;

		global.cur += 2;
		global.col += 2;

		tmp_line = global.line;
		tmp_col = global.col;

		/* Determine length of multi line value */
		for (;;) {
			if (global.cur[val_len] == '\0') {
				global.line = tmp_line;
				global.col = tmp_col;
				print_error("Unexpected end-of-file");
				return -1;
			}

			/* Multi line value terminated by '?>' */
			if (global.cur[val_len] == '?'
			    && global.cur[val_len + 1] == '>') {
				break;
			}

			if (global.cur[val_len] == '\n') {
				++val_len;
				++tmp_line;
				tmp_col = 1;
			} else {
				++val_len;
				++tmp_col;
			}
		}

		/* Allocate and copy value string */
		tmp = malloc(val_len + 1);

		if (tmp == NULL) {
			print_error(strerror(errno));
			return -1;
		}

		memcpy(tmp, global.cur, val_len);
		tmp[val_len] = '\0';

		global.cur += val_len + 2;
		global.line = tmp_line;
		global.col = tmp_col + 2;

		rule_skip_ws();
	} else {
		/* Determine single line value length */
		while (global.cur[val_len] != '\n'
		       && global.cur[val_len] != '#'
		       && global.cur[val_len] != '\0') {
			++val_len;
		}

		if (global.cur[val_len] == '\0') {
			global.col += val_len;
			print_error("Unexpected end-of-file");
			return -1;
		}

		/* Allocate and copy value string */
		tmp = malloc(val_len + 1);

		if (tmp == NULL) {
			print_error(strerror(errno));
			return -1;
		}

		memcpy(tmp, global.cur, val_len);
		tmp[val_len] = '\0';

		global.cur += val_len;
		global.col += val_len;

		/* Remove trailing white space */
		while (val_len > 0 && (tmp[val_len - 1] == ' '
		                       || tmp[val_len - 1] == '\t')) {
			tmp[val_len - 1] = '\0';
			--val_len;
		}
	}

	*val = tmp;

	return 0;
}

/* <assign> ::= <varid> <ws>* '=' <ws>* <value> */
static int rule_assign()
{
	char *var, *val;

	/* Obligatory variable name */
	if (!is_varid(*global.cur)) {
		print_error_expected("variable name");
		return -1;
	}

	if (rule_varid(&var) == -1) {
		return -1;
	}

	/* Optional white space */
	rule_skip_ws();

	/* Obligatory variable assignment character */
	if (*global.cur != '=') {
		print_error_expected("'='");
		free(var);
		return -1;
	}

	++global.cur;
	++global.col;

	/* Optional white space */
	rule_skip_ws();

	/* Obligatory value */
	if (rule_value(&val) == -1) {
		free(var);
		return -1;
	}

	g_hash_table_insert(global.vars, var, val);

	return 0;
}

/* <comment> ::= '#' [^\n]* */
static int rule_comment()
{
	/* Obligatory line comment initialiser character */
	if (*global.cur != '#') {
		print_error_expected("'#'");
		return -1;
	}

	/* Zero or more non-newline characters */
	while (*global.cur != '\n' && *global.cur != '\0') {
		++global.cur;
		++global.col;
	}

	if (*global.cur == '\0') {
		print_error("Unexpected end-of-file");
		return -1;
	}

	return 0;
}

/* <config> ::= ( <ws>* <assign>? <comment>? '\n' )* */
static int rule_config()
{
	/* Zero or more lines */
	while (*global.cur != '\0') {
		/* Optional white space */
		rule_skip_ws();

		/* Optional variable assignment */
		if (is_varid(*global.cur)) {
			if (rule_assign() == -1) {
				return -1;
			}
		}

		/* Optional comment */
		if (*global.cur == '#') {
			if (rule_comment() == -1) {
				return -1;
			}
		}

		/* Obligatory newline */
		if (*global.cur != '\n') {
			print_error_expected("end-of-line");
			return -1;
		}

		++global.cur;
		++global.line;
		global.col = 1;
	}

	return 0;
}

GHashTable *simplescim_parse_config_file(const char *filename)
{
	GHashTable *vars;

	/* Copy config file name to global data structure */
	global.name = filename;

	/* Read config file contents to string in global
	   data structure */
	if (global_read_inp() == -1) {
		return NULL;
	}

	/* Initialise parser position variables in global
	   data structure */
	global.line = 1;
	global.col = 1;

	/* Initialise variable hash table in global
	   data structure */
	global.vars = g_hash_table_new_full(g_str_hash,
	                                    g_str_equal,
	                                    free,
	                                    free);

	/* Start parsing */
	global.cur = global.inp;

	if (rule_config() == -1) {
		/* An error occurred while parsing */
		free(global.inp);
		g_hash_table_destroy(global.vars);
		return NULL;
	}

	/* Free string containing config file contents from
	   global data structure */
	free(global.inp);

	/* Save variable hash table */
	vars = global.vars;

	/* Reset global data structure */
	global_reset();

	return vars;
}
