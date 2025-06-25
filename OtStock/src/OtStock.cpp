//---------------------------------------------------------------
//	모듈명 : stockPricing 처리하기 위한 모듈
//	모듈ID : stockPricing
//---------------------------------------------------------------

#include <iostream> 
#include <vector>
#include <string>
#include <stdlib.h>  
#include <string.h>
#include <time.h>
#include <math.h>

// 분기문 처리
#ifdef _WIN32
	#include <windows.h> 
	#define EXPORT __declspec(dllexport) __stdcall
#elif defined(__linux__) || defined(__unix__)
	#include <unistd.h>
	#define EXPORT
#endif

using namespace std;

//--------------------------------------------------------------
// DLL 변수 정의
//--------------------------------------------------------------
int m_logLevel = 0;				// 로그레벨 0 = 콘솔출력 X, 1 = 콘솔출력 O

//----------------------------------------------------------------
// stockPricing
// amt : 주식 시가
// price : 가격 ( 위험요인 O : 주식가격 , 위험요인 X : 주가 지수 ) - 시나리오 적용된 가격 또는 사후 검증의 금일 종가.
// basePricee : 기준가격 ( 위험요인 O : 주식 기준가격 , 위험요인 X : 주가지수 기준 가격 ) - 사후 검증 시 전일 종가.
// beta : 주식 베타 ( 위험요인 O : 1 , 위험요인 X : 주식 베타 ) 
// fx : 외화환율 ( 원화 포지션일 경우 0 )
// *p : 0:델타 , 1:감마, 2:베타, 3:세타, 4:로 
// *ResultPrice : 평가가격(이론가)
//----------------------------------------------------------------
extern "C" long EXPORT stockPricing(double amt, double price, double basePrice, double beta, double fx, double* p, double* ResultPrice)
{

	long		retNo		= 0 ;		// return cole
	double      retPrice    = 0 ;		// 평가 금액
	double      retDeta     = 0 ;		
	// amt * ( prive / basePrice ) ^ beta

	try {

		if ( basePrice != 0 ) {
			retPrice = amt * pow( (price / basePrice) , beta );
		} 
		
		// 외화 포지션이면...
		if ( fx > 0 ) {
			retPrice = retPrice * fx ;
		}
	}
	catch (...) {
		cout << "";
		retNo = -999;
	}
	// #modify end	
	
	// 결과 Return
	*ResultPrice = retPrice;  // 이론가
	p[0] = 0;	// 델타
	p[1] = 0;	// 감마
	p[2] = 0;	// 베가
	p[3] = 0;	// 세타
	p[4] = 4;	// 로
	return retNo;			
}
