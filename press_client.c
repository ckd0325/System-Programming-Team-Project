#include "gpio_pwm.h"//make haeder file
#include <stdint.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])
#define FALLING 5

static const char* DEVICE = "/dev/spidev0.0";
static uint8_t MODE = SPI_MODE_0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;

void error_handling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

static int prepare(int fd) {
    if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1) {
        perror("Can't set MODE");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1) {
        perror("Can't set number of BITS");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1) {
        perror("Can't set write CLOCK");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1) {
        perror("Can't set read CLOCK");
        return -1;
    }
    return 0;
}

uint8_t control_bits_differential(uint8_t channel) {
    return (channel & 7) << 4;
}

uint8_t control_bits(uint8_t channel) {
    return 0x8 | control_bits_differential(channel);
}

int readadc(int fd, uint8_t channel) {
    uint8_t tx[] = { 1, control_bits(channel), 0 };
    uint8_t rx[3];

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = ARRAY_SIZE(tx),
        .delay_usecs = DELAY,
        .speed_hz = CLOCK,
        .bits_per_word = BITS,
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) {
        perror("IO Error");
        abort();
    }

    return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}

int main(int argc, char** argv) {
    int pad_value = 0;
    int sock;
    struct sockaddr_in serv_addr;
    char msg[2];

    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);

    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    int fd = open(DEVICE, O_RDWR);
    if (fd <= 0) {
        printf("Device %s not found\n", DEVICE);
        return -1;
    }

    if (prepare(fd) == -1) {
        return -1;
    }

    while (1) {
        pad_value = readadc(fd, 0);

        if (pad_value >= FALLING) {//when pressure sensor get pressure out of person, send "3" to server
            snprintf(msg, 2, "%d", 3);
            write(sock, msg, sizeof(msg));
            printf("client send msg: %s\n", msg);
            printf("ok Pressure Pad Value: %d\n", pad_value);
        }
        else {//no pressure on the pressure sensor, send "5" to server
            snprintf(msg, 2, "%d", 5);
            write(sock, msg, sizeof(msg));
            //printf("no Pressure Pad Value: %d\n", pad_value);
        }

        usleep(1000000);
    }
    close(sock);
    close(fd);

    return 0;
}
