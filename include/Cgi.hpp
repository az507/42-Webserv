/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xzhang <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/12 10:29:48 by xzhang            #+#    #+#             */
/*   Updated: 2025/01/06 14:57:11 by achak            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
# define CGI_HPP

# include "Client.hpp"
# include <string>
# include <sys/types.h>
# include <sys/socket.h>
# include <unistd.h>

class Client;

class CGI {
    public:
        CGI(int, int, int);
        int socketSend();
        void socketRecv();
        void setCgiInput(std::string const&);
        void cleanup();
        std::pair<int, int> getFds() const;
        int getIOState() const;
        bool operator==(int) const;
    private:
        int ignoreStartLine();
        void parseCgiOutput(const char *, ssize_t);
        int parseMsgBody();
        int parseCgiHeaders();
        int unchunkMsgBody();
        void setClientReady();
        void setErrorState(int);
        void setFinishedState();
        void configureIOHandling();

        int _pstate;
        int _iostate;
        int _activefd;
        int _passivefd;
        bool _unchunkflag;
        bool _tracklength;
        size_t _recvbytes;
        std::string _cgistdin;
        std::string _clientbuf;
        std::string::const_iterator _send_it;
        std::string::const_iterator _send_ite;
        std::map<std::string, std::string> _headers;
};

#endif
