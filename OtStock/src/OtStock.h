#ifndef OTSTOCK_H
#define OTSTOCK_H

#ifdef _WIN32
#define EXPORT __declspec(dllimport) __stdcall
#elif defined(__linux__) || defined(__unix__)
#define EXPORT
#endif

// extern "C" ó���Ͽ� C ��Ÿ�Ϸ� �ܺο��� �Լ��� ȣ��� �� �ֵ��� ó��
extern "C" long EXPORT stockPricing(double amt, double price, double basePrice, double beta, double fx, double* p, double* ResultPrice);

#endif