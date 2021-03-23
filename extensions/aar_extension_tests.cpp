#include "spdlog/spdlog.h"
#include "dataSender.hpp"

int main() {
    dataSender sender;

    sender.sendData("foo", 3);

    return 0;
}
