#ifndef __DEBUG_H
#define __DEBUG_H

#include <iostream>

#ifdef DEBUG_BUILD
#define DEBUG(x) do { std::cerr << x; } while (0)
#else
#define DEBUG(x)
#endif

#endif