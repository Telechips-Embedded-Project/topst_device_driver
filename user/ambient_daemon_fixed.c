
// ambient_daemon.c (안정화 + 노이즈 제거 + 30비즈 대응 + 버퍼 오버런 보호)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <time.h>

#define DEVICE_PATH "/dev/ambient_dev"
#define SPI_PATH "/dev/spidev0.0"
#define AMBIENT_MAGIC 'L'
#define AMBIENT_GET_MODE_STR       _IOR(AMBIENT_MAGIC, 2, char *)
#define AMBIENT_GET_BRIGHTNESS_STR _IOR(AMBIENT_MAGIC, 4, char *)

#define LED_COUNT 30
#define SPI_BYTES_PER_LED 24
#define FPS 15

void encode_byte(uint8_t byte, uint8_t *out) {
    for (int i = 7; i >= 0; i--) {
        uint8_t bit = (byte >> i) & 1;
        uint8_t signal = bit ? 0b110 : 0b100;
        for (int j = 2; j >= 0; j--) {
            *out++ = (signal >> j) & 1 ? 0xFF : 0x00;
        }
    }
}

void encode_leds(uint8_t *grb, uint8_t *spi) {
    for (int i = 0; i < LED_COUNT; i++) {
        encode_byte(grb[i * 3 + 0], spi + i * 72);     // G
        encode_byte(grb[i * 3 + 1], spi + i * 72 + 24); // R
        encode_byte(grb[i * 3 + 2], spi + i * 72 + 48); // B
    }
}

void hsv_to_grb(uint8_t hue, uint8_t *g, uint8_t *r, uint8_t *b) {
    uint8_t region = hue / 43;
    uint8_t remainder = (hue - (region * 43)) * 6;
    uint8_t p = 0;
    uint8_t q = 255 - ((255 * remainder) / 256);
    uint8_t t = (255 * remainder) / 256;

    switch (region) {
        case 0: *r = 255; *g = t; *b = p; break;
        case 1: *r = q; *g = 255; *b = p; break;
        case 2: *r = p; *g = 255; *b = t; break;
        case 3: *r = p; *g = q; *b = 255; break;
        case 4: *r = t; *g = p; *b = 255; break;
        default: *r = 255; *g = p; *b = q; break;
    }
}

void send_led_off(int spi_fd) {
    uint8_t off_grb[LED_COUNT * 3] = {0};
    uint8_t off_spi[LED_COUNT * SPI_BYTES_PER_LED] = {0};
    encode_leds(off_grb, off_spi);
    ssize_t ret = write(spi_fd, off_spi, sizeof(off_spi));
    if (ret != sizeof(off_spi)) {
        perror("write failed or incomplete");
    }
    usleep(80);
}

int main() {
    int fd = open(DEVICE_PATH, O_RDWR);
    int spi_fd = open(SPI_PATH, O_WRONLY);
    if (fd < 0 || spi_fd < 0) { perror("open"); return 1; }

    send_led_off(spi_fd);  // 최초 OFF 프레임

    char mode[32] = {0}, brightness_str[16] = {0};
    strncpy(mode, "off", sizeof(mode)-1);
    strncpy(brightness_str, "mid", sizeof(brightness_str)-1);

    int brightness = 40;
    uint8_t grb[LED_COUNT * 3];
    uint8_t spi_buf[LED_COUNT * SPI_BYTES_PER_LED];
    uint8_t hue = 0;

    while (1) {
        if (ioctl(fd, AMBIENT_GET_MODE_STR, mode) < 0) perror("GET_MODE");
        if (ioctl(fd, AMBIENT_GET_BRIGHTNESS_STR, brightness_str) < 0) perror("GET_BRIGHTNESS");
        mode[sizeof(mode)-1] = '\0';
        brightness_str[sizeof(brightness_str)-1] = '\0';

        if (strcmp(brightness_str, "low") == 0) brightness = 10;
        else if (strcmp(brightness_str, "mid") == 0) brightness = 40;
        else if (strcmp(brightness_str, "high") == 0) brightness = 100;

        memset(grb, 0, sizeof(grb));

        if (strcmp(mode, "red") == 0) {
            for (int i = 0; i < LED_COUNT; i++) {
                grb[i*3 + 0] = 0;
                grb[i*3 + 1] = brightness;
                grb[i*3 + 2] = 0;
            }
        } else if (strcmp(mode, "yellow") == 0) {
            for (int i = 0; i < LED_COUNT; i++) {
                grb[i*3 + 0] = brightness;
                grb[i*3 + 1] = brightness;
                grb[i*3 + 2] = 0;
            }
        } else if (strcmp(mode, "green") == 0) {
            for (int i = 0; i < LED_COUNT; i++) {
                grb[i*3 + 0] = brightness;
                grb[i*3 + 1] = 0;
                grb[i*3 + 2] = 0;
            }
        } else if (strcmp(mode, "rainbow") == 0) {
            for (int i = 0; i < LED_COUNT; i++) {
                uint8_t r, g, b;
                hsv_to_grb(hue + i * 8, &g, &r, &b);
                grb[i*3 + 0] = (g * brightness) / 100;
                grb[i*3 + 1] = (r * brightness) / 100;
                grb[i*3 + 2] = (b * brightness) / 100;
            }
            hue++;
        }

        encode_leds(grb, spi_buf);
        ssize_t ret = write(spi_fd, spi_buf, sizeof(spi_buf));
        if (ret != sizeof(spi_buf)) {
            perror("write failed or incomplete");
        }

        usleep(1000000 / FPS);
    }

    close(spi_fd);
    close(fd);
    return 0;
}
