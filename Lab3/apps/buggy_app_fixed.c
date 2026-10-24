#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * LAB-03: Embedded Linux Systems
 * Part A: Remote Debugging with GDB
 *
 * Fixed Application
 *
 * Bug 1: Array Out-of-Bounds
 * Bug 2: Use-after-free
 * Bug 3: Integer Overflow
 */

/* =========================================
 * BUG 1 FIX: Array Out-of-Bounds
 * ========================================= */

void process_data(int *data, int len) {
    int sum = 0;

    /* Fixed: i < len instead of i <= len */
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }

    printf("Sum = %d\n", sum);
}


/* =========================================
 * BUG 2 FIX: Use-after-free
 * ========================================= */

char *create_message(const char *text) {

    char *buf = malloc(strlen(text) + 1);

    /* Check memory allocation */
    if (buf == NULL) {
        printf("Memory allocation failed!\n");
        return NULL;
    }

    strcpy(buf, text);

    /* Fixed: Do not free memory here */
    return buf;
}


/* =========================================
 * BUG 3 FIX: Integer Overflow
 * ========================================= */

long long calculate(int a, int b) {

    /* Fixed: Use 64-bit integer */
    long long result = (long long)a * b;

    printf("calculate(%d, %d) = %lld\n",
           a, b, result);

    return result;
}


/* =========================================
 * MAIN FUNCTION
 * ========================================= */

int main(void) {

    printf("=== Buggy App Fixed - LAB-03 ===\n");


    /* =====================================
     * TEST 1: Array Processing
     * ===================================== */

    int arr[5] = {10, 20, 30, 40, 50};

    printf("[TEST1] Processing array\n");

    process_data(arr, 5);


    /* =====================================
     * TEST 2: String Message
     * ===================================== */

    printf("[TEST2] Creating message\n");

    char *msg = create_message("Hello Embedded Linux");

    if (msg != NULL) {

        printf("Message: %s\n", msg);

        /* Free memory after using it */
        free(msg);

        msg = NULL;
    }


    /* =====================================
     * TEST 3: Integer Calculation
     * ===================================== */

    printf("[TEST3] Integer calculation\n");

    calculate(100000, 100000);


    /* =====================================
     * PROGRAM COMPLETED
     * ===================================== */

    printf("Done.\n");

    return 0;
}
