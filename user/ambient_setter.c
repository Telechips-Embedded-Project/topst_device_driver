
// ambient_setter.c (단일 off 명령어 지원)
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/ambient_dev"
#define AMBIENT_MAGIC 'A'
#define AMBIENT_SET_MODE           _IOW(AMBIENT_MAGIC, 1, char*)
#define AMBIENT_SET_BRIGHTNESS     _IOW(AMBIENT_MAGIC, 3, char *)

void print_usage(const char *prog) {
    printf("사용법:\n");
    printf("  %s mode <red|green|yellow|rainbow|off>\n", prog);
    printf("  %s brightness <low|mid|high>\n", prog);
    printf("  %s off\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open ambient_dev");
        return 1;
    }

    if (argc == 2 && strcmp(argv[1], "off") == 0) {
        char mode[] = "off";
        char bright[] = "low";
        ioctl(fd, AMBIENT_SET_MODE, mode);
        ioctl(fd, AMBIENT_SET_BRIGHTNESS, bright);
        printf("LED 끔 (off + low)");
    } else if (argc == 3 && strcmp(argv[1], "mode") == 0) {
        ioctl(fd, AMBIENT_SET_MODE, argv[2]);
        printf("모드 설정: %s", argv[2]);
    } else if (argc == 3 && strcmp(argv[1], "brightness") == 0) {
        ioctl(fd, AMBIENT_SET_BRIGHTNESS, argv[2]);
        printf("밝기 설정: %s", argv[2]);
    } else {
        print_usage(argv[0]);
    }

    close(fd);
    return 0;
}
