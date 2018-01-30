#include <iostream>
#include <mutex>
#include "runtime/Runtime.h"

int main(int argc, char *argv[]) {
    std::condition_variable cv;
    std::mutex m;
    std::unique_lock<std::mutex> lk(m);
    bool is_ready(false);
    std::string destinationAddress;

    std::cout << "Inside main()" << std::endl;

    if (argc > 1) {
        destinationAddress = std::string(argv[1]);
    } else {
        destinationAddress = "10.90.244.81";
    }

    std::cout << "Initial value of destinationAddress is: " << destinationAddress << std::endl;

    Runtime *instance = Runtime::getInstance();
    if (instance) {
        bool status = instance->init(destinationAddress);
        if (status) {
            std::cout << "Runtime::init() successful" << std::endl;
        } else {
            std::cout << "Runtime::init() failed" << std::endl;
        }
    }
    cv.wait(lk, [&] { return is_ready; });
    printf("End of main(), Condition Variable notified");
    return 0;
}