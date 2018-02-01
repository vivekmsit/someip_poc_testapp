//
// Created by VSharma on 1/22/2018.
//

#ifndef SOMEIP_POC_TESTAPP_RUNTIME_H
#define SOMEIP_POC_TESTAPP_RUNTIME_H

using namespace std;

#include <string>
#include <thread>
#include <vector>
#include <map>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <netdb.h>

class Runtime {
public:
    static Runtime *getInstance();

    bool init(std::string destinationAddress);

    bool deInit();

    bool sendMultiCastMessage(std::string destinationAddress, int portNumber, std::string message);

    bool sendTcpMessage(std::string destinationAddress, int portNumber, std::string message);

    bool sendUdpMessage(std::string destinationAddress, int portNumber, std::string message);

private:

    void serverThreadFunction();

    void clientThreadFunction();

    bool mainLoopThread();

    bool initMultiCastTimeSocket(std::string multiCastAddress, int portNumber);

    bool sendMultiCastTimeMessage();

    int createMasterTcpSocket(int portNumber);

    int createMasterUdpSocket(int portNumber);

    int createMultiCastReceiverSocket(std::string multiCastAddress, int portNumber);

    bool handleTcpConnectionRequest(int fd);

    bool handleTcpData(int fd);

    bool handleUdpData(int fd);

    bool handleMultiCastData(int fd);

    bool sendPeriodicMessages();

    bool stopThreads;
    std::thread m_serverThread;
    std::thread m_clientThread;
    std::vector<struct pollfd> managedPollFds;
    std::vector<struct pollfd> pendingPollFds;
    std::map<int, std::function<bool(int)>> handlersMap;
    struct sockaddr_in multiCastAddress_;
    int multiCastSocketFd;
    std::string destinationAddress_;
};

#endif //SOMEIP_POC_TESTAPP_RUNTIME_H
