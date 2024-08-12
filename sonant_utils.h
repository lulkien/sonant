#ifndef SONANT_UTILS_H
#define SONANT_UTILS_H

#include <iostream>

#define UNREACHABLE()                                 \
    std::cerr                                         \
    << "Warning: Reached unexpected code section in " \
    << __FILE__                                       \
    << " at line " << __LINE__                        \
    << std::endl

#define MSG_LOG             \
    std::cout               \
    << __PRETTY_FUNCTION__  \
    << " "

#define ERR_LOG             \
    std::cerr               \
    << __PRETTY_FUNCTION__  \
    << " "

#endif // !SONANT_UTILS_H
