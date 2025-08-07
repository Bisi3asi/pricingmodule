#pragma once
#include <stddef.h>

#ifdef _WIN32  // For Windows
#ifdef BUILD_MY_MATH_LIBRARY
#define MY_MATH_API __declspec(dllexport)  // DLL 출력
#else
#define MY_MATH_API __declspec(dllimport)  // DLL 입력
#endif
#elif __linux__  // For Linux
#define MY_MATH_API __attribute__((visibility("default")))  // .so Export
#else
#error "Unsupported platform"
#endif
 
struct MyStruct {
    char* positionNm;
    char** bucket;
    size_t bucketSize;
    char* bsdt;
    int tenor;
    double value;
};

MY_MATH_API double __stdcall mathSum(double a, double b);
MY_MATH_API double __stdcall mathSub(double a, double b);
MY_MATH_API double __stdcall mathMul(double a, double b);
MY_MATH_API double __stdcall mathDiv(double a, double b);
MY_MATH_API void __stdcall printType(const char* a[], const int* b, const double* c, const long* d, double* e, size_t a_size, size_t b_size, size_t c_size, size_t d_size);
MY_MATH_API void __stdcall printStruct(struct MyStruct* s);
MY_MATH_API void __stdcall updateDoubleArr(double* a, size_t a_size);