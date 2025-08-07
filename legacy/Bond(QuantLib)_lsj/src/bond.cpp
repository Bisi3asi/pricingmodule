#include "bond.h"
#include "logger.h"

#include <spdlog/spdlog.h>

using namespace QuantLib;
using namespace std;
using namespace spdlog;

extern "C" {
    double EXPORT_API pricing(
        // ===================================================================================================
        double notional                             // INPUT 1.  명목금액
        , double exchangeRate                       // INPUT 2.  환율
        , long issueDateSerial                      // INPUT 3.  채권 발행일 (serial) 
        , long maturityDateSerial                   // INPUT 4.  채권 만기일 (serial)
        , long revaluationDateSerial                // INPUT 5.  채권 평가기준일 (serial)
        , double couponRate                         // INPUT 6.  쿠폰 금리
        , int couponFrequencyMonth                  // INPUT 7.  쿠폰 지급 주기 (month)
        , int couponDcb                             // INPUT 8.  쿠폰 지급일자 산출간 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        , int accrualDcb                            // INPUT 9.  쿠폰 가격 산출간 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        , int businessDayConvention                 // INPUT 10.  영업일 계산방법 [Following = 0, Modified Following = 1, Preceding = 2, Modified Preceding = 3]
        , int periodEndDateConvention               // INPUT 11.  기간 월말 계산방법 [Adjusted = 0, UnAdjusted = 1]

        , const char* dcCurveId                     // INPUT 12. 할인 커브 ID
        , int dcCurveDataSize                       // INPUT 13. 할인 커브 데이터 개수  
        , const double* dcCurveYearFrac             // INPUT 14. 할인 커브 연도분수
        , const double* dcCurveMarketData           // INPUT 15. 할인 커브 할인율

        // , const char* crsCurveId                    // INPUT 16. CRS 커브 ID 
        // , int crsCurveDataSize                      // INPUT 17. CRS 커브 데이터 개수
        // , const double* crsCurveYearFrac            // INPUT 18. CRS 커브 연도분수
        // , const double* crsCurveMarketData          // INPUT 19. CRS 커브 할인율

        , int logYn                                 // INPUT 20. 로그 파일 생성 여부 (0: No, 1: Yes)

        // OUTPUT 1. 결과값 (NET PV, Return Value)
        , double* resultGirrDelta 			        // OUTPUT 2. 결과값 ([index 0] array size, [index 1 ~ size] Girr Delta tenor, [size + 1 ~ end] Girr Delta Sensitivity)
        // , double* resultCsrDelta                         // OUTPUT 3. 결과값 ([index 0] array size, [index 1 ~ size] Girr Delta tenor, [size + 1 ~ end] Girr Delta Sensitivity)
) {

        /* 0. 로거 초기화 */
        // 디버그용 메소드는 아래 for debug 메소드를 참고
        if (logYn == 1) initLogger("bond.log"); // 생성 파일명 지정

        info("==============[bond Logging Started!]==============");
        // Input 데이터 로깅
        printAllInputData(notional, exchangeRate, issueDateSerial, maturityDateSerial, revaluationDateSerial, couponRate, couponFrequencyMonth, couponDcb, accrualDcb, businessDayConvention, periodEndDateConvention, dcCurveId, dcCurveDataSize, dcCurveYearFrac, dcCurveMarketData);

        TradeInformation tradeInfo{}; // 기본 상품 정보
        vector<CouponCashflow> cashflows{}; // 쿠폰 현금흐름
        vector<Curve> curves{}; // 커브 데이터
        vector<Girr> girrs{}; // GIRR Delta 데이터   
        vector<Csr> csrs{};  // CSR Delta 데이터
        vector<DayCounter> dayCounters{}; // Day Counter 데이터(캐시용 집합)
        vector<BusinessDayConvention> dayConventions{}; // Day Convention 데이터(캐시용 집합)
        //vector<Frequency> frequencies{}; // Frequency 데이터(캐시용 집합)

        initDayCounters(dayCounters);
        initDayConventions(dayConventions);
        //initFrequencies(frequencies);

        /* 1. 기본 데이터 세팅 */
        
        // 결과값 초기화
         initResult(resultGirrDelta, 25);    // Girr Delta
        // initResult(resultCsrDelta, 25);

        // Day Counter, Business Day Convention 데이터 생성
        initDayCounters(dayCounters);
        initDayConventions(dayConventions);

        // Trade inforamtion 데이터 생성
        inputTradeInformation(notional, exchangeRate, issueDateSerial, maturityDateSerial, revaluationDateSerial, couponRate, couponFrequencyMonth, couponDcb, accrualDcb, businessDayConvention, periodEndDateConvention, dcCurveId, /* crsCurveId, */ tradeInfo, dayCounters, dayConventions);
        // Coupon CashFlow 데이터 생성
        inputCouponCashFlow(tradeInfo, cashflows);
        // GIRR curve 데이터 생성(Curve Id, Tenor), GIRR Sensitiity 리턴 개수 정의
        inputGirrDelta(tradeInfo.dcCurveId, cashflows.back().yearFrac, girrs, resultGirrDelta);
        // Discount curve 데이터 생성 
        inputCurveData(dcCurveId, dcCurveDataSize, dcCurveYearFrac, dcCurveMarketData, curves);
        // Credit Spread curve 데이터 생성
        //inputCurveData(crsCurveId, crsCurveDataSize, crsCurveYearFrac, crsCurveMarketData, curves);
       
        /* 2. NetPV 산출 */ 
        double netPV = roundToDecimals(processNetPV(tradeInfo, cashflows, curves), 10); // Net PV (소수점 열째자리까지 반올림)

        /* 3. GIRR 커브별 Sensitivity 산출 */
        for (int i = 0; i < girrs.size(); ++i) {
            auto& girr = girrs[i];

            //GIRR Delta 산출 간 커브 데이터 초기화 (커브 MarKet Data Value 변경하며 Delta 산출)
            inputCurveData(dcCurveId, dcCurveDataSize, dcCurveYearFrac, dcCurveMarketData, curves);

            // GIRR Delta Sensitivity 산출
            processGirrSensitivity(tradeInfo, cashflows, curves, girr, netPV, resultGirrDelta /* ,resultCsrDelta */);

            // GiRR Delta 결과값을 resultGirrDelta에 저장
            resultGirrDelta[i + 1] = girr.yearFrac; // GIRR Delta의 yearFrac (index 1 ~ size)
            resultGirrDelta[i + 1 + girrs.size()] = roundToDecimals(girr.sensitivity, 10); // GIRR Delta의 Sensitivity (index size + 1 ~ end) (소수점 열째자리까지 반올림)

            // CSR Delta 결과값을 resultCsrDelta에 저장
            //resultCsrDelta[i + 1] = csr.yearFrac; // CSR Delta의 yearFrac (index 1 ~ size)
            //resultCsrDelta[i + 1 + csrs.size()] = roundToDecimals(csr.sensitivity, 10); // CSR Delta의 Sensitivity (index size + 1 ~ end) (소수점 열째자리까지 반올림)
        }
        
        /* Output 데이터 로깅 */
        // 모든 Struct 출력
		printAllData(tradeInfo, cashflows, curves, girrs); 
        // 모든 결과값 출력
		printAllOutputData(netPV, resultGirrDelta /* ,resultCsrDelta */); 
        info("==============[bond Logging Ended!]==============");

        return netPV;
    }
}
// 결과값 초기화
void initResult(double* result, int size) {

    fill_n(result, size, 0.0);
}

// Day Counter 객체 생성
void initDayCounters(vector<DayCounter>& dayCounters) {

    dayCounters.clear();
    dayCounters.reserve(6);

    dayCounters.emplace_back(Thirty360(Thirty360::USA));         // 0: 30U/360
    dayCounters.emplace_back(ActualActual(ActualActual::ISDA));  // 1: Act/Act
    dayCounters.emplace_back(Actual360());                       // 2: Act/360
    dayCounters.emplace_back(Actual365Fixed());                  // 3: Act/365
    dayCounters.emplace_back(Thirty360(Thirty360::European));    // 4: 30E/360
    dayCounters.emplace_back();                                  // 5: ERR
}

// Business Day Convention 객체 생성
void initDayConventions(vector<BusinessDayConvention>& dayConventions) {

    dayConventions.clear();
    dayConventions.reserve(5);

    dayConventions.emplace_back(Following);           // 0: Following
    dayConventions.emplace_back(ModifiedFollowing);   // 1: Modified Following 
    dayConventions.emplace_back(Preceding);           // 2: Preceding
    dayConventions.emplace_back(ModifiedPreceding);   // 3: Modified Preceding
    dayConventions.emplace_back();                    // 4: ERR
}

// 거래정보 입력
void inputTradeInformation(
    const long notional, const long exchangeRate, const long issueDateSerial, const long maturityDateSerial, const long revaluationDateSerial,
    const double couponRate, const double couponFrequencyMonth, const int couponDcb, const int accrualDcb, const int businessDayConvention, const int periodEndDateConvention,
    const char* dcCurveId, /* const char* crsCurveId, */
    TradeInformation& tradeInfo, /* vector<Frequency> frequencies, */ 
    const vector<DayCounter>& dayCounters, const vector<BusinessDayConvention>& dayConventions
) {
    
    tradeInfo.notional = notional; // 명목금액
    tradeInfo.exchangeRate = exchangeRate; // 상대 통화 거래 시 환율
    tradeInfo.issueDate = Date(issueDateSerial); // 채권 발행일
    tradeInfo.maturityDate = Date(maturityDateSerial); // 채권 만기일
    tradeInfo.revaluationDate = Date(revaluationDateSerial); // 채권 평가 기준일
    
    tradeInfo.couponRate = couponRate; // 쿠폰 금리
    //tradeInfo.couponFrequency = getFrequencyByMonth(couponFrequencyMonth, frequencies); // 쿠폰 지급주기
    tradeInfo.couponFrequencyMonth = couponFrequencyMonth; // 쿠폰 지급주기
    tradeInfo.couponDcb = getDayConterByCode(couponDcb, dayCounters); // 쿠폰 지급일자 Day Count Basis
    tradeInfo.accrualDcb = getDayConterByCode(accrualDcb, dayCounters); // 쿠폰 지급일자 Day Count Basis
    tradeInfo.dayConvention = getDayConventionByCode(businessDayConvention, dayConventions);
    tradeInfo.periodEndDateConvention = periodEndDateConvention;
    
    snprintf(tradeInfo.dcCurveId, sizeof(tradeInfo.dcCurveId), "%s", dcCurveId); // Discount curve ID
    // snprintf(tradeInfo.crsCurveId, sizeof(tradeInfo.crsCurveId), "%s", crsCurveId); // Crs curve ID
}

// 쿠폰 현금흐름 생성 및 입력
// TODO : 차후 불규칙 상환채에 대한 로직 보강 필요, 현재는 규칙 상환채만 호환 (만기일자 = n번째 쿠폰기간 종료일자)
void inputCouponCashFlow(const TradeInformation& tradeInfo, vector<CouponCashflow>& cashflows) {
    
    Date startDate = tradeInfo.issueDate; // 발행일부터 시작
    Date endDate, paymentDate; // 쿠폰 종료일자, 지급일자

    Calendar cal = SouthKorea(); // 영업일 달력 지정
    int row = 1;

    // 지급일 계산 
	// TODO : frequency 관련 인터페이스 논의 필요(현재는 M월 단위로 입력, 차후 Weekly, Daily 등으로 확장 필요)
    while (startDate < tradeInfo.maturityDate) {
        CouponCashflow cashflow = {}; // Coupon CashFlow 초기화

        endDate = cal.advance(tradeInfo.issueDate, Period(tradeInfo.couponFrequencyMonth * row, Months), Unadjusted); // 종료일자 : 단순 월 이동
        paymentDate = cal.adjust(endDate, tradeInfo.dayConvention); // 지급일자 : 영업일 조정
        
        cashflow.type = (endDate == tradeInfo.maturityDate) ? 1 : 0; // 쿠폰타입 [1 = 원금, 0 = 쿠폰]
        if (tradeInfo.periodEndDateConvention == 0) { // Adjusted인 경우 쿠폰 종료일자를 지급일자로 조정
            endDate = paymentDate;
        }

        // 만기일자 이전의 현금흐름만 계산
        if (endDate <= tradeInfo.maturityDate) { 
            cashflow.startDate = startDate; // 시작일자
            cashflow.endDate = endDate; // 종료일자
            cashflow.paymentDate = paymentDate; // 지급일자
            cashflow.coupon = tradeInfo.notional * tradeInfo.couponRate * tradeInfo.couponDcb.yearFraction(startDate, endDate); // 지급금액
            
            // 평가 기준일자 이전의 기지급된 현금흐름은 계산하지 않음
            cashflow.yearFrac = (paymentDate < tradeInfo.revaluationDate) ? 0.0 : tradeInfo.accrualDcb.yearFraction(tradeInfo.revaluationDate, paymentDate); // 연도분수

            // cashflow vector에 현금흐름 추가
            cashflows.push_back(cashflow);
        }

        // 다음 구간으로 이동
        startDate = endDate;
        row++;
    }
}

// 커브 데이터 입력 및 초기화
void inputCurveData(const char* curveId, const int curveDataSize, const double* yearFrac, const double* marketData, vector<Curve>& curves) {
	curves.clear(); // 기존 데이터 초기화
    curves.reserve(curves.capacity() + curveDataSize); // capacity 확보

    for (int i = 0; i < curveDataSize; ++i) {
		Curve curve{}; // Curve 초기화

        snprintf(curve.curveId, sizeof(curve.curveId), "%s", curveId); // 커브 ID
		curve.yearFrac = yearFrac[i]; // 연도분수
        curve.marketData = marketData[i]; // 마켓 데이터

		curves.push_back(curve); // 커브 데이터 추가
    }
}

// yearFrac의 올림값 기간을 초과하지 않는 GIRR Delta Curve 생성
void inputGirrDelta(const char* curveId, const double maxYearFrac, vector<Girr>& girrs, double* resultGIRRDelta) {

    girrs.reserve(girrs.capacity() + 25); // capacity 할당

    const array<double, 10> yearFracs = { 0.25, 0.5, 1, 2, 3, 5, 10, 15, 20, 30 };
    double limitYearFrac = ceil(maxYearFrac); // 최대 연도 분수 (올림값)

    for (double yearFrac : yearFracs) {
        if (yearFrac <= limitYearFrac) {
            ++resultGIRRDelta[0]; // 유효한 GIRR 커브 수를 카운트 (리턴용)

            Girr girr{};
            snprintf(girr.curveId, sizeof(girr.curveId), "%s", curveId); // 커브 ID
            girr.yearFrac = yearFrac; // 연도 분수
            girrs.push_back(girr);
        }
    }
}

// GIRR Sensitity 산출을 위해 GIRR Curve에 따른 마켓 데이터 할인 적용
void setCurveDcRate(const Girr& girr, vector<Curve>& curves, const Real dcRate) {

    for (auto& curve : curves) {
        // Basis curve 커브 ID 일치 시 할인
        if (strlen(girr.curveId) >= 5 && strcmp(girr.curveId + strlen(girr.curveId) - 5, "Basis") == 0) {
            if (strncmp(girr.curveId, curve.curveId, 9) == 0)
                curve.marketData += dcRate;
        }
        // (Basis curve 제외) 커브 ID 완전 일치 && yearFrac(tenor) 일치 시 할인
        else if (strcmp(girr.curveId, curve.curveId) == 0 && girr.yearFrac == curve.yearFrac) {
            curve.marketData += dcRate;
        }
    }
}

void bootstrapZeroCurveContinuous(vector<Curve>& curves) {

    // 커브 ID별로 그룹화
    unordered_map<string, vector<Curve*>> grouped;

    for (int i = 0; i < curves.size(); ++i) {
        grouped[string(curves[i].curveId)].push_back(&curves[i]);
    }

    // 커브별로 부트스트래핑 수행
    for (unordered_map<string, vector<Curve*>>::iterator it = grouped.begin(); it != grouped.end(); ++it) {
        vector<Curve*>& group = it->second;

        int n = group.size();
        vector<Real> discountFactors(n);

        for (int j = 0; j < n; ++j) {
            Real rate = group[j]->marketData / 100.0; // 현재 커브의 할인율
            Real yearFrac = group[j]->yearFrac; // 현재 커브의 연도 분수
            Real sumDf = 0.0; // 누적 할인 계수
            if (j == 0) {
                // 첫번째 커브 기간의 할인계수(단순 할인채 가정) = 1 / (1 + r * t)
                discountFactors[j] = 1.0 / (1.0 + rate * yearFrac);
            }
            else {
                // 두번째 기간부터 쿠폰 누적 적용, 이전까지 부트스트랩된 누적 할인 계수를 이용하여 현재 커브의 할인계수를 역산
                for (int k = 0; k < j; ++k) {
                    Real prevYearFrac = group[k]->yearFrac; // 이전 커브의 연도 분수
                    // 이전 쿠폰 지급 시점의 누계 할인계수 += 현재 커브의 할인율 * 이전 커브의 연도 분수 * 이전까지 부트스트랩된 누적 할인계수
                    // 현재 커브 구간 r(j)의 할인율을 기준으로, 과거 k 구간의 할인 발생 기간에 지급된 쿠폰을 현재가치화
                    sumDf += rate * prevYearFrac * discountFactors[k];
                }
                // 할인계수 = (1 - 누적 할인계수) / (1 + r * t)
                discountFactors[j] = (1.0 - sumDf) / (1.0 + rate * yearFrac);
            }

            // 연속 복리 기준 Zero Rate 계산
            Real zeroRate = -log(discountFactors[j]) / yearFrac;

            // 커브의 Discount Factor, Zero Rate 업데이트
            group[j]->dcFactor = discountFactors[j];
            group[j]->zeroRate = zeroRate * 100.0; // %로 변환
        }
    }
}

// Coupon discount rate, discount factor, PV 산출
void discountBond(const TradeInformation& tradeInfo, const vector<Curve>& curves, vector<CouponCashflow>& cashflows) {

    for (auto& cashflow : cashflows) {
        try {
            // dcRate : 선형보간을 통해 구한 Buy Side의 Year Fraction에 해당하는 Zero Rate(Discount Rate)
            cashflow.dcRate = linterp(getCurveYearFrac(tradeInfo.dcCurveId, curves), getCurveZeroRate(tradeInfo.dcCurveId, curves), cashflow.yearFrac);
            // dcFactor : dcRate를 이용하여 구한 Discount Factor (연속 복리)
            cashflow.dcFactor = exp(-1 * cashflow.dcRate / 100 * cashflow.yearFrac);
            // presentValue : (쿠폰) discount Factor * coupon, (원금) discount Factor * (coupon + notional)
            if (cashflow.type == 0) // 쿠폰
                cashflow.presentValue = cashflow.coupon * cashflow.dcFactor;
            if (cashflow.type == 1) // 원금
                cashflow.presentValue = (cashflow.coupon + tradeInfo.notional) * cashflow.dcFactor;
        }
        catch (exception& e) {
            error("[discountBond] cashflow Error: {}", e.what());
            cashflow.presentValue = 0.0;
        }
    }
}

// 선형보간을 통한 Year Fraction에 해당하는 Zero Rate(Discount Rate)를 계산하는 함수
// yearFraction이 오름차순 정렬되어 있어야 함
Real linterp(const vector<Real>& yearFractions, const vector<Real>& zeroRates, Real targetYearFraction) {

    if (yearFractions.size() != zeroRates.size() || yearFractions.size() < 2) {
        // zeroRates와 yearFractions의 크기가 다르거나, 2개 미만일 경우
        error("[linterp] yearFractions size: {}, zeroRates size: {}", yearFractions.size(), zeroRates.size());
        throw invalid_argument("input zero rates cnt is less then 2, can't interpolate.");
    }
    // 보간기 생성
    LinearInterpolation interpolator(yearFractions.begin(), yearFractions.end(), zeroRates.begin());

    // 외삽 허용 (VBA에서는 자동으로 외삽 허용)
    interpolator.enableExtrapolation();

    // 보간 결과 반환
    return interpolator(targetYearFraction);
}

// Net Present Value 산출 프로세스
Real processNetPV(const TradeInformation& tradeInfo, vector<CouponCashflow>& cashflows, vector<Curve>& curves) {
    
    // 1. 부트스트래핑으로 discount factor, zero rate 산출
    bootstrapZeroCurveContinuous(curves);
    // 2. Coupon discount rate, discount factor, PV 산출
    discountBond(tradeInfo, curves, cashflows);
    // 3. Net PV 산출
	return calNetPV(cashflows, tradeInfo.exchangeRate);
}

// Net Value 계산
Real calNetPV(const vector<CouponCashflow>& cashflows, const Real exchangeRate) {

    double sumPV = 0.0;

    for (auto& cashflow : cashflows) {
        sumPV += cashflow.presentValue;
    }

    return sumPV * exchangeRate; // Sum (Present Value) * 환율
}

// 각 GIRR Curve 별 Sensitivity 산출 프로세스
void processGirrSensitivity(const TradeInformation& tradeInfo, vector<CouponCashflow> cashflows, vector<Curve>& curves, Girr& girr, double domesticNetPV, double* resultGirrDelta /*, double* resultCsrDelta */ ) {
        
	// 1. GIRR Curve에 따른 마켓 데이터 할인 적용
    setCurveDcRate(girr, curves, 0.01);
    // 2. 부트스트래핑으로 discount factor, zero rate 산출
    bootstrapZeroCurveContinuous(curves);
    // 3. Coupon Cashflow PV 산출
    discountBond(tradeInfo, curves, cashflows);
    // 4. 마켓 데이터 변화에 따른 GIRR Delta Sensitivity 산출
    girr.sensitivity = calGirrSensitivity(domesticNetPV, calNetPV(cashflows, tradeInfo.exchangeRate));
}

// GIRR Delta Sensitivity 계산
Real calGirrSensitivity(const Real baseNetPV, const Real shockedNetPV) {
	
    // 커브 마켓데이터 1bp의 변화값에 따른 NET PV 차이
    return (baseNetPV - shockedNetPV) / 0.0001;
}

// 특정 Curve ID와 일치하는 Year Fraction들을 반환
vector<Real> getCurveYearFrac(const char* curveId, const vector<Curve>& curves) {
    vector<Real> resultRange;
    for (const auto& curve : curves) {
        if (strcmp(curve.curveId, curveId) == 0)
            resultRange.push_back(curve.yearFrac);
    }
    return resultRange;
}

// 특정 Curve ID와 일치하는 Zero Rate들을 반환
vector<Real> getCurveZeroRate(const char* curveId, const vector<Curve>& curves) {
    vector<Real> resultRange;
    for (const auto& curve : curves) {
        if (strcmp(curve.curveId, curveId) == 0)
            resultRange.push_back(curve.zeroRate);
    }
    return resultRange;
}

// dcb 코드에 의한 QuantLib DayCounter 객체를 리턴
DayCounter getDayConterByCode(const int dcb, const vector<DayCounter>& dayCounters) {

    switch (dcb) {
    case 0: // 30U/360
        return dayCounters[0];
    case 1: // Act/Act
        return dayCounters[1];
    case 2: // Act/360
        return dayCounters[2];
    case 3: // Act/365
        return dayCounters[3];
    case 4: // 30E/360
        return dayCounters[4];
    default:
        warn("[getDayConterByCode] Unknown dcb: default dayCounter applied.");
        return dayCounters[5];
    }
}

// bdc 코드에 의한 QuantLib BusinessDayConvention 객체를 리턴
BusinessDayConvention getDayConventionByCode(const int businessDayConvention, const vector<BusinessDayConvention>& dayConventions) {

    switch (businessDayConvention) {
    case 0: // Following
        return dayConventions[0];
    case 1: // Modified Following
        return dayConventions[1];
    case 2: // Preceding
        return dayConventions[2];
    case 3: // Modified Preceding
        return dayConventions[3];
    default:
        warn("[getDayConventionByCode] Unknown business Day Convention: default Business Day Convention applied.");
        return dayConventions[4];
    }
}

double roundToDecimals(const double value, const int n) {
	double factor = pow(10.0, n);
	return round(value * factor) / factor;
}

/* FOR DEBUG */
string qDateToString(const Date& date) {
    
    ostringstream oss;
    oss << date;
    return oss.str();
}

void printAllInputData(
    const double notional                             
    , const double exchangeRate
    , const long issueDateSerial
    , const long maturityDateSerial
    , const long revaluationDateSerial
    , const double couponRate
    , const int couponFrequencyMonth
    , const int couponDcb
    , const int accrualDcb
    , const int businessDayConvention
    , const int periodEndDateConvention

    , const char* dcCurveId                     
    , const int dcCurveDataSize
    , const double* dcCurveYearFrac             
    , const double* dcCurveMarketData           
) {

    info("[Print All: Input Data]");

    info("notional : {:0.4f}", notional);
    info("exchangeRate : {:0.4f}", exchangeRate);
    info("issueDateSerial : {}", issueDateSerial);
    info("maturityDateSerial : {}", maturityDateSerial);
    info("revaluationDateSerial : {}", revaluationDateSerial);
    info("couponRate : {:0.4f}", couponRate);
    info("couponFrequencyMonth : {}", couponFrequencyMonth);
    info("couponDcb : {}", couponDcb);
    info("accrualDcb : {}", accrualDcb);
    info("businessDayConvention : {}", businessDayConvention);
    info("periodEndDateConvention : {}", periodEndDateConvention);
    info("");

    info("dcCurveId : {}", dcCurveId);
    info("dcCurveDataSize : {}", dcCurveDataSize);
    for (int i = 0; i < dcCurveDataSize; ++i) {
        info("dcCurveYearFrac[{}] : {:0.6f}", i, dcCurveYearFrac[i]);
        info("dcCurveMarketData[{}] : {:0.15f}", i, dcCurveMarketData[i]);
    }
    info("------------------------------------------------------------");
    info("");
}

void printAllOutputData(const double netPV, const double* resultGirrDelta) {

    info("------------------------------------------------------------");
	info("[Print All: Output Data]");
    info("[netPV (Return)]");
	info("Net PV (소수점 고정) : {:0.10f}", netPV);
	info("Net PV (raw Double) : {}", netPV);
	info("");
	
    info("[resultGirrDelta]");
    info("INDEX 0. resultGirrDelta Size : {}", static_cast<int>(resultGirrDelta[0]));
	for (int i = 1; i < static_cast<int>(resultGirrDelta[0]) + 1; ++i) {
		info("INDEX {}. Girr Delta Tenor : {:0.4f}", i, resultGirrDelta[i]);
	}
    for (int i = static_cast<int>(resultGirrDelta[0]) + 1; i < static_cast<int>(resultGirrDelta[0]) * 2 + 1; ++i) {
		info("INDEX {}. Girr Delta Sensitivity : {:0.10f}", i, resultGirrDelta[i]);
    }
    for (int i = static_cast<int>(resultGirrDelta[0]) * 2 + 1; i < 25; ++i) {
		info("INDEX {}. Empty Value : ", i, resultGirrDelta[i]);
    }
    info("");
}

void printAllData(const TradeInformation& tradeInfo, const vector<CouponCashflow>& cashflows, const vector<Curve>& curves, const vector<Girr>& girrs) {
    
    printAllData(tradeInfo);
	printAllData(cashflows);
	printAllData(curves);
	printAllData(girrs);
 }

void printAllData(const TradeInformation& tradeInfo) {
    
    info("[Print All: Trade Information]");

    info("Notional: {}", tradeInfo.notional);
    info("Issue Date: {}", qDateToString(tradeInfo.issueDate));
    info("Maturity Date: {}", qDateToString(tradeInfo.maturityDate));
    info("Revaluation Date: {}", qDateToString(tradeInfo.revaluationDate));
    info("Coupon Rate: {:0.4f}", tradeInfo.couponRate);
    info("Coupon Frequency Month: {}", static_cast<int>(tradeInfo.couponFrequencyMonth));
    info("Coupon DCB: {}", tradeInfo.couponDcb.name());
    info("Accrual DCB: {}", tradeInfo.accrualDcb.name());
    info("Day Convention: {}", static_cast<int>(tradeInfo.dayConvention));
    info("Period End Convention: {}", tradeInfo.periodEndDateConvention == 0 ? "Adjusted" : "UnAdjusted");
    info("Discount Curve ID: {}", tradeInfo.dcCurveId);
    info("CRS Curve ID: {}", tradeInfo.crsCurveId);

    info("");
}

void printAllData(const vector<CouponCashflow>& cashflows) {
    
    info("[Print All: Coupon Cashflows]");
    info("Total Cashflow Count: {}", cashflows.size());
    info("------------------------------------------------------------");

    for (int i = 0; i < cashflows.size(); ++i) {
        const auto& cf = cashflows[i];
        info("[{}] Coupon Cashflow", i + 1);
        info("  Type : {}", cf.type == 0 ? "Coupon" : "Principal");
        info("  Start Date : {}", qDateToString(cf.startDate));
        info("  End Date : {}", qDateToString(cf.endDate));
        info("  Payment Date : {}", qDateToString(cf.paymentDate));
        info("  Coupon : {:0.10f}", cf.coupon);
        info("  Year Fraction : {:0.10f}", cf.yearFrac);
        info("  DC Rate : {:0.10f}", cf.dcRate);
        info("  DC Factor : {:0.10f}", cf.dcFactor);
        info("  Present Value : {:0.10f}", cf.presentValue);
        info("----------------------------------------------");
    }

    info("");
}

void printAllData(const vector<Curve>& curves) {
    
    info("[Print All: Curves]");
    info("Total Curves Count: {}", curves.size());
    info("------------------------------------------------------------");

    for (int i = 0; i < curves.size(); ++i) {
        const auto& curve = curves[i];
        info("[{}] Curve", i + 1);
        info("Curve ID : {}", curve.curveId);
        info("Year Fraction : {}", curve.yearFrac);
        info("Market Data : {:0.15f}", curve.marketData);
        info("Discount Factor : {}", curve.dcFactor);
        info("Zero Rate : {}", curve.zeroRate);
        info("----------------------------------------------");
    }

    info("");
}

void printAllData(const std::vector<Girr>& girrs) {
    
    info("[Print All: GIRR Delta Risk Factors]");
    info("Total GIRR Delta Count: {}", girrs.size());
    info("------------------------------------------------------------");

    for (int i = 0; i < girrs.size(); ++i) {
        const auto& girr = girrs[i];
        info("GIRR Curve : {}", girr.curveId);
        info("Year Fraction : {}", girr.yearFrac);
        info("Sensitivity : {:0.15f}", girr.sensitivity);
        info("----------------------------------------------");
    }

    info("");
}

/* TODO */
// Frequency 객체 생성
//void initFrequencies(vector<Frequency>& frequencies) {
//    frequencies.clear();
//    frequencies.reserve(13);
//
//    frequencies.emplace_back(Annual);           // 0: Annual (연간)
//    frequencies.emplace_back(Semiannual);       // 1: Semiannual (반기)
//    frequencies.emplace_back(Quarterly);        // 2: Quarterly (분기)
//    frequencies.emplace_back(Monthly);          // 3: Monthly (매달)
//    frequencies.emplace_back(Bimonthly);        // 4: Bimonthly (2달)
//    frequencies.emplace_back(Weekly);           // 5: Weekly (매주)
//    frequencies.emplace_back(Biweekly);         // 6: Biweekly (2주)
//    frequencies.emplace_back(Daily);            // 7: Daily (매일)
//    frequencies.emplace_back(NoFrequency);      // 8: No Frequency (주기 없음)
//    frequencies.emplace_back(Once);             // 9: Once  (최초 1회)
//    frequencies.emplace_back(EveryFourthMonth); // 10: Every Fourth Month (4달)
//    frequencies.emplace_back(EveryFourthWeek);  // 11: Every Fourth Week  (4주)
//    frequencies.emplace_back(OtherFrequency);   // Error
//}

// Frequency 코드(지급 주기, n달))에 의한 QuantLib Frequency 객체를 리턴
//Frequency getFrequencyByMonth(int month, vector<Frequency>& frequencies) {
//
//    // TODO : 실수형 (예 : 1주 ~ 2주 지급주기에 대한 로직 보강 필요)
//    switch (month) {
//    case 12: // Annual
//        return frequencies[0];
//    case 6: // SemiAnnual
//        return frequencies[1];
//    case 3: // Quaterly
//        return frequencies[2];
//    case 1: // Monthly
//        return frequencies[3];
//    //case 2: // Bimonthly
//        //return frequencies[4];
//    //case 5: // Weekly
//        //return frequencies[5];
//    //case 6: // Biweekly
//        //return frequencies[6];
//    //case 365: // Daily
//        //return frequencies[7];
//    //case 8: // NoFrequency
//        //return frequencies[8];
//    //case 9: // Once
//        //return frequencies[9];
//    //case 4: // EveryFourthMonth
//        //return frequencies[10];
//    //case 11: // EveryFourthWeek
//        //return frequencies[11];
//    default:
//        warn("[getFrequencyByCode] Unknown Frequency: default Frequency applied.");
//        return frequencies[12];
//    }
//}