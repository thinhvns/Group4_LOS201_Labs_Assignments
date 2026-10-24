#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_RECORDS 100
#define MSG_SIZE 256

typedef struct {
    int id;
    char *name;
    int *scores;
} Record;

/* Create a record */
Record *create_record(int id, const char *name, int num_scores) {

    Record *r = malloc(sizeof(Record));

    if (r == NULL)
        return NULL;

    r->id = id;

    r->name = malloc(strlen(name) + 1);

    if (r->name == NULL) {
        free(r);
        return NULL;
    }

    strcpy(r->name, name);

    r->scores = malloc(sizeof(int) * num_scores);

    if (r->scores == NULL) {
        free(r->name);
        free(r);
        return NULL;
    }

    for (int i = 0; i < num_scores; i++) {
        r->scores[i] = id * 10 + i;
    }

    return r;
}

/* FIX 1: Free all allocated memory */
void delete_record(Record *r) {

    if (r == NULL)
        return;

    free(r->name);
    free(r->scores);
    free(r);
}

/* FIX 2: Free buffer after processing */
void process_messages(int count) {

    for (int i = 0; i < count; i++) {

        char *buf = malloc(MSG_SIZE);

        if (buf == NULL) {
            printf("Memory allocation failed!\n");
            return;
        }

        snprintf(
            buf,
            MSG_SIZE,
            "Message #%d: data payload here",
            i
        );

        if (i % 10 == 0) {
            printf("Processed: %s\n", buf);
        }

        free(buf);
    }
}

int main(void) {

    printf("=== Leaky App Fixed - LAB-03 ===\n");

    printf("[1] Creating %d records...\n", NUM_RECORDS);

    Record **records =
        calloc(NUM_RECORDS, sizeof(Record *));

    if (records == NULL) {
        return 1;
    }

    for (int i = 0; i < NUM_RECORDS; i++) {

        records[i] = create_record(
            i,
            "EmbeddedLinuxStudent",
            5
        );

        if (records[i] == NULL) {
            printf("Record allocation failed!\n");

            for (int j = 0; j < i; j++) {
                delete_record(records[j]);
            }

            free(records);
            return 1;
        }
    }

    printf("[2] Deleting records...\n");

    for (int i = 0; i < NUM_RECORDS; i++) {
        delete_record(records[i]);
    }

    free(records);

    printf("[3] Processing messages...\n");

    process_messages(50);

    printf("Program finished successfully!\n");

    return 0;
}
