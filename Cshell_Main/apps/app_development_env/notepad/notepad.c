#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINES 1000
#define MAX_LINE_LEN 2048

int main(void) {
    printf("Notepad V1 - for CShell\n");
    printf("Type your text. Commands:\n");
    printf("  :save <filename>   - save your text to a file\n");
    printf("  :exit               - quit without saving\n");
    printf("\n");

    // buffer to hold every line typed so far
    char lines[MAX_LINES][MAX_LINE_LEN];
    int line_count = 0;

    while (true) {
        printf(">> ");

        char input[MAX_LINE_LEN];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Notepad experienced an error...\n");
            break;
        }
        input[strcspn(input, "\n")] = '\0';

        // check for :save <filename>
        if (strncmp(input, ":save", 5) == 0) {
            // grab whatever comes after ":save "
            const char *filename = NULL;
            if (strlen(input) > 6) {
                filename = input + 6; // skip past ":save "
            }

            if (filename == NULL || filename[0] == '\0') {
                printf("Usage: :save <filename>\n");
                continue;
            }

            FILE *fp = fopen(filename, "w");
            if (fp == NULL) {
                printf("[!] Couldn't save to %s\n", filename);
                continue;
            }

            for (int i = 0; i < line_count; i++) {
                fprintf(fp, "%s\n", lines[i]); // always add a newline, even for blank lines
            }

            fclose(fp);
            printf("Saved %d line(s) to %s\n", line_count, filename);
            continue;
        }

        // check for :exit
        if (strcmp(input, ":exit") == 0) {
            printf("Exiting Notepad. Unsaved changes are lost.\n");
            break;
        }

        // otherwise, treat it as a line of text to store
        if (line_count >= MAX_LINES) {
            printf("[!] Max line limit reached (%d). Save and restart to continue.\n", MAX_LINES);
            continue;
        }

        strncpy(lines[line_count], input, MAX_LINE_LEN - 1);
        lines[line_count][MAX_LINE_LEN - 1] = '\0';
        line_count++;
    }

    return 0;
}