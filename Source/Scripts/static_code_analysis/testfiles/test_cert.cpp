#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "stdio.h"

// CERT FIO30-C violation: Hardcoded credentials
const char* wifi_password = "MySecretPassword123";
char api_key[] = "sk-1234567890abcdef";
#define DB_PASSWORD "admin123456"

// Global variables for testing
char* global_ptr = nullptr;
int shared_counter = 0;
pthread_mutex_t mutex;

// CERT SIG30-C violation: Unsafe signal handler
void signal_handler(int sig) {
    printf("Signal received: %d\n", sig);  // Unsafe in signal handler
    malloc(100);  // Very unsafe in signal handler
    free(global_ptr);  // Unsafe in signal handler
}

// CERT DCL30-C violation: Missing const correctness
char* get_buffer() {
    static char buffer[256];
    return buffer;  // Should return const char*
}

// CERT ARR30-C violation: Array bounds not checked
void unsafe_array_access() {
    int array[10];
    int index = 15;  // Will cause out-of-bounds access
    array[index] = 42;  // No bounds checking
}

// CERT EXP12-C violation: NULL pointer dereference
void null_pointer_test() {
    char* ptr = nullptr;
    *ptr = 'A';  // Dereferencing NULL pointer
    
    char* another_ptr = malloc(100);
    another_ptr->field = 10;  // No NULL check after malloc
}

// CERT MEM30-C and MEM31-C violations: Memory management issues
void memory_issues() {
    char* ptr1 = (char*)malloc(100);
    if (ptr1 == nullptr) {
        return;  // Memory leak - allocated but not freed on error
    }
    
    char* ptr2 = (char*)malloc(200);
    free(ptr2);
    free(ptr2);  // Double free vulnerability
    
    // Missing free(ptr1) - memory leak
}

// CERT INT32-C violation: Signed integer overflow
int arithmetic_overflow() {
    int a = 2147483647;  // INT_MAX
    int b = 10;
    int result = a + b;  // Potential signed overflow
    result = a * b;      // Potential signed overflow
    return result;
}

// CERT ERR33-C violation: Not checking return values
void unchecked_returns() {
    malloc(100);  // Return value not checked
    fopen("test.txt", "r");  // Return value not checked
    
    FILE* file = fopen("data.txt", "r");
    fread(global_ptr, 1, 100, file);  // Return value not checked
    fwrite("data", 1, 4, file);       // Return value not checked
}

// CERT ENV33-C violation: Dangerous system calls
void dangerous_system_calls() {
    char command[256];
    strcpy(command, "rm -rf /");  // Also STR31-C violation
    system(command);  // Dangerous system call
    
    popen("cat /etc/passwd", "r");  // Dangerous popen call
}

// CERT CON34-C violation: Race condition
void race_condition_test() {
    pthread_mutex_lock(&mutex);
    if (shared_counter < 100) {
        // Race condition - check and use not atomic
        shared_counter++;
    }
    pthread_mutex_unlock(&mutex);
}

// IoT/Embedded specific violations
void iot_violations() {
    // Large stack array - embedded violation
    char large_buffer[2048];  // Too large for embedded stack
    
    // Infinite loop without watchdog
    while(1) {
        // Missing wdt_reset() call
        delay(1000);
    }
}

// Interrupt handler violation (ESP32 specific)
IRAM_ATTR void interrupt_handler() {
    Serial.println("Interrupt!");  // Unsafe in interrupt
    malloc(50);  // Very unsafe in interrupt
    delay(10);   // Blocking call in interrupt
}

int main() {
    char buffer[100];
    char source[200] = "This is a very long string that will overflow the destination buffer";
    
    // CERT STR31-C violations: Unbounded string operations
    strcpy(buffer, source);  // Buffer overflow risk
    strcat(buffer, " more text");  // Buffer overflow risk
    gets(buffer);  // Very dangerous function
    
    sprintf(buffer, "Hello %s", "World"); 
    std::sprintf(buffer, "Test %d", 42);
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    
    // Test various violations
    unsafe_array_access();
    null_pointer_test();
    memory_issues();
    arithmetic_overflow();
    unchecked_returns();
    dangerous_system_calls();
    
    // Random number without seeding
    int random_val = rand();  // MSC30-C violation
    
    return 0;
}