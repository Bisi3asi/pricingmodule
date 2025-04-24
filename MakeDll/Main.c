#include "MyMath.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // 구조체 초기화
    struct MyStruct myStruct;
    myStruct.positionNm = "Trader1";

    // 동적 할당된 문자열 배열
    char* bucketList[] = { "Bucket A", "Bucket B", "Bucket C", NULL };
    myStruct.bucket = bucketList;
    myStruct.bucketSize = 4;
    myStruct.bsdt = "2025-03-19";
    myStruct.tenor = 365;
    myStruct.value = 123.45;

    // 함수 호출
    printStruct(&myStruct);

    // 화면 종료 방지
    getchar();
    return 0;
}
