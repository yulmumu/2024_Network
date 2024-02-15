#include "lib.h"
#include <fstream>
#include <sstream>
#include <map>

const int MAX_BUFFER_SIZE = 1024;
const int TIMEOUT_SECONDS = 5;

std::map<std::string, std::string> pathToContent = {
    {"/", "<html><body><h1>Welcome to the Main Page</h1><ul><li><a href='/page1'>Page 1</a></li><li><a href='/page2'>Page 2</a></li><li><a href='/data1'>Data 1</a></li><li><a href='/data2'>Data 2</a></li><li><a href='/about'>About</a></li></ul></body></html>"},
    {"/page1", "<html><body><h1>This is Page 1</h1></body></html>"},
    {"/page2", "<html><body><h1>This is Page 2</h1></body></html>"},
    {"/data1", R"({"key1": "value1", "key2": "value2"})"},
    {"/data2", R"({"key3": "value3", "key4": "value4"})"},
    {"/about", "<html><body><h1>About Us</h1></body></html>"},
    
};

void HandleConnection(SOCKET clientSocket);
std::string GetContent(const std::string& path);

int main() {
    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup() error" << std::endl;
        return 1;
    }
    #endif

    SOCKET servsock = socket(AF_INET, SOCK_STREAM, 0);
    if (servsock == INVALID_SOCKET) {
        std::cerr << "socket() error" << std::endl;
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }

    sockaddr_in servaddr{};
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8080);  

    if (bind(servsock, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)) == SOCKET_ERROR) {
        std::cerr << "bind() error" << std::endl;
        closesocket(servsock);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }

    if (listen(servsock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen() error" << std::endl;
        closesocket(servsock);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }

    std::cout << "Server is running. Waiting for connections..." << std::endl;

    while (true) {
        SOCKET clisock = accept(servsock, nullptr, nullptr);
        if (clisock == INVALID_SOCKET) {
            std::cerr << "accept() error" << std::endl;
            closesocket(servsock);
            #ifdef _WIN32
            WSACleanup();
            #endif
            return 1;
        }

        #ifdef _WIN32
        u_long mode = 1; 
        ioctlsocket(clisock, FIONBIO, &mode);
        #else
        fcntl(clisock, F_SETFL, O_NONBLOCK);
        #endif

        std::cout << "Client Connected" << std::endl;

        HandleConnection(clisock);
    }

    #ifdef _WIN32
    WSACleanup();
    #endif

    return 0;
}

void HandleConnection(SOCKET clientSocket) {
    char buffer[MAX_BUFFER_SIZE];

    int recvlen = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (recvlen == SOCKET_ERROR) {
        std::cerr << "recv() error" << std::endl;
        closesocket(clientSocket);
        return;
    }

    std::string request(buffer, recvlen);

    std::string path;
    size_t start = request.find(" ") + 1;
    size_t end = request.find(" ", start);
    if (start != std::string::npos && end != std::string::npos) {
        path = request.substr(start, end - start);
    }

    std::string content = GetContent(path);

    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + content;
    send(clientSocket, response.c_str(), response.size(), 0);

    closesocket(clientSocket);
}

std::string GetContent(const std::string& path) {
    auto it = pathToContent.find(path);
    if (it != pathToContent.end()) {
        if (path == "/about") {
            std::ifstream file("about.html");
            if (file) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                return buffer.str();
            }
        }
        return it->second;
    } else {
        return "<html><body><h1>Welcome to the Main Page</h1><ul><li><a href='/page1'>Page 1</a></li><li><a href='/page2'>Page 2</a></li><li><a href='/data1'>Data 1</a></li><li><a href='/data2'>Data 2</a></li><li><a href='/about'>About</a></li></ul></body></html>";
    }
}
