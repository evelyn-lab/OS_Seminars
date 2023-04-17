#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>

#define MSG_SIZE 10

int main()
{
    int fd[2]; // дескрипторы канала
    pid_t pid;
    sem_t *sem;
    char buf[MSG_SIZE];
    int i;

    // Создаем неименованный канал
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Создаем семафор и устанавливаем его значение в 1
    sem = sem_open("/my_semaphore", O_CREAT | O_EXCL, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Создаем дочерний процесс
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Дочерний процесс
        for (i = 0; i < 10; i++) {
            sem_wait(sem); // Ждем разрешения на запись
            read(fd[0], buf, MSG_SIZE); // Читаем из канала
            printf("Child received: %s\n", buf);
            sem_post(sem); // Даем разрешение на чтение
        }
        close(fd[0]);
        close(fd[1]);
        sem_close(sem);
        sem_unlink("/my_semaphore");
        exit(EXIT_SUCCESS);
    } else { // Родительский процесс
        for (i = 0; i < 10; i++) {
            sem_wait(sem); // Ждем разрешения на запись
            sprintf(buf, "Message %d", i);
            write(fd[1], buf, MSG_SIZE); // запись в канал
        printf("Parent sent: %s\n", buf);
        sem_post(sem); // Даем разрешение на чтение
    }
    close(fd[0]);
    close(fd[1]);
    sem_close(sem);
    sem_unlink("/my_semaphore");
    exit(EXIT_SUCCESS);
}

return 0;
