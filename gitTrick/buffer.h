#pragma once
#include <QByteArray>

inline QByteArray COMPILATION_TIME_buffer = nullptr;
inline QByteArray GIT_STATUS_buffer       = nullptr;
inline QByteArray GIT_SUBMODULES_buffer   = nullptr;
void               loadBuffer();
