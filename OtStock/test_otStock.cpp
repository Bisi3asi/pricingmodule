#include <iostream>
#include <iomanip>

#include "src/otStock.h"

// 분기문 처리
#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__) || defined(__unix__)
#include <unistd.h>
#endif

using namespace std;

int main() {
	/* OtStock 테스트 */
	const double amount = 10000.0;
    const double price = 9000.0;
	const double basePrice = 9500.0;
	const double beta = 1.2;
	const int calType = 1; // 1: pric
	const int scenCalcu = 1;
	const int logYn = 1; // 0: No, 1: Yes
    double resultBasel2[6] = { 0 };
    double resultBasel3[1] = { 0 };
	double resultCashflow[1] = { 0 }; // Cashflow array, size 1 for simplicity
    double result = 0;
    result = pricing(amount, price, basePrice, beta, calType, scenCalcu, logYn, resultBasel2, resultBasel3, resultCashflow);

	cout << "CalType: " << calType << endl;
	cout << "scenCalcu : " << scenCalcu << endl;
	cout << fixed << setprecision(2) << "Result: " << result << endl;


    // 화면 종료 방지 (윈도우와 리눅스 호환)
    #ifdef _WIN32
    system("pause");
    #else
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    #endif

    return 0;
}
