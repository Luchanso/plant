#include "avr-new.h"

// Definitions for operators new and delete
void *operator new(size_t size) { return malloc(size); }
void operator delete(void *ptr) { free(ptr); }
