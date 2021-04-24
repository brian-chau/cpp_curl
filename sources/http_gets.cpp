#define _XOPEN_SOURCE 700
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <curl/curl.h>
#include "http_gets.h"
#include <iostream>
#include <regex>
#include <string>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <stdio.h>

http_gets::http_gets(std::string hostname, unsigned int port)
{
    m_hostname = hostname;
    m_port     = port;
    m_result   = "";
}

http_gets::~http_gets()
{

}

template<typename... Args>
std::string fmt_str(std::string fmt, Args... args)
{
    size_t bufferSize = 1000;
    char *buffer = new char[bufferSize];
    int n = sprintf(buffer, fmt.c_str(), args...);
    assert (n >= 0 and n < (int) bufferSize - 1  && "check fmt_str output");

    std::string fmtStr (buffer);
    delete[] buffer;
    return fmtStr;
}

int http_gets::get_posix_way()
{
    m_result = "";

    char buffer[BUFSIZ];
    enum CONSTEXPR { MAX_REQUEST_LEN = 1024};
    char request[MAX_REQUEST_LEN];
    char request_template[] = "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
    struct protoent *protoent;
    in_addr_t in_addr;
    int request_len;
    int socket_file_descriptor;
    ssize_t nbytes_total, nbytes_last;
    struct hostent *hostent;
    struct sockaddr_in sockaddr_in;

    request_len = snprintf(request, MAX_REQUEST_LEN, request_template, m_hostname.c_str());
    if (request_len >= MAX_REQUEST_LEN) {
        std::cerr << fmt_str("request length large: %d", request_len) << std::endl;
        return EXIT_FAILURE;
    }

    /* Build the socket. */
    protoent = getprotobyname("tcp");
    if (NULL == protoent) {
        perror("getprotobyname");
        return EXIT_FAILURE;
    }

    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (-1 == socket_file_descriptor) {
        perror("socket");
        return EXIT_FAILURE;
    }

    /* Build the address. */
    hostent = gethostbyname(m_hostname.c_str());
    if (NULL == hostent){
        std::cerr << fmt_str("error: gethostbyname(%s)", m_hostname.c_str()) << std::endl;
        return EXIT_FAILURE;
    }
    in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
    if (in_addr == (in_addr_t)-1) {
        std::cerr << fmt_str("error: inetaddr(%s)", *(hostent->h_addr_list)) << std::endl;
        return EXIT_FAILURE;
    }
    sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(m_port);

    /* Actually connect. */
    if (connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
        perror("connect");
        return EXIT_FAILURE;
    }

    /* Send HTTP request. */
    nbytes_total = 0;
    while (nbytes_total < request_len) {
        nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
        if (-1 == nbytes_last) {
            perror("write");
            return EXIT_FAILURE;
        }
        nbytes_total += nbytes_last;
    }

    /* Read the response. */
    std::cerr << "debug: before first read" << std::endl;
    while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0) {
        std::cerr << "debug: after a read" << std::endl;
        m_result += buffer;
        memset(&buffer, 0, BUFSIZ);
    }

    std::cerr << "debug: after last read" << std::endl;
    if (-1 == nbytes_total) {
        perror("read");
        return EXIT_FAILURE;
    }

    close(socket_file_descriptor);

    // Remove invalid characters
    std::regex pattern("[^ -~\n\r]");
    m_result = std::regex_replace(m_result,pattern,"");
    
    std::cerr << "debug: before returning" << std::endl;
    return EXIT_SUCCESS;
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int http_gets::get_curl_way()
{
    CURL *curl;
    //CURLcode res;
    m_result = "";
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, m_hostname.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &m_result);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return EXIT_SUCCESS;
}
