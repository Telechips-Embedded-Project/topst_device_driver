    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <stdint.h>
    #include <signal.h>
    #include <pthread.h>
    #include <sys/ioctl.h>
    #include <linux/spi/spidev.h>

    #define LED_COUNT 30
    #define SPI_DEV "/dev/spidev1.0"
    #define AMBIENT_DEV "/dev/ambient_dev"
    #define FPS 10

    #define AMBIENT_MAGIC 'L'
    #define AMBIENT_GET_MODE        _IOR(AMBIENT_MAGIC, 2, char *)
    #define AMBIENT_GET_BRIGHTNESS  _IOR(AMBIENT_MAGIC, 4, int)

    static volatile int running = 1;
    static uint8_t hue = 0;

    void handle_sigint(int sig) {
        running = 0;
    }

    void map_color(const char *mode, uint8_t *r, uint8_t *g, uint8_t *b) {
        if (strcmp(mode, "red") == 0)        { *r = 255; *g = 0;   *b = 0;   }
        else if (strcmp(mode, "green") == 0) { *r = 0;   *g = 255; *b = 0;   }
        else if (strcmp(mode, "blue") == 0)  { *r = 0;   *g = 0;   *b = 255; }
        else if (strcmp(mode, "yellow") == 0){ *r = 255; *g = 255; *b = 0;   }
        else if (strcmp(mode, "cyan") == 0)  { *r = 0;   *g = 255; *b = 255; }
        else if (strcmp(mode, "magenta") == 0){*r = 255; *g = 0;   *b = 255; }
        else if (strcmp(mode, "white") == 0) { *r = 255; *g = 255; *b = 255; }
        else                                 { *r = 0;   *g = 0;   *b = 0;   }
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

    int main() {
        signal(SIGINT, handle_sigint);
        signal(SIGTERM, handle_sigint);

        int spi_fd = open(SPI_DEV, O_WRONLY);
        if (spi_fd < 0) {
            perror("open spi device");
            return 1;
        }

        uint32_t speed = 25000000;
        if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
            perror("SPI: set speed failed");
        }

        printf("[ambient_daemon] Started. Reading from /dev/ambient_dev");

        while (running) {
            int dev_fd = open(AMBIENT_DEV, O_RDONLY);
            if (dev_fd < 0) {
                perror("open ambient device");
                break;
            }

            char mode_buf[16] = "off";
            int brightness = 0;

            ioctl(dev_fd, AMBIENT_GET_MODE, mode_buf);
            ioctl(dev_fd, AMBIENT_GET_BRIGHTNESS, &brightness);
            close(dev_fd);

            if (brightness < 0) brightness = 0;
            if (brightness > 100) brightness = 100;

            uint8_t grb_data[LED_COUNT * 3];
            if (strcmp(mode_buf, "rainbow") == 0) {
                for (int i = 0; i < LED_COUNT; i++) {
                    uint8_t rr, gg, bb;
                    hue_to_grb(hue + i * 10, &gg, &rr, &bb);
                    grb_data[i*3 + 0] = (gg * brightness) / 100;
                    grb_data[i*3 + 1] = (rr * brightness) / 100;
                    grb_data[i*3 + 2] = (bb * brightness) / 100;
                }
                hue += 3;
            } else {
                uint8_t rr, gg, bb;
                map_color(mode_buf, &rr, &gg, &bb);
                uint8_t br = (rr * brightness) / 100;
                uint8_t bg = (gg * brightness) / 100;
                uint8_t bb2 = (bb * brightness) / 100;
                for (int i = 0; i < LED_COUNT; i++) {
                    grb_data[i*3 + 0] = bg;
                    grb_data[i*3 + 1] = br;
                    grb_data[i*3 + 2] = bb2;
                }
            }

            uint8_t spi_data[LED_COUNT * 24 * 3] = {0};
            encode_leds(grb_data, spi_data, LED_COUNT);
            ssize_t ret = write(spi_fd, spi_data, sizeof(spi_data));
            if (ret != sizeof(spi_data)) {
                perror("spi write failed");
            }

            usleep(1000000 / FPS);
        }

        close(spi_fd);
        printf("[ambient_daemon] Terminated.");
        return 0;
    }
