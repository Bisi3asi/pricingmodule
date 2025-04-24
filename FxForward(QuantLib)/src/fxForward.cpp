#include "fxForward.h"
#include "logger.h"

#include <spdlog/spdlog.h>

using namespace QuantLib;
using namespace std;
using namespace spdlog;

extern "C" {
    void EXPORT_API pricing(
        // ===================================================================================================
        long maturityDate                           // INPUT 1.  만기일 (Maturity Date) 
        , long revaluationDate                      // INPUT 2.  평가일 (Revaluation Date)
        , double exchangeRate                       // INPUT 3.  현물환율 (DOM / FOR)  (Exchange Rate)

        , const char* buySideCurrency               // INPUT 4.  매입 통화 (Buy Side Currency)
        , double notionalForeign                    // INPUT 5.  매입 외화기준 명목금액 (NotionalF)
        , unsigned short buySideDCB                 // INPUT 6.  매입 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        , const char* buySideDcCurve                // INPUT 7.  매입 기준 할인 커브 (Buy Side Discount Curve)

        , unsigned int buyCurveDataSize             // INPUT 8.  매입 커브 데이터 사이즈
        , const double* buyCurveYearFrac            // INPUT 9.  매입 커브 만기 기간 (Buy Curve Term)  
        , const double* buyMarketData               // INPUT 10. 매입 커브 마켓 데이터 (Buy Curve Market Data)

        , const char* sellSideCurrency              // INPUT 11. 매도 통화 (sell Side Currency)
        , double notionalDomestic                   // INPUT 12. 매도 원화기준 명목금액 (NotionalD)
        , unsigned short sellSideDCB                // INPUT 13. 매도 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        , const char* sellSideDcCurve               // INPUT 14. 매도 기준 할인 커브 (Buy Side Discount Curve)

        , unsigned int sellCurveDataSize            // INPUT 15. 매도 커브 데이터 사이즈
        , const double* sellCurveYearFrac		    // INPUT 16. 매도 커브 만기 기간 (Sell Curve Term)     
        , const double* sellMarketData              // INPUT 17. 매도 커브 마켓 데이터 (Sell Curve Market Data) 

        , unsigned short calType                    // INPUT 18. 평가 타입 (1: NetPV, 2: GIRR Sensitivity)
        , unsigned short logYn                      // INPUT 19. 로그 파일 생성 여부 (0: No, 1: Yes)

        , double* resultNetPvFxSensitivity          // OUTPUT 1. 결과값 ([index 0] Net PV, [index 1] FX sensitivity)
        , double* resultGirrDelta 			        // OUTPUT 2. 결과값 ([index 0] array size, [index 1 ~ size] Girr Delta tenor, [size + 1 ~ end] Girr Delta Sensitivity)    
        // ===================================================================================================
    ) {

        /* 로거 초기화 */
        // 디버그용 메소드는 아래 for debug 메소드를 참고
        if (logYn == 1) {
            initLogger("fxForward.log"); // 생성 파일명 지정
            info("==============[fxForward Logging Started!]==============");
            printAllInputData( // Input 데이터 로깅
                maturityDate, revaluationDate, exchangeRate,
                buySideCurrency, notionalForeign, buySideDCB, buySideDcCurve,
                buyCurveDataSize, buyCurveYearFrac, buyMarketData,
                sellSideCurrency, notionalDomestic, sellSideDCB, sellSideDcCurve,
                sellCurveDataSize, sellCurveYearFrac, sellMarketData,
                calType, logYn
            );
        }

        TradeInformation tradeInfo{}; // 기본 상품 정보
        BuySideValuationCashFlow bSideCashFlow{}; // 매입 통화 현금흐름
        SellSideValuationCashFlow sSideCashFlow{}; // 매도 통화 현금흐름
        vector<Curve> curves{}; // 커브 데이터
        vector<Girr> girrs{}; // GIRR Delta 데이터
        vector<DayCounter> dayCounters{}; // Day Counter 데이터(캐시용 집합)

        /* 기본 데이터 세팅 */

        // 결과 배열 초기화
        initResult(resultNetPvFxSensitivity, 2);
        initResult(resultGirrDelta, 25);
        // Day Counter 데이터 생성
        initDayCounters(dayCounters);
        // Trade inforamtion  데이터 생성
        inputTradeInformation(maturityDate, revaluationDate, exchangeRate, tradeInfo);
        // Buy Side CashFlow  데이터 생성
        inputBuySideValuationCashFlow(buySideCurrency, notionalForeign, revaluationDate, maturityDate, buySideDCB, buySideDcCurve, bSideCashFlow, dayCounters);
        // Sell side CashFlow 데이터 생성
        inputSellSideValuationCashFlow(sellSideCurrency, notionalDomestic, revaluationDate, maturityDate, sellSideDCB, sellSideDcCurve, sSideCashFlow, dayCounters);
        // GIRR curve         데이터 생성 및 GIRR Sensitiity 리턴 개수 정의
        girrDeltaRiskFactor(bSideCashFlow, sSideCashFlow, girrs, resultGirrDelta);
        // Curve              데이터 생성 
        inputCurveData(buyCurveDataSize, buySideDcCurve, buyCurveYearFrac, buyMarketData, sellCurveDataSize, sellSideDcCurve, sellCurveYearFrac, sellMarketData, curves);

        if (calType != 1 && calType != 2) {
            error("[pricing] Unknown calType");
            return;
        }

        /* CalType 1 or 2. NetPV, FX Sensitivity 산출 */
        if (calType == 1 || calType == 2) {
            resultNetPvFxSensitivity[0] = processNetPV(tradeInfo, bSideCashFlow, sSideCashFlow, curves); // Net PV
            resultNetPvFxSensitivity[1] = processFxSensitivity(tradeInfo, bSideCashFlow, sSideCashFlow); // FX Sensitivity
        }

        /* CalType 2. GIRR 커브별 Sensitivity 산출 */
        if (calType == 2) {
            for (unsigned int i = 0; i < girrs.size(); ++i) {
                auto& girr = girrs[i];

                // GIRR Delta 산출 간 커브 데이터 초기화 (커브 MarKet Data Value 변경하며 Delta 산출)
                inputCurveData(buyCurveDataSize, buySideDcCurve, buyCurveYearFrac, buyMarketData, sellCurveDataSize, sellSideDcCurve, sellCurveYearFrac, sellMarketData, curves);

                // GIRR Delta Sensitivity 산출
                processGirrSensitivity(tradeInfo, bSideCashFlow, sSideCashFlow, curves, girr, resultGirrDelta, resultNetPvFxSensitivity);

                // GiRR Delta 결과값을 resultGirrDelta에 저장
                resultGirrDelta[i + 1] = girr.yearFrac; // GIRR Delta의 yearFrac (index 1 ~ size)
                resultGirrDelta[i + 1 + girrs.size()] = girr.sensitivity; // GIRR Delta의 Sensitivity (index size + 1 ~ end)
            }
        }
        // Output 데이터 로깅
        printAllOutputData(resultNetPvFxSensitivity, resultGirrDelta);
        info("==============[fxForward Logging Ended!]==============");
    }
}

// 결과값 초기화
void initResult(double* result, int size) {
	
    for (int i = 0; i < size; ++i) {
		result[i] = 0.0;
	}
}

// 거래정보 입력
void inputTradeInformation(long maturityDateSerial, long revaluationDateSerial, double exchangeRate, TradeInformation& tradeInformation) {
    
    tradeInformation.maturityDate = Date(maturityDateSerial);
    tradeInformation.revaluationDate = Date(revaluationDateSerial);
    tradeInformation.exchangeRate = exchangeRate;
}

// 매입 통화 현금흐름 입력
void inputBuySideValuationCashFlow(const char* currency, double principalAmount, long revaluationDateSerial, long cashFlowDateSerial, unsigned short dcb, const char* dcCurve, BuySideValuationCashFlow& cashflow, vector<DayCounter>& dayCounters) {
    
    strncpy(cashflow.currency, currency, sizeof(cashflow.currency) - 1);
    cashflow.currency[sizeof(cashflow.currency) - 1] = '\0';

    cashflow.principalAmount = principalAmount;
    cashflow.cashFlowDate = Date(cashFlowDateSerial);
    cashflow.dcb = dcb;
    cashflow.yearFrac = getDayCounterByDCB(dcb, dayCounters).yearFraction(Date(revaluationDateSerial), Date(cashFlowDateSerial));

    strncpy(cashflow.dcCurve, dcCurve, sizeof(cashflow.dcCurve) - 1);
    cashflow.dcCurve[sizeof(cashflow.dcCurve) - 1] = '\0';
    cashflow.domesticNetPV = 0.0;
}

// 매도 통화 현금흐름 입력
void inputSellSideValuationCashFlow(const char* currency, double principalAmount, long revaluationDateSerial, long cashFlowDateSerial, unsigned short dcb, const char* dcCurve, SellSideValuationCashFlow& cashflow, vector<DayCounter>& dayCounters) {
    
    strncpy(cashflow.currency, currency, sizeof(cashflow.currency) - 1);
    cashflow.currency[sizeof(cashflow.currency) - 1] = '\0';

    cashflow.principalAmount = principalAmount;
    cashflow.cashFlowDate = Date(cashFlowDateSerial);

    cashflow.dcb = dcb;
	cashflow.yearFrac = getDayCounterByDCB(dcb, dayCounters).yearFraction(Date(revaluationDateSerial), Date(cashFlowDateSerial));

    strncpy(cashflow.dcCurve, dcCurve, sizeof(cashflow.dcCurve) - 1);
    cashflow.dcCurve[sizeof(cashflow.dcCurve) - 1] = '\0';
}

// 커브 데이터 입력 및 초기화
void inputCurveData(unsigned int buyCurveDataSize, const char* buySideDcCurve, const double* buyYearFrac, const double* buyMarketData, unsigned int sellCurveDataSize, const char* sellSideDcCurve, const double* sellYearFrac, const double* sellMarketData, vector<Curve>& curves) {
    
    curves.clear();  // 커브 데이터 초기화
    curves.reserve(buyCurveDataSize + sellCurveDataSize); // capacity 확보

    for (unsigned int i = 0; i < buyCurveDataSize; ++i) {
        Curve curve{};

        strncpy(curve.curveId, buySideDcCurve, sizeof(curve.curveId) - 1);
        curve.curveId[sizeof(curve.curveId) - 1] = '\0';

		curve.yearFrac = buyYearFrac[i];
        curve.marketData = buyMarketData[i];

        curves.push_back(curve);
    }

    for (unsigned int i = 0; i < sellCurveDataSize; ++i) {
        Curve curve{};

        strncpy(curve.curveId, sellSideDcCurve, sizeof(curve.curveId) - 1);
        curve.curveId[sizeof(curve.curveId) - 1] = '\0';

		curve.yearFrac = sellYearFrac[i];
        curve.marketData = sellMarketData[i];

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
void processGirrSensitivity(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, vector<Curve>& curves, Girr& girr, double* resultGirrDelta, double* resultNetPvFxSensitivity) {
        
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

// GIRR Sensitity 산출을 위해 GIRR Curve에 따른 마켓 데이터 할인 적용
void setDcRate(Girr& girr, vector<Curve>& curves) {
    
    for (auto& curve : curves) {
        // Basis curve
        if (strlen(girr.irCurveId) >= 5 && strcmp(girr.irCurveId + strlen(girr.irCurveId) - 5, "Basis") == 0) {
            if (strncmp(girr.irCurveId, curve.curveId, 9) == 0)
                curve.marketData += 0.01;
        }
        // (Basis curve 제외) 완전 일치 && yearFrac(tenor) 일치
        else if (strcmp(girr.irCurveId, curve.curveId) == 0 && girr.yearFrac == curve.yearFrac) {
            curve.marketData += 0.01;
        }
    }
}

// Buy Side, Sell Side의 yearFrac의 올림값 기간을 초과하지 않는 GIRR Delta Curve 생성
void girrDeltaRiskFactor(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, vector<Girr>& girrs, double* resultGIRRDelta) {
    
    girrs.clear();
    const array<double, 10> yearFracs = { 0.25, 0.5, 1, 2, 3, 5, 10, 15, 20, 30 };

    auto processCurve = [&](const char* curveId, const char* currency, const double limitYearFrac) {
        for (double yearFrac : yearFracs) {
            if (yearFrac <= limitYearFrac) {
                ++resultGIRRDelta[0]; // 유효한 GIRR 커브 수를 카운트 (리턴용)
                
                Girr girr{};
                strncpy(girr.irCurveId, curveId, sizeof(girr.irCurveId));
                girr.irCurveId[sizeof(girr.irCurveId) - 1] = '\0';

                girr.yearFrac = yearFrac;
                girrs.push_back(girr);
            }
        }
    };

    // Basis Curve 추가 (USD 아닌 경우)
    auto processBasisCurve = [&](const char* curveId, const char* currency) {
        if (strcmp(currency, "USD") != 0) {
            ++resultGIRRDelta[0]; // 유효한 GIRR 커브 수를 카운트 (리턴용)
            
			Girr basis{};
            snprintf(basis.irCurveId, sizeof(basis.irCurveId), "%s-Basis", curveId);
            
            basis.yearFrac = 0.0;
            girrs.push_back(basis);
        }
    };

    // Buy side, Sell side의 커브별 GIRR 데이터 생성
    processCurve(bSideCashFlow.dcCurve, bSideCashFlow.currency, ceil(bSideCashFlow.yearFrac));
    processCurve(sSideCashFlow.dcCurve, sSideCashFlow.currency, ceil(sSideCashFlow.yearFrac));
    processBasisCurve(bSideCashFlow.dcCurve, bSideCashFlow.currency);
    processBasisCurve(sSideCashFlow.dcCurve, sSideCashFlow.currency);
}

void bootstrapZeroCurveContinuous(vector<Curve>& curves) {
    
    // 커브 ID별로 그룹화
    unordered_map<string, vector<Curve*>> grouped;

    for (unsigned int i = 0; i < curves.size(); ++i) {
        grouped[string(curves[i].curveId)].push_back(&curves[i]);
    }

    // 커브별로 부트스트래핑 수행
    for (unordered_map<string, vector<Curve*>>::iterator it = grouped.begin(); it != grouped.end(); ++it) {
        vector<Curve*>& group = it->second;

        for (unsigned int j = 0; j < group.size(); ++j) {
            Curve* curve = group[j];

            double rate = curve->marketData / 100.0;
            double yf = curve->yearFrac;

            // Discount Factor 계산 (연속 복리 기준)
            double df = exp(-rate * yf);
            if (df <= 0.0) {
                df = 1e-7; // underflow 방지 (0.0000001)
            }

            // Zero Rate 계산 (연속복리 → % 변환)
            double zeroRate = log(1.0 + rate) * 100.0;

            // 결과 저장
            curve->dcFactor = df;
            curve->zeroRate = zeroRate;
        }
    }
}

void valuateFxForward(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, const vector<Curve>& curves) {
    
    // buy side
    try {
		// dcRate : 선형보간을 통해 구한 Buy Side의 Year Fraction에 해당하는 Zero Rate(Discount Rate)
        bSideCashFlow.dcRate = linterp(getCurveYearFrac(bSideCashFlow.dcCurve, curves), getCurveZeroRate(bSideCashFlow.dcCurve, curves), bSideCashFlow.yearFrac);
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
        sSideCashFlow.dcRate = linterp(getCurveYearFrac(sSideCashFlow.dcCurve, curves), getCurveZeroRate(sSideCashFlow.dcCurve, curves), sSideCashFlow.yearFrac);
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
		throw invalid_argument("input zero rates cnt is less then 2, can't iterpolate.");
    }
    // 보간기 생성
    LinearInterpolation interpolator(yearFractions.begin(), yearFractions.end(), zeroRates.begin());

    // 외삽 허용 (VBA에서는 자동으로 외삽 허용)
    interpolator.enableExtrapolation();

    // 보간 결과 반환
    return interpolator(targetYearFraction);
}

// Domestic Net Present Value 계산
Real calDomesticNetPV(const Real buySidePV, const Real sellSidePV, const Real exchangeRate, const char* buySideCurrency) {
    
    if (strcmp(buySideCurrency, "KRW") == 0)
        // 원화인 경우 매입 - 매도 * 환율
        return buySidePV - (sellSidePV * exchangeRate);
	else
		// 외화인 경우 매입 * 환율 - 매도
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
DayCounter getDayCounterByDCB(unsigned short dcb, vector<DayCounter>& dayCounters) {
    
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

    const char* buySideCurrency,
    double notionalForeign,
    unsigned short buySideDCB,
    const char* buySideDcCurve,
    unsigned int buyCurveDataSize,
    const double* buyCurveYearFrac,
    const double* buyMarketData,

    const char* sellSideCurrency,
    double notionalDomestic,
    unsigned short sellSideDCB,
    const char* sellSideDcCurve,
    unsigned int sellCurveDataSize,
    const double* sellCurveYearFrac,
    const double* sellMarketData,

    unsigned short calType,
    unsigned short logYn
) {

    info("[Print All : Input Data]");

    info("maturityDate : {}", maturityDate);
    info("revaluationDate : {}", revaluationDate);
    info("exchangeRate : {:0.15f}", exchangeRate);
    info("");

    info("buySideCurrency : {}", buySideCurrency);
    info("notionalForeign : {:0.15f}", notionalForeign);
    info("buySideDCB : {}", buySideDCB);
    info("buySideDcCurve : {}", buySideDcCurve);
    info("");

    info("buyCurveDataSize : {}", buyCurveDataSize);
    for (unsigned int i = 0; i < buyCurveDataSize; ++i) {
        info("buyCurveYearFrac[{}] : {:0.4f}", i, buyCurveYearFrac[i]);
        info("buyMarketData[{}] : {:0.15f}", i, buyMarketData[i]);
    }
    info("");

    info("sellSideCurrency : {}", sellSideCurrency);
    info("notionalDomestic : {:0.15f}", notionalDomestic);
    info("sellSideDCB : {}", sellSideDCB);
    info("sellSideDcCurve : {}", sellSideDcCurve);
    info("");

    info("sellCurveDataSize : {}", sellCurveDataSize);
    for (unsigned int i = 0; i < sellCurveDataSize; ++i) {
        info("sellCurveYearFrac[{}] : {:0.6f}", i, sellCurveYearFrac[i]);
        info("sellMarketData[{}] : {:0.15f}", i, sellMarketData[i]);
    }
    info("");

    info("calType : {}", calType);
    info("logYn : {}", logYn);
    info("----------------------------------------------");
}

void printAllOutputData(const double* resultNetPvFxSensitivity, const double* resultGirrDelta) {

	info("[Print All: Output Data]");
	info("INDEX 0. Net PV : {:0.15f}", resultNetPvFxSensitivity[0]);
	info("INDEX 1. Sensitivity : {:0.15f}", resultNetPvFxSensitivity[1]);
	info("");
	
    info("INDEX 0. resultGirrDelta Size : {}", static_cast<int>(resultGirrDelta[0]));
	for (int i = 1; i < static_cast<int>(resultGirrDelta[0]) + 1; ++i) {
		info("INDEX {}. Girr Delta Tenor : {:0.4f}", i, resultGirrDelta[i]);
	}
    for (int i = static_cast<int>(resultGirrDelta[0]) + 1; i < static_cast<int>(resultGirrDelta[0]) * 2 + 1; ++i) {
		info("INDEX {}. Girr Delta Sensitivity : {:0.15f}", i, resultGirrDelta[i]);
    }
    for (int i = static_cast<int>(resultGirrDelta[0]) * 2 + 1; i < 25; ++i) {
		info("INDEX {}. Empty Value : ", i, resultGirrDelta[i]);
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
    
    info("[Print All: Trade Information]");
    info("Maturity Date     : {}", qDateToString(tradeInfo.maturityDate));
    info("Revaluation Date  : {}", qDateToString(tradeInfo.revaluationDate));
    info("Exchange Rate     : {:0.15f}", tradeInfo.exchangeRate);
    info("");
}

void printAllData(const BuySideValuationCashFlow& bSideCashFlow) {
    
    info("[Print All: Buy Side Valuation Cash Flow]");
    info("Currency          : {}", bSideCashFlow.currency);
    info("Principal Amount  : {:0.15f}", bSideCashFlow.principalAmount);
    info("Cash Flow Date    : {}", qDateToString(bSideCashFlow.cashFlowDate));
    info("Day Count Basis   : {}", bSideCashFlow.dcb);
    info("Discount Curve    : {}", bSideCashFlow.dcCurve);
    info("Year Fraction     : {}", bSideCashFlow.yearFrac);
    info("Discount Rate     : {}", bSideCashFlow.dcRate);
    info("Discount Factor   : {}", bSideCashFlow.dcFactor);
    info("Present Value     : {:0.15f}", bSideCashFlow.presentValue);
    info("");
}

void printAllData(const SellSideValuationCashFlow& sSideCashFlow) {
    
    info("[Print All: Sell Side Valuation Cash Flow]");
    info("Currency          : {}", sSideCashFlow.currency);
    info("Principal Amount  : {:0.15f}", sSideCashFlow.principalAmount);
    info("Cash Flow Date    : {}", qDateToString(sSideCashFlow.cashFlowDate));
    info("Day Count Basis   : {}", sSideCashFlow.dcb);
    info("Discount Curve    : {}", sSideCashFlow.dcCurve);
    info("Year Fraction     : {}", sSideCashFlow.yearFrac);
    info("Discount Rate     : {}", sSideCashFlow.dcRate);
    info("Discount Factor   : {}", sSideCashFlow.dcFactor);
    info("Present Value     : {:0.15f}", sSideCashFlow.presentValue);
    info("");
}

void printAllData(const std::vector<Curve>& curves) {
    
    info("[Print All: Curves]");
    for (const auto& curve : curves) {
        info("Curve ID       : {}", curve.curveId);
        info("Year Fraction  : {}", curve.yearFrac);
        info("Market Data    : {:0.15f}", curve.marketData);
        info("Discount Factor: {}", curve.dcFactor);
        info("Zero Rate      : {}", curve.zeroRate);
        info("----------------------------------------------");
    }
    info("");
}

void printAllData(const std::vector<Girr>& girrs) {
    
    info("[Print All: GIRR Delta Risk Factors]");
    for (const auto& g : girrs) {
        info("IR Curve       : {}", g.irCurveId);
        info("Year Fraction  : {}", g.yearFrac);
        info("Sensitivity    : {:0.15f}", g.sensitivity);
        info("-----------------------------");
    }
    info("");
}