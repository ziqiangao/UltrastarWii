#ifndef MIIHANDLER_H
#define MIIHANDLER_H

#include <cstdint>

//---------------------------------------------------------------------------------
// Convert a UTF-16 Mii name to UTF-8 for rendering
// utf16_name: pointer to UTF-16 code units (usually 11 for Wii Mii names)
// len: number of code units (11 normally)
// utf8_name: output buffer for UTF-8 string
// utf8_bufsize: size of output buffer in bytes
//---------------------------------------------------------------------------------
void MiiName_UTF16_to_UTF8(const uint16_t* utf16_name, int len, char* utf8_name, int utf8_bufsize);

#endif // MIIHANDLER_H
