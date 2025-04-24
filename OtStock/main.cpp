#include <iostream>
#include <vector>
#include <string>
#include "src/OtStock.h"

// �б⹮ ó��
#ifdef _WIN32
    #include <windows.h>
    #define EXPORT __declspec(dllimport) __stdcall
#elif defined(__linux__) || defined(__unix__)
    #include <unistd.h>
    #define EXPORT
#endif

//----------------------------------------------------------------
// stockPricing
// amt : �ֽ� �ð�
// price : ���� ( ������� O : �ֽİ��� , ������� X : �ְ� ���� ) - �ó����� ����� ���� �Ǵ� ���� ������ ���� ����.
// basePricee : ���ذ��� ( ������� O : �ֽ� ���ذ��� , ������� X : �ְ����� ���� ���� ) - ���� ���� �� ���� ����.
// beta : �ֽ� ��Ÿ ( ������� O : 1 , ������� X : �ֽ� ��Ÿ ) 
// fx : ��ȭȯ�� ( ��ȭ �������� ��� 0 )
// *p : 0:��Ÿ , 1:����, 2:��Ÿ, 3:��Ÿ, 4:�� 
// *ResultPrice : �򰡰���(�̷а�)
//----------------------------------------------------------------
int main() {
    // �Է°� ����
    double amt = 1200.0;          // �ֽ� �ð�
    double price = 1100.0;        // ���� ����
    double basePrice = 1000.0;    // ���� ����
    double beta = 1.2;            // ��Ÿ ��
    double fx = 0.0;              // ��ȯ ȯ�� (��ȭ �������� ��� 0)

    double p[5];             // ��Ÿ, ����, ����, ��Ÿ, �� ���� �迭
    double ResultPrice = 0.0;     // �� ���� ���� ����

    // �Լ� ȣ��
    long result = stockPricing(amt, price, basePrice, beta, fx, p, &ResultPrice);

    // ��� ���
    std::cout << "Pricing Calculation Result:" << std::endl;
    std::cout << "  - Theoretical Price: " << ResultPrice << std::endl;
    std::cout << "  - Delta: " << p[0] << std::endl;
    std::cout << "  - Gamma: " << p[1] << std::endl;
    std::cout << "  - Vega: " << p[2] << std::endl;
    std::cout << "  - Theta: " << p[3] << std::endl;
    std::cout << "  - Rho: " << p[4] << std::endl;
    std::cout << "  - Return Code: " << result << std::endl;

    // ȭ�� ���� ���� (������� ������ ȣȯ)
#ifdef _WIN32
    system("pause");
#else
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
#endif

    return 0;
}
