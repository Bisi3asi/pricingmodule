#include "MyMath.h"
#include <stdio.h>
#include <stddef.h>

// Math 함수들
double  __stdcall mathSum(double a, double b) {
    return a + b;
}

double  __stdcall mathSub(double a, double b) {
    return a - b;
}

double  __stdcall mathMul(double a, double b) {
    return a * b;
}

double  __stdcall mathDiv(double a, double b) {
    if (b != 0) {
        return a / b;
    }
    else {
        // 0으로 나누는 에러 처리
        return 0;
    }
}

// 배열과 포인터를 받아 출력하고, 포인터를 통해 참조하는 값을 1 증가시킨다.
void __stdcall printType(
    const char* a[], const int* b, const double* c, const long* d, double* e,
    size_t a_size, size_t b_size, size_t c_size, size_t d_size
) {
    size_t i;

    // a 배열 전체 출력
    printf("param 1 print: ");
    for (i = 0; i < a_size; i++) {
        printf("%s ", a[i]);
    }
    printf("\n");

    // b 배열 전체 출력
    printf("param 2 print: ");
    for (i = 0; i < b_size; i++) {
        printf("%d ", b[i]);
    }
    printf("\n");

    // c 배열 전체 출력
    printf("param 3 print: ");
    for (i = 0; i < c_size; i++) {
        printf("%lf ", c[i]);
    }
    printf("\n");

    // d 배열 전체 출력
    printf("param 4 print: ");
    for (i = 0; i < d_size; i++) {
        printf("%ld ", d[i]);
    }
    printf("\n");

    // 포인터 e의 주소와 값 출력
    printf("param 5 e pointer address: %p\n", (void*)e);  // e의 주소
    printf("param 5 e value: %lf\n", *e);  // e가 가리키는 값

    // e가 가리키는 값 1 증가
    (*e)++;
    printf("dynamic library increased +1 param 5\n");
    printf("param 5 e value after change: %lf\n", *e);
}

void __stdcall printStruct(struct MyStruct* s) {
    printf("Position Name: %s\n", s->positionNm);
    printf("Buckets:\n");
    for (int i = 0; i < s->bucketSize; i++) {
        printf("  - %s\n", s->bucket[i]);
    }
    printf("BSDT: %s\n", s->bsdt);
    printf("Tenor: %d\n", s->tenor);
    printf("Value: %f\n", s->value);
}


void __stdcall updateDoubleArr(double* a, size_t a_size) {
    int i;
    printf("updateDoubleArr - Before update: ");
    for (i = 0; i < a_size; i++) {
        printf("%lf ", a[i]);
    }
    printf("\n");

    // 각 원소마다 10씩 곱하기 (즉, 10배로 업데이트)
    for (i = 0; i < a_size; i++) {
        a[i] = a[i] * 10;
    }

    printf("updateDoubleArr - After update: ");
    for (i = 0; i < a_size; i++) {
        printf("%lf ", a[i]);
    }
    printf("\n");
}