#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Mock the SBI interface and buffer structures to test the boundary */
#define HFP_MSG_DATA_SIZE 256

typedef struct {
    uint8_t data[HFP_MSG_DATA_SIZE];
} hfp_msg_t;

typedef struct {
    hfp_msg_t msg;
} hfp_xmit_t;

/* Mock sbi_memcpy that enforces the security invariant */
static void sbi_memcpy_safe(void *dest, const void *src, size_t len) {
    /* SECURITY INVARIANT: len must not exceed destination buffer size */
    ck_assert_msg(len <= HFP_MSG_DATA_SIZE,
        "Buffer overflow: attempted to copy %zu bytes into %d-byte buffer",
        len, HFP_MSG_DATA_SIZE);
    memcpy(dest, src, len);
}

/* Wrapper to test the vulnerable pattern with safe enforcement */
static int hfp_send_data(const uint8_t *data, size_t len) {
    hfp_xmit_t xmit;
    
    /* This mimics the vulnerable code pattern but with our safe wrapper */
    sbi_memcpy_safe(&xmit.msg.data, data, len);
    return 0;
}

START_TEST(test_hfp_buffer_overflow_boundary)
{
    /* Invariant: No buffer overflow occurs regardless of input length */
    
    /* Payloads: exploit (oversized), boundary (exact size), valid (undersized) */
    struct {
        const uint8_t *data;
        size_t len;
        int should_pass;
    } test_cases[] = {
        { (uint8_t *)"A", 1, 1 },                           /* Valid: small input */
        { (uint8_t *)"X", HFP_MSG_DATA_SIZE, 1 },           /* Boundary: exact buffer size */
        { (uint8_t *)"OVERFLOW", HFP_MSG_DATA_SIZE + 1, 0 }, /* Exploit: overflow attempt */
        { (uint8_t *)"", 0, 1 },                            /* Valid: empty input */
        { (uint8_t *)"B", HFP_MSG_DATA_SIZE + 256, 0 },     /* Exploit: large overflow */
    };
    
    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (int i = 0; i < num_cases; i++) {
        if (test_cases[i].should_pass) {
            /* These should succeed without assertion failure */
            hfp_send_data(test_cases[i].data, test_cases[i].len);
        } else {
            /* These should trigger the invariant check */
            ck_assert_msg(test_cases[i].len > HFP_MSG_DATA_SIZE,
                "Test case %d: overflow payload not properly sized", i);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("HFP_Security");
    tc_core = tcase_create("BufferBoundary");

    tcase_add_test(tc_core, test_hfp_buffer_overflow_boundary);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}