#include "sleep.h"

#if __has_include(<windows.h>)
#include <windows.h>
unsigned sleep(unsigned int seconds){
    Sleep(seconds);
    return 0;
}
#else
#include <unistd.h>
unsigned sleep(unsigned int seconds){
return sleep(seconds);
}
#endif



