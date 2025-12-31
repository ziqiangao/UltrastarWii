#pragma once

#include <fat.h>
#include <stdio.h>
#include <string>

class SDCard {
public:
    // Constructor / Destructor
    SDCard();
    ~SDCard();

    // Initialize the SD card
    bool init();

    // Read a text file from the SD card and return its contents
    std::string readFile(const std::string &filepath);

private:
    bool initialized;
};
