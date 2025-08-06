#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

#define LED_COUNT 30
#define SPI_DEV "/dev/spidev1.0"
#define FIFO_PATH "/tmp/spi_fifo"
#define FPS 10

static volatile int running = 1;
static uint8_t hue = 0;
static int rainbow_mode = 0;
static uint8_t r = 0, g = 0, b = 0;
static int brightness = 40;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void handle_sigint(int sig) {
    running = 0;
}

void map_color(int color_code, uint8_t *r, uint8_t *g, uint8_t *b) {
    switch (color_code) {
        case 1: *r = 255; *g = 0;   *b = 0;   break;
        case 2: *r = 0;   *g = 255; *b = 0;   break;
        case 3: *r = 0;   *g = 0;   *b = 255; break;
        case 4: *r = 255; *g = 255; *b = 0;   break;
        case 5: *r = 0;   *g = 255; *b = 255; break;
        case 6: *r = 255; *g = 0;   *b = 255; break;
        case 7: *r = 255; *g = 255; *b = 255; break;
        default: *r = *g = *b = 0;  break;
    }
}

void hue_to_grb(uint8_t hue, uint8_t *g, uint8_t *r, uint8_t *b) {
    int h = hue % 256;
    if (h < 85) {
        *r = h * 3;
        *g = 255 - h * 3;
        *b = 0;
    } else if (h < 170) {
        h -= 85;
        *r = 255 - h * 3;
        *g = 0;
        *b = h * 3;
    } else {
        h -= 170;
        *r = 0;
        *g = h * 3;
        *b = 255 - h * 3;
    }
}

void encode_byte(uint8_t byte, uint8_t *out_buf) {
    for (int i = 7; i >= 0; i--) {
        uint8_t bit = (byte >> i) & 1;
        uint8_t bits = bit ? 0b110 : 0b100;
        for (int j = 2; j >= 0; j--) {
            *out_buf++ = (bits >> j) & 1 ? 0xFF : 0x00;
        }
    }
}

void encode_leds(uint8_t *grb_data, uint8_t *spi_data, int led_count) {
    for (int i = 0; i < led_count; i++) {
        encode_byte(grb_data[i*3 + 0], spi_data + i*3*24 + 0);   // G
        encode_byte(grb_data[i*3 + 1], spi_data + i*3*24 + 24);  // R
        encode_byte(grb_data[i*3 + 2], spi_data + i*3*24 + 48);  // B
    }
}

void *fifo_thread_func(void *arg) {
    char buf[128];
    while (running) {
        int fifo_fd = open(FIFO_PATH, O_RDONLY);
        if (fifo_fd >= 0) {
            ssize_t len = read(fifo_fd, buf, sizeof(buf) - 1);
            if (len > 0) {
                buf[len] = '\0';
                printf("[DEBUG] Received command: %s\n", buf);

                pthread_mutex_lock(&lock);
                if (strstr(buf, "color")) {
                    if (strstr(buf, "rainbow")) {
                        rainbow_mode = 1;
                        hue = 0;
                    } else {
                        rainbow_mode = 0;
                        if (strstr(buf, "red")) map_color(1, &r, &g, &b);
                        else if (strstr(buf, "green")) map_color(2, &r, &g, &b);
                        else if (strstr(buf, "blue")) map_color(3, &r, &g, &b);
                        else if (strstr(buf, "yellow")) map_color(4, &r, &g, &b);
                        else if (strstr(buf, "cyan")) map_color(5, &r, &g, &b);
                        else if (strstr(buf, "magenta")) map_color(6, &r, &g, &b);
                        else if (strstr(buf, "white")) map_color(7, &r, &g, &b);
                        else map_color(0, &r, &g, &b);
                    }
                } else if (strstr(buf, "brightness")) {
                    sscanf(buf, "{\"command\":\"brightness\", \"value\":%d}", &brightness);
                    if (brightness < 0) brightness = 0;
                    if (brightness > 100) brightness = 100;
                }
                pthread_mutex_unlock(&lock);
            }
            close(fifo_fd);
        }
        usleep(100000);  // 100ms 대기
    }
    return NULL;
}

int main() {
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    int spi_fd = open(SPI_DEV, O_WRONLY);
    if (spi_fd < 0) {
        perror("open spi device");
        return 1;
    }

    // SPI 속도 강제 설정 (WS2812용 안정 속도)
    uint32_t speed = 25000000;  
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("SPI: set speed failed");
    }
    if (access(FIFO_PATH, F_OK) == -1) {
        if (mkfifo(FIFO_PATH, 0666) == -1) {
            perror("mkfifo failed");
            return 1;
        }
    }

    pthread_t fifo_thread;
    pthread_create(&fifo_thread, NULL, fifo_thread_func, NULL);

    uint8_t grb_data[LED_COUNT * 3];

    printf("[spi_daemon] Started. Waiting for commands on %s\n", FIFO_PATH);

    while (running) {
        pthread_mutex_lock(&lock);
        if (rainbow_mode) {
            for (int i = 0; i < LED_COUNT; i++) {
                uint8_t rr, gg, bb;
                hue_to_grb(hue + i * 10, &gg, &rr, &bb);
                grb_data[i*3 + 0] = (gg * brightness) / 100;
                grb_data[i*3 + 1] = (rr * brightness) / 100;
                grb_data[i*3 + 2] = (bb * brightness) / 100;
            }
            hue += 3;
        } else {
            uint8_t br = (r * brightness) / 100;
            uint8_t bg = (g * brightness) / 100;
            uint8_t bb = (b * brightness) / 100;
            for (int i = 0; i < LED_COUNT; i++) {
                grb_data[i*3 + 0] = bg;
                grb_data[i*3 + 1] = br;
                grb_data[i*3 + 2] = bb;
            }
        }
        pthread_mutex_unlock(&lock);

        uint8_t spi_data[LED_COUNT * 24 * 3] = {0};
        encode_leds(grb_data, spi_data, LED_COUNT);
        ssize_t ret = write(spi_fd, spi_data, sizeof(spi_data));
        if (ret != sizeof(spi_data)) {
            perror("spi write failed");
        }

        usleep(1000000 / FPS);
    }

    pthread_join(fifo_thread, NULL);
    close(spi_fd);
    unlink(FIFO_PATH);
    printf("[spi_daemon] Terminated.\n");
    return 0;
}
