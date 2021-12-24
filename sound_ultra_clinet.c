#include "gpio_pwm.h"

#define SOUNDPIN 21 //sound sensor

#define ULTRAPIN 24 //ultra wave
#define ULTRAPOUT 23

#define TOOCLOSE 5.0

double distance = 1;
int soundlevel = 0;

void error_handling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void* ultrawave_thd() {
    clock_t start_t, end_t;
    double time;

    if (-1 == GPIOExport(ULTRAPOUT) || -1 == GPIOExport(ULTRAPIN)) {
        printf("gpio export err\n");
        exit(0);
    }
    usleep(100000);

    if (-1 == GPIODirection(ULTRAPOUT, OUT) || -1 == GPIODirection(ULTRAPIN, IN)) {
        printf("gpio direftion err\n");
        exit(0);
    }

    GPIOWrite(ULTRAPOUT, 0);
    usleep(10000);
    //start
    while (1) {
        if (-1 == GPIOWrite(ULTRAPOUT, 1)) {
            printf("gpio write/trigger err\n");
            exit(0);
        }

        usleep(10);
        GPIOWrite(ULTRAPOUT, 0);

        while (GPIORead(ULTRAPIN) == 0) {
            start_t = clock();
        }

        while (GPIORead(ULTRAPIN) == 1) {
            end_t = clock();
        }

        time = (double)(end_t - start_t) / CLOCKS_PER_SEC;
        distance = time / 2 * 34000;

        if (distance > 900)
            distance = 900;

        //printf("time: %.4lf\n", time);
        //printf("distance: %.2lfcm\n", distance);

        usleep(50000);
    }
}

void* sound_thd() {
    if (-1 == GPIOExport(SOUNDPIN)) {
        printf("gpio export err\n");
        exit(0);
    }
    usleep(10000);

    if (-1 == GPIODirection(SOUNDPIN, IN)) {
        printf("gpio direftion err\n");
        exit(0);
    }

    while (1) {
        soundlevel = GPIORead(SOUNDPIN);
        //printf("sound level: %d\n", soundlevel);
        if (soundlevel == 1) {
            //printf("sound is high!\n");
        }
        usleep(10000);
    }
}


int main(int argc, char* argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    char msg[2];
    char on[2] = "1";
    int str_len;

    pthread_t p_thread[2];
    int thr_id;
    int status;

    //Create two threads for sound sensor and ultrasonic sensor
    thr_id = pthread_create(&p_thread[0], NULL, ultrawave_thd, NULL);
    if (thr_id < 0) {
        perror("thread create error: ");
        exit(0);
    }
    thr_id = pthread_create(&p_thread[1], NULL, sound_thd, NULL);
    if (thr_id < 0) {
        perror("thread create error: ");
        exit(0);
    }

    printf("addr: %s\nport: %s\n", argv[1], argv[2]); //check input value
    
    //For socket communication with the server
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    printf("client connectd.\n");

    //Send a message to the server if Pi recognize the problem
    while (1) {
        if (soundlevel == 1) {
            snprintf(msg, 2, "1");
            write(sock, msg, sizeof(msg));
            //printf("send req msg: %s\n", msg);
        }
        else if (distance < TOOCLOSE) {
            printf("distance: %d\n", (int)distance);
            snprintf(msg, 2, "2");
            write(sock, msg, sizeof(msg));
            printf("it's close\n");
            printf("send req msg: %s\n", msg);
            usleep(10000);
            //printf("send req msg: %s\n", msg);
        }
        else {
            snprintf(msg, 2, "5");
            write(sock, msg, sizeof(msg));
            //printf("send req msg: %s\n", msg);
        }
        usleep(1000000);
    }
    close(sock);
    pthread_join(p_thread[0], (void**)&status);
    pthread_join(p_thread[1], (void**)&status);

    GPIOUnexport(SOUNDPIN);
    GPIOUnexport(ULTRAPOUT);
    GPIOUnexport(ULTRAPIN);

    return 0;
}
