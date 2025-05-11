#include "bond.h"
#include "logger.h"

#include <spdlog/spdlog.h>

using namespace QuantLib;
using namespace std;
using namespace spdlog;

extern "C" {
    double EXPORT_API pricing(
        // ===================================================================================================
        long notional                               // INPUT 1.  명목금액
        , long issueDateSerial                      // INPUT 1.  채권 발행일 (serial) 
        , long maturityDateSerial                   // INPUT 2.  채권 만기일 (serial)
        , long revaluationDateSerial                // INPUT 3.  채권 평가기준일 (serial)
        , double couponRate                         // INPUT 4.  쿠폰 금리
        , int couponFrequencyMonth                  // INPUT 5.  쿠폰 지급 주기 (month)
        , int couponDcb                             // INPUT 6.  쿠폰 지급 Day Count Basis  [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        , int businessDayConvention                 // INPUT 7.  영업일 계산방법            [Following = 0, Modified Following = 1, Preceding = 2, Modified Preceding = 3]
        , int periodEndDateConvention               // INPUT 8.  기간 월말 계산방식         [Adjusted = 0, UnAdjusted = 1]
        , int accuralDcb                            // INPUT 9.  이자 누적 일수 계산 방식   [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        , const char* dcCurveId                     // INPUT 10. 할인 커브 ID
        , int dcCurveDataSize                       // INPUT 11. 할인 커브 데이터 개수  
        , const double* dcCurveYearFrac             // INPUT 12. 할인 커브 연도분수
        , const double* dcCurveMarketData           // INPUT 13. 할인 커브 할인율

        // , const char* crsCurveId                    // INPUT 14. CRS 커브 ID 
        // , int crsCurveDataSize                      // INPUT 15. CRS 커브 데이터 개수
        // , const double* crsCurveYearFrac            // INPUT 16. CRS 커브 연도분수
        // , const double* crsCurveMarketData          // INPUT 17. CRS 커브 할인율

        , int logYn                                 // INPUT 18. 로그 파일 생성 여부 (0: No, 1: Yes)

        // OUTPUT 1. 결과값 (NET PV, 리턴값)
// , double* resultGirrDelta 			        // OUTPUT 2. 결과값 ([index 0] array size, [index 1 ~ size] Girr Delta tenor, [size + 1 ~ end] Girr Delta Sensitivity)
// , double* resultCsrDelta                 // OUTPUT 3. 결과값 ([index 0] array size, [index 1 ~ size] Csr Delta tenor, [size + 1 ~ end] Csr Delta Sensitivity)
// ===================================================================================================
) {

        /* 로거 초기화 */
        // 디버그용 메소드는 아래 for debug 메소드를 참고
        if (logYn == 1) {
            initLogger("bond.log"); // 생성 파일명 지정
            info("==============[bond Logging Started!]==============");
            //printAllInputData( // Input 데이터 로깅
            //    maturityDate, revaluationDate, exchangeRate,
            //    buySideCurrency, notionalForeign, buySideDCB, buySideDcCurve,
            //    buyCurveDataSize, buyCurveYearFrac, buyMarketData,
            //    sellSideCurrency, notionalDomestic, sellSideDCB, sellSideDcCurve,
            //    sellCurveDataSize, sellCurveYearFrac, sellMarketData,
            //    calType, logYn
            //);
        }

        TradeInformation tradeInfo{}; // 기본 상품 정보
        vector<CouponCashflow> cashflows{}; // 쿠폰 현금흐름
        vector<Curve> curves{}; // 커브 데이터
        vector<Girr> girrs{}; // GIRR Delta 데이터   
        vector<Csr> csrs{};  // CSR Delta 데이터
        vector<DayCounter> dayCounters{}; // Day Counter 데이터(캐시용 집합)
        vector<BusinessDayConvention> dayConventions{}; // Day Convention 데이터(캐시용 집합)
        vector<Frequency> frequencies{}; // Frequency 데이터(캐시용 집합)

        initDayCounters(dayCounters);
        initDayConventions(dayConventions);
        initFrequencies(frequencies);

        /* 기본 데이터 세팅 */

        // 결과값 초기화
        double netPV = 0.0;                 // Net PV
        // initResult(resultGirrDelta, 25);    // Girr Delta
        // initResult(resultCsrDelta, 25);

        // Day Counter, Business Day Convention 데이터 생성
        initDayCounters(dayCounters);
        initDayConventions(dayConventions);

        // Trade inforamtion  데이터 생성
        inputTradeInformation(notional, issueDateSerial, maturityDateSerial, revaluationDateSerial, couponRate, couponFrequencyMonth, couponDcb, accuralDcb, businessDayConvention, periodEndDateConvention, dcCurveId, /* crsCurveId, */
            tradeInfo, frequencies, dayCounters, dayConventions);
        // Coupon CashFlow 데이터 생성
        inputCouponCashFlow(tradeInfo, cashflows);

        printAllData(tradeInfo);
        printAllData(cashflows);

        // GIRR curve         데이터 생성 및 GIRR Sensitiity 리턴 개수 정의
        //girrDeltaRiskFactor(bSideCashFlow, sSideCashFlow, girrs, resultGirrDelta);
        // Curve              데이터 생성 
        //inputCurveData(buyCurveDataSize, buySideDcCurve, buyCurveYearFrac, buyMarketData, sellCurveDataSize, sellSideDcCurve, sellCurveYearFrac, sellMarketData, curves);

        /* CalType 1 or 2. NetPV, FX Sensitivity 산출 */
        //netPV = roundToDecimals(processNetPV(tradeInfo, bSideCashFlow, sSideCashFlow, curves), 10); // Net PV (소수점 열째자리까지 반올림

        /* CalType 2. GIRR 커브별 Sensitivity 산출 */
        //for (unsigned int i = 0; i < girrs.size(); ++i) {
            //auto& girr = girrs[i];

            // GIRR Delta 산출 간 커브 데이터 초기화 (커브 MarKet Data Value 변경하며 Delta 산출)
            //inputCurveData(buyCurveDataSize, buySideDcCurve, buyCurveYearFrac, buyMarketData, sellCurveDataSize, sellSideDcCurve, sellCurveYearFrac, sellMarketData, curves);

            // GIRR Delta Sensitivity 산출
            //processGirrSensitivity(tradeInfo, bSideCashFlow, sSideCashFlow, curves, girr, resultGirrDelta, resultNetPvFxSensitivity);

            // GiRR Delta 결과값을 resultGirrDelta에 저장
            //resultGirrDelta[i + 1] = girr.yearFrac; // GIRR Delta의 yearFrac (index 1 ~ size)
            //resultGirrDelta[i + 1 + girrs.size()] = roundToDecimals(girr.sensitivity, 10); // GIRR Delta의 Sensitivity (index size + 1 ~ end) (소수점 열째자리까지 반올림)
        //}
        // Output 데이터 로깅
        //printAllOutputData(resultNetPvFxSensitivity, resultGirrDelta);
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
    dayConventions.reserve(6);

    dayConventions.emplace_back(Following);           // 0: Following
    dayConventions.emplace_back(ModifiedFollowing);   // 1: Modified Following 
    dayConventions.emplace_back(Preceding);           // 2: Preceding
    dayConventions.emplace_back(ModifiedPreceding);   // 3: Modified Preceding
    dayConventions.emplace_back();                    // 4: ERR
}

// Frequency 객체 생성
void initFrequencies(vector<Frequency>& frequencies) {
    frequencies.clear();
    frequencies.reserve(13);

    frequencies.emplace_back(Annual);           // 0: Annual (연간)
    frequencies.emplace_back(Semiannual);       // 1: Semiannual (반기)
    frequencies.emplace_back(Quarterly);        // 2: Quarterly (분기)
    frequencies.emplace_back(Monthly);          // 3: Monthly (매달)
    frequencies.emplace_back(Bimonthly);        // 4: Bimonthly (2달)
    frequencies.emplace_back(Weekly);           // 5: Weekly (매주)
    frequencies.emplace_back(Biweekly);         // 6: Biweekly (2주)
    frequencies.emplace_back(Daily);            // 7: Daily (매일)
    frequencies.emplace_back(NoFrequency);      // 8: No Frequency (주기 없음)
    frequencies.emplace_back(Once);             // 9: Once  (최초 1회)
    frequencies.emplace_back(EveryFourthMonth); // 10: Every Fourth Month (4달)
    frequencies.emplace_back(EveryFourthWeek);  // 11: Every Fourth Week  (4주)
    frequencies.emplace_back(OtherFrequency);   // Error
}


// dcb 코드에 의한 QuantLib DayCounter 객체를 리턴
DayCounter getDayConterByCode(int dcb, vector<DayCounter>& dayCounters) {

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
BusinessDayConvention getDayConventionByCode(int businessDayConvention, vector<BusinessDayConvention>& dayConventions) {

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

// Freqeucny 코드(지급 주기, n달))에 의한 QuantLib Frequency 객체를 리턴
Frequency getFrequencyByMonth(int month, vector<Frequency>& frequencies) {

    // TODO : 실수형 (예 : 1주 ~ 2주 지급주기에 대한 로직 보강 필요)
    switch (month) {
    case 12: // Annual
        return frequencies[0];
    case 6: // SemiAnnual
        return frequencies[1];
    case 3: // Quaterly
        return frequencies[2];
    case 1: // Monthly
        return frequencies[3];
    //case 2: // Bimonthly
        //return frequencies[4];
    //case 5: // Weekly
        //return frequencies[5];
    //case 6: // Biweekly
        //return frequencies[6];
    //case 365: // Daily
        //return frequencies[7];
    //case 8: // NoFrequency
        //return frequencies[8];
    //case 9: // Once
        //return frequencies[9];
    //case 4: // EveryFourthMonth
        //return frequencies[10];
    //case 11: // EveryFourthWeek
        //return frequencies[11];
    default:
        warn("[getFrequencyByCode] Unknown Frequency: default Frequency applied.");
        return frequencies[12];
    }
}

// 거래정보 입력
void inputTradeInformation(
    long notional, long issueDateSerial, long maturityDateSerial, long revaluationDateSerial, 
    double couponRate, double couponFrequencyMonth, int couponDcb, int accuralDcb, int businessDayConvention, int periodEndDateConvention,
    const char* dcCurveId, /* const char* crsCurveId, */
    TradeInformation& tradeInfo, vector<Frequency> frequencies, vector<DayCounter> dayCounters, vector<BusinessDayConvention> dayConventions
) {
    tradeInfo.notional = notional; // 명목금액
    tradeInfo.issueDate = Date(issueDateSerial); // 채권 발행일
    tradeInfo.maturityDate = Date(maturityDateSerial); // 채권 만기일
    tradeInfo.revaluationDate = Date(revaluationDateSerial); // 채권 평가 기준일
    tradeInfo.couponRate = couponRate; // 쿠폰 금리
    tradeInfo.couponFrequency = getFrequencyByMonth(couponFrequencyMonth, frequencies); // 쿠폰 지급주기
    tradeInfo.couponDcb = getDayConterByCode(couponDcb, dayCounters); // 쿠폰 지급일자 Day Count Basis
    tradeInfo.accuralDcb = getDayConterByCode(accuralDcb, dayCounters); // 쿠폰 지급일자 Day Count Basis
    tradeInfo.dayConvention = getDayConventionByCode(businessDayConvention, dayConventions);
    tradeInfo.periodEndDateConvention = periodEndDateConvention;
    snprintf(tradeInfo.dcCurveId, sizeof(tradeInfo.dcCurveId), "%s", dcCurveId); // 커브
    // snprintf(tradeInfo.crsCurveId, sizeof(tradeInfo.crsCurveId), "%s", crsCurveId); // 커브
}

// 쿠폰 현금흐름 생성 및 입력
void inputCouponCashFlow(TradeInformation& tradeInfo, vector<CouponCashflow>& cashflows) {
    
    Date startDate = tradeInfo.issueDate; // 발행일부터 시작
    Date endDate;

    Calendar cal = SouthKorea(SouthKorea::Settlement); // 달력일 지정
    int row = 1;

    while (startDate < tradeInfo.maturityDate) {
        CouponCashflow cashflow = {}; // Coupon CashFlow 초기화
        
        // 지급일 계산
        endDate = cal.advance(tradeInfo.issueDate, Period(tradeInfo.couponFrequency) * row);
        Date paymentDate = cal.adjust(endDate, tradeInfo.dayConvention);

        if (tradeInfo.periodEndDateConvention == 0) { // Adjusted
            endDate = paymentDate;
        }

        cashflow.type = (endDate == tradeInfo.maturityDate) ? 1 : 0; // 1 = 원금, 0 = 쿠폰
        cashflow.startDate = startDate;
        cashflow.endDate = endDate;
        cashflow.paymentDate = paymentDate;

        // 일수 계산 (윤년 포함)
        Time lyDays = getLeapDays(startDate, endDate); // 윤년일자 산출
        Time totalDays = (endDate - startDate); 
        Time nlyDays = totalDays - lyDays; // 비윤년일자 산출

        cashflow.lyDays = static_cast<long>(lyDays);
        cashflow.nlyDays = static_cast<long>(nlyDays);

        // 연도분수 및 쿠폰 금액 계산
        Real coupon = 0.0;
        Real yearFrac = tradeInfo.couponDcb.yearFraction(startDate, endDate);

        if (tradeInfo.couponDcb.name() == "Actual/365 (Fixed)") {
            // 무조건 365일 기준
            coupon = tradeInfo.notional * tradeInfo.couponRate * (lyDays + nlyDays) / 365.0;
        }
        else if (tradeInfo.couponDcb.name().find("Actual/Actual") != std::string::npos) {
            // 윤년/평년 일수를 나눠서 처리
            coupon = tradeInfo.notional * tradeInfo.couponRate * (lyDays / 366.0 + nlyDays / 365.0);
        }
        else {
            // 기타 DCB는 yearFraction 사용
            coupon = tradeInfo.notional * tradeInfo.couponRate * yearFrac;
        }

        cashflow.couponAmount = coupon;
        cashflow.yearFrac = (paymentDate < tradeInfo.revaluationDate) ? 0.0 :
            tradeInfo.accuralDcb.yearFraction(paymentDate, tradeInfo.revaluationDate);

        // 할인율 및 현재가치 계산은 외부에서 입력 혹은 별도 처리
        // cashflow.dcRate = 0.0;
        // cashflow.dcFactor = 0.0;
        // cashflow.presentValue = 0.0;

        cashflows.push_back(cashflow);

        // 다음 구간으로 이동
        startDate = endDate;
        row++;
    }
}

// 윤년에 포함된 일자 계산
int getLeapDays(const Date& start, const Date& end) {
    
    long count = 0;
    for (Date d = start; d < end; ++d) {
        if (Date::isLeap(d.year()) && d.month() == February && d.dayOfMonth() == 29) {
            ++count;
        }
    }
    return count;
}

//// 커브 데이터 입력 및 초기화
//void inputCurveData(unsigned int buyCurveDataSize, const char* buySideDcCurve, const double* buyYearFrac, const double* buyMarketData, unsigned int sellCurveDataSize, const char* sellSideDcCurve, const double* sellYearFrac, const double* sellMarketData, vector<Curve>& curves) {
//    
//    curves.clear();  // 커브 데이터 초기화
//    curves.reserve(buyCurveDataSize + sellCurveDataSize); // capacity 확보
//
//    for (unsigned int i = 0; i < buyCurveDataSize; ++i) {
//        Curve curve{};
//
//        snprintf(curve.curveId, sizeof(curve.curveId), "%s", buySideDcCurve); // 커브 ID
//		curve.yearFrac = buyYearFrac[i]; // 연도분수
//        curve.marketData = buyMarketData[i]; // 마켓 데이터
//
//        curves.push_back(curve);
//    }
//
//    for (unsigned int i = 0; i < sellCurveDataSize; ++i) {
//        Curve curve{};
//
//        snprintf(curve.curveId, sizeof(curve.curveId), "%s", sellSideDcCurve); // 커브 ID
//		curve.yearFrac = sellYearFrac[i]; // 연도분수
//        curve.marketData = sellMarketData[i]; // 마켓 데이터
//
//        curves.push_back(curve);
//    }
//}
//
//// Net Present Value 산출 프로세스
//double processNetPV(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, vector<Curve>& curves) {
//    
//    // 1. 부트스트래핑으로 discount factor, zero rate 산출
//    bootstrapZeroCurveContinuous(curves);
//    // 2. Buy Side, Sell Side PV 산출
//    valuateFxForward(bSideCashFlow, sSideCashFlow, curves);
//    // 3. Buy Side Domestic Net PV 산출
//    bSideCashFlow.domesticNetPV = calDomesticNetPV(bSideCashFlow.presentValue, sSideCashFlow.presentValue, tradeInfo.exchangeRate, bSideCashFlow.currency);
//
//	return bSideCashFlow.domesticNetPV;
//}
//
//// Fx Sensitivity 산춢 프로세스
//double processFxSensitivity(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow) {
//	
//    return calFxSensitivity(bSideCashFlow.presentValue, sSideCashFlow.presentValue, tradeInfo.exchangeRate, bSideCashFlow.currency, bSideCashFlow.domesticNetPV);
//}
//
//// 각 GIRR Curve 별 Sensitivity 산출 프로세스
//void processGirrSensitivity(TradeInformation& tradeInfo, BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, vector<Curve>& curves, Girr& girr, double* resultGirrDelta, double* resultNetPvFxSensitivity) {
//        
//	// 1. GIRR Curve에 따른 마켓 데이터 할인 적용
//    setDcRate(girr, curves);
//    // 2. 부트스트래핑으로 discount factor, zero rate 산출
//    bootstrapZeroCurveContinuous(curves);
//    // 3. Buy Side, Sell Side PV 산출
//    valuateFxForward(bSideCashFlow, sSideCashFlow, curves);
//    // 4. Buy Side Domestic Net PV 산출
//    bSideCashFlow.domesticNetPV = calDomesticNetPV(bSideCashFlow.presentValue, sSideCashFlow.presentValue, tradeInfo.exchangeRate, bSideCashFlow.currency);
//    // 5. 무위험금리 커브별 GIRR Delta (Sensitivity) 산출 
//    girr.sensitivity = calGirrSensitivity(bSideCashFlow.domesticNetPV, resultNetPvFxSensitivity[0]);
//}
//
//// GIRR Sensitity 산출을 위해 GIRR Curve에 따른 마켓 데이터 할인 적용
//void setDcRate(Girr& girr, vector<Curve>& curves) {
//    
//    for (auto& curve : curves) {
//        // Basis curve
//        if (strlen(girr.irCurveId) >= 5 && strcmp(girr.irCurveId + strlen(girr.irCurveId) - 5, "Basis") == 0) {
//            if (strncmp(girr.irCurveId, curve.curveId, 9) == 0)
//                curve.marketData += 0.01;
//        }
//        // (Basis curve 제외) 완전 일치 && yearFrac(tenor) 일치
//        else if (strcmp(girr.irCurveId, curve.curveId) == 0 && girr.yearFrac == curve.yearFrac) {
//            curve.marketData += 0.01;
//        }
//    }
//}
//
//// Buy Side, Sell Side의 yearFrac의 올림값 기간을 초과하지 않는 GIRR Delta Curve 생성
//void girrDeltaRiskFactor(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, vector<Girr>& girrs, double* resultGIRRDelta) {
//    
//    girrs.clear(); // GIRR Delta Sensirivity 초기화
//    girrs.reserve(22); // capacity 할당
//
//    const array<double, 10> yearFracs = { 0.25, 0.5, 1, 2, 3, 5, 10, 15, 20, 30 };
//
//    auto processCurve = [&](const char* curveId, const char* currency, const double limitYearFrac) {
//        for (double yearFrac : yearFracs) {
//            if (yearFrac <= limitYearFrac) {
//                ++resultGIRRDelta[0]; // 유효한 GIRR 커브 수를 카운트 (리턴용)
//                
//                Girr girr{};
//                snprintf(girr.irCurveId, sizeof(girr.irCurveId), "%s", curveId); // 커브 ID
//                girr.yearFrac = yearFrac; // 연도 분수
//                girrs.push_back(girr);
//            }
//        }
//    };
//
//    // Basis Curve 추가 (USD 아닌 경우)
//    auto processBasisCurve = [&](const char* curveId, const char* currency) {
//        if (strcmp(currency, "USD") != 0) {
//            ++resultGIRRDelta[0]; // 유효한 GIRR 커브 수를 카운트 (리턴용)
//            
//			Girr basis{};
//            snprintf(basis.irCurveId, sizeof(basis.irCurveId), "%s-Basis", curveId); // 커브 
//            basis.yearFrac = 0.0; // 연도 분수
//            girrs.push_back(basis);
//        }
//    };
//
//    // Buy side, Sell side의 커브별 GIRR 데이터 생성
//    processCurve(bSideCashFlow.dcCurve, bSideCashFlow.currency, ceil(bSideCashFlow.yearFrac));
//    processCurve(sSideCashFlow.dcCurve, sSideCashFlow.currency, ceil(sSideCashFlow.yearFrac));
//    processBasisCurve(bSideCashFlow.dcCurve, bSideCashFlow.currency);
//    processBasisCurve(sSideCashFlow.dcCurve, sSideCashFlow.currency);
//}
//
//void bootstrapZeroCurveContinuous(vector<Curve>& curves) {
//    
//    // 커브 ID별로 그룹화
//    unordered_map<string, vector<Curve*>> grouped;
//
//    for (unsigned int i = 0; i < curves.size(); ++i) {
//        grouped[string(curves[i].curveId)].push_back(&curves[i]);
//    }
//
//    // 커브별로 부트스트래핑 수행
//    for (unordered_map<string, vector<Curve*>>::iterator it = grouped.begin(); it != grouped.end(); ++it) {
//        vector<Curve*>& group = it->second;
//
//        unsigned int n = group.size();
//        vector<Real> discountFactors(n);
//
//        for (unsigned int j = 0; j < n; ++j) {
//            Real rate = group[j]->marketData / 100.0; // 현재 커브의 할인율
//            Real yearFrac = group[j]->yearFrac; // 현재 커브의 연도 분수
//            Real sumDf = 0.0; // 누적 할인 계수
//            if (j == 0) {
//                // 첫번째 커브 기간의 할인계수(단순 할인채 가정) = 1 / (1 + r * t)
//                discountFactors[j] = 1.0 / (1.0 + rate * yearFrac);
//            }
//            else {
//                // 두번째 기간부터 쿠폰 누적 적용, 이전까지 부트스트랩된 누적 할인 계수를 이용하여 현재 커브의 할인계수를 역산
//                for (unsigned int k = 0; k < j; ++k) {
//                    Real prevYearFrac = group[k]->yearFrac; // 이전 커브의 연도 분수
//                    // 이전 쿠폰 지급 시점의 누계 할인계수 += 현재 커브의 할인율 * 이전 커브의 연도 분수 * 이전까지 부트스트랩된 누적 할인계수
//                    // 현재 커브 구간 r(j)의 할인율을 기준으로, 과거 k 구간의 할인 발생 기간에 지급된 쿠폰을 현재가치화
//                    sumDf += rate * prevYearFrac * discountFactors[k];
//                }
//                // 할인계수 = (1 - 누적 할인계수) / (1 + r * t)
//                discountFactors[j] = (1.0 - sumDf) / (1.0 + rate * yearFrac);
//            }
//
//            // 연속 복리 기준 Zero Rate 계산
//			Real zeroRate = -log(discountFactors[j]) / yearFrac; 
//            
//			// 커브의 Discount Factor, Zero Rate 업데이트
//            group[j]->dcFactor = discountFactors[j];
//            group[j]->zeroRate = zeroRate * 100.0; // %로 변환
//        }
//    }
//}
//
//void valuateFxForward(BuySideValuationCashFlow& bSideCashFlow, SellSideValuationCashFlow& sSideCashFlow, const vector<Curve>& curves) {
//    
//    // buy side
//    try {
//		// dcRate : 선형보간을 통해 구한 Buy Side의 Year Fraction에 해당하는 Zero Rate(Discount Rate)
//        bSideCashFlow.dcRate = linterp(getCurveYearFrac(bSideCashFlow.dcCurve, curves), getCurveZeroRate(bSideCashFlow.dcCurve, curves), bSideCashFlow.yearFrac);
//		// dcFactor : dcRate를 이용하여 구한 Discount Factor
//        bSideCashFlow.dcFactor = 1.0 / pow(1.0 + (bSideCashFlow.dcRate / 100.0), bSideCashFlow.yearFrac);
//		// presentValue : discount Factor * principal Amount
//        bSideCashFlow.presentValue = bSideCashFlow.principalAmount * bSideCashFlow.dcFactor;
//	}
//    catch (exception& e) {
//        error("[valuateFxForward] buySideValuationCashFlow Error: {}", e.what());
//        bSideCashFlow.presentValue = 0.0;
//    }
//    // sell side
//    try {
//		// dcRate : 선형보간을 통해 구한 Sell Side의 Year Fraction에 해당하는 Zero Rate(Discount Rate)
//        sSideCashFlow.dcRate = linterp(getCurveYearFrac(sSideCashFlow.dcCurve, curves), getCurveZeroRate(sSideCashFlow.dcCurve, curves), sSideCashFlow.yearFrac);
//		// dcFactor : dcRate를 이용하여 구한 Discount Factor
//        sSideCashFlow.dcFactor = 1.0 / pow(1.0 + (sSideCashFlow.dcRate / 100.0), sSideCashFlow.yearFrac);
//		// presentValue : discount Factor * principal Amount
//        sSideCashFlow.presentValue = sSideCashFlow.principalAmount * sSideCashFlow.dcFactor;
//	}
//    catch (exception& e) {
//        error("[valuateFxForward] sellSideValuationCashFlow Error: {}", e.what());
//        sSideCashFlow.presentValue = 0.0;
//    }
//}
//
//// 선형보간을 통한 Buy Side의 Year Fraction에 해당하는 Zero Rate(Discount Rate)를 계산하는 함수
//// 커브별 yearFraction의 정렬을 전제하고 있음
//Real linterp(const vector<Real>& yearFractions, const vector<Real>& zeroRates, Real targetYearFraction) {
//	
//    if (yearFractions.size() != zeroRates.size() || yearFractions.size() < 2) { 
//        // zeroRates와 yearFractions의 크기가 다르거나, 2개 미만일 경우
//		error("[linterp] yearFractions size: {}, zeroRates size: {}", yearFractions.size(), zeroRates.size());
//		throw invalid_argument("input zero rates cnt is less then 2, can't interpolate.");
//    }
//    // 보간기 생성
//    LinearInterpolation interpolator(yearFractions.begin(), yearFractions.end(), zeroRates.begin());
//
//    // 외삽 허용 (VBA에서는 자동으로 외삽 허용)
//    interpolator.enableExtrapolation();
//
//    // 보간 결과 반환
//    return interpolator(targetYearFraction);
//}
//
//// Domestic Net Present Value 계산
//Real calDomesticNetPV(const Real buySidePV, const Real sellSidePV, const Real exchangeRate, const char* buySideCurrency) {
//    
//    if (strcmp(buySideCurrency, "KRW") == 0)
//        // 원화인 경우 매입 - 매도 * 환율
//        return buySidePV - (sellSidePV * exchangeRate);
//	else
//		// 외화인 경우 매입 * 환율 - 매도
//		return (buySidePV * exchangeRate) - sellSidePV;
//}
//
//// FX Sensitivity 계산
//Real calFxSensitivity(const Real buySidePV, const Real sellSidePV, const Real exchangeRate, const char* buySideCurrency, const Real domesticNetPV) {
//    
//	// 환율에 1.1을 곱한 경우의 Domestic Net PV를 계산
//    return (calDomesticNetPV(buySidePV, sellSidePV, exchangeRate * 1.01, buySideCurrency) - domesticNetPV) * 100;
//}
//
//// GIRR Delta Sensitivity 계산
//Real calGirrSensitivity(const Real domesticNetPV, const Real originalNetPV) {
//	
//    // 커브 마켓데이터 1bp * 1bp의 변화값에 따른 NET PV 차이
//    return (domesticNetPV - originalNetPV) / 0.0001;
//}
//
//// 특정 Curve ID와 일치하는 Year Fraction들을 반환
//vector<Real> getCurveYearFrac(const char* curveId, const vector<Curve>& curves) {
//    vector<Real> resultRange;
//    for (const auto& curve : curves) {
//        if (strcmp(curve.curveId, curveId) == 0)
//            resultRange.push_back(curve.yearFrac);
//    }
//    return resultRange;
//}
//
//// 특정 Curve ID와 일치하는 Zero Rate들을 반환
//vector<Real> getCurveZeroRate(const char* curveId, const vector<Curve>& curves) {
//    vector<Real> resultRange;
//    for (const auto& curve : curves) {
//        if (strcmp(curve.curveId, curveId) == 0)
//            resultRange.push_back(curve.zeroRate);
//    }
//    return resultRange;
//}
//
//double roundToDecimals(double value, int n) {
//	double factor = pow(10.0, n);
//	return round(value * factor) / factor;
//}
//
/* FOR DEBUG */
string qDateToString(const Date& date) {
    
    ostringstream oss;
    oss << date;
    return oss.str();
}
//
//void printAllInputData(
//    long maturityDate,
//    long revaluationDate,
//    double exchangeRate,
//
//    const char* buySideCurrency,
//    double notionalForeign,
//    unsigned short buySideDCB,
//    const char* buySideDcCurve,
//    unsigned int buyCurveDataSize,
//    const double* buyCurveYearFrac,
//    const double* buyMarketData,
//
//    const char* sellSideCurrency,
//    double notionalDomestic,
//    unsigned short sellSideDCB,
//    const char* sellSideDcCurve,
//    unsigned int sellCurveDataSize,
//    const double* sellCurveYearFrac,
//    const double* sellMarketData,
//
//    unsigned short calType,
//    unsigned short logYn
//) {
//
//    info("[Print All : Input Data]");
//
//    info("maturityDate : {}", maturityDate);
//    info("revaluationDate : {}", revaluationDate);
//    info("exchangeRate : {:0.15f}", exchangeRate);
//    info("");
//
//    info("buySideCurrency : {}", buySideCurrency);
//    info("notionalForeign : {:0.15f}", notionalForeign);
//    info("buySideDCB : {}", buySideDCB);
//    info("buySideDcCurve : {}", buySideDcCurve);
//    info("");
//
//    info("buyCurveDataSize : {}", buyCurveDataSize);
//    for (unsigned int i = 0; i < buyCurveDataSize; ++i) {
//        info("buyCurveYearFrac[{}] : {:0.4f}", i, buyCurveYearFrac[i]);
//        info("buyMarketData[{}] : {:0.15f}", i, buyMarketData[i]);
//    }
//    info("");
//
//    info("sellSideCurrency : {}", sellSideCurrency);
//    info("notionalDomestic : {:0.15f}", notionalDomestic);
//    info("sellSideDCB : {}", sellSideDCB);
//    info("sellSideDcCurve : {}", sellSideDcCurve);
//    info("");
//
//    info("sellCurveDataSize : {}", sellCurveDataSize);
//    for (unsigned int i = 0; i < sellCurveDataSize; ++i) {
//        info("sellCurveYearFrac[{}] : {:0.6f}", i, sellCurveYearFrac[i]);
//        info("sellMarketData[{}] : {:0.15f}", i, sellMarketData[i]);
//    }
//    info("");
//
//    info("calType : {}", calType);
//    info("logYn : {}", logYn);
//    info("----------------------------------------------");
//}
//
//void printAllOutputData(const double* resultNetPvFxSensitivity, const double* resultGirrDelta) {
//
//	info("[Print All: Output Data]");
//    info("[resultnetPvFxSensitivity]");
//	info("INDEX 0. Net PV (소수점 고정) : {:0.10f}", resultNetPvFxSensitivity[0]);
//	info("INDEX 0. Net PV (raw Double) : {}", resultNetPvFxSensitivity[0]);
//	info("INDEX 1. Sensitivity (소수점 고정) : {:0.10f}", resultNetPvFxSensitivity[1]);
//	info("INDEX 1. Sensitivity (raw Double) : {}", resultNetPvFxSensitivity[1]);
//	info("");
//	
//    info("[resultGirrDelta]");
//    info("INDEX 0. resultGirrDelta Size : {}", static_cast<int>(resultGirrDelta[0]));
//	for (int i = 1; i < static_cast<int>(resultGirrDelta[0]) + 1; ++i) {
//		info("INDEX {}. Girr Delta Tenor : {:0.4f}", i, resultGirrDelta[i]);
//	}
//    for (int i = static_cast<int>(resultGirrDelta[0]) + 1; i < static_cast<int>(resultGirrDelta[0]) * 2 + 1; ++i) {
//		info("INDEX {}. Girr Delta Sensitivity : {:0.10f}", i, resultGirrDelta[i]);
//    }
//    for (int i = static_cast<int>(resultGirrDelta[0]) * 2 + 1; i < 25; ++i) {
//		info("INDEX {}. Empty Value : ", i, resultGirrDelta[i]);
//    }
//    info("");
//}
//
//void printAllData(const TradeInformation& tradeInfo, const BuySideValuationCashFlow& bSideCashFlow, const SellSideValuationCashFlow& sSideCashFlow, const vector<Curve>& curves, const vector<Girr>& girrs) {
//	
//  printAllData(tradeInfo);
//	printAllData(bSideCashFlow);
//	printAllData(sSideCashFlow);
//	printAllData(curves);
//	printAllData(girrs);
// }
//
void printAllData(const TradeInformation& tradeInfo) {
    
    info("[Print All: Trade Information]");

    info("Notional: {}", tradeInfo.notional);
    info("Issue Date: {}", qDateToString(tradeInfo.issueDate));
    info("Maturity Date: {}", qDateToString(tradeInfo.maturityDate));
    info("Revaluation Date: {}", qDateToString(tradeInfo.revaluationDate));
    info("Coupon Rate: {:0.4f}", tradeInfo.couponRate);
    info("Coupon Frequency: {}", static_cast<int>(tradeInfo.couponFrequency));
    info("Coupon DCB: {}", tradeInfo.couponDcb.name());
    info("Accrual DCB: {}", tradeInfo.accuralDcb.name());
    info("Day Convention: {}", static_cast<int>(tradeInfo.dayConvention));
    info("Period End Convention: {}", tradeInfo.periodEndDateConvention);
    info("Discount Curve ID: {}", tradeInfo.dcCurveId);
    info("CRS Curve ID: {}", tradeInfo.crsCurveId);

    info("");
}

void printAllData(const vector<CouponCashflow>& cashflows) {
    
    info("[Print All: Coupon Cashflows]");
    info("Total Cashflow Count: {}", cashflows.size());
    info("------------------------------------------------------------");

    for (size_t i = 0; i < cashflows.size(); ++i) {
        const auto& cf = cashflows[i];
        info("[{}] Coupon Cashflow", i + 1);
        info("  Type: {}", cf.type);
        info("  Start Date: {}", qDateToString(cf.startDate));
        info("  End Date: {}", qDateToString(cf.endDate));
        info("  Payment Date: {}", qDateToString(cf.paymentDate));
        info("  NLY Days: {}", cf.nlyDays);
        info("  LY Days: {}", cf.lyDays);
        info("  Coupon Amount: {:0.10f}", cf.couponAmount);
        info("  Year Fraction: {:0.10f}", cf.yearFrac);
        info("  DC Rate: {:0.10f}", cf.dcRate);
        info("  DC Factor: {:0.10f}", cf.dcFactor);
        info("  Present Value: {:0.10f}", cf.presentValue);
        info("------------------------------------------------------------");

        info("");
    }
}


void printAllData(const vector<Curve>& curves) {
    
    info("[Print All: Curves]");
    for (const auto& curve : curves) {
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
    for (const auto& g : girrs) {
        info("IR Curve : {}", g.curveId);
        info("Year Fraction : {}", g.yearFrac);
        info("Sensitivity : {:0.15f}", g.sensitivity);
        info("-----------------------------");
    }
    info("");
}