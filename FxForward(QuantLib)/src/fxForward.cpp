#include "fxForward.h"
#include "logger.h"

#include <spdlog/spdlog.h>

using namespace QuantLib;
using namespace std;
using namespace spdlog;

extern "C" {
    void EXPORT_API pricing(
        // ===================================================================================================
        long maturityDateSerial                     // INPUT 1.  만기일 (Maturity Date) 
        , long revaluationDateSerial                // INPUT 2.  평가일 (Revaluation Date)
        , double exchangeRate                       // INPUT 3.  현물환율 (Domestic / Foreign)  (Exchange Rate)
		, int isBuySideDomestic					    // INPUT 4.  매입 통화 원화 여부 (0: No, 1: Yes)     

        , const char* buySideCurrency               // INPUT 5.  매입 통화 (Buy Side Currency)
        , double notionalForeign                    // INPUT 6.  매입 외화기준 명목금액 (Notional Foreign)
        , int buySideDCB                            // INPUT 7.  매입 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        , int buyCurveDataSize                      // INPUT 8.  매입 커브 데이터 사이즈
        , const double* buyCurveYearFrac            // INPUT 9.  매입 커브 만기 기간 (Buy Curve Term)  
        , const double* buyMarketData               // INPUT 10. 매입 커브 마켓 데이터 (Buy Curve Market Data)

        , const char* sellSideCurrency              // INPUT 11. 매도 통화 (sell Side Currency)
        , double notionalDomestic                   // INPUT 12. 매도 원화기준 명목금액 (Notional Domestic)
        , int sellSideDCB                           // INPUT 13. 매도 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        , int sellCurveDataSize                     // INPUT 14. 매도 커브 데이터 사이즈
        , const double* sellCurveYearFrac		    // INPUT 15. 매도 커브 만기 기간 (Sell Curve Term)     
        , const double* sellMarketData              // INPUT 16. 매도 커브 마켓 데이터 (Sell Curve Market Data) 

        , int logYn                                 // INPUT 17. 로그 파일 생성 여부 (0: No, 1: Yes)

        , double* resultNetPvFxSensitivity          // OUTPUT 1. [index 0] Net PV, [index 1] FX Sensitivity
		, double* resultGirrTenor                   // OUTPUT 2. GIRR Delta Tenor [index 0 ~ 10] {0(Basis), 3m, 6m, 1y, 2y, 3y, 5y, 10y, 15y, 20y, 30y}
        , double* resultGirrSensitivity 			// OUTPUT 3. GIRR Delta Sensitivity [index 0 ~ 10] Tenor에 해당하는 민감도
        // ===================================================================================================
    ) {

        /* 로거 초기화 */
        // 디버그용 메소드는 아래 for debug 메소드를 참고
        if (logYn == 1) {
            initLogger("fxForward.log"); // 생성 파일명 지정
            info("==============[fxForward Logging Started!]==============");
            printAllInputData( // Input 데이터 로깅
                maturityDateSerial, revaluationDateSerial, exchangeRate, isBuySideDomestic,
                buySideCurrency, notionalForeign, buySideDCB,
                buyCurveDataSize, buyCurveYearFrac, buyMarketData,
                sellSideCurrency, notionalDomestic, sellSideDCB,
                sellCurveDataSize, sellCurveYearFrac, sellMarketData,
                logYn
            );
        }

        TradeInformation tradeInfo{}; // 기본 상품 정보
        BuySideValuationCashFlow bSideCashFlow{}; // 매입 통화 현금흐름
        SellSideValuationCashFlow sSideCashFlow{}; // 매도 통화 현금흐름
        vector<Curve> curves{}; // 커브 데이터
        vector<Girr> girrs{}; // GIRR Delta 데이터
        vector<DayCounter> dayCounters{}; // Day Counter 데이터(캐시용 집합)

        initResult(resultNetPvFxSensitivity, 2); // OUTPUT 1 데이터 초기화
        initResult(resultGirrTenor, 11); // OUTPUT 2 데이터 초기화
        initResult(resultGirrSensitivity, 22); // OUTPUT 3 데이터 초기화

        /* 기본 데이터 세팅 */
        // Day Counter        데이터 생성
        initDayCounters(dayCounters);
        // Trade inforamtion  데이터 생성
        inputTradeInformation(maturityDateSerial, revaluationDateSerial, exchangeRate, isBuySideDomestic, tradeInfo);
        // Buy Side CashFlow  데이터 생성
        inputBuySideValuationCashFlow(buySideCurrency, notionalForeign, revaluationDateSerial, maturityDateSerial, buySideDCB, bSideCashFlow, dayCounters);
        // Sell Side CashFlow 데이터 생성
        inputSellSideValuationCashFlow(sellSideCurrency, notionalDomestic, revaluationDateSerial, maturityDateSerial, sellSideDCB, sSideCashFlow, dayCounters);
        // GIRR Delta Curve   데이터 생성
        setGirrDeltaCurve(bSideCashFlow, sSideCashFlow, girrs);
        // Curve              데이터 생성 
        inputCurveData(buyCurveDataSize, buyCurveYearFrac, buyMarketData, sellCurveDataSize, sellCurveYearFrac, sellMarketData, curves);

        /* OUTPUT 1. NetPV, FX Sensitivity 산출 및 결과 적재 */
        resultNetPvFxSensitivity[0] = roundToDecimals(processNetPV(tradeInfo, bSideCashFlow, sSideCashFlow, curves), 10); // Net PV (소수점 열째자리까지 반올림)
        resultNetPvFxSensitivity[1] = roundToDecimals(processFxSensitivity(tradeInfo, bSideCashFlow, sSideCashFlow), 10); // FX Sensitivity (소수점 열째자리까지 반올림)

        /*  GIRR 커브별 Sensitivity 산출 */
        for (int i = 0; i < girrs.size(); ++i) {
            auto& girr = girrs[i];

            // GIRR Delta 산출 간 커브 데이터 초기화 (커브 MarKet Data Value 변경하며 Delta 산출)
            inputCurveData(buyCurveDataSize, buyCurveYearFrac, buyMarketData, sellCurveDataSize, sellCurveYearFrac, sellMarketData, curves);

            // GIRR Delta Sensitivity 산출
            processGirrSensitivity(tradeInfo, bSideCashFlow, sSideCashFlow, curves, girr, resultNetPvFxSensitivity);

            // GiRR Delta 결과값을 resultGirrDelta에 저장
            resultGirrTenor[i] = girr.yearFrac; // GIRR Delta의 yearFrac (index 1 ~ size)
            resultGirrSensitivity[i] = roundToDecimals(girr.sensitivity, 10); // GIRR Delta의 Sensitivity (index size + 1 ~ end) (소수점 열째자리까지 반올림)
        }

        // OUTPUT 2. GIRR Delta Tenor
        double tenor[11] = { 0.0, 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 15.0, 20.0, 30.0 };
        resultGirrTenor = tenor;

        // OUTPUT 3. GIRR Delta Sensitivity
        std::fill(resultGirrSensitivity, resultGirrSensitivity + 22, 0.0);

        for (const auto& girr : girrs) {
            for (int i = 0; i < 11; ++i) {  // tenor에서 yearFrac 위치 찾기
                if (girr.yearFrac == tenor[i]) {
					int offset = (girr.sideFlag == 0) ? 0 : 11; // Buy Side이면 index 0, Sell Side이면 index 11부터 적재
                    resultGirrSensitivity[offset + i] = girr.sensitivity;
                    break;
                }
            }
        }

        // Output 데이터 로깅
        printAllOutputData(resultNetPvFxSensitivity, resultGirrTenor, resultGirrSensitivity);
        info("==============[fxForward Logging Ended!]==============");
    }
}

// 결과값 초기화
void initResult(double* result, int size) {
    fill_n(result, size, 0.0);
}

// 거래정보 입력
void inputTradeInformation(long maturityDateSerial, long revaluationDateSerial, double exchangeRate, int isBuySideDomestic, TradeInformation& tradeInformation) {
    
    tradeInformation.maturityDate = Date(maturityDateSerial); // 만기일
    tradeInformation.revaluationDate = Date(revaluationDateSerial); // 평가일
    tradeInformation.exchangeRate = exchangeRate; // 환율
    tradeInformation.isBuySideDomestic = isBuySideDomestic; // 매입 통화 원화 여부
}

// 매입 통화 현금흐름 입력
void inputBuySideValuationCashFlow(const char* currency, double principalAmount, long revaluationDateSerial, long cashFlowDateSerial, int dcb, BuySideValuationCashFlow& cashflow, vector<DayCounter>& dayCounters) {
    
    snprintf(cashflow.currency, sizeof(cashflow.currency), "%s", currency); // 통화
    cashflow.principalAmount = principalAmount; // 금액
    cashflow.cashFlowDate = Date(cashFlowDateSerial); // 만기일
    cashflow.dcb = dcb; // DCB
    cashflow.yearFrac = getDayCounterByDCB(dcb, dayCounters).yearFraction(Date(revaluationDateSerial), Date(cashFlowDateSerial)); // 연도분수
    cashflow.domesticNetPV = 0.0; // Domestic Net PV
}

// 매도 통화 현금흐름 입력
void inputSellSideValuationCashFlow(const char* currency, double principalAmount, long revaluationDateSerial, long cashFlowDateSerial, int dcb, SellSideValuationCashFlow& cashflow, vector<DayCounter>& dayCounters) {
    
    snprintf(cashflow.currency, sizeof(cashflow.currency), "%s", currency); // 통화
    cashflow.principalAmount = principalAmount; // 금액
    cashflow.cashFlowDate = Date(cashFlowDateSerial); // 만기일
    cashflow.dcb = dcb; // DCB
	cashflow.yearFrac = getDayCounterByDCB(dcb, dayCounters).yearFraction(Date(revaluationDateSerial), Date(cashFlowDateSerial)); // 연도분수
}

// 커브 데이터 입력 및 초기화
void inputCurveData(int buyCurveDataSize, const double* buyYearFrac, const double* buyMarketData, int sellCurveDataSize, const double* sellYearFrac, const double* sellMarketData, vector<Curve>& curves) {
    
    curves.clear();  // 커브 데이터 초기화
    curves.reserve(buyCurveDataSize + sellCurveDataSize); // capacity 확보

    for (int i = 0; i < buyCurveDataSize; ++i) {
        Curve curve{};

		curve.sideFlag = 0; // 0: Buy Side, 1: Sell Side
   		curve.yearFrac = buyYearFrac[i]; // 연도분수
        curve.marketData = buyMarketData[i]; // 마켓 데이터

        curves.push_back(curve);
    }

    for (int i = 0; i < sellCurveDataSize; ++i) {
        Curve curve{};

		curve.sideFlag = 1; // 0: Buy Side, 1: Sell Side
		curve.yearFrac = sellYearFrac[i]; // 연도분수
        curve.marketData = sellMarketData[i]; // 마켓 데이터

        curves.push_back(curve);
    }
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

// Net Present Value 산출 프로세스
double processNetPV(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, vector<Curve>& curves) {
    
    // 1. 부트스트래핑으로 discount factor, zero rate 산출
    bootstrapZeroCurveContinuous(curves);
    // 2. Buy Side, Sell Side PV 산출
    valuateFxForward(bSideCashFlow, sSideCashFlow, curves);
    // 3. Buy Side Domestic Net PV 산출
    bSideCashFlow.domesticNetPV = calDomesticNetPV(bSideCashFlow.presentValue, sSideCashFlow.presentValue, tradeInfo.exchangeRate, bSideCashFlow.currency);

	return bSideCashFlow.domesticNetPV;
}

// Fx Sensitivity 산춢 프로세스
double processFxSensitivity(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow) {
	
    return calFxSensitivity(bSideCashFlow.presentValue, sSideCashFlow.presentValue, tradeInfo.exchangeRate, bSideCashFlow.currency, bSideCashFlow.domesticNetPV);
}

// 각 GIRR Curve 별 Sensitivity 산출 프로세스
void processGirrSensitivity(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, vector<Curve>& curves, Girr& girr, double* resultNetPvFxSensitivity) {
        
	// 1. GIRR Curve에 따른 마켓 데이터 할인 적용
    setDcRate(girr, curves);
    // 2. 부트스트래핑으로 discount factor, zero rate 산출
    bootstrapZeroCurveContinuous(curves);
    // 3. Buy Side, Sell Side PV 산출
    valuateFxForward(bSideCashFlow, sSideCashFlow, curves);
    // 4. Buy Side Domestic Net PV 산출
    bSideCashFlow.domesticNetPV = calDomesticNetPV(bSideCashFlow.presentValue, sSideCashFlow.presentValue, tradeInfo.exchangeRate, bSideCashFlow.currency);
    // 5. 무위험금리 커브별 GIRR Delta (Sensitivity) 산출 
    girr.sensitivity = calGirrSensitivity(bSideCashFlow.domesticNetPV, resultNetPvFxSensitivity[0]);
}

// GIRR Sensitity 산출을 위해 각 GIRR Curve에 따른 마켓 데이터 할인 적용
void setDcRate(Girr& girr, vector<Curve>& curves) {
    
    for (auto& curve : curves) {
		// Basis curve : yearFrac 상관없이 동일한 Curve에 대해 마켓 데이터 할인 적용
        if (girr.yearFrac == 0.0 && girr.sideFlag == curve.sideFlag) {
            curve.marketData += 0.01;
        }
        // Basis curve 제외 : yearFrac이 일치하는 Curve에 대해 마켓 데이터 할인 적용
        else if (girr.yearFrac == curve.yearFrac && girr.sideFlag == curve.sideFlag) {
            curve.marketData += 0.01;
        }
    }
}

// GIRR Delta Curve 데이터 생성
void setGirrDeltaCurve(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, vector<Girr>& girrs) {
    
    girrs.clear(); // GIRR Delta Sensirivity 초기화
    girrs.reserve(22); // capacity 할당

    const array<double, 11> yearFracs = { 0.0, 0.25, 0.5, 1, 2, 3, 5, 10, 15, 20, 30 };

    auto processCurve = [&](const char* currency, const int sideFlag) {
        for (double yearFrac : yearFracs) {
            Girr girr{};
            girr.sideFlag = sideFlag; // 0: Buy Side, 1: Sell Side
            
            if (strcmp(currency, "USD") != 0 && yearFrac == 0.0) { // Basis Curve 추가 (USD 아닌 경우 == Tenor 값은 0) 
                girr.yearFrac = yearFrac; // 연도 분수
            }
			else if (yearFrac == 0.0) { // Tenor 값이 0인 경우 (Basis Curve 제외)
				continue; // Skip
			}
			else {
				girr.yearFrac = yearFrac; // 연도 분수
			}
            girrs.push_back(girr);
        }
    };

    // Buy side, Sell side의 커브별 GIRR 데이터 생성
    processCurve(bSideCashFlow.currency, 0);
    processCurve(sSideCashFlow.currency, 1);
}

void bootstrapZeroCurveContinuous(vector<Curve>& curves) {
    
    // 커브별로 그룹화
    unordered_map<int, vector<Curve*>> grouped;

    for (int i = 0; i < curves.size(); ++i) {
        grouped[(curves[i].sideFlag)].push_back(&curves[i]);
    }

    // 커브별로 부트스트래핑 수행
    for (unordered_map<int, vector<Curve*>>::iterator it = grouped.begin(); it != grouped.end(); ++it) {
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

void valuateFxForward(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, const vector<Curve>& curves) {
    
    // buy side
    try {
		// dcRate : 선형보간을 통해 구한 Buy Side의 Year Fraction에 해당하는 Zero Rate(Discount Rate)
        bSideCashFlow.dcRate = linterp(getCurveYearFrac(0, curves), getCurveZeroRate(0, curves), bSideCashFlow.yearFrac);
		// dcFactor : dcRate를 이용하여 구한 Discount Factor
        bSideCashFlow.dcFactor = 1.0 / pow(1.0 + (bSideCashFlow.dcRate / 100.0), bSideCashFlow.yearFrac);
		// presentValue : discount Factor * principal Amount
        bSideCashFlow.presentValue = bSideCashFlow.principalAmount * bSideCashFlow.dcFactor;
	}
    catch (exception& e) {
        error("[valuateFxForward] buySideValuationCashFlow Error: {}", e.what());
        bSideCashFlow.presentValue = 0.0;
    }
    // sell side
    try {
		// dcRate : 선형보간을 통해 구한 Sell Side의 Year Fraction에 해당하는 Zero Rate(Discount Rate)
        sSideCashFlow.dcRate = linterp(getCurveYearFrac(1, curves), getCurveZeroRate(1, curves), sSideCashFlow.yearFrac);
		// dcFactor : dcRate를 이용하여 구한 Discount Factor
        sSideCashFlow.dcFactor = 1.0 / pow(1.0 + (sSideCashFlow.dcRate / 100.0), sSideCashFlow.yearFrac);
		// presentValue : discount Factor * principal Amount
        sSideCashFlow.presentValue = sSideCashFlow.principalAmount * sSideCashFlow.dcFactor;
	}
    catch (exception& e) {
        error("[valuateFxForward] sellSideValuationCashFlow Error: {}", e.what());
        sSideCashFlow.presentValue = 0.0;
    }
}

// 선형보간을 통한 Buy Side의 Year Fraction에 해당하는 Zero Rate(Discount Rate)를 계산하는 함수
// 커브별 yearFraction의 정렬을 전제하고 있음
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

// Domestic Net Present Value 계산
Real calDomesticNetPV(const Real buySidePV, const Real sellSidePV, const Real exchangeRate, const int isBuySideDomestic) {
    
    if (isBuySideDomestic == 1) // 원화인 경우 매입 - 매도 * 환율
        return buySidePV - (sellSidePV * exchangeRate);
	else if (isBuySideDomestic == 0) // 외화인 경우 매입 * 환율 - 매도
		return (buySidePV * exchangeRate) - sellSidePV;
}

// FX Sensitivity 계산
Real calFxSensitivity(const Real buySidePV, const Real sellSidePV, const Real exchangeRate, const char* buySideCurrency, const Real domesticNetPV) {
    
	// 환율에 1.1을 곱한 경우의 Domestic Net PV를 계산
    return (calDomesticNetPV(buySidePV, sellSidePV, exchangeRate * 1.01, buySideCurrency) - domesticNetPV) * 100;
}

// GIRR Delta Sensitivity 계산
Real calGirrSensitivity(const Real domesticNetPV, const Real originalNetPV) {
	
    // 커브 마켓데이터 1bp * 1bp의 변화값에 따른 NET PV 차이
    return (domesticNetPV - originalNetPV) / 0.0001;
}

// dcb 코드에 의한 QuantLib DayCounter 객체를 리턴
DayCounter getDayCounterByDCB(int dcb, vector<DayCounter>& dayCounters) {
    
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
        warn("[getDayCounterByDCB] Unknown dcb: default dayCounter applied.");
        return dayCounters[5];
    }
}

// 특정 Side에 있는 Curve의 Year Fraction들을 반환
vector<Real> getCurveYearFrac(const int sideFlag, const vector<Curve>& curves) {
    vector<Real> resultRange;
    for (const auto& curve : curves) {
        if (curve.sideFlag == sideFlag)
            resultRange.push_back(curve.yearFrac);
    }
    return resultRange;
}

// 특정 Curve ID와 일치하는 Zero Rate들을 반환
vector<Real> getCurveZeroRate(const int sideFlag, const vector<Curve>& curves) {
    vector<Real> resultRange;
    for (const auto& curve : curves) {
        if (curve.sideFlag == sideFlag)
            resultRange.push_back(curve.zeroRate);
    }
    return resultRange;
}

double roundToDecimals(double value, int n) {
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
    long maturityDate,
    long revaluationDate,
    double exchangeRate,
    int isBuySideDomestic,

    const char* buySideCurrency,
    double notionalForeign,
    int buySideDCB,
    int buyCurveDataSize,
    const double* buyCurveYearFrac,
    const double* buyMarketData,

    const char* sellSideCurrency,
    double notionalDomestic,
    int sellSideDCB,
    int sellCurveDataSize,
    const double* sellCurveYearFrac,
    const double* sellMarketData,

    int logYn
) {

    info("[Print All : Input Data]");

    info("maturityDate : {}", maturityDate);
    info("revaluationDate : {}", revaluationDate);
    info("exchangeRate : {:0.15f}", exchangeRate);
    info("");

    info("buySideCurrency : {}", buySideCurrency);
    info("notionalForeign : {:0.15f}", notionalForeign);
    info("buySideDCB : {}", buySideDCB);
    info("");

    info("buyCurveDataSize : {}", buyCurveDataSize);
    for (int i = 0; i < buyCurveDataSize; ++i) {
        info("buyCurveYearFrac[{}] : {:0.4f}", i, buyCurveYearFrac[i]);
        info("buyMarketData[{}] : {:0.15f}", i, buyMarketData[i]);
    }
    info("");

    info("sellSideCurrency : {}", sellSideCurrency);
    info("notionalDomestic : {:0.15f}", notionalDomestic);
    info("sellSideDCB : {}", sellSideDCB);
    info("");

    info("sellCurveDataSize : {}", sellCurveDataSize);
    for (int i = 0; i < sellCurveDataSize; ++i) {
        info("sellCurveYearFrac[{}] : {:0.6f}", i, sellCurveYearFrac[i]);
        info("sellMarketData[{}] : {:0.15f}", i, sellMarketData[i]);
    }
    info("----------------------------------------------");
    info("");
}

void printAllOutputData(const double* resultNetPvFxSensitivity, const double* resultGirrTenor, const double* resultGirrSensitivity) {

	info("[Print All: Output Data]");
    info("[resultnetPvFxSensitivity]");
	info("INDEX 0. Net PV (소수점 고정) : {:0.10f}", resultNetPvFxSensitivity[0]);
	info("INDEX 0. Net PV (raw Double) : {}", resultNetPvFxSensitivity[0]);
	info("INDEX 1. Sensitivity (소수점 고정) : {:0.10f}", resultNetPvFxSensitivity[1]);
	info("INDEX 1. Sensitivity (raw Double) : {}", resultNetPvFxSensitivity[1]);
	info("");
	
    info("[resultGirrTenor]");
	for (int i = 0; i < 11; ++i) {
		info("INDEX {}. Tenor : {:0.4f}", i, resultGirrTenor[i]);
	}
}

void printAllData(const TradeInformation& tradeInfo, const BuySideValuationCashFlow& bSideCashFlow, const SellSideValuationCashFlow& sSideCashFlow, const vector<Curve>& curves, const vector<Girr>& girrs) {
	
    printAllData(tradeInfo);
	printAllData(bSideCashFlow);
	printAllData(sSideCashFlow);
	printAllData(curves);
	printAllData(girrs);
 }

void printAllData(const TradeInformation& tradeInfo) {
    
    info("[Print All : Trade Information]");
    info("Maturity Date : {}", qDateToString(tradeInfo.maturityDate));
    info("Revaluation Date : {}", qDateToString(tradeInfo.revaluationDate));
    info("Exchange Rate : {:0.15f}", tradeInfo.exchangeRate);
	info("isBuySideDomestic : {}", tradeInfo.isBuySideDomestic);
    info("");
}

void printAllData(const BuySideValuationCashFlow& bSideCashFlow) {
    
    info("[Print All : Buy Side Valuation Cash Flow]");
    info("Currency : {}", bSideCashFlow.currency);
    info("Principal Amount : {:0.15f}", bSideCashFlow.principalAmount);
    info("Cash Flow Date : {}", qDateToString(bSideCashFlow.cashFlowDate));
    info("Day Count Basis : {}", bSideCashFlow.dcb);
    info("Year Fraction : {}", bSideCashFlow.yearFrac);
    info("Discount Rate : {}", bSideCashFlow.dcRate);
    info("Discount Factor : {}", bSideCashFlow.dcFactor);
    info("Present Value : {:0.15f}", bSideCashFlow.presentValue);
    info("");
}

void printAllData(const SellSideValuationCashFlow& sSideCashFlow) {
    
    info("[Print All : Sell Side Valuation Cash Flow]");
    info("Currency : {}", sSideCashFlow.currency);
    info("Principal Amount : {:0.15f}", sSideCashFlow.principalAmount);
    info("Cash Flow Date : {}", qDateToString(sSideCashFlow.cashFlowDate));
    info("Day Count Basis : {}", sSideCashFlow.dcb);
    info("Year Fraction : {}", sSideCashFlow.yearFrac);
    info("Discount Rate : {}", sSideCashFlow.dcRate);
    info("Discount Factor : {}", sSideCashFlow.dcFactor);
    info("Present Value : {:0.15f}", sSideCashFlow.presentValue);
    info("");
}

void printAllData(const std::vector<Curve>& curves) {
    
    info("[Print All: Curves]");
    for (const auto& curve : curves) {
		info("Curve Side : {}", curve.sideFlag == 0 ? "Buy Side" : "Sell Side");
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
    for (const auto& g : girrs) {
		info("GIRR Curve Side : {}", g.sideFlag == 0 ? "Buy Side" : "Sell Side");
        info("Year Fraction : {}", g.yearFrac);
        info("Sensitivity : {:0.15f}", g.sensitivity);
        info("-----------------------------");
    }
    info("");
}