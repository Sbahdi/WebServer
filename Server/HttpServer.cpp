/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpServer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ybourais <ybourais@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/07 21:14:22 by ybourais          #+#    #+#             */
/*   Updated: 2024/07/08 02:51:48 by ybourais         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpServer.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <map>
#include <stdint.h> // Include for uint32_t definition
#include <sys/_select.h>
#include <sys/_types/_fd_def.h>
#include <sys/socket.h>
#include <vector>
#include "../Tools/Tools.hpp"


#define SP " "

HttpServer::HttpServer(const ErrorsChecker &checker)
{
    // PF_INET: ipv4 or (AF_UNSPEC) for both
    // SOCK_STREAM: TCP
    this->ServerFd = socket(AF_INET, SOCK_STREAM, 0);
    this->FdConnection = 0;
    if (ServerFd  == -1) 
    { 
        throw std::runtime_error(std::string("Socket: ") + strerror(errno));
    }
    memset(&Address, 0,sizeof(Address));
    Address.sin_family = AF_INET; // ipv4
    Address.sin_addr.s_addr = htonl(INADDR_ANY); // any availabe local iP, we use htonl to convert the INADDR_ANY constant to network byte order
    Address.sin_port = htons(PORT); // 
    memset(RecivedRequest, 0, MAXLEN);
    
    Parsing parse(checker.GetConfigFilePath());
    this->ServerSetting = parse.getServers();
}

void HttpServer::SetFdToNonBlocking()
{

    int flags = fcntl(this->ServerFd, F_GETFL, 0);
    if (flags == -1) 
    {
        throw std::runtime_error(std::string("fcntl: ") + strerror(errno));
    }
    if (fcntl(this->ServerFd, F_SETFL, flags | O_NONBLOCK) == -1) 
    {
        throw std::runtime_error(std::string("fcntl: ") + strerror(errno));
    }
}

HttpServer::~HttpServer()
{
    close(ServerFd);
}

std::string HttpServer::GetRequest() const
{
    return this->RecivedRequest;
}

void HttpServer::ForceReuse() const
{
    int opt = 1;
    // it helps in reuse of address and port. Prevents error such as: “address already in use”
    if (setsockopt(this->ServerFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
    {
        throw std::runtime_error(std::string("setsockopt:") + strerror(errno));
    }
}

void HttpServer::BindSocketToAddr() const
{

    if(bind(ServerFd, (struct sockaddr *)&Address, sizeof(Address)) < 0)
    {
        throw std::runtime_error(std::string("bind: ") + strerror(errno));
    }
}

void HttpServer::StartListining(int PendingConection) const
{
    //socket turn into passive listening state
    if(listen(ServerFd, PendingConection) < 0)
    {
        throw std::runtime_error(std::string("listen:") + strerror(errno));
    }
}


#include <iostream>
#include <arpa/inet.h>     // for inet_ntop

void printClientInfo(const struct sockaddr* client_addr, socklen_t client_addr_size) 
{
    if (client_addr == NULL || client_addr_size == 0) 
    {
        std::cerr << "Invalid client address information." << std::endl;
        return;
    }

    // Check address family (IPv4 or IPv6)
    if (client_addr->sa_family == AF_INET) 
    {
        const struct sockaddr_in* addr_in = reinterpret_cast<const struct sockaddr_in*>(client_addr);

        // Convert IP address to string
        char ip_str[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, sizeof(ip_str)) == NULL) 
        {
            perror("inet_ntop");
            return;
        }
        // Print IP address and port
        std::cout << "Client IP: " << ip_str << std::endl;
        std::cout << "Client Port: " << ntohs(addr_in->sin_port) << std::endl;
    }
    else 
    {
        std::cerr << "Unsupported address family." << std::endl;
    }
}

void setNonBlocking(int fd) 
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) 
    {
        throw std::runtime_error(std::string("fcntl: ") + strerror(errno));
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) 
    {
        throw std::runtime_error(std::string("fcntl: ") + strerror(errno));
    }
}

void HttpServer::AccepteConnectionAndRecive()
{
    std::cout <<"waiting for conection on Port: " << PORT <<std::endl;
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    this->FdConnection = accept(ServerFd, (struct sockaddr *)&client_addr, &client_addrlen);
    // printClientInfo((struct sockaddr *)&client_addr, client_addrlen);
    if(FdConnection < 0)
    {
        throw std::runtime_error(std::string("accept:") + strerror(errno));
    }
    int r = recv(FdConnection, this->RecivedRequest, MAXLEN - 1, 0);
    if(r < 0)
    {
        throw std::runtime_error(std::string("reaciving from file:"));
    }
}

// void HttpServer::AccepteMultipleConnectionAndRecive()
// {
//
//     fd_set  master_set, working_set;
//     int max_sd;
//     int close_con;
//
//    /* Initialize the master fd_set */
//     FD_ZERO(&master_set);
//     max_sd = this->ServerFd;
//     FD_SET(this->ServerFd, &master_set);
//
//
//    /* Initialize the timeval struct to 3 minutes */
//    struct timeval      timeout;
//    timeout.tv_sec  = 3 * 60;
//    timeout.tv_usec = 0;
//
//    int    desc_ready, end_server = false;
//
//    /* Loop waiting for incoming connects or for incoming data */
//    /* on any of the connected sockets.*/
//     while(true)
//     {
//
//         /* Copy the master fd_set over to the working fd_set.     */
//         memcpy(&working_set, &master_set, sizeof(master_set));
//
//         /* Call select() and wait 3 minutes for it to complete.   */
//         int rc = select(max_sd + 1, &working_set, NULL, NULL, NULL);
//         if (rc < 0)
//             throw std::runtime_error(std::string("select:"));
//
//         /* Check to see if the 3 minute time out expired.         */
//         if (rc == 0)
//             throw std::runtime_error(std::string("select:timeout:"));
//
//         /* One or more descriptors are readable.  Need to         */
//         /* determine which ones they are.                         */
//         int i = 0;
//         int desc_ready = rc;
//         while(i <= max_sd && desc_ready > 0)
//         {
//
//             /* Check to see if this descriptor is ready            */
//             if (FD_ISSET(i, &working_set))
//             {
//
//                 /* A descriptor was found that was readable - one   */
//                 /* less has to be looked for.  This is being done   */
//                 /* so that we can stop looking at the working set   */
//                 /* once we have found all of the descriptors that   */
//                 /* were ready.                                      */
//                 desc_ready -= 1;
//
//                 /* Check to see if this is the listening socket     */
//                 if (i == this->ServerFd)
//                 {
//                     std::printf("Listening socket is readable\n");
//
//
//                     /* Loop and accept another incoming conncection */
//                     // while(FdConnection != 1)
//                     // {
//
//                         /* Accept all incoming connections that are      */
//                         /* queued up on the listening socket before we   */
//                         /* loop back and call select again.              */
//                         this->AccepteConnectionAndRecive();
//                     // }
//
//                     /* Add the new incoming connection to the     */
//                     /* master read set                            */
//                     std::printf("New incoming connection - %d\n", this->FdConnection);
//                     FD_SET(this->FdConnection, &master_set);
//                     if(this->FdConnection > max_sd)
//                         max_sd = this->FdConnection;
//
//                 }
//
//                 /* This is not the listening socket, therefore an   */
//                 /* existing connection must be readable             */
//                 else 
//                 {
//
//                     std::printf("  Descriptor %d is readable\n", i);
//                     close_con = false;
//                     while (true) 
//                     {
//
//
//                         /* Receive all incoming data on this socket      */
//                         /* before we loop back and call select again.    */
//                         HttpRequest Request(this->GetRequest());
//                         HttpResponse Response(Request);
//                         this->SendMultiResponse(Response, i);
//                         break;
//                     }
//                     if(close_con)
//                     {
//                         close(i);
//                         FD_CLR(i, &master_set);
//                         if(i == max_sd)
//                         {
//                             while(FD_ISSET(max_sd, &master_set) == false)
//                                 max_sd -= 1;
//                         }
//                     }
//
//                 } /* End of existing connection is readable */
//
//
//             }/*End of if (FD_ISSET(i, &working_set)) */
//
//         } /* End of loop through selectable descriptors */
//     }
//
//     ///close all
//     for (int i = 0; i <= max_sd; ++i)
//     {
//       if (FD_ISSET(i, &master_set))
//          close(i);
//     }
// }





int HttpServer::AccepteConnection()
{   
    static int a;
    std::cout <<"waiting for conection on Port: " << PORT << " :"<< a++<<std::endl;
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    int neww = accept(ServerFd, (struct sockaddr *)&client_addr, &client_addrlen);
    // this->FdConnection = accept(ServerFd, (struct sockaddr *)&client_addr, &client_addrlen);
    // printClientInfo((struct sockaddr *)&client_addr, client_addrlen);
    if(neww < 0)
    {
        throw std::runtime_error(std::string("accept:") + strerror(errno));
    }
    return neww;
}


const std::string HttpServer::ReciveData(int fd)
{    
    int r = recv(fd, this->RecivedRequest, MAXLEN - 1, 0);
    if(r < 0)
    {
        throw std::runtime_error(std::string("recv: "));
    }
    return this->RecivedRequest;
}

const std::string HttpServer::GenarateResponse(const HttpResponse &Response, int fd) const
{
    std::string FinalResponse = Response.GetHttpVersion() + SP + Response.HTTPStatusCodeToString() + SP + Response.GetHttpStatusMessage() + "\r\n";
    std::list<KeyValue>::const_iterator it = Response.GetHeadersBegin();
    std::list<KeyValue>::const_iterator itend = Response.GetHeadersEnd();
    while(it != itend)
    {
        FinalResponse += it->HttpHeader + it->HttpValue + '\n';
        it++;
    }
    FinalResponse += "\r\n";
    FinalResponse += Response.GetResponseBody();
    
    int response_length = strlen(FinalResponse.c_str());
    return FinalResponse;
}

ssize_t HttpServer::SendMultiResponse(const HttpResponse &Response, int fd, int *helper) const
{
    std::string FinalResponse = Response.GetHttpVersion() + SP + Response.HTTPStatusCodeToString() + SP + Response.GetHttpStatusMessage() + "\r\n";
    std::list<KeyValue>::const_iterator it = Response.GetHeadersBegin();
    std::list<KeyValue>::const_iterator itend = Response.GetHeadersEnd();
    while(it != itend)
    {
        FinalResponse += it->HttpHeader + it->HttpValue + '\n';
        it++;
    }
    FinalResponse += "\r\n";
    FinalResponse += Response.GetResponseBody();
    
    int response_length = strlen(FinalResponse.c_str());
    *helper = response_length;
    ssize_t sendingbyt;
    sendingbyt = send(fd, FinalResponse.c_str(), response_length, 0) ;
    if(sendingbyt != response_length) 
    {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
        {
            std::cout << "byte_sent: "<< sendingbyt << "response_length: "<< response_length<<std::endl;
            throw std::runtime_error(std::string("send to FdConnection:"));
        }
        else 
        {
            std::cout << "send blocked"<<std::endl;
        }
    }
    return sendingbyt;
}

void logMessage(const std::string &message) {
    std::ofstream log_file("server.log", std::ios_base::out | std::ios_base::app);
    std::time_t now = std::time(0);
    log_file << std::ctime(&now) << ": " << message << std::endl;
}

void HttpServer::AccepteMultipleConnectionAndRecive()
{
    int activity;
    int max_fd;
    fd_set current_set, ready_set;

    fd_set ready_readfds, ready_writefds, current_readfds, current_writefds;

    //init the set of fd
    FD_ZERO(&current_readfds);
    FD_ZERO(&current_writefds);
    FD_SET(this->ServerFd, &current_readfds);
    max_fd = this->ServerFd;

    struct timeval      timeout;

    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;

    std::vector<int > Fds;
    std::map<int , std::string> buffers;
    std::map<int, std::string> ResponseMsg;
    while(true)
    {
        ready_readfds = current_readfds; ready_writefds = current_writefds;

        activity = select(max_fd + 1, &ready_readfds, &ready_writefds, NULL, NULL);
        if(activity < 0)
            throw std::runtime_error(std::string("selcet:") + strerror(errno));
        if(activity == 0)
            continue;

        //loping to accepte new fd and read from them
        for(int i = this->ServerFd;i <= max_fd;i++)
        {
            if(FD_ISSET(i, &ready_readfds))
            {
                // new connection
                if(i == this->ServerFd)
                {
                    int new_fd = this->AccepteConnection();
                    logMessage("Accepted connection, fd: " + std::to_string(new_fd));
                    setNonBlocking(new_fd);
                    FD_SET(new_fd, &current_readfds);
                    FD_SET(new_fd, &current_writefds);
                    if (new_fd > max_fd) 
                        max_fd = new_fd;
                }
                // reading operation
                else 
                {
                    //reding data from the socket of the client
                    buffers[i] = this->ReciveData(i);
                    logMessage("Received data from fd: " + std::to_string(i));
                    Fds.push_back(i);
                    FD_SET(i, &current_writefds);
                }
            }
        }
        // loping and checking if some fd in the writing set is ready to write to its fd (send respose to socket client)
        for (int j = 0;j < Fds.size();j++) 
        {
            int fd = Fds[j];
            if (FD_ISSET(fd, &ready_writefds)) // indicates that the socket's send buffer has space available to accept more data
            {
                if(ResponseMsg.find(fd) == ResponseMsg.end())
                {
                    HttpRequest Request(buffers[fd]);
                    HttpResponse Response(Request);
                    ResponseMsg[fd] = this->GenarateResponse(Response, fd);
                }
                
                std::string &message = ResponseMsg[fd];
                int bytes_sent = this->SendChunckedResponse(fd, message);
                std::cout << bytes_sent<<std::endl;
                if (bytes_sent < 0)
                {
                    close(fd);
                    FD_CLR(fd, &current_readfds);
                    FD_CLR(fd, &current_writefds);
                    Fds.erase(Fds.begin() + j);
                    j--;
                    ResponseMsg.erase(fd);
                }
                else
                {
                    logMessage("Sent " + std::to_string(bytes_sent) + " bytes to fd: " + std::to_string(fd));
                    message.erase(0, bytes_sent); // Remove sent bytes from buffer
                    if (message.empty())
                    {
                        // Entire response has been sent
                        close(fd);
                        FD_CLR(fd, &current_readfds);
                        FD_CLR(fd, &current_writefds);
                        Fds.erase(Fds.begin() + j);
                        j--;
                        ResponseMsg.erase(fd);
                        logMessage("Connection closed, fd: " + std::to_string(fd));
                    }
                }
            }
        }
    }
}

int HttpServer::SendChunckedResponse(int fd, std::string &message)
{
    int bytes_sent = send(fd, message.c_str(), message.size(), 0);
    return bytes_sent;
}

// void HttpServer::AccepteMultipleConnectionAndRecive()
// {
//     int activity;
//     int max_fd;
//     fd_set current_set, ready_set;
//
//     FD_ZERO(&current_set);
//     FD_SET(this->ServerFd, &current_set);
//     max_fd = this->ServerFd;
//
//     struct timeval      timeout;
//     timeout.tv_sec  = 3 * 60;
//     timeout.tv_usec = 0;
//
//     while(true)
//     {
//         ready_set = current_set;
//
//         activity = select(max_fd + 1, &ready_set, NULL, NULL, NULL);
//         if(activity < 0)
//             throw std::runtime_error(std::string("selcet:") + strerror(errno));
//         for(int i = ServerFd;i <= max_fd;i++)
//         {
//             if(FD_ISSET(i, &ready_set))
//             {
//
//                 //new connection
//                 if(i == this->ServerFd)
//                 {
//                     int new_fd = this->AccepteConnection();
//                     setNonBlocking(new_fd);
//                     if (new_fd >= 0) 
//                     {
//                         FD_SET(new_fd, &current_set);
//                         if (new_fd > max_fd) 
//                             max_fd = new_fd;
//                     }
//                 }
//                 else 
//                 {
//                     this->ReciveData(i);
//                     HttpRequest Request(this->GetRequest());
//                     HttpResponse Response(Request);
//                     this->SendMultiResponse(Response, i);
//                     close(i);
//                     FD_CLR(i, &current_set);
//                 }
//             }
//         }
//     }
// }

void HttpServer::SendResponse(const HttpResponse &Response) const
{
    std::string FinalResponse = Response.GetHttpVersion() + SP + Response.HTTPStatusCodeToString() + SP + Response.GetHttpStatusMessage() + "\r\n";
    std::list<KeyValue>::const_iterator it = Response.GetHeadersBegin();
    std::list<KeyValue>::const_iterator itend = Response.GetHeadersEnd();
    while(it != itend)
    {
        FinalResponse += it->HttpHeader + it->HttpValue + '\n';
        it++;
    }
    FinalResponse += "\r\n";
    FinalResponse += Response.GetResponseBody();
    
    int response_length = strlen(FinalResponse.c_str());
    // if(send(FdConnection, FinalResponse.c_str(), response_length, MSG_DONTWAIT) != response_length) 
    if(send(FdConnection, FinalResponse.c_str(), response_length, 0) != response_length) 
    {
        throw std::runtime_error(std::string("send to FdConnection:"));
    }
    close(FdConnection);
}


