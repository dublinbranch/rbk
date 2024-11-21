#include "sleep.h"

#if __has_include(<windows.h>)
#include <windows.h>
unsigned sleep(unsigned int seconds){
    Sleep(seconds);
    return 0;
}
#else
//literally do nothing
#endif



