#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_MESSAGE_SIZE 1024

int main(int argc, char *argv[]) {
    // Проверяем, что IP-адрес и порт заданы в аргументах командной строки
    if (argc != 3) {
        printf("Usage: %s <server-ip> <server-port>\n", argv[0]);
        return 1;
    }

    // Создаем сокет
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        return 1;
    }

    // Устанавливаем соединение с сервером
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    // Читаем сообщения от сервера и выводим их на экран
    char message[MAX_MESSAGE_SIZE];
    int bytes_read;
    while ((bytes_read = read(client_socket, message, MAX_MESSAGE_SIZE)) > 0) {
        message[bytes_read] = '\0';
        printf("Received message from server: %s\n", message);

        // Если получено сообщение "The End", завершаем работу
        if (strcmp(message, "The End") == 0) {
            break;
        }
    }

    // Закрываем сокет и завершаем работу
    close(client_socket);
    return 0;
}
