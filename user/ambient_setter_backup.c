
// ambient_setter.c
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/ambient_dev"
#define AMBIENT_MAGIC 'L'
#define AMBIENT_SET_MODE           _IOW(AMBIENT_MAGIC, 1, char*)
#define AMBIENT_SET_BRIGHTNESS     _IOW(AMBIENT_MAGIC, 3, char *)

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("사용법: %s [mode: red/yellow/green/rainbow/off] [brightness: low/mid/high]\n", argv[0]);
        return 1;
    }

    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open ambient_dev");
        return 1;
    }

    if (ioctl(fd, AMBIENT_SET_MODE, argv[1]) < 0) {
        perror("ioctl set mode");
    }

    if (ioctl(fd, AMBIENT_SET_BRIGHTNESS, argv[2]) < 0) {
        perror("ioctl set brightness");
    }

    close(fd);
    return 0;
}
