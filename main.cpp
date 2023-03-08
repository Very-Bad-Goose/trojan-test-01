#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "libcrypto.lib")

#define AES_KEY_LENGTH 256
#define AES_BLOCK_SIZE 16

int main()
{
    SOCKET sock;
    sockaddr_in server_addr;
    WSADATA wsa;
    int connection;
    char ip_addr[16];
    int port;
    char RecvServer[512];
    SSL_CTX* ssl_ctx;
    SSL* ssl;
    char aes_key[AES_KEY_LENGTH/8];

    // initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("(!) Failed to initialize Winsock\n");
        return 1;
    }

    // get the IP address and port number from the user
    printf("Enter the IP address of the server: ");
    fgets(ip_addr, sizeof(ip_addr), stdin);
    ip_addr[strlen(ip_addr) - 1] = '\0'; // remove newline
    printf("Enter the port number of the server: ");
    scanf("%d", &port);
    getchar(); // remove newline

    // create a TCP socket
    if ((sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, (unsigned int)NULL, (unsigned int)NULL)) == INVALID_SOCKET) {
        printf("(!) Failed to create socket\n");
        WSACleanup();
        return 1;
    }

    // set up the socket address structure
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);

    // connect to the server
    if ((connection = WSAConnect(sock, (SOCKADDR*)&server_addr, sizeof(server_addr), NULL, NULL, NULL, NULL)) == SOCKET_ERROR)
    {
        printf("(!) Connection to the target server failed\n");
        WSACleanup();
        return 1;
    }

    // receive the server's welcome message
    recv(sock, RecvServer, sizeof(RecvServer), 0);
    printf("%s", RecvServer);

    // initialize OpenSSL
    SSL_load_error_strings();
    SSL_library_init();

    // create an SSL context
    ssl_ctx = SSL_CTX_new(TLSv1_2_client_method());
    if (!ssl_ctx) {
        printf("(!) Failed to create SSL context\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // set up the AES key
    memset(aes_key, 0, sizeof(aes_key));
    printf("Enter the AES encryption key (must be %d bits long): ", AES_KEY_LENGTH);
    fgets(aes_key, sizeof(aes_key), stdin);
    aes_key[strlen(aes_key) - 1] = '\0'; // remove newline

    // set up the SSL context to use AES-256 encryption
    if (SSL_CTX_set_cipher_list(ssl_ctx, "AES256-SHA") == 0) {
        printf("(!) Failed to set cipher list\n");
        SSL_CTX_free(ssl_ctx);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // create an SSL connection and attach it to the socket
    ssl = SSL_new(ssl_ctx);
    SSL_set_fd(ssl, sock);
    if (SSL_connect(ssl) == -1) {
        printf("(!) Failed to create SSL connection\n");
        SSL_CTX_free(ssl_ctx);
        SSL_free(ssl);
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    
    // send a message to the server
    char send_msg[512] = "Hello from the client!";
    if (SSL_write(ssl, send_msg, strlen(send_msg)) <= 0) {
        printf("(!) Failed to send message to the server\n");
        SSL_free(ssl);
        SSL_CTX_free(ssl_ctx);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // receive the server's response
    char recv_msg[512];
    int bytes_received = SSL_read(ssl, recv_msg, sizeof(recv_msg) - 1);
    if (bytes_received <= 0) {
        printf("(!) Failed to receive response from the server\n");
        SSL_free(ssl);
        SSL_CTX_free(ssl_ctx);
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    recv_msg[bytes_received] = '\0'; // null-terminate the received message
    printf("Received from server: %s\n", recv_msg);

    // clean up
    SSL_free(ssl);
    SSL_CTX_free(ssl_ctx);
    closesocket(sock);
    WSACleanup();
    return 0;
}
