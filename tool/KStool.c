#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#define PROCFS_PATH "/proc/secrets"
#define MAX_SECRET_SIZE 1024

void usage(const char *prog_name) {
    fprintf(stderr, "usage: %s -d | -r | -w\n", prog_name);
    fprintf(stderr, "  -d   delete the secret from memory\n");
    fprintf(stderr, "  -r   read the secret from memory\n");
    fprintf(stderr, "  -w   write the secret to memory from stdin\n");
}

void write_secret() {
    char secret[MAX_SECRET_SIZE];
    printf("Enter the secret: ");
    if (!fgets(secret, MAX_SECRET_SIZE, stdin)) {
        perror("Failed to read secret");
        exit(EXIT_FAILURE);
    }
    secret[strcspn(secret, "\n")] = 0;  // Remove newline character

    int fd = open(PROCFS_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open proc file");
        exit(EXIT_FAILURE);
    }

    if (write(fd, secret, strlen(secret)) < 0) {
        perror("Failed to write secret");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}

void read_secret() {
    int fd = open(PROCFS_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open proc file");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_SECRET_SIZE];
    ssize_t bytes_read = read(fd, buffer, MAX_SECRET_SIZE);
    if (bytes_read < 0) {
        perror("Failed to read secret");
        close(fd);
        exit(EXIT_FAILURE);
    }

    buffer[bytes_read] = '\0';
    printf("Secret: %s\n", buffer);

    close(fd);
}

void delete_secret() {
    int fd = open(PROCFS_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open proc file");
        exit(EXIT_FAILURE);
    }

    if (write(fd, "", 0) < 0) {
        perror("Failed to delete secret");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int opt;
    while ((opt = getopt(argc, argv, "drw")) != -1) {
        switch (opt) {
            case 'd':
                delete_secret();
                break;
            case 'r':
                read_secret();
                break;
            case 'w':
                write_secret();
                break;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
