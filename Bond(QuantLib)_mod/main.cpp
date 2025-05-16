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
    const double notional = 6000000000.0;       // ä�� ���� ���ݾ�
    const long issueDateSerial = 44175;         // ä�� ������ (serial number)
    const long maturityDateSerial = 47827;      // ä�� ������ (serial number)
    const long revaluationDateSerial = 45657;   // ä�� �� ������ (serial number)
    const long settlementDays = 0;              // ������ offset 
	
    const long couponCnt = 12;                  // ���� ����
    const double couponRate = 0.015;            // ä�� ���� ���� 
    const int couponDcb = 5;                    // DayCounter code (��: 5 = Actual/Actual(Bond))
    const int businessDayConvention = 1;        // ������ ���� Convention (1 = Modified Following) 
    const long realStartDatesSerial[] = { 45636, 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644 };   // �� ���� ������
    const long realEndDatesSerial[] = { 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644, 47827 };     // �� ���� ������
    const long paymentDatesSerial[] = { 45818, 46001, 46183, 46366, 46548, 46731, 46916, 47098, 47280, 47462, 47644, 47827 };     // ������ �迭
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
   
	/* Deloitte VBA �׽�Ʈ �Լ� ������ */
    /*
    const double notional = 3000000.0;          // ä�� ���� ���ݾ�
    const long issueDateSerial = 45504;         // ä�� ������ (serial number)
    const long maturityDateSerial = 46599;      // ä�� ������ (serial number)
    const long revaluationDateSerial = 45657;   // ä�� �� ������ (serial number)
    const long settlementDays = 0;              // ������ offset 
    
    const long couponCnt = 7;                   // ���� ����
    const double couponRate = 0.055;            // ä�� ���� ���� 
    const int couponDCB = 1;                    // ���� Day Count Basis (1 = Actual/Actual(Bond))
    const int businessDayConvention = 1;        // ������ ���� Convention (1 = Modified Following) 
    const long realStartDatesSerial[] = { 45504, 45688, 45869, 46052, 46234, 46416, 46598 };   // �� ���� ������
    const long realEndDatesSerial[] = { 45688, 45869, 46052, 46234, 46416, 46598, 46783 };     // �� ���� ������
    const long paymentDatesSerial[] = { 45688, 45869, 46052, 46234, 46416, 46598, 46783 };     // ������ �迭
    
    const long girrCnt = 10;                    // GIRR ���� ��
    const double girrRates[] = { 0.0434026344, 0.0552167219, 0.0539846356, 0.0403908251, 0.0527258415, 0.0524191116, 0.0526506237, 0.0405914923, 0.0403457538, 0.0377604176 }; // GIRR �ݸ�
    const long girrDCB = 3;                     // GIRR Day Count Basis (3 = Actual/365)
    //const long girrInterpolator = 1;          // ������ (Linear)
    //const long girrCompounding = 1;           // ���� ��� ��� (Continuous)
    //const long girrFrequency = 1;             // �� (Annual)
    
    // const double spreadOverYield = 0.001389; // ä���� ���� Credit Spread
    const double spreadOverYield = 0.00;
    // const int spreadOverYieldCompounding = 1;// Credit Spread ���� ��� ��� (Continuous)
    const int spreadOverYieldDCB = 3;           // Credit Spread Day Count Basis  (3 = Actual/365)
    
    // const long csrCnt = 5;                   // CSR ���� ��
    // const double csrSpreads[] = { 0.0, 0.0, 0.0, 0.0005, 0.001 }; // CSR �ݸ� (�ݸ� ����)
    const long csrCnt = 5;                      // CSR ���� ��
    const double csrSpreads[] = { 0.0, 0.0, 0.0, 0.0, 0.0 }; // CSR �ݸ� (�ݸ� ����)

    const unsigned short logYn = 1;             // �α� ���� ���� ���� (0: No, 1: Yes)
    
                                                // OUTPUT 1. NETPV (���ϰ�)
    double resultGirrDelta[25];                 // OUTPUT 2. GIRR ����� ([index 0] array size, [index 1 ~ size -1] Girr Delta tenor, [size ~ end] Girr Delta Sensitivity)    
	double resultCsrDelta[25];                  // OUTPUT 3. CSR ����� ([index 0] array size, [index 1 ~ size -1] Csr Delta tenor, [size ~ end] Csr Delta Sensitivity)   
    */

    double netPV = 
    pricing(
        notional,
        issueDateSerial,
        maturityDateSerial,
        revaluationDateSerial,
        settlementDays,
        
        couponCnt,
        couponRate,
        couponDcb,
        businessDayConvention,
        realStartDatesSerial,
        realEndDatesSerial,
        paymentDatesSerial,

        girrCnt,
        girrRates,
        girrDCB,
        //girrInterpolator,
        //girrCompounding,
        //girrFrequency,

        spreadOverYield,
        //spreadOverYieldCompounding,
        spreadOverYieldDCB,

        csrCnt,
        csrSpreads,

        logYn,

        resultGirrDelta,
        resultCsrDelta
    );

    std::cout << "Net PV(�Ҽ��� ����): " << std::fixed << std::setprecision(10) << netPV << std::endl;
    std::cout << "Net PV(raw Double): " << netPV << std::endl;
    
    std::cout << "GIRR Delta Size: " << static_cast<int>(resultGirrDelta[0]) << std::endl;
    for (int i = 1; i < 25; ++i) {
        std::cout << i  << ". GIRR Delta Sensitivity: " << std::fixed << std::setprecision(10) << resultGirrDelta[i] << std::endl;

    }

    std::cout << "CSR Delta Size: " << static_cast<int>(resultCsrDelta[0]) << std::endl;
    for (int i = 1; i < 25; ++i) {
        std::cout << i << ". CSRR Delta Sensitivity: " << std::fixed << std::setprecision(10) << resultCsrDelta[i] << std::endl;
    }

    // ȭ�� ���� ����
    #ifdef _WIN32
    system("pause");
    #else
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    #endif

    return 0;
}
