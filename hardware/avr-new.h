#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

// Definitions for operators new and delete
inline void *operator new(size_t size) { return malloc(size); }
inline void operator delete(void *ptr) { free(ptr); }
