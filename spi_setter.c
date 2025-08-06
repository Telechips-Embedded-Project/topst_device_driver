// spi_setter.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/spi_fifo"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <color|brightness> <value>\n", argv[0]);
        return 1;
    }

    const char *cmd = argv[1];
    const char *val = argv[2];

    char msg[128];

    if (strcmp(cmd, "color") == 0) {
        snprintf(msg, sizeof(msg), "{\"command\":\"color\", \"value\":\"%s\"}", val);
    } else if (strcmp(cmd, "brightness") == 0) {
        snprintf(msg, sizeof(msg), "{\"command\":\"brightness\", \"value\":%s}", val);
    } else {
        fprintf(stderr, "Invalid command: %s\n", cmd);
        return 1;
    }

    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd < 0) {
        perror("open fifo");
        return 1;
    }
    ssize_t ret = write(fifo_fd, msg, strlen(msg));
    if (ret != (ssize_t)strlen(msg)) {
	    perror("fifo write failed");
    }
    close(fifo_fd);
    return 0;
}
