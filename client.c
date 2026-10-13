#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int read_all(int fd, void *buffer, size_t n)
{
    size_t total = 0;
    char *ptr = buffer;

    while (total < n)
    {
        ssize_t bytes = read(fd, ptr + total, n - total);

        if (bytes <= 0)
            return -1;

        total += bytes;
    }

    return 0;
}

int write_all(int fd, const void *buffer, size_t n)
{
    size_t total = 0;
    const char *ptr = buffer;

    while (total < n)
    {
        ssize_t bytes = write(fd, ptr + total, n - total);

        if (bytes <= 0)
            return -1;

        total += bytes;
    }

    return 0;
}

int main(void)
{
    int client_socket;
    int student_id;
    int data_length;
    int response_length;

    char data[BUFFER_SIZE];
    char response[512];

    struct sockaddr_in server_address;

    /* Open local data.txt */
    FILE *file = fopen("data.txt", "r");

    if (file == NULL)
    {
        perror("data.txt");
        return 1;
    }

    /* Read file contents */
    data_length = (int)fread(
        data, 1, sizeof(data) - 1, file
    );

    if (ferror(file))
    {
        perror("fread");
        fclose(file);
        return 1;
    }

    data[data_length] = '\0';
    fclose(file);

    /* Ask for Student ID */
    printf("Enter Student ID to search: ");

    if (scanf("%d", &student_id) != 1)
    {
        fprintf(stderr, "Invalid Student ID\n");
        return 1;
    }

    /* Create TCP socket */
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket < 0)
    {
        perror("socket");
        return 1;
    }

    /* Configure server address */
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    /* Connect to server running on the same machine */
    if (inet_pton(AF_INET, "127.0.0.1",
                  &server_address.sin_addr) != 1)
    {
        fprintf(stderr, "Invalid server IP address\n");
        close(client_socket);
        return 1;
    }

    if (connect(client_socket,
                (struct sockaddr *)&server_address,
                sizeof(server_address)) < 0)
    {
        perror("connect");
        close(client_socket);
        return 1;
    }

    /* Send Student ID */
    if (write_all(client_socket, &student_id,
                  sizeof(student_id)) < 0)
    {
        perror("write Student ID");
        close(client_socket);
        return 1;
    }

    /* Send file data length */
    if (write_all(client_socket, &data_length,
                  sizeof(data_length)) < 0)
    {
        perror("write data length");
        close(client_socket);
        return 1;
    }

    /* Send file contents */
    if (write_all(client_socket, data,
                  data_length) < 0)
    {
        perror("write file data");
        close(client_socket);
        return 1;
    }

    /* Receive response length */
    if (read_all(client_socket, &response_length,
                 sizeof(response_length)) < 0)
    {
        fprintf(stderr, "Failed to receive response\n");
        close(client_socket);
        return 1;
    }

    if (response_length < 0 ||
        response_length >= (int)sizeof(response))
    {
        fprintf(stderr, "Invalid response length\n");
        close(client_socket);
        return 1;
    }

    /* Receive response */
    if (read_all(client_socket, response,
                 response_length) < 0)
    {
        fprintf(stderr, "Failed to receive response\n");
        close(client_socket);
        return 1;
    }

    response[response_length] = '\0';

    printf("Server Response: %s\n", response);

    close(client_socket);

    return 0;
}