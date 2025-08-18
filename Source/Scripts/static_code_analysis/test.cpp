#include <cstdio>
#include "stdio.h"

int main() {
    char buffer[100];
    sprintf(buffer, "Hello %s", "World");
    std::sprintf(buffer, "Test %d", 42);
    return 0;
}