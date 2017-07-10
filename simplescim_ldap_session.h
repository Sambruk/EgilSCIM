#ifndef SIMPLESCIM_LDAP_SESSION_H
#define SIMPLESCIM_LDAP_SESSION_H

int simplescim_ldap_session_start();
int simplescim_ldap_session_search();
int simplescim_ldap_session_print_result();
int simplescim_ldap_session_destroy_result();
int simplescim_ldap_session_close();

#endif
