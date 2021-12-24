#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <pthread.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define BUFFER_MAX 3
#define DIRECTION_MAX 45
#define VALUE_MAX 256

static int
PWMExport(int pwmnum) {
	char buffer[BUFFER_MAX];
	int bytes_written;
	int fd;

	fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in unexport!\n");
		return -1;
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
	write(fd, buffer, bytes_written);
	close(fd);
	sleep(1);
	return 0;
}

static int
PWMUnexport(int pwmnum) {
	char buffer[BUFFER_MAX];
	int bytes_written;
	int fd;

	fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in unexport!\n");
		return -1;
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
	write(fd, buffer, bytes_written);
	close(fd);

	sleep(1);

	return 0;
}

static int
PWMEnable(int pwmnum) {
	static const char s_unenable_str[] = "0";
	static const char s_enable_str[] = "1";

	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in enable!\n");
		return -1;
	}

	write(fd, s_unenable_str, strlen(s_unenable_str));
	close(fd);

	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in enable!\n");
		return -1;
	}

	write(fd, s_enable_str, strlen(s_enable_str));
	close(fd);
	return 0;
}

static int
PWMWritePeriod(int pwmnum, int value) {
	char s_values_str[VALUE_MAX];
	char path[VALUE_MAX];
	int fd, byte;

	snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/period", pwmnum);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in period!\n");
		return(-1);
	}

	byte = snprintf(s_values_str, 10, "%d", value);

	if (-1 == write(fd, s_values_str, byte)) {
		fprintf(stderr, "Failed to write value in period!\n");
		close(fd);
		return(-1);
	}

	close(fd);
	return 0;
}

static int
PWMWriteDutyCycle(int pwmnum, int value) {
	char path[VALUE_MAX];
	char s_values_str[VALUE_MAX];
	int fd, byte;

	snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmnum);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in duty_cycle!\n");
		return -1;
	}

	byte = snprintf(s_values_str, 10, "%d", value);

	if (-1 == write(fd, s_values_str, byte)) {
		fprintf(stderr, "Failed to write value! in duty_cycle\n");
		close(fd);
		return -1;
	}

	close(fd);
	return(0);
}

static int GPIOExport(int pin) {
#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

static int GPIOUnexport(int pin) {
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

static int GPIODirection(int pin, int dir) {
	static const char s_directions_str[] = "in\0out";

	//char path[DIRECTION_MAX]="/sys/class/gpio/gpio24/direction";
	char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return(-1);
	}

	if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

static int GPIORead(int pin) {
	char path[VALUE_MAX];
	char value_str[3];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		return(-1);
	}

	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Failed to read value!\n");
		return(-1);
	}

	close(fd);

	return(atoi(value_str));
}

static int GPIOWrite(int pin, int value) {
	static const char s_values_str[] = "01";

	char path[VALUE_MAX];
	int fd;


	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return(-1);
	}

	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		return(-1);

		close(fd);
		return(0);
	}
}