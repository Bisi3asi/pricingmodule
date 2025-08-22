#ifndef BOND_H
#define BOND_H

// function 외부 인터페이스 export 정의
#ifdef _WIN32
#ifdef BUILD_LIBRARY
#define EXPORT __declspec(dllexport) __stdcall
#else
#define EXPORT __declspec(dllimport) __stdcall
#endif
#elif defined(__linux__) || defined(__unix__)
#define EXPORT
#endif

#pragma once

/* include */
#include <iostream>
#include <iomanip>

/* include(QuantLib) */

/* dll export method(extern "C", EXPORT 명시 필요) */
extern "C" double EXPORT pricingNET(
    const int evaluationDate                // INPUT 1. 평가일 (serial number)
  , const double notional                   // INPUT 2. 채권 원금
  , const int logYn 					    // INPUT 3. 로깅 여부 (0: No, 1: Yes)
);

#endif