#include "simplescim_config_file_parser.h"

#include <stdio.h>      /* perror(), fprintf() */
#include <stdlib.h>     /* malloc(), free() */
#include <string.h>     /* strerror(), memcpy() */
#include <ctype.h>      /* isgraph() */
#include <errno.h>      /* errno */
#include <unistd.h>     /* fstat(), close(), read() */
#include <sys/types.h>  /* open(), fstat() */
#include <sys/stat.h>   /* open(), fstat() */
#include <fcntl.h>      /* open() */
#include <glib.h>       /* GHashTable */

#include "simplescim_globals.h"

static struct {
	char *inp;
	const char *cur;
	size_t line;
	size_t col;
	GHashTable *vars;
} parser;

static void parser_reset()
{
	parser.inp = NULL;
	parser.cur = NULL;
	parser.line = 0;
	parser.col = 0;
	parser.vars = NULL;
}

static int read_config_file()
{
	int fd;
	struct stat sb;
	char *inp;
	size_t inp_len;
	ssize_t nread;
	unsigned char tmp;

	/* Open file */
	fd = open(simplescim_global_filename, O_RDONLY);

	if (fd == -1) {
		/* Could not open file */
		perror(simplescim_global_filename);
		return -1;
	}

	/* Stat file for type/mode and size */
	if (fstat(fd, &sb) == -1) {
		/* Could not stat file */
		perror(simplescim_global_filename);
		close(fd);
		return -1;
	}

	/* Ensure that file is regular */
	if (!S_ISREG(sb.st_mode)) {
		/* File is not regular */
		fprintf(stderr,
		        "%s: not a regular file\n",
		        simplescim_global_filename);
		close(fd);
		return -1;
	}

	/* Fetch the reported file size */
	inp_len = sb.st_size;

	/* Allocate string for file contents */
	inp = malloc(inp_len + 1);

	if (inp == NULL) {
		/* Could not allocate string */
		perror(simplescim_global_filename);
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
		perror(simplescim_global_filename);
		free(inp);
		close(fd);
		return -1;
	}

	if ((size_t)nread < inp_len) {
		/* The actual file size is smaller than the reported
		   file size */
		fprintf(stderr,
"%s: file size reported as %lu B but could only read %ld B\n",
		        simplescim_global_filename,
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
		perror(simplescim_global_filename);
		free(inp);
		close(fd);
		return -1;
	}

	if (nread == 1) {
		/* The actual file size is larger than the reported
		   file size */
		fprintf(stderr,
"%s: file size reported as %lu B but actual file size is larger\n",
		        simplescim_global_filename,
		        inp_len);
		free(inp);
		close(fd);
		return -1;
	}

	/* Close file */
	close(fd);

	/* Terminate string */
	inp[inp_len] = '\0';

	/* Store string global data structure */
	parser.inp = inp;

	return 0;
}

static void syntax_error(const char *str)
{
	fprintf(stderr,
	        "%s:%lu:%lu: syntax error: %s\n",
	        simplescim_global_filename,
	        parser.line,
	        parser.col,
	        str);
}

static void syntax_error_expected(const char *str)
{
	fprintf(stderr,
	        "%s:%lu:%lu: syntax error: expected %s, found ",
	        simplescim_global_filename,
	        parser.line,
	        parser.col,
	        str);

	if (isgraph(*parser.cur)) {
		fprintf(stderr, "'%c'\n", *parser.cur);
	} else {
		fprintf(stderr, "0x%02X\n", *parser.cur);
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
	while (*parser.cur == ' ' || *parser.cur == '\t') {
		++parser.cur;
		++parser.col;
	}
}

/* <varid> ::= [-_a-zA-Z0-9]+ */
static int rule_varid(char **var)
{
	size_t var_len = 0;
	char *tmp;

	/* Determine variable name length */
	while (is_varid(parser.cur[var_len])) {
		++var_len;
	}

	if (var_len == 0) {
		syntax_error("variable name cannot be empty");
		return -1;
	}

	/* Allocate and copy variable name string */
	tmp = malloc(var_len + 1);

	if (tmp == NULL) {
		syntax_error(strerror(errno));
		return -1;
	}

	memcpy(tmp, parser.cur, var_len);
	tmp[var_len] = '\0';
	*var = tmp;

	parser.cur += var_len;
	parser.col += var_len;

	return 0;
}

/* <value> ::= '<?' [^('?>')]* '?>' <ws>* | [^('#'|'\n')]* */
static int rule_value(char **val)
{
	size_t val_len = 0;
	char *tmp;

	/* Multi line value or single line value */
	if (parser.cur[0] == '<' && parser.cur[1] == '?') {
		size_t tmp_line, tmp_col;

		parser.cur += 2;
		parser.col += 2;

		tmp_line = parser.line;
		tmp_col = parser.col;

		/* Determine length of multi line value */
		for (;;) {
			if (parser.cur[val_len] == '\0') {
				parser.line = tmp_line;
				parser.col = tmp_col;
				syntax_error("unexpected end-of-file");
				return -1;
			}

			/* Multi line value terminated by '?>' */
			if (parser.cur[val_len] == '?'
			    && parser.cur[val_len + 1] == '>') {
				break;
			}

			if (parser.cur[val_len] == '\n') {
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
			syntax_error(strerror(errno));
			return -1;
		}

		memcpy(tmp, parser.cur, val_len);
		tmp[val_len] = '\0';

		parser.cur += val_len + 2;
		parser.line = tmp_line;
		parser.col = tmp_col + 2;

		rule_skip_ws();
	} else {
		/* Determine single line value length */
		while (parser.cur[val_len] != '\n'
		       && parser.cur[val_len] != '#'
		       && parser.cur[val_len] != '\0') {
			++val_len;
		}

		if (parser.cur[val_len] == '\0') {
			parser.col += val_len;
			syntax_error("unexpected end-of-file");
			return -1;
		}

		/* Allocate and copy value string */
		tmp = malloc(val_len + 1);

		if (tmp == NULL) {
			syntax_error(strerror(errno));
			return -1;
		}

		memcpy(tmp, parser.cur, val_len);
		tmp[val_len] = '\0';

		parser.cur += val_len;
		parser.col += val_len;

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
	if (!is_varid(*parser.cur)) {
		syntax_error_expected("variable name");
		return -1;
	}

	if (rule_varid(&var) == -1) {
		return -1;
	}

	/* Optional white space */
	rule_skip_ws();

	/* Obligatory variable assignment character */
	if (*parser.cur != '=') {
		syntax_error_expected("'='");
		free(var);
		return -1;
	}

	++parser.cur;
	++parser.col;

	/* Optional white space */
	rule_skip_ws();

	/* Obligatory value */
	if (rule_value(&val) == -1) {
		free(var);
		return -1;
	}

	g_hash_table_insert(parser.vars, var, val);

	return 0;
}

/* <comment> ::= '#' [^\n]* */
static int rule_comment()
{
	/* Obligatory line comment initialiser character */
	if (*parser.cur != '#') {
		syntax_error_expected("'#'");
		return -1;
	}

	/* Zero or more non-newline characters */
	while (*parser.cur != '\n' && *parser.cur != '\0') {
		++parser.cur;
		++parser.col;
	}

	if (*parser.cur == '\0') {
		syntax_error("unexpected end-of-file");
		return -1;
	}

	return 0;
}

/* <config> ::= ( <ws>* <assign>? <comment>? '\n' )* */
static int rule_config()
{
	/* Zero or more lines */
	while (*parser.cur != '\0') {
		/* Optional white space */
		rule_skip_ws();

		/* Optional variable assignment */
		if (is_varid(*parser.cur)) {
			if (rule_assign() == -1) {
				return -1;
			}
		}

		/* Optional comment */
		if (*parser.cur == '#') {
			if (rule_comment() == -1) {
				return -1;
			}
		}

		/* Obligatory newline */
		if (*parser.cur != '\n') {
			syntax_error_expected("end-of-line");
			return -1;
		}

		++parser.cur;
		++parser.line;
		parser.col = 1;
	}

	return 0;
}

GHashTable *simplescim_parse_config_file()
{
	GHashTable *vars;

	/* Read config file contents to string in global data
	   structure */
	if (read_config_file() == -1) {
		return NULL;
	}

	/* Initialise parser position variables in global data
	   structure */
	parser.line = 1;
	parser.col = 1;

	/* Initialise variable hash table in global data structure */
	parser.vars = g_hash_table_new_full(g_str_hash,
	                                    g_str_equal,
	                                    free,
	                                    free);

	/* Start parsing */
	parser.cur = parser.inp;

	if (rule_config() == -1) {
		/* An error occurred while parsing */
		free(parser.inp);
		g_hash_table_destroy(parser.vars);
		return NULL;
	}

	/* Free string containing config file contents from global
	   data structure */
	free(parser.inp);

	/* Save variable hash table */
	vars = parser.vars;

	/* Reset global data structure */
	parser_reset();

	return vars;
}
