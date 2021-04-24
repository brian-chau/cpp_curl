#include <string>

class http_gets{
public:
        http_gets(std::string hostname, unsigned int port=80);
       ~http_gets();
    int get_posix_way();
    int get_curl_way();

    std::string    m_result;
private:
    std::string    m_hostname;
    unsigned short m_port;
};
