#include "gpio_pwm.h"

#define LED1 18
#define LED2 17
#define VIBE 27

void error_handling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void clnt1_thd(int serv_sock) {
    int clnt_sock = -1;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;
    char msg[2];
    int str_len;

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    if (clnt_sock < 0) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            error_handling("accept() error");
    }

    while (1)
    {
        str_len = read(clnt_sock, msg, sizeof(msg));
        //printf("[SERVER] Receive message from client 1: %s\n",msg);

        if (str_len == -1)
            error_handling("read() error");

        if (strncmp("1", msg, 1) == 0) {
            GPIOWrite(LED2, 1);
            GPIOWrite(VIBE, 1);
        }
        else if (strncmp("2", msg, 1) == 0) {
            GPIOWrite(LED1, 1);
            GPIOWrite(VIBE, 1);
        }
        else{
            GPIOWrite(LED1, 0);
            GPIOWrite(LED2, 0);
            GPIOWrite(VIBE, 0);
        }

    }
}

void clnt2_thd(int serv_sock) {
    int clnt_sock = -1;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;
    char msg[2];
    int str_len;

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    if (clnt_sock < 0) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            error_handling("accept() error");
    }
    printf("server2 connected.\n");


    while (1)
    {
        str_len = read(clnt_sock, msg, sizeof(msg));
        //printf("[SERVER] Receive message from client 2: %s\n",msg);

        if (str_len == -1)
            error_handling("read() error");

        if (strncmp("3", msg, 1) == 0) {
            GPIOWrite(LED1, 1);
            GPIOWrite(LED2, 1);
            GPIOWrite(VIBE, 1);
        }

        else{
            GPIOWrite(LED1, 0);
            GPIOWrite(LED2, 0);
            GPIOWrite(VIBE, 0);
        }
    }
}

int main(int argc, char* argv[]) {
    int str_len = 0;
    pthread_t p_thread[2];
    int thr_id;
    int status;

    int serv_sock = -1;
    struct sockaddr_in serv_addr;

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
    }

    if (-1 == GPIOExport(LED1) || -1 == GPIOExport(LED2) || -1 == GPIOExport(VIBE))
        return(1);

    if (-1 == GPIODirection(VIBE, OUT) || -1 == GPIODirection(LED1, OUT) || -1 == GPIODirection(LED2, OUT))
        return(2);


    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    thr_id = pthread_create(&p_thread[0], NULL, clnt1_thd, serv_sock);
    if (thr_id < 0) {
        perror("thread create error: ");
        exit(0);
    }
    usleep(10000);

    thr_id = pthread_create(&p_thread[1], NULL, clnt2_thd, serv_sock);
    if (thr_id < 0) {
        perror("thread create error: ");
        exit(0);
    }

    pthread_join(p_thread[0], (void**)&status);
    pthread_join(p_thread[1], (void**)&status);
    close(serv_sock);

    return 0;
}
