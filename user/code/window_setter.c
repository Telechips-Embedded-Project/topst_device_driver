#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/window_dev"

#define WINDOW_MAGIC 'M'
#define WINDOW_SET_STATE _IOW(WINDOW_MAGIC, 0, int)
#define WINDOW_GET_STATE _IOR(WINDOW_MAGIC, 1, int)

int main(int argc, char *argv[])
{
    int fd;
    int cmd;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s [stop | open | close | get]\n", argv[0]);
        return 1;
    }

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (strcmp(argv[1], "stop") == 0) {
        cmd = 0;
        ioctl(fd, WINDOW_SET_STATE, &cmd);
        printf("Motor stopped\n");
    } else if (strcmp(argv[1], "open") == 0) {
        cmd = 1;
        ioctl(fd, WINDOW_SET_STATE, &cmd);
        printf("Motor opening (UP)\n");
    } else if (strcmp(argv[1], "close") == 0) {
        cmd = 2;
        ioctl(fd, WINDOW_SET_STATE, &cmd);
        printf("Motor closing (DOWN)\n");
    } else if (strcmp(argv[1], "get") == 0) {
        int state;
        ioctl(fd, WINDOW_GET_STATE, &state);
        switch (state) {
            case 0: printf("Current State: STOPPED\n"); break;
            case 1: printf("Current State: OPENING\n"); break;
            case 2: printf("Current State: CLOSING\n"); break;
            default: printf("Unknown state: %d\n", state);
        }
    } else {
        fprintf(stderr, "Invalid command: %s\n", argv[1]);
    }

    close(fd);
    return 0;
}
