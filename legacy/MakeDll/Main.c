#include "MyMath.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // ����ü �ʱ�ȭ
    struct MyStruct myStruct;
    myStruct.positionNm = "Trader1";

    // ���� �Ҵ�� ���ڿ� �迭
    char* bucketList[] = { "Bucket A", "Bucket B", "Bucket C", NULL };
    myStruct.bucket = bucketList;
    myStruct.bucketSize = 4;
    myStruct.bsdt = "2025-03-19";
    myStruct.tenor = 365;
    myStruct.value = 123.45;

    // �Լ� ȣ��
    printStruct(&myStruct);

    // ȭ�� ���� ����
    getchar();
    return 0;
}
