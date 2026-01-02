#include <cstdint>
#include <cstring>
#include "miihandler.h"


// input: utf16_name = array of 16-bit values, length = 11 (Mii max)
// output: utf8_name = buffer large enough, at least 4*length+1
void MiiName_UTF16_to_UTF8(const uint16_t* utf16_name, int len, char* utf8_name, int utf8_bufsize)
{
    int pos = 0;
    for (int i = 0; i < len; i++)
    {
        uint16_t wc = utf16_name[i];
        if (wc == 0) break; // null terminator

        if (wc < 0x80) {
            // 1 byte
            if (pos + 1 >= utf8_bufsize) break;
            utf8_name[pos++] = (char)wc;
        } 
        else if (wc < 0x800) {
            // 2 bytes
            if (pos + 2 >= utf8_bufsize) break;
            utf8_name[pos++] = 0xC0 | (wc >> 6);
            utf8_name[pos++] = 0x80 | (wc & 0x3F);
        } 
        else {
            // 3 bytes
            if (pos + 3 >= utf8_bufsize) break;
            utf8_name[pos++] = 0xE0 | (wc >> 12);
            utf8_name[pos++] = 0x80 | ((wc >> 6) & 0x3F);
            utf8_name[pos++] = 0x80 | (wc & 0x3F);
        }
    }

    utf8_name[pos] = '\0';
}
