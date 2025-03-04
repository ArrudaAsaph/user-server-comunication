#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "include/cJSON.h"

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

int get_num_processors() {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}

long get_free_ram() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullAvailPhys / (1024 * 1024); // MB
}

long get_free_disk_space() {
    ULARGE_INTEGER freeBytesAvailable;
    GetDiskFreeSpaceEx("C:\\", &freeBytesAvailable, NULL, NULL);
    return freeBytesAvailable.QuadPart / (1024 * 1024 * 1024); // GB
}

float get_cpu_temperature() {
    FILE *fp;
    char path[1035];
    float temperatura = -273.15; // Valor padrão caso falhe

    fp = _popen("powershell -Command \"(Get-WmiObject MSAcpi_ThermalZoneTemperature -Namespace 'root/wmi').CurrentTemperature\"", "r");
    if (fp) {
        if (fgets(path, sizeof(path), fp) != NULL) {
            temperatura = atof(path) / 10.0 - 273.15; // Celsius
        }
        _pclose(fp);
    }

    return temperatura;
}

float calcular_media(int cpu, long ram, long disk, float temperatura) {
    return (cpu + ram + disk + temperatura) / 4.0;
}

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Erro ao inicializar Winsock.\n");
        return 1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Erro ao criar socket.\n");
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Erro ao conectar ao servidor.\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Coletar informações do sistema
    int cpu = get_num_processors();
    long ram = get_free_ram();
    long disk = get_free_disk_space();
    float temp = get_cpu_temperature();
    float media = calcular_media(cpu, ram, disk, temp);

    // Criar JSON com as informações
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "cpu", cpu);
    cJSON_AddNumberToObject(json, "ram", ram);
    cJSON_AddNumberToObject(json, "disk", disk);
    cJSON_AddNumberToObject(json, "temperature", temp);
    cJSON_AddNumberToObject(json, "average", media);

    char *jsonString = cJSON_Print(json);
    send(clientSocket, jsonString, strlen(jsonString), 0);

    cJSON_Delete(json);
    free(jsonString);

    // Menu de interação
    while (1) {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            printf("Servidor desconectou.\n");
            break;
        }

        buffer[bytesReceived] = '\0';
        printf("%s", buffer);

        printf("Escolha uma opção: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        send(clientSocket, buffer, strlen(buffer), 0);

        if (strncmp(buffer, "sair", 4) == 0) break;
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
