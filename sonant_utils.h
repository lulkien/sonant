#ifndef SONANT_UTILS_H
#define SONANT_UTILS_H

#include <iostream>

#define LOG_TAG "[SONANT]"
#define LOG_ENDL std::endl

#define UNREACHABLE()                                                                              \
    std::cerr << LOG_TAG << "Warning: Reached unexpected code section in " << __FILE__             \
              << " at line " << __LINE__ << LOG_ENDL

#define MSG_LOG std::cout << LOG_TAG << " "
#define ERR_LOG std::cerr << LOG_TAG << " "

#endif // !SONANT_UTILS_H
