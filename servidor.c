#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "include/cJSON.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

typedef struct {
    SOCKET socket;
    struct sockaddr_in address;
    char ip[16];  
    int port;
    cJSON *data;  //  recebidos do cliente
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
int clientCount = 0;
CRITICAL_SECTION cs;
int serverRunning = 1;  

// Criptografia XOR
void xor_cipher(const char *input, char *output, const char *key, int length) {
    int key_len = strlen(key);
    for (int i = 0; i < length; i++) {
        output[i] = input[i] ^ key[i % key_len]; 
    }
}

void save_clients_to_json() {
    EnterCriticalSection(&cs);
    cJSON *root = cJSON_CreateArray();

    for (int i = 0; i < clientCount; i++) {
        cJSON *clientObj = cJSON_CreateObject();
        cJSON_AddStringToObject(clientObj, "ip", clients[i].ip);
        cJSON_AddNumberToObject(clientObj, "port", clients[i].port);

        // CRIPTOGRAFIA JSON
        if (clients[i].data) {
            char *json_data = cJSON_Print(clients[i].data);  
            char encrypted_data[BUFFER_SIZE];
            const char *key = "minhachave";  

            
            xor_cipher(json_data, encrypted_data, key, strlen(json_data));

            
            cJSON_AddStringToObject(clientObj, "encrypted_data", encrypted_data);
            free(json_data);  
        }

        cJSON_AddItemToArray(root, clientObj);
    }

    char *jsonString = cJSON_Print(root);
    FILE *file = fopen("clientes.json", "w");
    if (file) {
        fprintf(file, "%s", jsonString);
        fclose(file);
    }

    cJSON_Delete(root);
    free(jsonString);
    LeaveCriticalSection(&cs);
}
// Remove o cliente da lista
void remove_client(int index) {
    EnterCriticalSection(&cs);

    
    if (clients[index].data) {
        cJSON_Delete(clients[index].data);
    }

    
    for (int i = index; i < clientCount - 1; i++) {
        clients[i] = clients[i + 1];
    }
    clientCount--;

   
    save_clients_to_json();

    LeaveCriticalSection(&cs);
}

DWORD WINAPI handle_client(LPVOID arg) {
    ClientInfo *client = (ClientInfo *)arg;
    char buffer[BUFFER_SIZE];
    char decrypted_buffer[BUFFER_SIZE];
    const char *key = "minhachave";  // Chave 

    
    int bytesReceived = recv(client->socket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived > 0) {
        // Descriptografa 
        xor_cipher(buffer, decrypted_buffer, key, bytesReceived);
        decrypted_buffer[bytesReceived] = '\0';  
        printf("Dados recebidos de %s:%d -> %s\n", client->ip, client->port, decrypted_buffer);

        
        EnterCriticalSection(&cs);
        client->data = cJSON_Parse(decrypted_buffer);
        LeaveCriticalSection(&cs);
        save_clients_to_json();
    }

    //  opções ao cliente
    char *msg = "Escolha uma opção: \n1. listar \n2. detalhar \n3. sair\n";
    send(client->socket, msg, strlen(msg), 0);

    while (1) {
        //  escolha do cliente
        bytesReceived = recv(client->socket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            printf("Cliente %s:%d desconectado.\n", client->ip, client->port);

            // Remove o cliente da lista
            for (int i = 0; i < clientCount; i++) {
                if (clients[i].socket == client->socket) {
                    remove_client(i);
                    break;
                }
            }

            break;
        }
        buffer[bytesReceived] = '\0';

        if (strncmp(buffer, "listar", 6) == 0) {
            // Lista os clientes 
            char listMsg[1024] = "Clientes conectados:\n";
            for (int i = 0; i < clientCount; i++) {
                char clientInfo[100];
                sprintf(clientInfo, "%s:%d\n", clients[i].ip, clients[i].port);
                strcat(listMsg, clientInfo);
            }
            send(client->socket, listMsg, strlen(listMsg), 0);
        } else if (strncmp(buffer, "detalhar", 8) == 0) {
            
            send(client->socket, "Digite o número do cliente: ", 26, 0);
            recv(client->socket, buffer, BUFFER_SIZE, 0);
            int clientIndex = atoi(buffer);
            if (clientIndex >= 0 && clientIndex < clientCount) {
                
                char details[1024];
                sprintf(details, "Cliente %d: %s:%d\n", clientIndex, clients[clientIndex].ip, clients[clientIndex].port);
                send(client->socket, details, strlen(details), 0);
                if (clients[clientIndex].data) {
                    char *jsonDetails = cJSON_Print(clients[clientIndex].data);
                    send(client->socket, jsonDetails, strlen(jsonDetails), 0);
                    free(jsonDetails);
                }
            } else {
                send(client->socket, "Cliente inválido.\n", 18, 0);
            }
        } else if (strncmp(buffer, "sair", 4) == 0) {
            
            break;
        }
    }

    closesocket(client->socket);
    return 0;
}

void server_control() {
    char command[BUFFER_SIZE];
    while (serverRunning) {
        printf("Digite 'close' para encerrar o servidor: ");
        fgets(command, BUFFER_SIZE, stdin);
        command[strcspn(command, "\n")] = '\0';  

        if (strcmp(command, "close") == 0) {
            printf("Encerrando o servidor...\n");
            serverRunning = 0;
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int addrLen = sizeof(clientAddr);

    WSAStartup(MAKEWORD(2, 2), &wsaData);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, MAX_CLIENTS);
    printf("Servidor ouvindo na porta %d...\n", PORT);

    InitializeCriticalSection(&cs);

   
    HANDLE controlThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)server_control, NULL, 0, NULL);

    while (serverRunning) {
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
        if (clientSocket == INVALID_SOCKET) continue;

        EnterCriticalSection(&cs);
        if (clientCount < MAX_CLIENTS) {
            clients[clientCount].socket = clientSocket;
            clients[clientCount].address = clientAddr;
            strncpy(clients[clientCount].ip, inet_ntoa(clientAddr.sin_addr), 16);
            clients[clientCount].port = ntohs(clientAddr.sin_port);
            printf("Novo cliente conectado: %s:%d\n", clients[clientCount].ip, clients[clientCount].port);
            CreateThread(NULL, 0, handle_client, &clients[clientCount], 0, NULL);
            clientCount++;
        } else {
            char *msg = "Servidor cheio.\n";
            send(clientSocket, msg, strlen(msg), 0);
            closesocket(clientSocket);
        }
        LeaveCriticalSection(&cs);
    }

    // Fecha o servidor
    closesocket(serverSocket);
    WSACleanup();
    DeleteCriticalSection(&cs);

    // Espera a thread de controle terminar
    WaitForSingleObject(controlThread, INFINITE);
    CloseHandle(controlThread);

    printf("Servidor encerrado.\n");
    return 0;
}
