#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

#include <stdlib.h>

// Definitions for operators new and delete, since avr-gcc has no full C++
// support
void *operator new(size_t size);
void operator delete(void *ptr);
