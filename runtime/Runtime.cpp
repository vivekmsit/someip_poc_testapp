//
// Created by VSharma on 1/22/2018.
//

//
// Created by VSharma on 1/5/2018.
//
#include <iostream>
#include <fcntl.h>
#include "Runtime.h"

Runtime *Runtime::getInstance() {
    static Runtime runtime_;
    return &runtime_;
}

void Runtime::serverThreadFunction() {
    std::cout << "Runtime::serverThreadFunction() start" << std::endl;
    mainLoopThread();
    std::cout << "Runtime::serverThreadFunction() end" << std::endl;
}

void Runtime::clientThreadFunction() {
    std::cout << "Runtime::clientThreadFunction() start" << std::endl;
    initMultiCastTimeSocket("224.244.224.246", 30491);
    while (!stopThreads) {
        sendPeriodicMessages();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    std::cout << "Runtime::clientThreadFunction() end" << std::endl;
}

bool Runtime::init(std::string destinationAddress) {
    std::cout << "Runtime::init() start" << std::endl;
    destinationAddress_ = destinationAddress;
    m_serverThread = std::thread(&Runtime::serverThreadFunction, this);
    m_clientThread = std::thread(&Runtime::clientThreadFunction, this);
    std::cout << "Runtime::init() end" << std::endl;
    return true;
}

bool Runtime::deInit() {
    std::cout << "Runtime::deInit() start" << std::endl;
    stopThreads = true;
    std::cout << "Runtime::deInit() end" << std::endl;
    return true;
}

bool
Runtime::sendMultiCastMessage(std::string broadCastAddress, int portNumber, std::string message) {
    struct sockaddr_in multiAddress;
    int multiFd;

    std::cout << "Runtime::sendMultiCastMessage() start, broadcast address: " << broadCastAddress << " portNumber: "
              << portNumber << std::endl;

    /* set up socket */
    multiFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (multiFd < 0) {
        std::cout << "error Inside Runtime::sendMultiCastMessage() function, after socket, error: "
                  << std::string(strerror(errno)) << std::endl;
        return false;
    }

    bzero((char *) &multiAddress, sizeof(multiAddress));
    multiAddress.sin_family = AF_INET;
    multiAddress.sin_port = htons(portNumber);
    multiAddress.sin_addr.s_addr = inet_addr(broadCastAddress.c_str());
    socklen_t addressLength = sizeof(multiAddress);

    std::cout << "Runtime::sendMultiCastMessage(), sending multiCast message: " << message << std::endl;
    ssize_t cnt = sendto(multiFd, message.c_str(), message.length(), 0,
                         (struct sockaddr *) &multiAddress, addressLength);
    if (cnt < 0) {
        std::cout << "error Inside Runtime::sendMultiCastMessage() function, sendto() failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        return false;
    } else {
        std::cout << "Runtime::sendMultiCastMessage() multicast data sent successfully" << std::endl;
    }
    close(multiFd);
    std::cout << "Runtime::sendMultiCastMessage() end" << std::endl;
    return true;
}

bool Runtime::sendTcpMessage(std::string destinationAddress, int portNumber, std::string message) {
    std::cout <<
              "Runtime::sendTcpMessage() start, destination: " << destinationAddress << " portNumber:" << portNumber
              << std::endl;
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "error Inside Runtime::sendTcpMessage(), socket() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        return false;
    }

    // set sock file descriptor as NON BLOCKING
    int flags = fcntl(sock, F_GETFL);
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cout << "error Inside Runtime::sendTcpMessage(), fcntl() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        close(sock);
        return false;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portNumber);
    std::cout << "Runtime::sendTcpMessage() Test1" << std::endl;

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, destinationAddress.c_str(), &serv_addr.sin_addr) <= 0) {
        // Invalid address or Address not supported
        std::cout << "error Inside Runtime::sendTcpMessage(), inet_pton() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        close(sock);
        return false;
    }

    std::cout << "Runtime::sendTcpMessage() Test2" << std::endl;
    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        if (errno == EINPROGRESS) {
            struct pollfd fds[5];
            memset(fds, 0, sizeof(fds));
            fds[0].fd = sock;
            fds[0].events = POLLOUT;
            nfds_t numberOfFds = 1;
            int result = poll(fds, numberOfFds, 5000);
            if (result < 0) {
                std::cout << "error Inside Runtime::sendTcpMessage(), poll() system call failed, error: "
                          << std::string(strerror(errno)) << std::endl;
                close(sock);
                return false;
            } else if (result == 0) {
                std::cout << "error Inside Runtime::sendTcpMessage(), timeout happened on poll()" << std::endl;
                close(sock);
                return false;
            } else {
                std::cout << "Runtime::sendTcpMessage(), TCP connection created successfully" << std::endl;
            }
        } else {
            std::cout << "error Inside Runtime::sendTcpMessage(), connect() call failed, error: "
                      << std::string(strerror(errno)) << std::endl;
            close(sock);
            return false;
        }
    }

    std::cout << "Runtime::sendTcpMessage() Test3" << std::endl;
    if (send(sock, message.c_str(), message.length(), 0) <= 0) {
        std::cout << "error Inside Runtime::sendTcpMessage(), send() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        close(sock);
        return false;
    }

    std::cout << "Runtime::sendTcpMessage() Test4" << std::endl;
    if (read(sock, buffer, 1024) <= 0) {
        std::cout << "error Inside Runtime::sendTcpMessage(), read() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        close(sock);
        return false;
    } else {
        std::cout << "Runtime::sendTcpMessage(), received message " << buffer << std::endl;
    }

    close(sock);
    std::cout << "Runtime::sendTcpMessage() end" << std::endl;
    return true;
}

bool Runtime::sendUdpMessage(std::string destinationAddress, int portNumber, std::string message) {
    int sockfd;
    socklen_t serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char buffer[1024];

    std::cout << "Runtime::sendUdpMessage() start, destination: " << destinationAddress << " portNumber: " << portNumber
              << std::endl;

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cout << "error Inside Runtime::sendUdpMessage(), socket() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        return false;
    }

    // set sock file descriptor as NON BLOCKING
    int flags = fcntl(sockfd, F_GETFL);
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cout << "error Inside Runtime::sendUdpMessage(), fcntl() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        close(sockfd);
        return false;
    }

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(destinationAddress.c_str());
    if (server == nullptr) {
        std::cout << "error Inside Runtime::sendUdpMessage(), gethostbyname() call returned NULL, error: "
                  << std::string(strerror(errno)) << std::endl;
        close(sockfd);
        return false;
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy(server->h_addr,
          (char *) &serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portNumber);

    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    if (sendto(sockfd, message.c_str(), message.length(), 0, (const sockaddr *) &serveraddr,
               serverlen) < 0) {
        std::cout << "error Inside Runtime::sendUdpMessage(), sendto() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        close(sockfd);
        return false;
    }

    /* print the server's reply */
    /*if (recvfrom(sockfd, buffer, strlen(buffer), 0, (sockaddr *) &serveraddr, &serverlen) < 0) {
        std::cout << "error Inside Runtime::sendUdpMessage(), recvfrom() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        return false;
    } else {
        std::cout << "Runtime::sendUdpMessage(), received message: " << buffer << std::endl;
    }*/
    close(sockfd);
    std::cout << "Runtime::sendUdpMessage() end" << std::endl;
    return true;
}

bool Runtime::initMultiCastTimeSocket(std::string multiCastAddress, int portNumber) {
    std::cout << "Runtime::initMulticastUdpConnection() start" << std::endl;
    /* set up socket */
    multiCastSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (multiCastSocketFd < 0) {
        std::cout << "error Inside Runtime::initMulticastUdpConnection() function, after socket, error: "
                  << std::string(strerror(errno)) << std::endl;
        return false;
    }
    bzero((char *) &multiCastAddress_, sizeof(multiCastAddress_));
    multiCastAddress_.sin_family = AF_INET;
    multiCastAddress_.sin_port = htons(portNumber);
    multiCastAddress_.sin_addr.s_addr = inet_addr(multiCastAddress.c_str());
    std::cout << "Runtime::initMulticastUdpConnection() end" << std::endl;
    return true;
}

bool Runtime::sendMultiCastTimeMessage() {
    char message[100];
    time_t t = time(0);
    socklen_t addressLength = sizeof(multiCastAddress_);
    std::cout << "Runtime::sendMultiCastMessage() start" << std::endl;
    sprintf(message, "time of linux machine is %-24.24s", ctime(&t));
    std::cout <<
              "Runtime::sendMultiCastMessage(), sending multiCast message: " << message << std::endl;
    ssize_t cnt = sendto(multiCastSocketFd, message, sizeof(message), 0,
                         (struct sockaddr *) &multiCastAddress_, addressLength);
    if (cnt < 0) {
        std::cout << "error Inside Runtime::sendMultiCastMessage() function, sendto() failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        return false;
    } else {
        std::cout << "Runtime::sendMultiCastMessage() multicast data sent successfully" << std::endl;
    }
    return true;
}


int Runtime::createMasterTcpSocket(int portNumber) {
    int opt = 1;
    int masterTcpSocket;
    struct sockaddr_in tcpServerAddress;

    std::cout << "Runtime::createMasterTcpSocket() start" << std::endl;

    //create a master socket
    if ((masterTcpSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cout << "error Inside Runtime::createMasterTcpSocket() function, socket() failed for TCP, error: "
                  << std::string(strerror(errno)) << std::endl;
        return 0;
    }

    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if (setsockopt(masterTcpSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
        std::cout << "error Inside Runtime::createMasterTcpSocket() function, setsockopt() failed for TCP, error: "
                  << std::string(strerror(errno)) << std::endl;
        return 0;
    }

    //type of socket created
    tcpServerAddress.sin_family = AF_INET;
    tcpServerAddress.sin_addr.s_addr = INADDR_ANY;
    tcpServerAddress.sin_port = htons(portNumber);

    //bind the socket to given localhost port
    if (bind(masterTcpSocket, (struct sockaddr *) &tcpServerAddress, sizeof(tcpServerAddress)) <
        0) {
        std::cout <<
                  "error Inside Runtime::createMasterTcpSocket() function, bind failed for TCP socket, error: "
                  << std::string(strerror(errno)) << std::endl;
        return 0;
    }

    std::cout << "Runtime::createMasterTcpSocket() listening on TCP port " << portNumber << std::endl;

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(masterTcpSocket, 3) < 0) {
        std::cout << "error Inside Runtime::createMasterTcpSocket() function, listen failed for TCP socket, error: "
                  << std::string(strerror(errno)) << std::endl;
        return 0;
    }
    return masterTcpSocket;
}

int Runtime::createMasterUdpSocket(int portNumber) {
    struct sockaddr_in udpServerAddress;
    int masterUdpSocket;
    std::cout << "Runtime::createMasterUdpSocket() start" << std::endl;

    /* create a UDP socket */
    if ((masterUdpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cout << "error Inside Runtime::createMasterUdpSocket() function, after socket, error: "
                  << std::string(strerror(errno)) << std::endl;
        return 0;
    }

    /* bind the socket to any valid IP address and a specific port */
    memset((char *) &udpServerAddress, 0, sizeof(udpServerAddress));
    udpServerAddress.sin_family = AF_INET;
    udpServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    udpServerAddress.sin_port = htons(portNumber);

    if (bind(masterUdpSocket, (struct sockaddr *) &udpServerAddress, sizeof(udpServerAddress)) <
        0) {
        std::cout << "error Inside Runtime::createMasterUdpSocket() function, bind failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        return 0;
    }
    std::cout << "Runtime::createMasterUdpSocket() end" << std::endl;
    return masterUdpSocket;
}

int Runtime::createMultiCastReceiverSocket(std::string multiCastAddress, int portNumber) {
    struct sockaddr_in addr;
    int fd;
    struct ip_mreq mreq;
    u_int yes = 1;

    std::cout << "Runtime::createMultiCastReceiverSocket() start" << std::endl;
    /* create what looks like an ordinary UDP socket */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cout << "error Inside Runtime::createMultiCastReceiverSocket() function, socket() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        return 0;
    }

    /* allow multiple sockets to use the same PORT number */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        std::cout << "error Inside Runtime::createMultiCastReceiverSocket() function, setsockopt() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        return 0;
    }

    /* set up destination address */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); /* N.B.: differs from sender */
    addr.sin_port = htons(portNumber);

    /* bind to receive address */
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        std::cout << "error Inside Runtime::createMultiCastReceiverSocket() function, bind() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        return 0;
    }

    /* use setsockopt() to request that the kernel join a multicast group */
    mreq.imr_multiaddr.s_addr = inet_addr(multiCastAddress.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        std::cout << "error Inside Runtime::createMultiCastReceiverSocket() function, setsockopt() call failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        return 0;
    }
    std::cout << "Runtime::createMultiCastReceiverSocket() end" << std::endl;
    return fd;
}

bool Runtime::mainLoopThread() {
    std::cout << "Runtime::mainLoopThread() start" << std::endl;

    int masterTcpSocket = createMasterTcpSocket(30509);
    int masterUdpSocket = createMasterUdpSocket(30499);
    int multiCastReceiverSocket = createMultiCastReceiverSocket("224.244.224.245", 30490);

    struct pollfd pfd1;
    pfd1.fd = masterTcpSocket;
    pfd1.revents = 0;
    pfd1.events = POLLIN;
    managedPollFds.push_back(pfd1);
    handlersMap[masterTcpSocket] = std::bind(&Runtime::handleTcpConnectionRequest, this,
                                             placeholders::_1);

    struct pollfd pfd2;
    pfd2.fd = masterUdpSocket;
    pfd2.revents = 0;
    pfd2.events = POLLIN;
    managedPollFds.push_back(pfd2);
    handlersMap[masterUdpSocket] = std::bind(&Runtime::handleUdpData, this,
                                             placeholders::_1);

    struct pollfd pfd3;
    pfd3.fd = multiCastReceiverSocket;
    pfd3.revents = 0;
    pfd3.events = POLLIN;
    managedPollFds.push_back(pfd3);
    handlersMap[multiCastReceiverSocket] = std::bind(&Runtime::handleMultiCastData, this,
                                                     placeholders::_1);

    std::cout << "Runtime::mainLoopThread(), Waiting for TCP and UDP connections" << std::endl;
    while (!managedPollFds.empty()) {
        std::cout << " MainLoop::mainLoopThread blocked on polling" << std::endl;
        int numReadyFileDescriptors = ::poll(&(managedPollFds[0]),
                                             (nfds_t) managedPollFds.size(),
                                             10000); //10s timeout for polling
        if (numReadyFileDescriptors > 0) {
            std::cout << " MainLoop::mainLoopThread number of fd's ready: " << numReadyFileDescriptors << std::endl;
            for (auto managedPollFd = managedPollFds.begin(); managedPollFd
                                                              !=
                                                              managedPollFds.end(); ++managedPollFd) {
                auto watchIterator = handlersMap.find(managedPollFd->fd);
                if (watchIterator != handlersMap.end() && managedPollFd->revents) {
                    if (managedPollFd->revents & POLLIN) {
                        bool status = watchIterator->second(managedPollFd->fd);
                        if (status) {
                            std::cout << "Runtime::mainLoopThread() message handled successfully" << std::endl;
                            managedPollFd->revents = 0;
                        } else {
                            std::cout << "Runtime::mainLoopThread() message handling failed, removing Fd from the list"
                                      << std::endl;
                            managedPollFd = managedPollFds.erase(managedPollFd);
                            handlersMap.erase(watchIterator);
                            break;
                        }
                    } else {
                        std::cout << "Runtime::mainLoopThread() event other than POLLIN happened on fd" << std::endl;
                    }

                } else if (managedPollFd->revents) {
                    std::cout << " MainLoop::mainLoopThread no handler found for fd " << managedPollFd->fd << std::endl;
                }
            }
        } else if (numReadyFileDescriptors == 0) {
            std::cout << " MainLoop::mainLoopThread poll timed out" << std::endl;
        } else {
            std::cout << " MainLoop::mainLoopThread poll error error: " << std::string(strerror(errno)) << std::endl;
        }

        if (pendingPollFds.size() > 0) {
            for (auto pollFd : pendingPollFds) {
                managedPollFds.push_back(pollFd);
            }
            pendingPollFds.clear();
        }
    }

    std::cout << "Runtime::mainLoopThread() end" << std::endl;
    return true;
}

bool Runtime::handleTcpConnectionRequest(int fd) {
    int new_socket;
    struct sockaddr_in clientAddress;
    int addrlen = sizeof(clientAddress);
    std::cout <<
              "Runtime::handleTcpConnectionRequest() start" << std::endl;
    if ((new_socket = accept(fd, (struct sockaddr *) &clientAddress,
                             (socklen_t *) &addrlen)) < 0) {
        std::cout << "error Inside Runtime::handleTcpConnectionRequest() function, accept() failed, error: "
                  << std::string(strerror(errno)) << std::endl;
        return false;
    }

    //inform user of socket number - used in send and receive commands
    std::cout << "Runtime::handleTcpConnectionRequest(), New connection , socket fd is: " << new_socket << ",ip is :"
              << inet_ntoa(clientAddress.sin_addr) << " and port is: " << ntohs(clientAddress.sin_port) << std::endl;

    struct pollfd pfd;
    pfd.fd = new_socket;
    pfd.revents = 0;
    pfd.events = POLLIN;
    pendingPollFds.push_back(pfd);
    handlersMap[new_socket] = std::bind(&Runtime::handleTcpData, this, placeholders::_1);
    std::cout << "Runtime::handleTcpConnectionRequest() end" << std::endl;
    return true;
}

bool Runtime::handleTcpData(int fd) {
    bool status = false;
    ssize_t valread = 0;
    struct sockaddr_in clientAddress;
    int addrlen = sizeof(clientAddress);
    char buffer[1025];  //data buffer of 1K
    std::cout << "Runtime::handleTcpData() start" << std::endl;
    if ((valread = read(fd, buffer, 1024)) == 0) {
        //Somebody disconnected , get his details and print
        getpeername(fd, (struct sockaddr *) &clientAddress, (socklen_t *) &addrlen);
        std::cout << "Host disconnected , ip: " << inet_ntoa(clientAddress.sin_addr) << ", port: "
                  << ntohs(clientAddress.sin_port) << std::endl;

        //Close the socket and mark as 0 in list for reuse
        close(fd);
    } else {
        //Echo back the message that came in
        std::cout << "Runtime::handleTcpData(), data read successfully on port " << ntohs(clientAddress.sin_port)
                  << std::endl;
        std::cout << "Runtime::handleTcpData(), received message is: " << buffer << std::endl;
        //set the string terminating NULL byte on the end of the data read
        buffer[valread] = '\0';
        if (send(fd, buffer, strlen(buffer), 0) != strlen(buffer)) {
            std::cout << "error Inside Runtime::handleTcpData() function, send() failed, error: "
                      << std::string(strerror(errno)) << std::endl;
        } else {
            std::cout << "Runtime::handleTcpData(), data sent back successfully" << std::endl;
            status = true;
        }
    }
    std::cout << "Runtime::handleTcpData() end" << std::endl;
    return status;
}

bool Runtime::handleUdpData(int masterUdpSocket) {
    struct sockaddr_in clientAddress;
    socklen_t addressLength = sizeof(clientAddress);
    ssize_t receivedLength;
    unsigned char buf[2048];
    std::cout << "Runtime::handleUdpData() start" << std::endl;
    receivedLength = recvfrom(masterUdpSocket, buf, 2048, 0, (struct sockaddr *) &clientAddress,
                              &addressLength);
    std::cout << "Runtime::handleUdpData(), received " << receivedLength << " bytes" << std::endl;
    if (receivedLength >= 0) {
        buf[receivedLength] = 0;
        std::cout << "Inside Runtime::handleUdpData() function, received message: " << buf << std::endl;
    } else {
        std::cout << "error Inside Runtime::handleUdpData() function, recvfrom() failed, error: "
                  << std::string(strerror(errno)) << std::endl;
    }
    std::cout << "Runtime::handleUdpData() end" << std::endl;
    return true;
}

bool Runtime::handleMultiCastData(int multiCastReceiverSocket) {
    struct sockaddr_in multiCastServerAddress;
    socklen_t addressLength = sizeof(multiCastServerAddress);
    ssize_t receivedLength;
    unsigned char buf[2048];
    std::cout << "Runtime::handleMultiCastData() start" << std::endl;
    receivedLength = recvfrom(multiCastReceiverSocket, buf, 2048, 0, (struct sockaddr *) &multiCastServerAddress,
                              &addressLength);
    std::cout << "Runtime::handleMultiCastData(), received " << receivedLength << " bytes" << std::endl;
    if (receivedLength >= 0) {
        buf[receivedLength] = 0;
        std::cout << "Inside Runtime::handleMultiCastData() function, received message: " << buf << std::endl;
    } else {
        std::cout << "error Inside Runtime::handleMultiCastData() function, recvfrom() failed, error: "
                  << std::string(strerror(errno)) << std::endl;
    }
    std::cout << "Runtime::handleMultiCastData() end" << std::endl;
    return true;
}

bool Runtime::sendPeriodicMessages() {
    sendMultiCastTimeMessage();
    sendTcpMessage(destinationAddress_, 30509, "hello world TCP message from Linux Machine");
    sendUdpMessage(destinationAddress_, 30499, "hello world UDP message from Linux Machine");
    return true;
}