#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "bite.h"

const char *bite_test_name    = "";
const char *bite_test_variant = "";

void bite_test_begin(const char *name, const char *variant)
{
	bite_test_name    = name;
	bite_test_variant = variant;
}

void bite_test_assert(bool assert, const char *str, int line)
{
	if (!assert) {
		printf(BITE_RED"[Assertion at line: %i]"
		       BITE_WHITE" %s\n"BITE_CRST,
		       line, str);
		printf(BITE_WHITE"%s"BITE_CRST" (%s): "
		       BITE_RED"FAILED\n\n\n"BITE_CRST,
		       bite_test_name, bite_test_variant);

		fflush(0);
		exit(1);
	}
}

#define BITE_TEST_ASSERT(a) bite_test_assert((a), (#a), __LINE__)

void bite_test_end()
{
	printf(BITE_WHITE"%s"BITE_CRST" (%s): "
	       BITE_GREEN"PASSED\n\n\n"BITE_CRST,
	       bite_test_name, bite_test_variant);
}

void bite_test_print_buf(uint8_t *buf)
{
	int i;
	int j;

	for (i = 0; i < 8; i++) {
		printf(BITE_YELLOW"0x%02X     ", buf[i]);
	}
	printf("\n");

	for (j = 0; j < 8; j++) {
		for (i = 8 - 1; i >= 0; i--) {
			printf((buf[j] & (1U << i)) ? "1" : "0");
		}
		printf(" ");
	}
	printf(BITE_CRST"\n\n");

	fflush(0);
}

void bite_test_highlight_section(int line)
{
	printf("----------------------------------------"
	       "---------------------------------------\n");
	printf(BITE_INFO"[Line: %i]"BITE_CRST" %s (%s)\n",
	       line, bite_test_name, bite_test_variant);
	printf("----------------------------------------"
	       "---------------------------------------\n");
}

#define BITE_TEST_HIGHLIGHT_SECTION bite_test_highlight_section(__LINE__)

/* This is a brute force test that checks various ranges of input parameters */
void bite_test_brute(enum bite_order order, bool verbose)
{
	struct bite bite;
	int i, j;

	/* Buffers will be compared and should remain intact 
	 * after all bitE operations! */
	uint8_t buf_a[8] = {0x12, 0x34, 0x56, 0x78, 0x89, 0x9A, 0xAB, 0xBC};
	uint8_t buf_b[8] = {0x12, 0x34, 0x56, 0x78, 0x89, 0x9A, 0xAB, 0xBC};
	
	/* Temporary buffer */
	uint8_t buf_t[8];

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;

	/* Limit BITE bufer with 4 bytes to test buffer overflow debug! */
	bite_init(&bite);
	bite_set_buf(&bite, buf_a, 4);
	if (verbose == false) {
#ifdef BITE_DEBUG
		bite.debug = false;
#endif
	}

	for (i = 0; i < 32; i++) {
		for (j = 1; j <= 32; j++) {
			if (verbose == true) {
				BITE_TEST_HIGHLIGHT_SECTION;
			}

			/* Read values from buffer buf_a and store to buf_t */
			bite_begin(&bite, i, j, order);
			buf_t[0] = bite_read(&bite);
			buf_t[1] = bite_read(&bite);
			buf_t[2] = bite_read(&bite);
			buf_t[3] = bite_read(&bite);
			bite_end(&bite);
			
			if (verbose == true) {
				printf("buf_a:\n");
				bite_test_print_buf(buf_a);
				printf("buf_b:\n");
				bite_test_print_buf(buf_b);
			}

			/* Check if buffers still intact */
			BITE_TEST_ASSERT(!memcmp(buf_a, buf_b, 8));

			if (verbose == true) {
				BITE_TEST_HIGHLIGHT_SECTION;
			}

			/* Override buf_a values */
			bite_begin(&bite, i, j, order);
			bite_write(&bite, 0xEF);
			bite_write(&bite, 0xFF);
			bite_write(&bite, 0xCD);
			bite_write(&bite, 0xAB);
			bite_end(&bite);

			if (verbose == true) {
				printf("buf_a overriden:\n");
				bite_test_print_buf(buf_a);
			}

			/* Rewrite buf_a with the previous values */
			bite_begin(&bite, i, j, order);
			bite_write(&bite, buf_t[0]);
			bite_write(&bite, buf_t[1]);
			bite_write(&bite, buf_t[2]);
			bite_write(&bite, buf_t[3]);
			bite_end(&bite);
		}
	}
}

void bite_test_use_cases_be()
{
	struct bite bite;
	uint8_t buf[8];

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;

	bite_init(&bite);
	bite_set_buf(&bite, buf, 8);

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;	
	memset((void *)buf, 0, 8);

	/* Test simple write case (write 1 byte) into first `buf` byte
	 * For BIG ENDIAN mode bits are numbered from MSB(7) to LSB(0) */
	bite_begin(&bite, 7, 8, BITE_ORDER_BIG_ENDIAN);

	/* Write operation ALWAYS accepts most significant bits first,
	 * regardless of bitE setup */
	bite_write(&bite, 0xAB);
	bite_end(&bite);

	/* Print `buf` contents both in hexadecimal and binary */
	bite_test_print_buf(buf);

	/* Read previously written byte and check its value */
	bite_begin(&bite, 7, 8, BITE_ORDER_BIG_ENDIAN);
	BITE_TEST_ASSERT(bite_read(&bite) == 0xAB);
	bite_end(&bite);

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;
	memset((void *)buf, 0, 8);

	/* Test multibyte case (write multiple bytes into `buf`)
	 * We only write 20 bits, so the last byte will write 4 LSB bits */
	bite_begin(&bite, 7, 20, BITE_ORDER_BIG_ENDIAN);

	bite_write(&bite, 0xAB);
	bite_write(&bite, 0xCD);
	bite_write(&bite, 0xEF); /* MSB part 0xE* will be masked! */
	bite_end(&bite);

	bite_test_print_buf(buf);

	bite_begin(&bite, 7, 20, BITE_ORDER_BIG_ENDIAN);
	BITE_TEST_ASSERT(bite_read(&bite) == 0xAB);
	BITE_TEST_ASSERT(bite_read(&bite) == 0xCD);
	BITE_TEST_ASSERT(bite_read(&bite) == 0x0F); /* MSB is masked */
	bite_end(&bite);

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;
	memset((void *)buf, 0, 8);

	/* Test with 3 bit offset from MSB (7-3=4) 
	 * Data will be fragmented inside `buf` in two bytes */
	bite_begin(&bite, 4, 8, BITE_ORDER_BIG_ENDIAN);
	bite_write(&bite, 0xAB);
	bite_end(&bite);

	bite_test_print_buf(buf);

	bite_begin(&bite, 4, 8, BITE_ORDER_BIG_ENDIAN);
	BITE_TEST_ASSERT(bite_read(&bite) == 0xAB);
	bite_end(&bite);

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;
	memset((void *)buf, 0, 8);

	/* Test with 3 bit offset from MSB (7-3=4) 
	 * Write bitfield of 3 bits */
	bite_begin(&bite, 4, 3, BITE_ORDER_BIG_ENDIAN);
	bite_write(&bite, 0xFF); /* MSB will be masked for remaining bits */
	bite_end(&bite);

	bite_test_print_buf(buf);

	bite_begin(&bite, 4, 3, BITE_ORDER_BIG_ENDIAN);
	BITE_TEST_ASSERT(bite_read(&bite) == 0x07); /* MSB is masked */
	bite_end(&bite);

	/* TODO more use cases */
}

void bite_test_use_cases_le()
{
	struct bite bite;
	uint8_t buf[8];

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;

	bite_init(&bite);
	bite_set_buf(&bite, buf, 8);

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;	
	memset((void *)buf, 0, 8);

	/* Test simple write case (write 1 byte) into first `buf` byte
	 * For LITTLE ENDIAN mode bits are numbered from LSB(0) to MSB(7) */
	bite_begin(&bite, 0, 8, BITE_ORDER_LIL_ENDIAN);

	/* Write operation ALWAYS accepts most significant bits first,
	 * regardless of bitE setup */
	bite_write(&bite, 0xAB);
	bite_end(&bite);

	/* Print `buf` contents both in hexadecimal and binary */
	bite_test_print_buf(buf);

	/* Read previously written byte and check its value */
	bite_begin(&bite, 0, 8, BITE_ORDER_LIL_ENDIAN);
	BITE_TEST_ASSERT(bite_read(&bite) == 0xAB);
	bite_end(&bite);

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;
	memset((void *)buf, 0, 8);

	/* Test multibyte case (write multiple bytes into `buf`)
	 * We only write 20 bits, so the last byte will write 4 LSB bits */
	bite_begin(&bite, 0, 20, BITE_ORDER_LIL_ENDIAN);

	bite_write(&bite, 0xAB);
	bite_write(&bite, 0xCD);
	bite_write(&bite, 0xEF); /* MSB part 0xE* will be masked! */
	bite_end(&bite);

	bite_test_print_buf(buf);

	bite_begin(&bite, 0, 20, BITE_ORDER_LIL_ENDIAN);
	BITE_TEST_ASSERT(bite_read(&bite) == 0xAB);
	BITE_TEST_ASSERT(bite_read(&bite) == 0xCD);
	BITE_TEST_ASSERT(bite_read(&bite) == 0x0F); /* MSB is masked */
	bite_end(&bite);

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;
	memset((void *)buf, 0, 8);

	/* Test with 3 bit offset from LSB 
	 * Data will be fragmented inside `buf` in two bytes */
	bite_begin(&bite, 3, 8, BITE_ORDER_LIL_ENDIAN);
	bite_write(&bite, 0xAB);
	bite_end(&bite);

	bite_test_print_buf(buf);

	bite_begin(&bite, 3, 8, BITE_ORDER_LIL_ENDIAN);
	BITE_TEST_ASSERT(bite_read(&bite) == 0xAB);
	bite_end(&bite);

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;
	memset((void *)buf, 0, 8);

	/* Test with 3 bit offset from LSB 
	 * Write bitfield of 3 bits */
	bite_begin(&bite, 3, 3, BITE_ORDER_LIL_ENDIAN);
	bite_write(&bite, 0xFF); /* MSB will be masked for remaining bits */
	bite_end(&bite);

	bite_test_print_buf(buf);

	bite_begin(&bite, 3, 3, BITE_ORDER_LIL_ENDIAN);
	BITE_TEST_ASSERT(bite_read(&bite) == 0x07); /* MSB is masked */
	bite_end(&bite);

	/* TODO more use cases */
}

void bite_test_special()
{
	struct bite bite;
	uint8_t buf[8] = {0};

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;

	/* Check for buffer overflow false positives */
	bite_init(&bite);
	bite_set_buf(&bite, buf, 1);
	bite_begin(&bite, 7, 1, BITE_ORDER_LIL_ENDIAN);
	bite_write(&bite, 0xFF);
	bite_end(&bite);

#ifndef   BITE_UNSAFE_OPTIMIZATIONS
	BITE_TEST_ASSERT(bite_get_flags(&bite) == BITE_FLAG_NONE);
#endif /* BITE_UNSAFE_OPTIMIZATIONS */

	/* Check for buffer overflow false positives */
	bite_init(&bite);
	bite_set_buf(&bite, buf, 1);
	bite_begin(&bite, 0, 2, BITE_ORDER_BIG_ENDIAN);
	bite_write(&bite, 0xFF);
	bite_end(&bite);

#ifndef   BITE_UNSAFE_OPTIMIZATIONS
	BITE_TEST_ASSERT(bite_get_flags(&bite) ==
			 (BITE_FLAG_MEMORY | BITE_FLAG_UNDERFLOW));
#endif /* BITE_UNSAFE_OPTIMIZATIONS */

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;
	memset((void *)buf, 0, 8);

	bite_init(&bite);
	bite_set_buf(&bite, buf, 8);

	/* Test 16bit write/read */
	bite_begin(&bite, 7, 16, BITE_ORDER_BIG_ENDIAN);
	bite_write_16(&bite, -1);
	bite_end(&bite);

	bite_test_print_buf(buf);

	bite_begin(&bite, 7, 16, BITE_ORDER_BIG_ENDIAN);
	BITE_TEST_ASSERT(bite_read_u16(&bite) == (uint16_t)-1);
	bite_end(&bite);

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;
	memset((void *)buf, 0, 8);

	bite_init(&bite);
	bite_set_buf(&bite, buf, 8);

	/* Test 16bit write/read */
	bite_begin(&bite, 7, 10, BITE_ORDER_BIG_ENDIAN);
	bite_write_16(&bite, 0x3CD);
	bite_end(&bite);

	bite_test_print_buf(buf);

	bite_begin(&bite, 7, 10, BITE_ORDER_BIG_ENDIAN);
	BITE_TEST_ASSERT(bite_read_u16(&bite) == 0x3CD);
	bite_end(&bite);

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;
	memset((void *)buf, 0, 8);

	bite_init(&bite);
	bite_set_buf(&bite, buf, 8);

	/* Test 16bit write/read */
	bite_begin(&bite, 7, 10, BITE_ORDER_LIL_ENDIAN);
	bite_write_16(&bite, 0x3CD);
	bite_end(&bite);

	bite_test_print_buf(buf);

	bite_begin(&bite, 7, 10, BITE_ORDER_LIL_ENDIAN);
	BITE_TEST_ASSERT(bite_read_u16(&bite) == 0x3CD);
	bite_end(&bite);

	/*********************************************************************/
	BITE_TEST_HIGHLIGHT_SECTION;
	memset((void *)buf, 0, 8);

	bite_init(&bite);
	bite_set_buf(&bite, buf, 8);

	/* Test correct byte representation in frame memory */
	/* SG_ LB_Remain_Capacity :
	 * 	7|10@0+ (1,0) [0|500] "gids" Vector__XXX */
	bite_begin(&bite, 7U, 10U, BITE_ORDER_DBC_0);
	bite_write_16(&bite, 125U);
	bite_end(&bite);

	/* SG_ LB_New_Full_Capacity :
	 * 	13|10@0+ (80,250) [20000|24000] "wh" Vector__XXX */
	bite_begin(&bite, 13U, 10U, BITE_ORDER_DBC_0);
	bite_write_16(&bite, 296U);
	bite_end(&bite);

	bite_test_print_buf(buf);
	/* Assert correct frame data representation */
	assert(buf[0] == 0x1FU && buf[1] == 0x52U && buf[2] == 0x80U);
}

int main()
{
	/* Set this flag to true if `bite_test_brute` has failed */
	bool verbose = false;

	bite_test_begin("bite_test_brute", "BITE_ORDER_BIG_ENDIAN");
	bite_test_brute(BITE_ORDER_BIG_ENDIAN, verbose);
	bite_test_end();

	bite_test_begin("bite_test_brute", "BITE_ORDER_LIL_ENDIAN");
	bite_test_brute(BITE_ORDER_LIL_ENDIAN, verbose);
	bite_test_end();

	bite_test_begin("bite_test_use_cases_be", "");
	bite_test_use_cases_be();
	bite_test_end();

	bite_test_begin("bite_test_use_cases_le", "");
	bite_test_use_cases_le();
	bite_test_end();

	bite_test_begin("bite_test_special", "");
	bite_test_special();
	bite_test_end();

	return 0;
}
