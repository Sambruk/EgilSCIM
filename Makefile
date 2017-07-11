CC = gcc
CFLAGS = -g -pedantic -Wall -Wextra -Werror `pkg-config --cflags glib-2.0`
LDFLAGS = `pkg-config --libs glib-2.0` -lldap -llber

all: SimpleSCIM

SimpleSCIM: simplescim_main.o \
            simplescim_globals.o \
            simplescim_config_file_parser.o \
            simplescim_config_file_required_variables.o \
            simplescim_ldap_session.o \
            simplescim_ldap_attrs_parser.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

simplescim_main.o: simplescim_main.c \
                   simplescim_globals.h \
                   simplescim_config_file_parser.h \
                   simplescim_config_file_required_variables.h \
                   simplescim_ldap_session.h
	$(CC) -c $(CFLAGS) -o $@ $<

simplescim_globals.o: simplescim_globals.c \
                      simplescim_globals.h
	$(CC) -c $(CFLAGS) -o $@ $<

simplescim_config_file_parser.o: simplescim_config_file_parser.c \
                                 simplescim_config_file_parser.h \
                                 simplescim_globals.h
	$(CC) -c $(CFLAGS) -o $@ $<

simplescim_config_file_required_variables.o: simplescim_config_file_required_variables.c \
                                             simplescim_config_file_required_variables.h \
                                             simplescim_globals.h
	$(CC) -c $(CFLAGS) -o $@ $<

simplescim_ldap_session.o: simplescim_ldap_session.c \
                           simplescim_ldap_session.h \
                           simplescim_globals.h \
                           simplescim_ldap_attrs_parser.h
	$(CC) -c $(CFLAGS) -o $@ $<

simplescim_ldap_attrs_parser.o: simplescim_ldap_attrs_parser.c \
                                simplescim_ldap_attrs_parser.h \
                                simplescim_globals.h
	$(CC) -c $(CFLAGS) -o $@ $<

.PHONY: clean distclean

clean:
	rm -f simplescim_main.o
	rm -f simplescim_globals.o
	rm -f simplescim_config_file_parser.o
	rm -f simplescim_config_file_required_variables.o
	rm -f simplescim_ldap_session.o
	rm -f simplescim_ldap_attrs_parser.o

distclean: clean
	rm -f SimpleSCIM
