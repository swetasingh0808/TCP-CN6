#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 4096

/*
 * Read exactly n bytes from a socket.
 * Returns 0 on success and -1 on failure.
 */
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

/*
 * Write exactly n bytes to a socket.
 */
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

void handle_client(int client_socket)
{
    int student_id;
    int data_length;
    char data[BUFFER_SIZE];
    char response[512];

    /* Receive Student ID */
    if (read_all(client_socket, &student_id,
                 sizeof(student_id)) < 0)
    {
        close(client_socket);
        _exit(1);
    }

    /* Receive file data length */
    if (read_all(client_socket, &data_length,
                 sizeof(data_length)) < 0)
    {
        close(client_socket);
        _exit(1);
    }

    if (data_length < 0 || data_length >= BUFFER_SIZE)
    {
        close(client_socket);
        _exit(1);
    }

    /* Receive file contents */
    if (read_all(client_socket, data,
                 data_length) < 0)
    {
        close(client_socket);
        _exit(1);
    }

    data[data_length] = '\0';

    /* Search for the requested Student ID */
    int found = 0;
    char *line = strtok(data, "\n");

    while (line != NULL)
    {
        int id;
        char name[100];
        char department[100];
        char extra;

        /*
         * Match the ID as a complete integer.
         * This prevents ID 10 from matching ID 101.
         */
        if (sscanf(line, "%d %99s %99s %c",
                   &id, name, department, &extra) == 3)
        {
            if (id == student_id)
            {
                snprintf(response, sizeof(response),
                         "Record Found: %s", line);

                found = 1;
                break;
            }
        }

        line = strtok(NULL, "\n");
    }

    if (!found)
    {
        snprintf(response, sizeof(response),
                 "Record Not Found");
    }

    /* Send response length followed by response */
    int response_length = (int)strlen(response);

    if (write_all(client_socket, &response_length,
                  sizeof(response_length)) == 0)
    {
        write_all(client_socket, response,
                  response_length);
    }

    close(client_socket);
    _exit(0);
}

int main(void)
{
    int server_socket;
    int client_socket;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    socklen_t client_length = sizeof(client_address);

    /*
     * Prevent terminated child processes from
     * becoming zombies.
     */
    signal(SIGCHLD, SIG_IGN);

    /* Create TCP socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;

    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    /* Configure server address */
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(PORT);

    /* Bind socket */
    if (bind(server_socket,
             (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0)
    {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    /* Listen for client connections */
    if (listen(server_socket, 5) < 0)
    {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d...\n", PORT);
    fflush(stdout);

    while (1)
    {
        /* Accept a client connection */
        client_socket = accept(
            server_socket,
            (struct sockaddr *)&client_address,
            &client_length
        );

        if (client_socket < 0)
        {
            perror("accept");
            continue;
        }

        /* Create a child process */
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork");
            close(client_socket);
            continue;
        }

        if (pid == 0)
        {
            /* Child process */
            close(server_socket);

            handle_client(client_socket);

            /* Normally unreachable because of _exit() */
            close(client_socket);
            _exit(0);
        }
        else
        {
            /* Parent process */
            close(client_socket);

            printf("Client connected. Child PID: %d\n", pid);
            fflush(stdout);
        }

        client_length = sizeof(client_address);
    }

    close(server_socket);
    return 0;
}
