#include <iostream>
#include <iomanip>

#include "src/bond.h"

// �б⹮ ó��
#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__) || defined(__unix__)
#include <unistd.h>
#endif

int main() {
    /* �λ���� Algo �׽�Ʈ �Լ� ������ */
    /*
    const double notional = 6000000000.0;       // ä�� ���� ���ݾ�
    const long evaluationDate = 45657;          // ä�� �� ������ (serial number)
    const long settlementDays = 0;              // ������ offset 
    const long issueDate = 44175;               // ä�� ������ (serial number)
	const long maturityDate = 47827;            // ä�� ������ (serial number)
	const double couponRate = 0.015;            // ä�� ���� ���� 
    const int couponDayCounter = 5;             // DayCounter code (��: 5 = Actual/Actual(Bond))
    const long numberOfCpnSch = 12;             // ���� ����
    const long paymentDates[] = { 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644, 47827 };     // ������ �迭
    const long realStartDates[] = { 45636, 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644 };   // �� ���� ������
	const long realEndDates[] = { 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644, 47827 };     // �� ���� ������
    const long numberOfGirrTenors = 10; // GIRR ���� ��
	const long girrDates[] = { 91, 183, 365, 730, 1095, 1825, 3650, 5475, 7300, 10950 };    // GIRR ���� (startDate�κ����� �ϼ�)
	const double girrRates[] = { 0.0337, 0.0317, 0.0285, 0.0272, 0.0269, 0.0271, 0.0278, 0.0272, 0.0254, 0.0222 }; // GIRR �ݸ�
    const long girrDayCounter = 1;      // GIRR DayCounter (��: 1 = Actual/365)
    const long girrInterpolator = 1;    // ������ (��: 1 = Linear)
    const long girrCompounding = 1;     // ���� ��� ��� (��: 1 = Continuous)
	const long girrFrequency = 1;       // Annual (��: 1 = Annual)
	const double spreadOverYield = 0.001389;    // ä���� ���� Credit Spread
    const int spreadOverYieldCompounding = 1;   // Credit Spread ���� ��� ��� (��: 1 = Continuous)
    const int spreadOverYieldDayCounter = 1;    // Credit Spread DayCounter (��: 1 = Actual/365)
	const long numberOfCsrTenors = 5;           // CSR ���� ��
	const long csrDates[] = { 183, 365, 1095, 1825, 3650 };     // CSR ���� (startDate�κ����� �ϼ�)
	const double csrRates[] = { 0.0, 0.0, 0.0, 0.0005, 0.001 }; // CSR �ݸ� (�ݸ� ����)
    */

	/* �λ���� Algo �׽�Ʈ �Լ� ������ */
    const double notional = 300000.0;           // ä�� ���� ���ݾ�
    const long revaluationDate = 45657;         // ä�� �� ������ (serial number)
    const long settlementDays = 0;              // ������ offset 
    const long issueDate = 45504;               // ä�� ������ (serial number)
    const long maturityDate = 46599;            // ä�� ������ (serial number)
    const double couponRate = 0.055;            // ä�� ���� ���� 
    const int couponDayCounter = 5;             // DayCounter code (��: 5 = Actual/Actual(Bond))
    const long numberOfCpnSch = 6;              // ���� ����
    const long paymentDates[] = { 45688, 45869, 46052, 46234, 46416, 46598 };     // ������ �迭
    const long realStartDates[] = { 45504, 45688, 45869, 46052, 46234, 46416 };   // �� ���� ������
    const long realEndDates[] = { 45688, 45869, 46052, 46234, 46416, 46598 };     // �� ���� ������
    const long numberOfGirrTenors = 10; // GIRR ���� ��
    const long girrDates[] = { 91, 183, 365, 730, 1095, 1825, 3650, 5475, 7300, 10950 };    // GIRR ���� (startDate�κ����� �ϼ�)
    const double girrRates[] = { 0.0434026344, 0.0552167219, 0.0539846356, 0.0403908251, 0.0527258415, 0.0524191116, 0.0526506237, 0.0405914923, 0.0403457538, 0.0377604176 }; // GIRR �ݸ�
    const long girrDayCounter = 1;      // GIRR DayCounter (��: 1 = Actual/365)
    const long girrInterpolator = 1;    // ������ (��: 1 = Linear)
    const long girrCompounding = 1;     // ���� ��� ��� (��: 1 = Continuous)
    const long girrFrequency = 1;       // Annual (��: 1 = Annual)
    const double spreadOverYield = 0.001389;    // ä���� ���� Credit Spread
    const int spreadOverYieldCompounding = 1;   // Credit Spread ���� ��� ��� (��: 1 = Continuous)
    const int spreadOverYieldDayCounter = 1;    // Credit Spread DayCounter (��: 1 = Actual/365)
    const long numberOfCsrTenors = 5;           // CSR ���� ��
    const long csrDates[] = { 183, 365, 1095, 1825, 3650 };     // CSR ���� (startDate�κ����� �ϼ�)
    const double csrRates[] = { 0.0, 0.0, 0.0, 0.0005, 0.001 }; // CSR �ݸ� (�ݸ� ����)


    /* �λ���� Algo �׽�Ʈ �Լ� ������ */
    

    ZeroBondTest(
        notional,
        revaluationDate,
        settlementDays,
        issueDate,
        maturityDate,
        couponRate,
        couponDayCounter,
        numberOfCpnSch,
        paymentDates,
        realStartDates,
        realEndDates,
        numberOfGirrTenors,
        girrDates,
        girrRates,
        girrDayCounter,
        girrInterpolator,
        girrCompounding,
        girrFrequency,
        spreadOverYield,
        spreadOverYieldCompounding,
        spreadOverYieldDayCounter,
        numberOfCsrTenors,
        csrDates,
        csrRates
    );

    // ȭ�� ���� ���� (������� ������ ȣȯ)

    #ifdef _WIN32
    system("pause");
    #else
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    #endif

    return 0;
}
