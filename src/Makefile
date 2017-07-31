CC = gcc
CFLAGS = -g -pedantic -Wall -Wextra -Werror `pkg-config --cflags json-c`
LDFLAGS = -lldap -llber `pkg-config --libs json-c`

all: SimpleSCIM

SimpleSCIM: \
	simplescim_main.o \
	simplescim_error_string.o \
	simplescim_user.o \
	simplescim_user_list.o \
	simplescim_config_file.o \
	simplescim_config_file_open.o \
	simplescim_config_file_to_string.o \
	simplescim_config_file_parser.o \
	simplescim_config_file_required_variables.o \
	simplescim_ldap.o \
	simplescim_ldap_attrs_parser.o \
	simplescim_cache_file.o \
	simplescim_scim.o \
	simplescim_scim_json.o
	@echo "LINK $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

simplescim_main.o: \
	simplescim_main.c \
	simplescim_error_string.h \
	simplescim_user_list.h \
	simplescim_config_file.h \
	simplescim_ldap.h \
	simplescim_cache_file.h \
	simplescim_scim.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_error_string.o: \
	simplescim_error_string.c \
	simplescim_error_string.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_user.o: \
	simplescim_user.c \
	simplescim_user.h \
	simplescim_error_string.h \
	simplescim_config_file.h \
	uthash.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_user_list.o: \
	simplescim_user_list.c \
	simplescim_user_list.h \
	simplescim_error_string.h \
	simplescim_user.h \
	uthash.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_config_file.o: \
	simplescim_config_file.c \
	simplescim_config_file.h \
	simplescim_error_string.h \
	simplescim_config_file_open.h \
	simplescim_config_file_to_string.h \
	simplescim_config_file_parser.h \
	simplescim_config_file_required_variables.h \
	uthash.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_config_file_open.o: \
	simplescim_config_file_open.c \
	simplescim_config_file_open.h \
	simplescim_error_string.h \
	simplescim_config_file.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_config_file_to_string.o: \
	simplescim_config_file_to_string.c \
	simplescim_config_file_to_string.h \
	simplescim_error_string.h \
	simplescim_config_file.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_config_file_parser.o: \
	simplescim_config_file_parser.c \
	simplescim_config_file_parser.h \
	simplescim_error_string.h \
	simplescim_config_file.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_config_file_required_variables.o: \
	simplescim_config_file_required_variables.c \
	simplescim_config_file_required_variables.h \
	simplescim_error_string.h \
	simplescim_config_file.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_ldap.o: \
	simplescim_ldap.c \
	simplescim_ldap.h \
	simplescim_error_string.h \
	simplescim_user.h \
	simplescim_user_list.h \
	simplescim_config_file.h \
	simplescim_ldap_attrs_parser.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_ldap_attrs_parser.o: \
	simplescim_ldap_attrs_parser.c \
	simplescim_ldap_attrs_parser.h \
	simplescim_error_string.h \
	simplescim_config_file.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_cache_file.o: \
	simplescim_cache_file.c \
	simplescim_cache_file.h \
	simplescim_error_string.h \
	simplescim_user.h \
	simplescim_user_list.h \
	simplescim_config_file.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_scim.o: \
	simplescim_scim.c \
	simplescim_scim.h \
	simplescim_error_string.h \
	simplescim_user.h \
	simplescim_user_list.h \
	simplescim_cache_file.h \
	simplescim_scim_json.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

simplescim_scim_json.o: \
	simplescim_scim_json.c \
	simplescim_scim_json.h \
	simplescim_error_string.h \
	simplescim_user.h
	@echo "CC $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

.PHONY: clean distclean

clean:
	rm -f simplescim_main.o
	rm -f simplescim_error_string.o
	rm -f simplescim_user.o
	rm -f simplescim_user_list.o
	rm -f simplescim_config_file.o
	rm -f simplescim_config_file_open.o
	rm -f simplescim_config_file_to_string.o
	rm -f simplescim_config_file_parser.o
	rm -f simplescim_config_file_required_variables.o
	rm -f simplescim_ldap.o
	rm -f simplescim_ldap_attrs_parser.o
	rm -f simplescim_cache_file.o
	rm -f simplescim_scim.o
	rm -f simplescim_scim_json.o

distclean: clean
	rm -f SimpleSCIM
