#include <sys/epoll.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
/*

       struct sockaddr_in {
           sa_family_t     sin_family;     AF_INET
           in_port_t       sin_port;       Port number
           struct in_addr  sin_addr;       IPv4 address
       };
*/

#define MAX_EVENTS 10

int setnonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
    {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void do_use_fd(int fd)
{
    char buf[4096];
    ssize_t r = 0;

    // Edge-triggered: we MUST read until EAGAIN, or we will miss data
    for (;;)
    {
        r = read(fd, buf, sizeof(buf));

        if (r > 0)
        {
            ssize_t sent = 0;

            while (sent < r)
            {
                // MSG_NOSIGNAL: a closed client must not kill the server with SIGPIPE
                ssize_t w = send(fd, buf + sent, r - sent, MSG_NOSIGNAL);

                if (w == -1)
                {
                    if (errno == EINTR) continue;
                    break;      // EAGAIN (send buffer full): proper handling is milestone 3
                }
                sent += w;
            }
        }
        else if (r == 0)
        {
            close(fd);          // client disconnected; close also removes it from epoll
            return;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;         // no more data right now
            }
            if (errno == EINTR) continue;

            perror("read");
            close(fd);
            return;
        }
    }
}    


int main(int argc, char * argv[]) 
{
    int listen_sock = 0;
    int Port = 0;
    int iRet = 0;
    int n = 0;

    struct epoll_event ev, events[MAX_EVENTS];
    
    int conn_sock, nfds,epollfd = 0;

    printf("Jay Ganesh....\n");

    struct sockaddr_in ServerAddr;
    struct sockaddr_in ClientAddr;

    socklen_t AddrLen = sizeof(ClientAddr);


    if((argc < 2) || (argc > 2))
    {
        printf("Unable to Proceed as Invalid Number of arguments\n");
        printf("Please Provide The Port Number\n");
        return -1;
    } 

    Port = atoi(argv[1]);

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Step 1: Create TCP Socket
    /////////////////////////////////////////////////////////////////////////////////////////////////
    listen_sock = socket(AF_INET,SOCK_STREAM,0);

    int opt = 1;
    
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if(listen_sock == -1)
    {
        perror("socket");

        return -1;
    }
    else 
    {
        printf("Socket created successfully with file fd: %d\n",listen_sock);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // Step 2: Bind Socket to IP and Port Number
    //////////////////////////////////////////////////////////////////////////////////////////////////

    memset(&ServerAddr,0,sizeof(ServerAddr));

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = INADDR_ANY;
    ServerAddr.sin_port = htons(Port);

    iRet = bind(listen_sock,(struct sockaddr *)&ServerAddr,sizeof(ServerAddr));

    if(iRet == -1)
    {
        printf("Unable to bind ip and port to socket\n");
        perror("bind");
        close(listen_sock);
        return -1;
    }
    else 
    {
        printf("IP address and port number binded successfully to client\n");
    }

    iRet = listen(listen_sock,11);

    if(iRet == -1)
    {
        printf("Server unable to listen\n");
        close(listen_sock);
        return -1;
    }


    printf("Server is running on port: %d\n",Port);

    epollfd = epoll_create1(0);    //same as epoll_create 

    if (epollfd == -1) 
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) 
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    for (;;) {

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //  Step 4: Accept the Client request
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        memset(&ClientAddr,0,sizeof(ClientAddr));

        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        
        if (nfds == -1) 
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (n = 0; n < nfds; ++n) 
        {
            if (events[n].data.fd == listen_sock) 
            {
                AddrLen = sizeof(ClientAddr);
                conn_sock = accept(listen_sock,(struct sockaddr *) &ClientAddr, &AddrLen);
                if (conn_sock == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                setnonblocking(conn_sock);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock,&ev) == -1) 
                {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            } 
            else 
            {
                do_use_fd(events[n].data.fd);
            }
        }
    }

    return 0;
}