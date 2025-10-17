#pragma once

// function 외부 인터페이스 export 정의
#ifdef _WIN32
#ifdef BUILD_LIBRARY
#define EXPORT __declspec(dllexport) __stdcall
#else
#define EXPORT __declspec(dllimport) __stdcall
#endif
#elif defined(__linux__) || defined(__unix__)
#define EXPORT
#endif

#include "ql/cashflows/cashflows.hpp"
#include "ql/cashflows/simplecashflow.hpp"
#include "ql/cashflows/fixedratecoupon.hpp"
#include "ql/cashflows/iborcoupon.hpp"
#include "ql/time/schedule.hpp"
#include "ql/instruments/bond.hpp"
#include "ql/instruments/bonds/zerocouponbond.hpp"
#include "ql/instruments/bonds/fixedratebond.hpp"
#include "ql/instruments/bonds/floatingratebond.hpp"
#include "ql/pricingengines/bond/discountingbondengine.hpp"

/* Zero Coupon Leg */
extern "C" double EXPORT pricingZCL(
    // ===================================================================================================
    const int evaluationDate                // INPUT 1. 평가일 (serial number)
    , const int issueDate                   // INPUT 2. 발행일 (serial number)
    , const int maturityDate                // INPUT 3. 만기일 (serial number)
    , const double notional                 // INPUT 4. 채권 원금

    , const int numberOfGirrTenors          // INPUT 5. GIRR 만기 수
    , const int* girrTenorDays              // INPUT 6. GIRR 만기 (startDate로부터의 일수)
    , const double* girrRates               // INPUT 7. GIRR 금리
    , const int* girrConvention             // INPUT 8. GIRR DayCountern, 보간법, 이자 계산 방식, 이자 빈도 (TODO)

    , const double girrRiskWeight           // INPUT 9. (추가)girr 리스크요소 버킷의 위험 가중치(Curvature 산출 시 사용)

    , const int calType			            // INPUT 10. 계산 타입 (1: Price, 2. BASEL 2 민감도, 3. BASEL 3 민감도)
    , const int logYn                       // INPUT 11. 로그 파일 생성 여부 (0: No, 1: Yes)

                                            // OUTPUT 1. Net PV (리턴값)
    , double* resultBasel2                  // OUTPUT 2. (추가)Basel 2 Result(Delta, Gamma, Duration, Convexity, PV01)
    , double* resultGirrDelta               // OUTPUT 3. GIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultGirrCvr			        // OUTPUT 4. (추가)GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultCashFlow                // OUTPUT 5. CF(index 0: size, index cfNum * 7 + 1 ~ cfNum * 7 + 7: 
                                            //              startDate, endDate, notional, rate, payDate, CF, DF)
// ===================================================================================================
);

/* Fixed Rate Leg */
extern "C" double EXPORT pricingFDL(
    // ===================================================================================================
    const int evaluationDate                // INPUT 1. 평가일 (serial number)
    , const int issueDate                   // INPUT 2. 발행일 (serial number)
    , const int maturityDate                // INPUT 3. 만기일 (serial number)
    , const double notional                 // INPUT 4. 채권 원금
    , const double couponRate               // INPUT 5. 쿠폰 이율
    , const int couponDayCounter            // INPUT 6. DayCounter code (TODO)
    , const int couponCalendar              // INPUT 7. Calendar code (TODO)
    , const int couponFrequency             // INPUT 8. Frequency code (TODO)
    , const int scheduleGenRule             // INPUT 9. 스케쥴 생성 기준(Forward/Backward) (TODO)
    , const int paymentBDC                  // INPUT 10. 지급일 휴일 적용 기준 (TODO)
    , const int paymentLag                  // INPUT 11. 지급일 지연 일수
    , const int isNotionalExchange          // INPUT 12. 원금 지급 여부(0: 이자만 지급, others: 이자 + 원금 지급)

    , const int numberOfCoupons             // INPUT 13. 쿠폰 개수
    , const int* paymentDates               // INPUT 14. 지급일 배열
    , const int* realStartDates             // INPUT 15. 각 구간 시작일
    , const int* realEndDates               // INPUT 16. 각 구간 종료일

    , const int numberOfGirrTenors          // INPUT 16. GIRR 만기 수
    , const int* girrTenorDays              // INPUT 17. GIRR 만기 (startDate로부터의 일수)
    , const double* girrRates               // INPUT 18. GIRR 금리
    , const int* girrConvention             // INPUT 19. GIRR 컨벤션 [index 0 ~ 3: GIRR DayCounter, 보간법, 이자 계산 방식, 이자 빈도] (TODO)

    , const double girrRiskWeight           // INPUT 20. (추가) girr 리스크요소 버킷의 위험 가중치(Curvature 산출 시 사용) (TODO)

    , const int calType			            // INPUT 21. 계산 타입 (1: Price, 2. BASEL 2 민감도, 3. BASEL 3 민감도, 9: SOY)
    , const int logYn                       // INPUT 22. 로그 파일 생성 여부 (0: No, 1: Yes)

                                            // OUTPUT 1. Net PV (리턴값)
    , double* resultBasel2                  // OUTPUT 2. Basel 2 Result [index 0 ~ 4: Delta, Gamma, Duration, Convexity, PV01]
    , double* resultGirrDelta               // OUTPUT 3. GIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultGirrCvr			        // OUTPUT 4. GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultCashFlow                // OUTPUT 5. CF(index 0: size, index cfNum * 7 + 1 ~ cfNum * 7 + 7: 
                                            //              startDate, endDate, notional, rate, payDate, CF, DF)
// ===================================================================================================
);

/* Floating Rate Leg */
extern "C" double EXPORT pricingFLL(
    // ===================================================================================================
    const int evaluationDate                // INPUT 1. 평가일 (serial number)
    , const int issueDate                   // INPUT 2. 발행일 (serial number)
    , const int maturityDate                // INPUT 3. 만기일 (serial number)
    , const double notional                 // INPUT 4. 채권 원금
    , const int couponDayCounter            // INPUT 5. DayCounter code
    , const int couponCalendar              // INPUT 6. Coupon Calendar
    , const int couponFrequency             // INPUT 7. 이자지급 주기
    , const int scheduleGenRule             // INPUT 8. 스케쥴 생성 기준(Forward/Backward) (TODO)
    , const int paymentBDC                  // INPUT 9. 지급일 휴일 적용 기준 (TODO)
    , const int paymentLag                  // INPUT 10. 지급일 지연 일수
    , const int isNotionalExchange          // INPUT 11. 원금 지급 여부(0: 이자만 지급, others: 이자 + 원금 지급)

    , const int fixingDays                  // INPUT 12. 금리 확정일 수
    , const double gearing                  // INPUT 13. 참여율
    , const double spread                   // INPUT 14. 스프레드
    , const double lastResetRate            // INPUT 15. 직전 확정 금리
    , const double nextResetRate            // INPUT 16. 차기 확정 금리

    , const int numberOfCoupons             // INPUT 17. 쿠폰 개수
    , const int* paymentDates               // INPUT 18. 지급일 배열
    , const int* realStartDates             // INPUT 19. 각 구간 시작일
    , const int* realEndDates               // INPUT 20. 각 구간 종료일

    , const int numberOfGirrTenors          // INPUT 21. GIRR 만기 수
    , const int* girrTenorDays              // INPUT 22. GIRR 만기 (startDate로부터의 일수)
    , const double* girrRates               // INPUT 23. GIRR 금리
    , const int* girrConvention             // INPUT 24. GIRR 컨벤션 [index 0 ~ 3: GIRR DayCounter, 보간법, 이자 계산 방식, 이자 빈도] (TODO)

    , const int numberOfIndexGirrTenors     // INPUT 25. GIRR 만기 수
    , const int* indexGirrTenorDays         // INPUT 26. GIRR 만기 (startDate로부터의 일수)
    , const double* indexGirrRates          // INPUT 27. GIRR 금리
    , const int* indexGirrConvention        // INPUT 28. GIRR 컨벤션 [index 0 ~ 3: GIRR DayCounter, 보간법, 이자 계산 방식, 이자 빈도] (TODO)
    , const int isSameCurve                 // INPUT 29. Discounting Curve와 Index Curve의 일치 여부(0: False, others: true)

    , const int indexTenor                  // INPUT 30. 금리 인덱스 만기의 날짜수(1 Month = 30 기준)
    , const int indexFixingDays             // INPUT 31. 금리 인덱스의 고시 확정일 수
    , const int indexCurrency               // INPUT 32. 금리 인덱스의 표시 통화
    , const int indexCalendar               // INPUT 33. 금리 인덱스의 휴일 기준 달력
    , const int indexBDC                    // INPUT 34. 금리 인덱스의 휴일 적용 기준
    , const int indexEOM                    // INPUT 35. 금리 인덱스의 월말 여부
    , const int indexDayCounter             // INPUT 36. 금리 인덱스의 날짜 계산 기준

    , const double girrRiskWeight           // INPUT 37. (추가) girr 리스크요소 버킷의 위험 가중치(Curvature 산출 시 사용) (TODO)

    , const int calType			            // INPUT 38. 계산 타입 (1: Price, 2. BASEL 2 Delta, 3. BASEL 3 GIRR / CSR, 9. SOY)
    , const int logYn                       // INPUT 39. 로그 파일 생성 여부 (0: No, 1: Yes)

                                            // OUTPUT 1. Net PV (리턴값)
    , double* resultGirrBasel2              // OUTPUT 2. Basel 2 Result [index 0 ~ 4: Delta, Gamma, Duration, Convexity, PV01]
    , double* resultIndexGirrBasel2         // OUTPUT 3. Basel 2 Result [index 0 ~ 4: Delta, Gamma, Duration, Convexity, PV01]
    , double* resultGirrDelta               // OUTPUT 4. GIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultIndexGirrDelta          // OUTPUT 5. IndexGIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultGirrCvr			        // OUTPUT 7. GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultIndexGirrCvr			// OUTPUT 8. GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultCashFlow                // OUTPUT 9. CF(index 0: size, index cfNum * 7 + 1 ~ cfNum * 7 + 7: 
                                            //              startDate, endDate, notional, rate, payDate, CF, DF)
// ===================================================================================================
);

 /* Wrapper class */
 class FixedRateBondCustom : public QuantLib::Bond {
 public:
     FixedRateBondCustom(QuantLib::Natural settlementDays,
         QuantLib::Real faceAmount,
         QuantLib::Schedule schedule,
         const std::vector<QuantLib::Rate>& coupons,
         const QuantLib::DayCounter& accrualDayCounter,
         QuantLib::BusinessDayConvention paymentConvention = QuantLib::Following,
         QuantLib::Integer paymentLag = 0,
         QuantLib::Real redemption = 100.0,
         const QuantLib::Date& issueDate = QuantLib::Date(),
         const QuantLib::Calendar& paymentCalendar = QuantLib::Calendar(),
         const QuantLib::Period& exCouponPeriod = QuantLib::Period(),
         const QuantLib::Calendar& exCouponCalendar = QuantLib::Calendar(),
         QuantLib::BusinessDayConvention exCouponConvention = QuantLib::Unadjusted,
         bool exCouponEndOfMonth = false,
         const QuantLib::DayCounter& firstPeriodDayCounter = QuantLib::DayCounter());

     QuantLib::Frequency frequency() const { return frequency_; }
     const QuantLib::DayCounter& dayCounter() const { return dayCounter_; }
     const QuantLib::DayCounter& firstPeriodDayCounter() const { return firstPeriodDayCounter_; }

 protected:
     QuantLib::Frequency frequency_;
     QuantLib::DayCounter dayCounter_;
     QuantLib::DayCounter firstPeriodDayCounter_;
 };

 class FloatingRateBondCustom : public QuantLib::Bond {
 public:
     FloatingRateBondCustom(QuantLib::Natural settlementDays,
         QuantLib::Real faceAmount,
         QuantLib::Schedule schedule,
         const QuantLib::ext::shared_ptr<QuantLib::IborIndex>& iborIndex,
         const QuantLib::DayCounter& accrualDayCounter,
         QuantLib::BusinessDayConvention paymentConvention = QuantLib::Following,
         QuantLib::Natural fixingDays = QuantLib::Null<QuantLib::Natural>(),
         QuantLib::Integer paymentLag = 0,
         const std::vector<QuantLib::Real>& gearings = { 1.0 },
         const std::vector<QuantLib::Spread>& spreads = { 0.0 },
         const std::vector<QuantLib::Rate>& caps = {},
         const std::vector<QuantLib::Rate>& floors = {},
         bool inArrears = false,
         QuantLib::Real redemption = 100.0,
         const QuantLib::Date& issueDate = QuantLib::Date(),
         const QuantLib::Period& exCouponPeriod = QuantLib::Period(),
         const QuantLib::Calendar& exCouponCalendar = QuantLib::Calendar(),
         QuantLib::BusinessDayConvention exCouponConvention = QuantLib::Unadjusted,
         bool exCouponEndOfMonth = false);
 };