// ambient_setter.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/ambient_dev"

#define AMBIENT_MAGIC 'L'
#define AMBIENT_SET_MODE        _IOW(AMBIENT_MAGIC, 1, char *)
#define AMBIENT_SET_BRIGHTNESS  _IOW(AMBIENT_MAGIC, 3, int)

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s color <red|green|blue|yellow|cyan|magenta|white|rainbow>\n", argv[0]);
        fprintf(stderr, "  %s brightness <0~100>\n", argv[0]);
        return 1;
    }

    int dev_fd = open(DEVICE_PATH, O_WRONLY);
    if (dev_fd < 0) {
        perror("open device");
        return 1;
    }

    if (strcmp(argv[1], "color") == 0) {
        const char *mode = argv[2];
        if (ioctl(dev_fd, AMBIENT_SET_MODE, mode) < 0) {
            perror("ioctl set mode");
        } else {
            printf("Mode set to: %s\n", mode);
        }
    } else if (strcmp(argv[1], "brightness") == 0) {
        int value = atoi(argv[2]);
        if (value < 0) value = 0;
        if (value > 100) value = 100;
        if (ioctl(dev_fd, AMBIENT_SET_BRIGHTNESS, &value) < 0) {
            perror("ioctl set brightness");
        } else {
            printf("Brightness set to: %d\n", value);
        }
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
    }

    close(dev_fd);
    return 0;
}
