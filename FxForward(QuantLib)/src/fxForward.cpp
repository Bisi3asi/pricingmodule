#include "fxForward.h"

using namespace QuantLib;
using namespace std;
using namespace spdlog;

extern "C" void EXPORT pricing(
    // ===================================================================================================
      long maturityDate                         // INPUT 1.  만기일 (Maturity Date) 
    , long revaluationDate                      // INPUT 2.  평가일 (Revaluation Date)
    , double exchangeRate                       // INPUT 3.  현물환율 (DOM / FOR)  (Exchange Rate)

    , const char* buySideCurrency               // INPUT 4.  매입 통화 (Buy Side Currency)
    , double notionalForeign                    // INPUT 5.  매입 외화기준 명목금액 (NotionalF)
    , unsigned short buySideDCB                 // INPUT 6.  매입 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
    , const char* buySideDcCurve                // INPUT 7.  매입 기준 할인 커브 (Buy Side Discount Curve)

    , unsigned int buyCurveDataSize             // INPUT 8.  매입 커브 데이터 사이즈
    , const unsigned short* buyCurveTerm        // INPUT 9.  매입 커브 만기 기간 (Buy Curve Term)  
    , const unsigned short* buyCurveUnit        // INPUT 10. 매입 커브 만기 기간 단위 (Buy Curve Unit) [Y = 1, M = 2, W = 3, D = 4]
    , const double* buyMarketData               // INPUT 11. 매입 커브 마켓 데이터 (Buy Curve Market Data)

    , const char* sellSideCurrency              // INPUT 12. 매도 통화 (sell Side Currency)
    , double notionalDomestic                   // INPUT 13. 매도 원화기준 명목금액 (NotionalD)
    , unsigned short sellSideDCB                // INPUT 14. 매도 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
    , const char* sellSideDcCurve               // INPUT 15. 매도 기준 할인 커브 (Buy Side Discount Curve)

    , unsigned int sellCurveDataSize            // INPUT 16. 매도 커브 데이터 사이즈
    , const unsigned short* sellCurveTerm       // INPUT 17. 매도 커브 만기 기간 (Sell Curve Term)
    , const unsigned short* sellCurveUnit       // INPUT 18. 매도 커브 만기 기간 단위 (Sell Curve Unit) [Y = 1, M = 2, W = 3, D = 4]
    , const double* sellMarketData              // INPUT 19. 매도 커브 마켓 데이터 (Sell Curve Market Data) 
	, short logYn                               // INPUT 20. 로그 파일 생성 여부 (0: No, 1: Yes)
    , double* result                            // OUTPUT : 결과값 (Net PV, FX sensitivity, GIRR Sensitivity)
    // ===================================================================================================
) {
    if (logYn)



    TradeInformation tradeInfo{};
    BuySideValuationCashFlow bSideCashFlow{};
    SellSideValuationCashFlow sSideCashFlow{};
    vector<Curve> curves{};
    vector<Girr> girrs{};
    vector<DayCounter> dayCounters{};


    /* 기본 데이터 세팅 */
    // Trade inforamtion  데이터 생성
    inputTradeInformation(maturityDate, revaluationDate, exchangeRate, tradeInfo);
    // DCB Daycounter     데이터 생성
    initDayCounters(dayCounters);
    // Buy Side CashFlow  데이터 생성
    inputBuySideValuationCashFlow(buySideCurrency, notionalForeign, revaluationDate, maturityDate, buySideDCB, buySideDcCurve, bSideCashFlow, dayCounters);
    // Sell side CashFlow 데이터 생성
    inputSellSideValuationCashFlow(sellSideCurrency, notionalDomestic, revaluationDate, maturityDate, sellSideDCB, sellSideDcCurve, sSideCashFlow, dayCounters); 
    // GIRR curve         데이터 생성
    girrDeltaRiskFactor(buySideCurrency, buySideDcCurve, sellSideCurrency, sellSideDcCurve, girrs); 

	/* NetPV, FX Sensitivity, GIRR Delta 산출 */
    for (unsigned int i= 0; i < girrs.size(); ++i) {
        auto& girr = girrs[i];
        
        // 1. 커브별 데이터 초기화, 추가
        inputCurveData(buyCurveDataSize, buySideDcCurve, buyCurveTerm, buyCurveUnit, buySideDCB, buyMarketData, sellCurveDataSize, sellSideDcCurve, sellCurveTerm, sellCurveUnit, sellSideDCB, sellMarketData, curves);
        // 2. 커브별 maturity Date 생성
        curveMaturity(revaluationDate, curves);
        // 3. 커브별 yearFraction 생성
        setYearFrac(tradeInfo.revaluationDate, curves, dayCounters);
            
        if (i != 0) { // 최초 loop 시에는 커브 금리 조정 없이 NetPV 산출
            setDcRate(girr.irCurveId, girr.term, girr.unit, curves); // 4. 무위험금리 커브별로 기존 커브 금리(market data) 조정 
        }
        // 5. 커브별 부트스트래핑 활용 discount Factor, zero Rate 산출
        bootstrapZeroCurveContinuous(curves); 
        // 6. Buy Side, Sell Side PV 산출
        valuateFxForward(bSideCashFlow, sSideCashFlow, curves); 
        // 7. Buy Side Domestic Net PV 산출   
        bSideCashFlow.domesticNetPV = calNetPV(bSideCashFlow.presentValue, sSideCashFlow.presentValue, tradeInfo.exchangeRate, bSideCashFlow.currency);          
            
        if (i == 0) { // 최초 loop 시에 산출된 Domestic Net PV 가 상품 Net PV이며, 이후 변하지 않음.
            // result array[index 0] : 상품 Net PV 산출 
            result[0] = bSideCashFlow.domesticNetPV;
        }
        // result array[index 2 ~ 23] : 무위험금리 커브별 GIRR Delta (Sensitivity) 산출 
        girr.sensitivity = (bSideCashFlow.domesticNetPV - result[0]) / 0.0001;
        result[i + 2] = girr.sensitivity;
    }   
    // result array [index 1] : GIRR Delta 값 반영 FX Sensitivity 산출
    result[1] = calFxSensitivity(bSideCashFlow.presentValue, sSideCashFlow.presentValue, tradeInfo.exchangeRate, bSideCashFlow.currency, bSideCashFlow.domesticNetPV);
}

void inputTradeInformation(long maturityDateSerial, long revaluationDateSerial, double exchangeRate, TradeInformation& tradeInformation) {
    tradeInformation.maturityDate = Date(maturityDateSerial);
    tradeInformation.revaluationDate = Date(revaluationDateSerial);
    tradeInformation.exchangeRate = exchangeRate;
}

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

void inputCurveData(
      unsigned int buyCurveDataSize, const char* buySideDcCurve, const unsigned short* buyCurveTerm, const unsigned short* buyCurveUnit, unsigned short buySideDcb, const double* buyMarketData
    , unsigned int sellCurveDataSize, const char* sellSideDcCurve, const unsigned short* sellCurveTerm, const unsigned short* sellCurveUnit, unsigned short sellSideDcb, const double* sellMarketData
    , vector<Curve>& curves
){
    curves.reserve(buyCurveDataSize + sellCurveDataSize); // capacity 확보
    curves.clear();  // 커브 데이터 초기화

    for (unsigned int i = 0; i < buyCurveDataSize; ++i) {
        Curve curve{};

        strncpy(curve.curveId, buySideDcCurve, sizeof(curve.curveId) - 1);
        curve.curveId[sizeof(curve.curveId) - 1] = '\0';

        curve.term = buyCurveTerm[i];
        curve.unit = buyCurveUnit[i];
        curve.dcb = buySideDcb;
        curve.marketData = buyMarketData[i];

        curves.push_back(curve);
    }

    for (unsigned int i = 0; i < sellCurveDataSize; ++i) {
        Curve curve;

        strncpy(curve.curveId, sellSideDcCurve, sizeof(curve.curveId) - 1);
        curve.curveId[sizeof(curve.curveId) - 1] = '\0';

        curve.term = sellCurveTerm[i];
        curve.unit = sellCurveUnit[i];
        curve.dcb = sellSideDcb;
        curve.marketData = sellMarketData[i];

        curves.push_back(curve);
    }
}

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

void curveMaturity(long revaluationDateSerial, vector<Curve>& curves) {
    for (auto& curve : curves) {
        Date baseDate = Date(revaluationDateSerial);
        short term = curve.term;
        char unit = curve.unit;

        Date maturityDate;
        switch (unit) {
            case 1:
                maturityDate = baseDate + term * Years;
                break;
            case 2:
                maturityDate = baseDate + term * Months;
                break;
            case 3:
                maturityDate = baseDate + term * 7 * Days;
                break;
		    case 4:
                maturityDate = baseDate + term * Days;
                break;
            default:
                cerr << "[curveMaturity] Unknown unit: " << unit << endl;
                continue;
        }

        curve.maturityDate = maturityDate;
    }
}

void setDcRate(const char* irCurveId, const short term, const short unit, vector<Curve>& curves) {
    for (auto& curve : curves) {
        // Basis curve
        if (strlen(irCurveId) >= 5 && strcmp(irCurveId + strlen(irCurveId) - 5, "Basis") == 0) {
            if (strncmp(irCurveId, curve.curveId, 9) == 0)
                curve.marketData += 0.01;
        }
        // (Basis curve 제외) 완전 일치 && term/unit 일치
        else if (strcmp(irCurveId, curve.curveId) == 0 && term == curve.term && unit == curve.unit) {
            curve.marketData += 0.01;
        }
    }
}

void setYearFrac(Date startDate, vector<Curve>& curves, vector<DayCounter> dayCounters) {
    for (auto& curve : curves) {
        DayCounter dc = getDayCounterByDCB(curve.dcb, dayCounters);

        switch (curve.unit) {
        case 1: // Year
            curve.yearFrac = curve.term;
            break;
        case 2: // Month
            curve.yearFrac = curve.term / 12.0;
            break;
        case 3: // Week
        case 4: // Day
            curve.yearFrac = dc.yearFraction(startDate, curve.maturityDate);
            break;
        default:
            cerr << "[setDcRate:yearFrac] Unknown unit: " << curve.unit << endl;
            continue;
        }
    }
}

void girrDeltaRiskFactor(const char* buySideCurrency, const char* buySideDcCurve, const char* sellSideCurrency, const char* sellSideDcCurve, vector<Girr>& girrs) 
{
    girrs.clear();

	// {term , unit}, unit : [1 = Year, 2 = Month, 3 = Week, 4 = Day]
    const vector<pair<unsigned short, unsigned short>> termUnitPairs = {
        {3, 2}, {6, 2}, {1, 1}, {2, 1}, {3, 1}, {5, 1}, {10, 1}, {15, 1}, {20, 1}, {30, 1}
    };

    auto processCurve = [&](const char* curveId, const char* currency) {
        for (const auto&  termUnit : termUnitPairs) {
            unsigned short term = termUnit.first;
            unsigned short unit = termUnit.second;
            
            Girr girr{};
            strncpy(girr.irCurveId, curveId, sizeof(girr.irCurveId));
            girr.irCurveId[sizeof(girr.irCurveId) - 1] = '\0';

            girr.term = term;
            girr.unit = unit;

            girrs.push_back(girr);
        }
    };
    // Basis Curve 추가 (USD 아닌 경우)
    auto processBasisCurve = [&](const char* curveId, const char* currency) {
        if (strcmp(currency, "USD") != 0) {
            Girr basis{};
            snprintf(basis.irCurveId, sizeof(basis.irCurveId), "%s-Basis", curveId);
            basis.term = 0;
            basis.unit = 0;
            girrs.push_back(basis);
        }
    };

    // Buy side, Sell side의 커브별 GIRR 데이터 생성
    processCurve(buySideDcCurve, buySideCurrency);
    processCurve(sellSideDcCurve, sellSideCurrency);
    processBasisCurve(buySideDcCurve, buySideCurrency);
	processBasisCurve(sellSideDcCurve, sellSideCurrency);
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
        bSideCashFlow.dcRate = linterp(getCurveYearFrac(bSideCashFlow.dcCurve, curves), getCurveZeroRate(bSideCashFlow.dcCurve, curves), bSideCashFlow.yearFrac);
        bSideCashFlow.dcFactor = 1.0 / pow(1.0 + (bSideCashFlow.dcRate / 100.0), bSideCashFlow.yearFrac);
		bSideCashFlow.presentValue = bSideCashFlow.principalAmount * bSideCashFlow.dcFactor;
	}
    catch (exception& e) {
        cerr << "[valuateFxForward] buySideValuationCashFlow Error: " << e.what() << endl;
        bSideCashFlow.presentValue = 0.0;
    }
    // sell side
    try {
        sSideCashFlow.dcRate = linterp(getCurveYearFrac(sSideCashFlow.dcCurve, curves), getCurveZeroRate(sSideCashFlow.dcCurve, curves), sSideCashFlow.yearFrac);
        sSideCashFlow.dcFactor = 1.0 / pow(1.0 + (sSideCashFlow.dcRate / 100.0), sSideCashFlow.yearFrac);
        sSideCashFlow.presentValue = sSideCashFlow.principalAmount * sSideCashFlow.dcFactor;
	}
    catch (exception& e) {
        cerr << "[valuateFxForward] sellSideValuationCashFlow Error: " << e.what() << endl;
        sSideCashFlow.presentValue = 0.0;
    }
}

// 선형보간을 통한 Buy Side의 Year Fraction에 해당하는 Zero Rate(Discount Rate)를 계산하는 함수
// 커브별 yearFraction의 정렬을 전제하고 있음
Real linterp(const vector<Real>& yearFractions, const vector<Real>& zeroRates, Real targetYearFraction) {
    if (yearFractions.size() != zeroRates.size() || yearFractions.size() < 2) {
        throw invalid_argument("input zero rates cnt is less then 2, can't iterpolate.");
    }
    // 보간기 생성
    LinearInterpolation interpolator(yearFractions.begin(), yearFractions.end(), zeroRates.begin());

    // 외삽 허용 (VBA에서는 자동으로 외삽 허용)
    interpolator.enableExtrapolation();

    // 보간 결과 반환
    return interpolator(targetYearFraction);
}

// Net Present Value 계산
Real calNetPV(const Real buySidePV, const Real sellSidePV, const Real exchangeRate, const char* buySideCurrency) {
    if (strcmp(buySideCurrency, "KRW") == 0)
        return buySidePV - (sellSidePV * exchangeRate);
	else
		return (buySidePV * exchangeRate) - sellSidePV;
}

// FX Sensitivity 계산
Real calFxSensitivity(const Real buySidePV, const Real sellSidePV, const Real exchangeRate, const char* buySideCurrency, const Real domesticNetPV) {
    return (calNetPV(buySidePV, sellSidePV, exchangeRate * 1.01, buySideCurrency) - domesticNetPV) * 100;
}

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
        cerr << "[getDayCounterByDCB] Unknown dcb: " << dcb << endl;
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
//void printAllData(
//    const TradeInformation& tradeInformation, 
//    const BuySideValuationCashFlow& buySideValuationCashFlow, 
//    const SellSideValuationCashFlow& sellSideValuationCashFlow, 
//    const vector<Curve>& curves,
//    const vector<Girr>& girrs
//) {
//    cout << "===== Trade Information =================" << endl;
//    cout << "Maturity Date     : " << tradeInformation.maturityDate << endl;
//    cout << "Revaluation Date  : " << tradeInformation.revaluationDate << endl;
//    cout << "Exchange Rate     : " << fixed << setprecision(15) << tradeInformation.exchangeRate << endl << endl;
//
//    cout << "===== Buy Side Valuation Cash Flow ======" << endl;
//    cout << "Currency          : " << buySideValuationCashFlow.currency << endl;
//    cout << "Principal Amount  : " << fixed << setprecision(15) << buySideValuationCashFlow.principalAmount << endl;
//    cout << "Cash Flow Date    : " << buySideValuationCashFlow.cashFlowDate << endl;
//    cout << "Day Count Basis   : " << buySideValuationCashFlow.dcb << endl;
//    cout << "Discount Curve    : " << buySideValuationCashFlow.dcCurve << endl;
//    cout << "Year Fraction     : " << buySideValuationCashFlow.yearFrac << endl;
//    cout << "Discount Rate     : " << buySideValuationCashFlow.dcRate << endl;
//    cout << "Discount Factor   : " << buySideValuationCashFlow.dcFactor << endl;
//    cout << "Present Value     : " << fixed << setprecision(15) << buySideValuationCashFlow.presentValue << endl;
//    cout << "Net PV            : " << fixed << setprecision(15) << buySideValuationCashFlow.domesticNetPV << endl << endl;
//
//    cout << "===== Sell Side Valuation Cash Flow =====" << endl;
//    cout << "Currency          : " << sellSideValuationCashFlow.currency << endl;
//    cout << "Principal Amount  : " << fixed << setprecision(15) << sellSideValuationCashFlow.principalAmount << endl;
//    cout << "Cash Flow Date    : " << sellSideValuationCashFlow.cashFlowDate << endl;
//    cout << "Day Count Basis   : " << sellSideValuationCashFlow.dcb << endl;
//    cout << "Discount Curve    : " << sellSideValuationCashFlow.dcCurve << endl;
//    cout << "Year Fraction     : " << sellSideValuationCashFlow.yearFrac << endl;
//    cout << "Discount Rate     : " << sellSideValuationCashFlow.dcRate << endl;
//    cout << "Discount Factor   : " << sellSideValuationCashFlow.dcFactor << endl;
//    cout << "Present Value     : " << fixed << setprecision(15) << sellSideValuationCashFlow.presentValue << endl << endl;
//
//    cout << "===== Curves ============================" << endl;
//    for (const auto& curve : curves) {
//        cout << "Curve ID          : " << curve.curveId << endl;
//        cout << "Term              : " << curve.term << endl;
//        cout << "Unit              : " << curve.unit << endl;
//        cout << "DCB               : " << curve.dcb << endl;
//        cout << "Market Data       : " << fixed << setprecision(15) << curve.marketData << endl;
//        cout << "Maturity Date     : " << curve.maturityDate << endl;
//        cout << "Year Fraction     : " << curve.yearFrac << endl;
//        cout << "Discount Factor   : " << curve.dcFactor << endl;
//        cout << "Zero Rate         : " << curve.zeroRate << endl;
//        cout << "-----------------------------" << endl;
//    }
//
//    cout << "===== GIRR Delta Risk Factors ===========" << endl;
//    for (const auto& g : girrs) {
//        cout << "IR Curve          : " << g.irCurveId << endl;
//        cout << "Term              : " << g.term << endl;
//        cout << "Unit              : " << g.unit << endl;
//        cout << "Sensitivity       : " << fixed << setprecision(15) << g.sensitivity << endl;
//        cout << "-----------------------------" << endl;
//    }
// }
