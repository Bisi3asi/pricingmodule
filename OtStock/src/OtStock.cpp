//---------------------------------------------------------------
//	���� : stockPricing ó���ϱ� ���� ���
//	���ID : stockPricing
//---------------------------------------------------------------

#include <iostream> 
#include <vector>
#include <string>
#include <stdlib.h>  
#include <string.h>
#include <time.h>
#include <math.h>

// �б⹮ ó��
#ifdef _WIN32
	#include <windows.h> 
	#define EXPORT __declspec(dllexport) __stdcall
#elif defined(__linux__) || defined(__unix__)
	#include <unistd.h>
	#define EXPORT
#endif

using namespace std;

//--------------------------------------------------------------
// DLL ���� ����
//--------------------------------------------------------------
int m_logLevel = 0;				// �α׷��� 0 = �ܼ���� X, 1 = �ܼ���� O

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
extern "C" long EXPORT stockPricing(double amt, double price, double basePrice, double beta, double fx, double* p, double* ResultPrice)
{

	long		retNo		= 0 ;		// return cole
	double      retPrice    = 0 ;		// �� �ݾ�
	double      retDeta     = 0 ;		
	// amt * ( prive / basePrice ) ^ beta

	try {

		if ( basePrice != 0 ) {
			retPrice = amt * pow( (price / basePrice) , beta );
		} 
		
		// ��ȭ �������̸�...
		if ( fx > 0 ) {
			retPrice = retPrice * fx ;
		}
	}
	catch (...) {
		cout << "";
		retNo = -999;
	}
	// #modify end	
	
	// ��� Return
	*ResultPrice = retPrice;  // �̷а�
	p[0] = 0;	// ��Ÿ
	p[1] = 0;	// ����
	p[2] = 0;	// ����
	p[3] = 0;	// ��Ÿ
	p[4] = 4;	// ��
	return retNo;			
}
