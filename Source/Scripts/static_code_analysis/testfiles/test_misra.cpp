#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>

// MISRA C++ macro-multiple-statements violation
#define BAD_MACRO(x, y) x = 10; y = 20; printf("Done")  // Should use do-while(0)

// Good macro example for comparison
#define GOOD_MACRO(x, y) do { x = 10; y = 20; printf("Done"); } while(0)

// MISRA C++ 8-4-4 violation: Function defined multiple times
int duplicate_function(int a, int b);  // Declaration

int duplicate_function(int a, int b) {  // First definition
    return a + b;
}

int duplicate_function(int a, int b) {  // Second definition - VIOLATION
    return a * b;
}

// MISRA C++ 15-3-4 violation: Exception specification (deprecated)
void old_exception_spec() throw(std::bad_alloc) {  // VIOLATION
    throw std::bad_alloc();
}

// MISRA C++ 6-4-2 violation: Switch without default
void switch_no_default(int value) {
    switch (value) {  // VIOLATION: Missing default case
        case 1:
            printf("One\n");
            break;
        case 2:
            printf("Two\n");
            break;
        // Missing default case
    }
}

// Good switch with default for comparison
void switch_with_default(int value) {
    switch (value) {
        case 1:
            printf("One\n");
            break;
        case 2:
            printf("Two\n");
            break;
        default:
            printf("Other\n");
            break;
    }
}

// MISRA C++ variable-initialization violations
void uninitialized_variables() {
    int uninitialized_var;  // VIOLATION: Not initialized at declaration
    int another_var;        // VIOLATION: Not initialized at declaration
    
    // Later assignment
    uninitialized_var = 42;
    
    // Conditional initialization
    if (true) {
        another_var = 100;  // VIOLATION: Conditional initialization
    }
}

// MISRA C++ 6-6-1 violation: goto statement
void goto_example() {
    int x = 10;
    
    if (x > 5) {
        goto error_handler;  // VIOLATION: goto usage
    }
    
    printf("Normal execution\n");
    return;
    
error_handler:
    printf("Error occurred\n");
}

// MISRA C++ magic-numbers violations
void magic_numbers_example() {
    int array_size = 100;           // VIOLATION: Magic number 100
    int timeout_value = 5000;       // VIOLATION: Magic number 5000
    
    if (array_size > 50) {          // VIOLATION: Magic number 50
        printf("Large array\n");
    }
    
    for (int i = 0; i < 25; i++) {  // VIOLATION: Magic number 25
        printf("Index: %d\n", i);
    }
    
    // Calculations with magic numbers
    int result = timeout_value * 60;  // VIOLATION: Magic number 60
}

// MISRA C++ 5-0-3 violations: C-style casts
void c_style_casts() {
    void* ptr = malloc(100);
    
    // VIOLATION: C-style cast instead of C++ cast
    int* int_ptr = (int*)ptr;
    
    // VIOLATION: C-style cast for floating point
    double value = 3.14159;
    int truncated = (int)value;
    
    // VIOLATION: Pointer cast
    char* char_ptr = (char*)int_ptr;
    
    // Good examples for comparison (should not trigger violations)
    int* proper_int_ptr = static_cast<int*>(ptr);
    int proper_truncated = static_cast<int>(value);
    char* proper_char_ptr = reinterpret_cast<char*>(int_ptr);
    
    free(ptr);
}

// MISRA C++ 5-0-4 violations: Implicit conversions
void implicit_conversions() {
    // VIOLATION: Implicit conversion from double to int
    double pi = 3.14159;
    int integer_pi = pi;  // Loss of precision
    
    // VIOLATION: Implicit conversion from larger to smaller type
    long long big_number = 9223372036854775807LL;
    int small_number = big_number;  // Potential truncation
    
    // VIOLATION: Implicit conversion between signed/unsigned
    int signed_value = -100;
    unsigned int unsigned_value = signed_value;  // Dangerous conversion
    
    // VIOLATION: Implicit pointer conversion
    char* char_ptr = "Hello";
    void* void_ptr = char_ptr;  // Implicit conversion
}

// MISRA C++ 2-10-6 violations: Unsafe sprintf
void unsafe_string_operations() {
    char buffer[50];
    char name[] = "John Doe";
    int age = 30;
    
    // VIOLATION: sprintf can cause buffer overflow
    sprintf(buffer, "Name: %s, Age: %d", name, age);
    
    // VIOLATION: std::sprintf also unsafe
    std::sprintf(buffer, "Hello %s", name);
    
    // More dangerous sprintf usage
    char small_buffer[10];
    sprintf(small_buffer, "This string is definitely too long for the buffer");  // VIOLATION
    
    // Good example for comparison (should not trigger)
    snprintf(buffer, sizeof(buffer), "Name: %s, Age: %d", name, age);
}

// MISRA C++ 18-4-1 violations: malloc/free pairing
void memory_management_issues() {
    // VIOLATION: malloc without free before return
    char* ptr1 = (char*)malloc(100);
    if (ptr1 == nullptr) {
        return;  // Memory leak - allocated but not freed
    }
    
    // VIOLATION: calloc without free
    int* ptr2 = (int*)calloc(10, sizeof(int));
    if (some_condition()) {
        return;  // Memory leak
    }
    
    // VIOLATION: Multiple allocations, only one freed
    char* ptr3 = (char*)malloc(200);
    char* ptr4 = (char*)malloc(300);
    free(ptr3);  // ptr4 not freed before potential return
    
    if (another_condition()) {
        return;  // ptr4 leaked
    }
    
    free(ptr4);
}

// MISRA C++ 15-1-2 violations: Exception safety
void exception_safety_issues() {
    // VIOLATION: new allocation before throw
    // nosemgrep: misra-cpp-15-1-2-exception-safety
    int* ptr = new int[100];
    
    if (some_error_condition()) {
        throw std::runtime_error("Error occurred");  // Memory leak
    }
    
    delete[] ptr;
    
    // VIOLATION: malloc before throw
    char* buffer = (char*)malloc(500);
    
    if (another_error_condition()) {
        throw std::bad_alloc();  // Memory leak
    }
    
    free(buffer);
}

// Helper functions for testing
bool some_condition() {
    return true;
}

bool another_condition() {
    return false;
}

bool some_error_condition() {
    return false;
}

bool another_error_condition() {
    return false;
}

int main() {
    printf("Testing MISRA C++ rules\n");
    
    // Test various violations
    switch_no_default(1);
    uninitialized_variables();
    goto_example();
    magic_numbers_example();
    c_style_casts();
    implicit_conversions();
    unsafe_string_operations();
    memory_management_issues();
    
    try {
        exception_safety_issues();
    } catch (const std::exception& e) {
        printf("Exception caught: %s\n", e.what());
    }
    
    // Test macro violations
    int x, y;
    BAD_MACRO(x, y);  // This will trigger the macro violation
    
    return 0;
}