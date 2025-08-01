#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#define WIPER_MAGIC 'W'
#define WIPER_SET_MODE _IOW(WIPER_MAGIC, 1, int)

#define WIPER_MODE_OFF  0
#define WIPER_MODE_FAST 1
#define WIPER_MODE_SLOW 2

void usage(const char *progname) {
    printf("Usage: %s [off|fast|slow]\n", progname);
}

int main(int argc, char *argv[]) {
    int fd, mode;

    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "off") == 0)
        mode = WIPER_MODE_OFF;
    else if (strcmp(argv[1], "fast") == 0)
        mode = WIPER_MODE_FAST;
    else if (strcmp(argv[1], "slow") == 0)
        mode = WIPER_MODE_SLOW;
    else {
        usage(argv[0]);
        return 1;
    }

    fd = open("/dev/wiper_dev", O_RDWR);
    if (fd < 0) {
        perror("open /dev/wiper_dev");
        return 1;
    }

    if (ioctl(fd, WIPER_SET_MODE, &mode) < 0) {
        perror("ioctl WIPER_SET_MODE");
        close(fd);
        return 1;
    }

    printf("Wiper mode set to %s.\n", argv[1]);
    close(fd);
    return 0;
}
