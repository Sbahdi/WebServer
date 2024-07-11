
#pragma once


#include <iostream>

#include <list>

struct KeyValuee 
{
    std::string HttpHeader;
    std::string HttpValue;
    KeyValuee(const std::string &k, const std::string &v) : HttpHeader(k), HttpValue(v) {}
};


class RequestParsser
{
    protected:
        std::string HttpVersion;
        std::list<KeyValuee> HttpHeaders;
        std::string Body;
        std::string Method;
        std::string Line;
        int Fd;
    public:
        RequestParsser(int Fd);
        ~RequestParsser();
        RequestParsser(const RequestParsser &copy);
        RequestParsser &operator=(const RequestParsser &src);



        std::string GetHeader(std::string key) const;
        void PrintHeaders() const;
        // std::string GetBody() const;
        // std::string GetHttpVersion() const;
        // std::string GetRecivedLine() const;
        //
        // std::string GetHost() const;
        // std::string GetHeader(std::string Key) const;
        //
        // void PrintRequest() const;
        // void PrintHeaders() const;
};

