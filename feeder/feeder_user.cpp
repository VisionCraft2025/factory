#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>

int main() {
    const std::string device = "/dev/feeder";
    std::ofstream feeder(device);

    if (!feeder.is_open()) {
        std::cerr << "장치 파일을 열 수 없습니다: " << device << std::endl;
        return 1;
    }

    std::cout << "=== Feeder Control ===\n";
    std::cout << "명령을 입력하세요 (on / reverse / off / exit):\n";

    std::string cmd;
    while (true) {
        std::cout << "> ";
        std::cin >> cmd;

        if (cmd == "exit") {
            break;
        }

        if (cmd == "on" || cmd == "off" || cmd == "reverse") {
            feeder << cmd << std::endl;
            feeder.flush();
        } else {
            std::cout << "잘못된 명령입니다. (on / reverse / off / exit)\n";
        }
    }

    feeder.close();
    return 0;
}
