#include <utility>
#include <iostream>
#include <fstream>
#include <memory>
#include "http_gets.h" 
int main( int argc, char** argv )
{
    std::string hostname("");
    unsigned int server_port(0);
    if (argc > 1)
        hostname = argv[1];
    if (argc > 2)
        server_port = strtoul(argv[2], NULL, 10);
    else
        server_port = 80;

    std::unique_ptr<http_gets> curl = std::unique_ptr<http_gets>(new http_gets(hostname,server_port));
    curl->get_posix_way();
    //curl->get_curl_way();

    std::fstream fs;
    fs.open ("test.txt", std::fstream::in | std::fstream::out | std::fstream::app);
    fs << curl->m_result;
    fs.close();

    return 0;
}
