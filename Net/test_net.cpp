#include <iostream>
#include <iomanip>

#include "src/net.h"

// 분기문 처리
#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__) || defined(__unix__)
#include <unistd.h>
#endif

int main() {
    /* Net 테스트 */
    const int evaluationDate = 45107;   // 2023-06-30
    const double notional = 3600000000;
	const int logYn = 0; // 로깅 여부 (0: No, 1: Yes)

    double result = pricingNET(evaluationDate, notional, logYn);

	// OUTPUT 1 결과 출력
    std::cout << "[Net]: " << std::setprecision(20) << result << std::endl;

    // 화면 종료 방지 (윈도우와 리눅스 호환)
    #ifdef _WIN32
    system("pause");
    #else
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    #endif

    return 0;
}
