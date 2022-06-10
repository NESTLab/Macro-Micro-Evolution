#ifndef __HEX_STRING__
#define __HEX_STRING__

#ifndef __SIZEOF_POINTER__
#warning "__SIZEOF_POINTER__ NOT DEFINED! MUST BE EQUAL TO THE NUMBER OF BYTES IN A POINTER"
#endif

#include <sstream>
#include <string>

std::string to_hex_string(void* value);


#endif // __HEX_STRING__