#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

struct Transfer {
    char request;
    char filename[64];
    int fileSize;
};

struct TransferResponse {
    char response;
    int fileSize;
};

int findSize(char * file_name);

int main(int argc, char** argv) {

    // Getting the IP from the user input
    char* ip = malloc(16 * sizeof(char));
    short unsigned port;
    scanf("%s %hu", ip, &port);

    // Establishing the connection with the server
    int sockFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFileDescriptor == -1) {
        printf("Error creating socket\n");
        return 1;
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = port; // htons?
    serverAddress.sin_addr.s_addr = inet_addr(ip);

    int connectResult = connect(sockFileDescriptor, (struct sockaddr*)&
        serverAddress, sizeof(serverAddress));
    if (connectResult == -1) {
        printf("Error connecting to the server\n");
        return 1;
    }

    // Getting the operation from the user input
    char op;
    char* filename = malloc(64 * sizeof(char));
    assert(filename != NULL);
    do {
        scanf(" %c", &op);
        if (op == 'C') {
            // Preparing the request
            struct Transfer transfer;
            memset(&transfer, 0, sizeof(transfer));
            transfer.request = 'C';
            transfer.fileSize = 0;
            send(sockFileDescriptor, &transfer, sizeof(transfer), 0);
            memset(&transfer, 0, sizeof(transfer));
        } else if (op == 'D') {
            scanf(" %s", filename);

            // Preparing the request
            struct Transfer transfer;
            memset(&transfer, 0, sizeof(transfer));
            transfer.request = 'D';
            strncpy(transfer.filename, filename, sizeof(transfer.filename));
            transfer.fileSize = 0;

            // Sending over the request
            send(sockFileDescriptor, &transfer, sizeof(transfer), 0);

            // Receiving the response
            struct TransferResponse response;
            memset(&response, 0, sizeof(response));
            recv(sockFileDescriptor, &response, sizeof(response), 0);

            // Checking if the download request is successful
            if (response.response == 'Y') {
                // Creating an empty file
                FILE *file = fopen(filename, "w");
                if (file == NULL) {
                    perror("Error opening file");
                    fclose(file);
                    exit(EXIT_FAILURE);
                }

                // Receiving the response with the data
                char buffer[1024];
                size_t total_bytes_received = 0;

                if (response.fileSize > 0) {
                    recv(sockFileDescriptor, buffer, response.fileSize, 0);
                } 
                

                // Writing the data to the file
                fwrite(buffer, 1, response.fileSize, file);
                fflush(file);

                printf("File %s downloaded.\n", filename);

                fwrite(buffer, 1, response.fileSize, stdout);
                printf("\n");

                fclose(file);
            } else {
                // File does not exist
                printf("File %s does not exist on server.\n", filename);
            }

            memset(&transfer, 0, sizeof(transfer));
            memset(&response, 0, sizeof(response));

        } else if (op == 'U') {
            scanf(" %s", filename);

            FILE *file = fopen(filename, "r");
            if (file == NULL) {
                perror("Error opening file");
                exit(EXIT_FAILURE);
            }

            char buffer[1024];
            size_t bytes_read;
            struct Transfer transfer;
            memset(&transfer, 0, sizeof(transfer));
            struct TransferResponse response;
            memset(&response, 0, sizeof(response));

            transfer.request = 'U';
            //strncpy(transfer.filename, filename, sizeof(transfer.filename));
            strcpy(transfer.filename, filename);
            transfer.fileSize = findSize(filename);

            // Sending the request to send data
            send(sockFileDescriptor, &transfer, sizeof(transfer), 0);

            // Receiving the server response
            recv(sockFileDescriptor, &response, sizeof(response), 0);

            // Request response can either be Y or N
            if (response.response == 'Y') {
                // Send over the data

                fread(buffer, 1, transfer.fileSize, file);

                send(sockFileDescriptor, buffer, transfer.fileSize, 0);

                printf("File %s uploaded.\n", filename);
            } else {
                printf("File %s already exists on server.\n", filename);
            }

            // Closing the file
            fclose(file);

            memset(&transfer, 0, sizeof(transfer));
            memset(&response, 0, sizeof(response));
        }
    } while(op != 'C');

    // Freeing up the memory
    memset(&serverAddress, 0, sizeof(serverAddress));
    free(filename);
    free(ip);

    // Closing the socket
    close(sockFileDescriptor);

    return 0;
}

int findSize(char * file_name) {
    FILE * fp = fopen (file_name, "r");
    if (fp == NULL) {
        printf ("File Not Found!\n");
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    long int res = ftell (fp);
    fclose(fp);
    return res;
}
