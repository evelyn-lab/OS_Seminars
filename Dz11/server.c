#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define BUFFER_SIZE 1024

int main(int argc, char const *argv[])
{
    // Создание сокета
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Задание параметров адреса сервера
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Привязка сокета к адресу сервера
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Ожидание подключения клиента
    if (listen(server_socket, 1) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Подключение клиента
    struct sockaddr_in client_addr = {0};
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_socket == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Цикл передачи сообщений от клиента №1 клиенту №2
    char buffer[BUFFER_SIZE];
    int bytes_received;
    while (1) {
        // Получение сообщения от клиента №1
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received == -1) {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        buffer[bytes_received] = '\0';

        // Передача сообщения клиенту №2
        if (strcmp(buffer, "The End") == 0) {
            // Если сообщение "The End", завершение работы
            break;
        } else {
            // Иначе, передача сообщения клиенту №2
            printf("Received message from client 1: %s\n", buffer);
            send(client_socket, buffer, strlen(buffer), 0);
        }
    }

    // Закрытие сокетов
    close(client_socket);
    close(server_socket);

    return 0;
}
