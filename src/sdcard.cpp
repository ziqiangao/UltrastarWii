#include "sdcard.hpp"
#include <stdlib.h>

SDCard::SDCard() : initialized(false) {}

SDCard::~SDCard() {}

bool SDCard::init() {
    initialized = fatInitDefault();
    return initialized;
}

std::string SDCard::readFile(const std::string &filepath) {
    if (!initialized) return "";

    FILE *file = fopen(("sd:/" + filepath).c_str(), "r");
    if (!file) return "";

    std::string contents;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), file)) {
        contents += buffer;
    }

    fclose(file);
    return contents;
}
