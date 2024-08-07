/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ybourais <ybourais@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/07 20:38:06 by ybourais          #+#    #+#             */
/*   Updated: 2024/07/09 16:09:40 by ybourais         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#define MAXLEN 8192

#include "HttpMessage.hpp"


class HttpRequest : public HttpMessage
{
    private:
        std::string HttpMethod;
        std::string Path;
    public:
        HttpRequest(std::string RecivedLine);
        ~HttpRequest();
        HttpRequest &operator=(const HttpRequest &s);
        HttpRequest(const HttpRequest &copy);

        std::string GetHttpMethod() const;
        std::string GetPath() const;
};


