#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "simplescim_arbval.h"

static void test_simplescim_arbval_constructor()
{
	struct simplescim_arbval av;
	int err;

	/* Test constructing with len = 0, val = NULL */

	err = simplescim_arbval_constructor(&av, 0, NULL);

	assert(err == 0);
	assert(av.av_len == 0);
	assert(av.av_val == NULL);

	simplescim_arbval_destructor(&av);

	/* Test constructing with len = 4711, val = NULL */

	err = simplescim_arbval_constructor(&av, 4711, NULL);

	assert(err == 0);
	assert(av.av_len == 4711);
	assert(av.av_val == NULL);

	simplescim_arbval_destructor(&av);

	/* Test constructing with len = 3, val = "hej" */

	err = simplescim_arbval_constructor(&av, 3, (const uint8_t *)"hej");

	assert(err == 0);
	assert(av.av_len == 3);
	assert(av.av_val != NULL);
	assert(av.av_val[0] == 'h');
	assert(av.av_val[1] == 'e');
	assert(av.av_val[2] == 'j');
	assert(av.av_val[3] == '\0');

	simplescim_arbval_destructor(&av);

	/* Test constructing with len = 7, val = "\x00\x01\x02\x03\x04\x05\x06\x07" */

	err = simplescim_arbval_constructor(&av, 7, (const uint8_t *)"\x00\x01\x02\x03\x04\x05\x06\x07");

	assert(err == 0);
	assert(av.av_len == 7);
	assert(av.av_val != NULL);
	assert(av.av_val[0] == 0x00);
	assert(av.av_val[1] == 0x01);
	assert(av.av_val[2] == 0x02);
	assert(av.av_val[3] == 0x03);
	assert(av.av_val[4] == 0x04);
	assert(av.av_val[5] == 0x05);
	assert(av.av_val[6] == 0x06);
	assert(av.av_val[7] == 0x00);

	simplescim_arbval_destructor(&av);
}

static void test_simplescim_arbval_constructor_copy()
{
	struct simplescim_arbval av, copy;
	int err;

	/* Test constructing with len = 0, val = NULL */

	simplescim_arbval_constructor(&av, 0, NULL);
	err = simplescim_arbval_constructor_copy(&copy, &av);

	assert(err == 0);
	assert(copy.av_len == 0);
	assert(copy.av_val == NULL);

	simplescim_arbval_destructor(&copy);
	simplescim_arbval_destructor(&av);

	/* Test constructing with len = 4711, val = NULL */

	simplescim_arbval_constructor(&av, 4711, NULL);
	err = simplescim_arbval_constructor_copy(&copy, &av);

	assert(err == 0);
	assert(copy.av_len == 4711);
	assert(copy.av_val == NULL);

	simplescim_arbval_destructor(&copy);
	simplescim_arbval_destructor(&av);

	/* Test constructing with len = 3, val = "hej" */

	simplescim_arbval_constructor(&av, 3, (const uint8_t *)"hej");
	err = simplescim_arbval_constructor_copy(&copy, &av);

	assert(err == 0);
	assert(copy.av_len == 3);
	assert(copy.av_val != NULL);
	assert(copy.av_val != av.av_val);
	assert(copy.av_val[0] == 'h');
	assert(copy.av_val[1] == 'e');
	assert(copy.av_val[2] == 'j');
	assert(copy.av_val[3] == '\0');

	simplescim_arbval_destructor(&copy);
	simplescim_arbval_destructor(&av);

	/* Test constructing with len = 7, val = "\x00\x01\x02\x03\x04\x05\x06\x07" */

	simplescim_arbval_constructor(&av, 7, (const uint8_t *)"\x00\x01\x02\x03\x04\x05\x06\x07");
	err = simplescim_arbval_constructor_copy(&copy, &av);

	assert(err == 0);
	assert(copy.av_len == 7);
	assert(copy.av_val != NULL);
	assert(copy.av_val != av.av_val);
	assert(copy.av_val[0] == 0x00);
	assert(copy.av_val[1] == 0x01);
	assert(copy.av_val[2] == 0x02);
	assert(copy.av_val[3] == 0x03);
	assert(copy.av_val[4] == 0x04);
	assert(copy.av_val[5] == 0x05);
	assert(copy.av_val[6] == 0x06);
	assert(copy.av_val[7] == 0x00);

	simplescim_arbval_destructor(&copy);
	simplescim_arbval_destructor(&av);
}

static void test_simplescim_arbval_constructor_string()
{
	struct simplescim_arbval av;
	int err;

	/* Test constructing with str = NULL */

	err = simplescim_arbval_constructor_string(&av, NULL);

	assert(err == 0);
	assert(av.av_len == 0);
	assert(av.av_val == NULL);

	simplescim_arbval_destructor(&av);

	/* Test constructing with str = "hej" */

	err = simplescim_arbval_constructor_string(&av, "hej");

	assert(err == 0);
	assert(av.av_len == 3);
	assert(av.av_val != NULL);
	assert(av.av_val[0] == 'h');
	assert(av.av_val[1] == 'e');
	assert(av.av_val[2] == 'j');
	assert(av.av_val[3] == '\0');

	simplescim_arbval_destructor(&av);

	/* Test constructing with str = "hejhej\x00hejhej" */

	err = simplescim_arbval_constructor_string(&av, "hejhej\x00hejhej");

	assert(err == 0);
	assert(av.av_len == 6);
	assert(av.av_val != NULL);
	assert(av.av_val[0] == 'h');
	assert(av.av_val[1] == 'e');
	assert(av.av_val[2] == 'j');
	assert(av.av_val[3] == 'h');
	assert(av.av_val[4] == 'e');
	assert(av.av_val[5] == 'j');
	assert(av.av_val[6] == '\0');

	simplescim_arbval_destructor(&av);

	/* Test constructing with str = "\x00\x01\x02\x03\x04\x05\x06\x07" */

	err = simplescim_arbval_constructor_string(&av, "\x00\x01\x02\x03\x04\x05\x06\x07");

	assert(err == 0);
	assert(av.av_len == 0);
	assert(av.av_val != NULL);
	assert(av.av_val[0] == 0x00);

	simplescim_arbval_destructor(&av);
}

static void test_simplescim_arbval_new()
{
	struct simplescim_arbval *av;

	/* Test constructing with len = 0, val = NULL */

	av = simplescim_arbval_new(0, NULL);

	assert(av != NULL);
	assert(av->av_len == 0);
	assert(av->av_val == NULL);

	simplescim_arbval_delete(av);

	/* Test constructing with len = 4711, val = NULL */

	av = simplescim_arbval_new(4711, NULL);

	assert(av != NULL);
	assert(av->av_len == 4711);
	assert(av->av_val == NULL);

	simplescim_arbval_delete(av);

	/* Test constructing with len = 3, val = "hej" */

	av = simplescim_arbval_new(3, (const uint8_t *)"hej");

	assert(av != NULL);
	assert(av->av_len == 3);
	assert(av->av_val != NULL);
	assert(av->av_val[0] == 'h');
	assert(av->av_val[1] == 'e');
	assert(av->av_val[2] == 'j');
	assert(av->av_val[3] == '\0');

	simplescim_arbval_delete(av);

	/* Test constructing with len = 7, val = "\x00\x01\x02\x03\x04\x05\x06\x07" */

	av = simplescim_arbval_new(7, (const uint8_t *)"\x00\x01\x02\x03\x04\x05\x06\x07");

	assert(av != NULL);
	assert(av->av_len == 7);
	assert(av->av_val != NULL);
	assert(av->av_val[0] == 0x00);
	assert(av->av_val[1] == 0x01);
	assert(av->av_val[2] == 0x02);
	assert(av->av_val[3] == 0x03);
	assert(av->av_val[4] == 0x04);
	assert(av->av_val[5] == 0x05);
	assert(av->av_val[6] == 0x06);
	assert(av->av_val[7] == 0x00);

	simplescim_arbval_delete(av);
}

static void test_simplescim_arbval_copy()
{
	struct simplescim_arbval *av, *copy;

	/* Test constructing with len = 0, val = NULL */

	av = simplescim_arbval_new(0, NULL);
	copy = simplescim_arbval_copy(av);

	assert(copy != NULL);
	assert(copy->av_len == 0);
	assert(copy->av_val == NULL);

	simplescim_arbval_delete(copy);
	simplescim_arbval_delete(av);

	/* Test constructing with len = 4711, val = NULL */

	av = simplescim_arbval_new(4711, NULL);
	copy = simplescim_arbval_copy(av);

	assert(copy != NULL);
	assert(copy->av_len == 4711);
	assert(copy->av_val == NULL);

	simplescim_arbval_delete(copy);
	simplescim_arbval_delete(av);

	/* Test constructing with len = 3, val = "hej" */

	av = simplescim_arbval_new(3, (const uint8_t *)"hej");
	copy = simplescim_arbval_copy(av);

	assert(copy != NULL);
	assert(copy->av_len == 3);
	assert(copy->av_val != NULL);
	assert(copy->av_val != av->av_val);
	assert(copy->av_val[0] == 'h');
	assert(copy->av_val[1] == 'e');
	assert(copy->av_val[2] == 'j');
	assert(copy->av_val[3] == '\0');

	simplescim_arbval_delete(copy);
	simplescim_arbval_delete(av);

	/* Test constructing with len = 7, val = "\x00\x01\x02\x03\x04\x05\x06\x07" */

	av = simplescim_arbval_new(7, (const uint8_t *)"\x00\x01\x02\x03\x04\x05\x06\x07");
	copy = simplescim_arbval_copy(av);

	assert(copy != NULL);
	assert(copy->av_len == 7);
	assert(copy->av_val != NULL);
	assert(copy->av_val != av->av_val);
	assert(copy->av_val[0] == 0x00);
	assert(copy->av_val[1] == 0x01);
	assert(copy->av_val[2] == 0x02);
	assert(copy->av_val[3] == 0x03);
	assert(copy->av_val[4] == 0x04);
	assert(copy->av_val[5] == 0x05);
	assert(copy->av_val[6] == 0x06);
	assert(copy->av_val[7] == 0x00);

	simplescim_arbval_delete(copy);
	simplescim_arbval_delete(av);
}

static void test_simplescim_arbval_string()
{
	struct simplescim_arbval *av;

	/* Test constructing with str = NULL */

	av = simplescim_arbval_string(NULL);

	assert(av != NULL);
	assert(av->av_len == 0);
	assert(av->av_val == NULL);

	simplescim_arbval_delete(av);

	/* Test constructing with str = "hej" */

	av = simplescim_arbval_string("hej");

	assert(av != NULL);
	assert(av->av_len == 3);
	assert(av->av_val != NULL);
	assert(av->av_val[0] == 'h');
	assert(av->av_val[1] == 'e');
	assert(av->av_val[2] == 'j');
	assert(av->av_val[3] == '\0');

	simplescim_arbval_delete(av);

	/* Test constructing with str = "hejhej\x00hejhej" */

	av = simplescim_arbval_string("hejhej\x00hejhej");

	assert(av != NULL);
	assert(av->av_len == 6);
	assert(av->av_val != NULL);
	assert(av->av_val[0] == 'h');
	assert(av->av_val[1] == 'e');
	assert(av->av_val[2] == 'j');
	assert(av->av_val[3] == 'h');
	assert(av->av_val[4] == 'e');
	assert(av->av_val[5] == 'j');
	assert(av->av_val[6] == '\0');

	simplescim_arbval_delete(av);

	/* Test constructing with str = "\x00\x01\x02\x03\x04\x05\x06\x07" */

	av = simplescim_arbval_string("\x00\x01\x02\x03\x04\x05\x06\x07");

	assert(av != NULL);
	assert(av->av_len == 0);
	assert(av->av_val != NULL);
	assert(av->av_val[0] == 0x00);

	simplescim_arbval_delete(av);
}

static void test_simplescim_arbval_eq()
{
	struct simplescim_arbval *av1, *av2;

	/* Test with non-allocated NULL */

	assert(simplescim_arbval_eq(NULL, NULL) == 1);
	assert(simplescim_arbval_eq(NULL, (struct simplescim_arbval *)1) == 0);
	assert(simplescim_arbval_eq((struct simplescim_arbval *)1, NULL) == 0);

	/* Test with allocated NULL */

	av1 = simplescim_arbval_new(0, NULL);
	av2 = simplescim_arbval_new(0, NULL);

	assert(simplescim_arbval_eq(av1, av2) == 1);
	assert(simplescim_arbval_eq(av2, av1) == 1);

	simplescim_arbval_delete(av2);
	simplescim_arbval_delete(av1);

	/* Test with allocated NULL of different lengths */

	av1 = simplescim_arbval_new(0, NULL);
	av2 = simplescim_arbval_new(4711, NULL);

	assert(simplescim_arbval_eq(av1, av2) == 0);
	assert(simplescim_arbval_eq(av2, av1) == 0);

	simplescim_arbval_delete(av2);
	simplescim_arbval_delete(av1);

	/* Test with equal strings */

	av1 = simplescim_arbval_new(3, (const uint8_t *)"hej");
	av2 = simplescim_arbval_new(3, (const uint8_t *)"hejhej");

	assert(simplescim_arbval_eq(av1, av2) == 1);
	assert(simplescim_arbval_eq(av2, av1) == 1);

	simplescim_arbval_delete(av2);
	simplescim_arbval_delete(av1);

	/* Test with almost equal strings */

	av1 = simplescim_arbval_new(3, (const uint8_t *)"hej");
	av2 = simplescim_arbval_new(3, (const uint8_t *)"heJ");

	assert(simplescim_arbval_eq(av1, av2) == 0);
	assert(simplescim_arbval_eq(av2, av1) == 0);

	simplescim_arbval_delete(av2);
	simplescim_arbval_delete(av1);

	/* Test with equal binary strings */

	av1 = simplescim_arbval_new(7, (const uint8_t *)"\x00\x01\x02\x03\x04\x05\x06\x07");
	av2 = simplescim_arbval_new(7, (const uint8_t *)"\x00\x01\x02\x03\x04\x05\x06\x07");

	assert(simplescim_arbval_eq(av1, av2) == 1);
	assert(simplescim_arbval_eq(av2, av1) == 1);

	simplescim_arbval_delete(av2);
	simplescim_arbval_delete(av1);

	/* Test with different binary strings */

	av1 = simplescim_arbval_new(7, (const uint8_t *)"\x00\x01\x02\x03\x04\x05\x06\x07");
	av2 = simplescim_arbval_new(7, (const uint8_t *)"\x00\x01\x02\x03\x04\x04\x06\x07");

	assert(simplescim_arbval_eq(av1, av2) == 0);
	assert(simplescim_arbval_eq(av2, av1) == 0);

	simplescim_arbval_delete(av2);
	simplescim_arbval_delete(av1);
}

int main()
{
	test_simplescim_arbval_constructor();
	test_simplescim_arbval_constructor_copy();
	test_simplescim_arbval_constructor_string();
	test_simplescim_arbval_new();
	test_simplescim_arbval_copy();
	test_simplescim_arbval_string();
	test_simplescim_arbval_eq();

	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

	return 0;
}
