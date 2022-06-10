#include "hexstring.h"

std::string to_hex_string(void* value) {
    std::stringstream out;

    #if(__SIZEOF_POINTER__ == 8)
        out << std::hex << reinterpret_cast<uint64_t>(value);
    #endif
    #if(__SIZEOF_POINTER__ == 4)
        out << std::hex << reinterpret_cast<uint32_t>(value);
    #endif
    #if(__SIZEOF_POINTER__ == 2)
        out << std::hex << reinterpret_cast<uint16_t>(value);
    #endif
    #if(__SIZEOF_POINTER__ == 1)
        out << std::hex << reinterpret_cast<uint8_t>(value);
    #endif

    return out.str();
}