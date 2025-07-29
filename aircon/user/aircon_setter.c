#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#define AIRCON_MAGIC 'A'
#define AIRCON_SET_LEVEL _IOW(AIRCON_MAGIC, 1, int)

#define AIRCON_LEVEL_OFF  0
#define AIRCON_LEVEL_LOW  1
#define AIRCON_LEVEL_MID  2
#define AIRCON_LEVEL_HIGH 3

void usage(const char *progname) {
    printf("Usage: %s [off|low|mid|high]\n", progname);
}

int main(int argc, char *argv[]) {
    int fd, level;

    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "off") == 0)
        level = AIRCON_LEVEL_OFF;
    else if (strcmp(argv[1], "low") == 0)
        level = AIRCON_LEVEL_LOW;
    else if (strcmp(argv[1], "mid") == 0)
        level = AIRCON_LEVEL_MID;
    else if (strcmp(argv[1], "high") == 0)
        level = AIRCON_LEVEL_HIGH;
    else {
        usage(argv[0]);
        return 1;
    }

    fd = open("/dev/aircon_dev", O_RDWR);
    if (fd < 0) {
        perror("open /dev/aircon_dev");
        return 1;
    }

    if (ioctl(fd, AIRCON_SET_LEVEL, &level) < 0) {
        perror("ioctl AIRCON_SET_LEVEL");
        close(fd);
        return 1;
    }

    printf("Aircon level set to %s.\n", argv[1]);
    close(fd);
    return 0;
}
