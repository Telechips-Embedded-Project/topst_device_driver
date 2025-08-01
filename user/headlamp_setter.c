#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#define DEVICE_PATH "/dev/headlamp_dev"
#define HEADLAMP_MAGIC 'H'
#define HEADLAMP_SET_STATE _IOW(HEADLAMP_MAGIC, 0, int)
#define HEADLAMP_GET_STATE _IOR(HEADLAMP_MAGIC, 1, int)

int main(int argc, char *argv[]) {
    int fd;
    int state;
    int ret;

    if (argc != 2) {
        printf("Usage: %s <0|1>\n", argv[0]);
        printf("  0: Turn off headlamp\n");
        printf("  1: Turn on headlamp\n");
        return 1;
    }

    state = atoi(argv[1]);
    if (state != 0 && state != 1) {
        printf("Invalid argument. Use 0 or 1.\n");
        return 1;
    }

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Open device failed");
        return 1;
    }

    // Set headlamp state
    ret = ioctl(fd, HEADLAMP_SET_STATE, &state);
    if (ret < 0) {
        perror("ioctl HEADLAMP_SET_STATE failed");
        close(fd);
        return 1;
    }
    printf("Headlamp set to %s\n", state ? "ON" : "OFF");

    // Get headlamp state
    ret = ioctl(fd, HEADLAMP_GET_STATE, &state);
    if (ret < 0) {
        perror("ioctl HEADLAMP_GET_STATE failed");
        close(fd);
        return 1;
    }
    printf("Headlamp current state: %s\n", state ? "ON" : "OFF");

    close(fd);
    return 0;
}
