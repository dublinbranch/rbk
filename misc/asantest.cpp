#include "asantest.h"

void asan1() {
    asan2();
}

void asan2() {
    asan3();
}

void asan3() {
    asan4();
}

void asan4() {
    asan5();
}

void asan5() {
    asan6();
}

void asan6() {
    asan7();
}

void asan7() {
    asan8();
}

void asan8() {
    asan9();
}

void asan9() {
    int* p = new int[10];
    //p[10] = 1;
    delete[] p;
    p[1] = 1;
}