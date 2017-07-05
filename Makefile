CC = gcc
CFLAGS = -g -pedantic -Wall -Wextra -Werror
INCGLIB = `pkg-config --cflags glib-2.0`
LDFLAGS = `pkg-config --libs glib-2.0`

all: simplescim_print_config_file

simplescim_print_config_file: simplescim_print_config_file.o simplescim_config_file_parser.o
	$(CC) $(CFLAGS) $(INCGLIB) -o $@ $^ $(LDFLAGS)

simplescim_print_config_file.o: simplescim_print_config_file.c simplescim_config_file_parser.h
	$(CC) -c $(CFLAGS) $(INCGLIB) -o $@ $<

simplescim_config_file_parser.o: simplescim_config_file_parser.c simplescim_config_file_parser.h
	$(CC) -c $(CFLAGS) $(INCGLIB) -o $@ $<

.PHONY: clean

clean:
	rm -f simplescim_config_file_parser.o
	rm -f simplescim_print_config_file.o
	rm -f simplescim_print_config_file
