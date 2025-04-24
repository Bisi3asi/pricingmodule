#ifndef OTSTOCK_H
#define OTSTOCK_H

#ifdef _WIN32
#define EXPORT __declspec(dllimport) __stdcall
#elif defined(__linux__) || defined(__unix__)
#define EXPORT
#endif

// extern "C" 처리하여 C 스타일로 외부에서 함수가 호출될 수 있도록 처리
extern "C" long EXPORT stockPricing(double amt, double price, double basePrice, double beta, double fx, double* p, double* ResultPrice);

#endif