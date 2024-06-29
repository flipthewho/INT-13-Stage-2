#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#define PROC_FILE "/proc/proc_dir/proc_file"

void usage(const char *prog_name) {
    fprintf(stderr, "usage: %s -d | -r | -w\n", prog_name);
    fprintf(stderr, "  -d   delete the secret from memory\n");
    fprintf(stderr, "  -r   read the secret from memory\n");
    fprintf(stderr, "  -w   write the secret to memory from stdin\n");
}

int main(int argc, char *argv[]) {
    
    int opt;
    FILE *fp;
    char buffer[1024];
    size_t n;

    while ((opt = getopt(argc, argv, "drw")) != -1) {
        switch (opt) {
            case 'd':
                fp = fopen(PROC_FILE, "w");
                if (fp == NULL) {
                    perror("unable to open file for writing");
                    return 1;
                }
                if (fwrite("", sizeof(char), 0, fp) != 0) {
                    perror("unable to write to file");
                    fclose(fp);
                    return 1;
                }
                fclose(fp);
                printf("Secret deleted from memory.\n");
                return 0;

            case 'r':
                fp = fopen(PROC_FILE, "r");
                if (fp == NULL) {
                    perror("unable to open file for reading");
                    return 1;
                }
                if (fgets(buffer, sizeof(buffer), fp) == NULL) {
                    perror("unable to read from file");
                    fclose(fp);
                    return 1;
                }
                printf("Read from file: %s\n", buffer);
                fclose(fp);
                return 0;

            case 'w':
                n = fread(buffer, sizeof(char), sizeof(buffer) - 1, stdin);
                if (n == 0) {
                    fprintf(stderr, "unable to read from stdin\n");
                    return 1;
                }
                buffer[n] = '\0'; 
                fp = fopen(PROC_FILE, "w");
                if (fp == NULL) {
                    perror("unable to open file for writing");
                    return 1;
                }
                if (fwrite(buffer, sizeof(char), n, fp) != n) {
                    perror("unable to write to file");
                    fclose(fp);
                    return 1;
                }
                fclose(fp);
                printf("wrote your secret.\n");
                return 0;

            default:
                usage(argv[0]);
                return 1;
        }
    }

    usage(argv[0]);
    return 1;
}