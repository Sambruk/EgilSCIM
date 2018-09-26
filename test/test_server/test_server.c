/**
 * Copyright © 2017-2018  Max Wällstedt <>
 *
 * This file is part of SimpleSCIM.
 *
 * SimpleSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SimpleSCIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <uuid/uuid.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

static int create_socket(int port)
{
	int s;
	struct sockaddr_in addr;

	s = socket(AF_INET, SOCK_STREAM, 0);

	if (s == -1) {
		perror("socket");
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("bind");
		close(s);
		return -1;
	}

	if (listen(s, SOMAXCONN) == -1) {
		perror("listen");
		close(s);
		return -1;
	}

	return s;
}

static void init_openssl()
{
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
}

static void cleanup_openssl()
{
	EVP_cleanup();
}

static SSL_CTX *create_context()
{
	const SSL_METHOD *method;
	SSL_CTX *ctx;

	method = SSLv23_server_method();
	ctx = SSL_CTX_new(method);

	if (ctx == NULL) {
		perror("SSL_CTX_new");
		ERR_print_errors_fp(stderr);
		return NULL;
	}

	return ctx;
}

static int configure_context(
	SSL_CTX *ctx,
	const char *cert,
	const char *key
)
{
	SSL_CTX_set_ecdh_auto(ctx, 1);

	/* Set the key and cert */
	if (SSL_CTX_use_certificate_file(
		ctx,
		cert,
		SSL_FILETYPE_PEM
	) <= 0) {
		ERR_print_errors_fp(stderr);
		return -1;
	}

	if (SSL_CTX_use_PrivateKey_file(
		ctx,
		key,
		SSL_FILETYPE_PEM
	) <= 0) {
		ERR_print_errors_fp(stderr);
		return -1;
	}

	return 0;
}

static int init = 0;
static int sfd = -1;
static SSL_CTX *ctx = NULL;

static void sig_handler(int signo)
{
	if (signo == SIGINT) {
		if (sfd != -1) {
			close(sfd);
		}

		if (ctx != NULL) {
			SSL_CTX_free(ctx);
		}

		if (init != 0) {
			cleanup_openssl();
		}

		fputc('\n', stdout);
		exit(EXIT_SUCCESS);
	}
}

static void create_user(SSL *ssl, char *inp __attribute__((unused)))
{
	char uuid_str[37];
	uuid_t uuid;
	static char response[1024];
	static char json[1024];

	uuid_generate(uuid);
	uuid_unparse(uuid, uuid_str);

	snprintf(
		json,
		1024,
		"{\"id\":\"%s\"}",
		uuid_str
	);

	snprintf(
		response,
		1024,
		"HTTP/1.1 201 Created\r\n"
		"Content-Type: application/scim+json\r\n"
		"\r\n"
		"%s",
		json
	);

	SSL_write(ssl, response, strlen(response));
}

static void update_user(SSL *ssl, char *inp __attribute__((unused)))
{
	static char response[1024];

	snprintf(
		response,
		1024,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: application/scim+json\r\n"
		"\r\n"
		"{}"
	);

	SSL_write(ssl, response, strlen(response));
}

static void delete_user(SSL *ssl, char *inp __attribute__((unused)))
{
	static char response[1024];

	snprintf(
		response,
		1024,
		"HTTP/1.1 204 No Content\r\n"
		"\r\n"
	);

	SSL_write(ssl, response, strlen(response));
}

static void test_server(SSL *ssl)
{
	static char buf[1024];
	int nread;
	const char *response = "HTTP/1.1 404 Not Found\r\n\r\n";

	nread = SSL_read(ssl, buf, sizeof(buf) - 1);
	buf[nread] = '\0';
	fputs(buf, stdout);
	fputc('\n', stdout);

	if (strncmp(buf, "POST", strlen("POST")) == 0) {
		create_user(ssl, buf);
	} else if (strncmp(buf, "PUT", strlen("PUT")) == 0) {
		update_user(ssl, buf);
	} else if (strncmp(buf, "DELETE", strlen("DELETE")) == 0) {
		delete_user(ssl, buf);
	} else {
		SSL_write(ssl, response, strlen(response));
	}
}

int main(int argc, char *argv[])
{
	int err;

	if (argc != 4) {
		fprintf(stderr, "usage: %s port cert key\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		perror("signal");
		exit(EXIT_FAILURE);
	}

	init_openssl();
	init = 1;
	ctx = create_context();

	if (ctx == NULL) {
		cleanup_openssl();
		exit(EXIT_FAILURE);
	}

	err = configure_context(ctx, argv[2], argv[3]);

	if (err == -1) {
		SSL_CTX_free(ctx);
		cleanup_openssl();
		exit(EXIT_FAILURE);
	}

	sfd = create_socket(atoi(argv[1]));

	if (sfd == -1) {
		SSL_CTX_free(ctx);
		cleanup_openssl();
		exit(EXIT_FAILURE);
	}

	/* Handle connections */

	for (;;) {
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(addr);
		SSL *ssl;
		int cfd;

		cfd = accept(sfd, (struct sockaddr *)&addr, &addrlen);

		if (cfd == -1) {
			perror("accept");
			continue;
		}

		ssl = SSL_new(ctx);
		SSL_set_fd(ssl, cfd);

		if (SSL_accept(ssl) <= 0) {
			ERR_print_errors_fp(stderr);
			SSL_free(ssl);
			close(cfd);
			continue;
		}

		test_server(ssl);
		SSL_free(ssl);
		close(cfd);
	}

	close(sfd);
	SSL_CTX_free(ctx);
	cleanup_openssl();

	return 0;
}
