#include "bond.h"
#include "logger.h"

#include <spdlog/spdlog.h>

// namespace
using namespace QuantLib;
using namespace std;
using namespace spdlog;

extern "C" {
    double EXPORT_API pricing(
        // ===================================================================================================
        double notional,                    // 채권 원금 명목금액
        long issueDateSerial,               // 발행일 (serial number)
        long revaluationDateSerial,         // 평가일 (serial number)
        long maturityDateSerial,            // 만기일 (serial number)
        long settlementDays,                // 결제일 offset (보통 2일)
        
        int couponCnt,                      // 쿠폰 개수
        double couponRate,                  // 쿠폰 이율
        int couponDCB,                      // 쿠폰 계산 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        const long* realStartDatesSerial,   // 각 구간 시작일
        const long* realEndDatesSerial,     // 각 구간 종료일
        const long* paymentDatesSerial,     // 지급일 배열
        
        int girrCnt,                        // GIRR 만기 수
        const double* girrRates,            // GIRR 금리
        int girrDCB,                        // GIRR 계산 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        // int girrInterpolator,               // 보간법 (Linear) (고정)
        // int girrCompounding,                // 이자 계산 방식 (Continuous) (고정)
        // int girrFrequency,                  // 이자 빈도 (Annual) (고정)
        
        double spreadOverYield,             // 채권의 종목 Credit Spread
        // int spreadOverYieldCompounding,     // Continuous (고정)
        int spreadOverYieldDCB,             // Credit Spread Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        
        int csrCnt,                         // CSR 만기 수
        const double* csrSpreads,           // CSR 스프레드 (금리 차이)

		unsigned short logYn,               // 로그 파일 생성 여부 (0: No, 1: Yes)
       
                                            // OUTPUT 1. NETPV (리턴값)
        double* resultGirrDelta,            // OUTPUT 2. GIRR 결과값 ([index 0] array size, [index 1 ~ size -1] Girr Delta tenor, [size ~ end] Girr Delta Sensitivity)    
		double* resultCsrDelta  	        // OUTPUT 3. CSR 결과값 ([index 0] array size, [index 1 ~ size -1] Csr Delta tenor, [size ~ end] Csr Delta Sensitivity)   
        // ===================================================================================================
    ) {
        std::vector<DayCounter> dayCounters{}; // Day Counter 데이터(캐시용 집합)
        initDayCounters(dayCounters);

        /* 로거 초기화 */
        // 디버그용 메소드는 아래 for debug 메소드를 참고
        if (logYn == 1) {
            initLogger("Bond.log"); // 생성 파일명 지정
            info("==============[Bond Logging Started!]==============");
            /*
            printAllInputData( // Input 데이터 로깅
                maturityDate, revaluationDate, exchangeRate,
                buySideCurrency, notionalForeign, buySideDCB, buySideDcCurve,
                buyCurveDataSize, buyCurveYearFrac, buyMarketData,
                sellSideCurrency, notionalDomestic, sellSideDCB, sellSideDcCurve,
                sellCurveDataSize, sellCurveYearFrac, sellMarketData,
                calType, logYn
            );
            */
        }

        Date revaluationDate = Date(revaluationDateSerial);
        
        // 전역 Settings에 평가일을 설정 (이후 모든 계산에 이 날짜 기준 적용)
        Settings::instance().evaluationDate() = revaluationDate;
        
        // 결제일 수 및 채권 액면가 설정
        Size settlementDaysSize = settlementDays;

        // 고정 쿠폰율 벡터 생성
        std::vector<Rate> couponRate_ = std::vector<Rate>(1, couponRate);
        
        // 쿠폰 이자 계산을 위한 일수계산 방식 설정 (Actual/Actual ISDA)
        DayCounter couponDayCounter = getDayCounterByDCB(couponDCB, dayCounters); 

        // GIRR 커브 구성용 날짜 및 금리 벡터
        std::vector<Date> girrDates_;
        std::vector<Real> girrRates_;

        // GIRR 커브의 주요 기간 설정 (시장 표준 테너)
        std::vector<Period> girrPeriod = { 
            Period(3, Months), Period(6, Months), Period(1, Years), Period(2, Years),    
            Period(3, Years), Period(5, Years), Period(10, Years), Period(15, Years),
            Period(20, Years), Period(30, Years) 
        };

        // GIRR 커브 시작점 (revaluationDate 기준) 입력
        girrDates_.emplace_back(revaluationDate);
        girrRates_.emplace_back(girrRates[0]);
        
        // 나머지 GIRR 커브 구성 요소 입력
        for (Size dateNum = 0; dateNum < girrCnt; ++dateNum) {
            girrDates_.emplace_back(revaluationDate + girrPeriod[dateNum]);
            girrRates_.emplace_back(girrRates[dateNum] + spreadOverYield);
        }
        
        // GIRR 커브 계산 사용 추가 요소
        DayCounter girrDayCounter = getDayCounterByDCB(girrDCB, dayCounters);  // DCB
        Linear girrInterpolator_ = Linear();                    // 선형 보간
        Compounding girrCompounding_ = Compounding::Continuous; // 연속 복리
		Frequency girrFrequency_ = Frequency::Annual;           // 연 단위 빈도
        
        // GIRR 커브 생성
        ext::shared_ptr<YieldTermStructure> girrTermstructure = ext::make_shared<ZeroCurve>(
            girrDates_, girrRates_, girrDayCounter, girrInterpolator_, girrCompounding_, girrFrequency_
        );

        // GIRR 커브를 RelinkableHandle로 연결
        RelinkableHandle<YieldTermStructure> girrCurve;
        girrCurve.linkTo(girrTermstructure);

        // spreadOverYield 값을 Interest Rate 객체로 래핑 (CSR 계산용)
        double tmpSpreadOverYield = spreadOverYield;
        Compounding spreadOverYieldCompounding_ = Compounding::Continuous;
        DayCounter spreadOverYieldDayCounter_ = getDayCounterByDCB(spreadOverYieldDCB, dayCounters);
        InterestRate tempRate(tmpSpreadOverYield, spreadOverYieldDayCounter_, spreadOverYieldCompounding_, Frequency::Annual);

        // CSR 커브 구성용 날짜 및 금리 벡터
        std::vector<Date> csrDates_;
        std::vector<Period> csrPeriod = { 
            Period(6, Months), Period(1, Years), Period(3, Years), Period(5, Years), Period(10, Years) 
        };

        // 시작 CSR 날짜 설정
        csrDates_.emplace_back(revaluationDate);

        // CSR 스프레드를 담을 핸들 벡터
        std::vector<Handle<Quote>> csrSpreads_;

        // 첫번째 CSR 스프레드에 spreadOverYield 적용
        double spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_, girrDayCounter.yearFraction(revaluationDate, revaluationDate));
        csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads[0]));

        // 나머지 CSR 기간별 스프레드 설정
        for (Size dateNum = 0; dateNum < csrCnt; ++dateNum) {
            csrDates_.emplace_back(revaluationDate + csrPeriod[dateNum]);
            spreadOverYield_ = tempRate.equivalentRate(
                girrCompounding_, girrFrequency_, girrDayCounter.yearFraction(revaluationDate, csrDates_.back())
            );
            csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads[dateNum]));
        }

        // GIRR + CSR 스프레드 커브 생성
        ext::shared_ptr<ZeroYieldStructure> discountingTermStructure =
            ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, csrSpreads_, csrDates_);
        
        // Discounting 커브 연결
        RelinkableHandle<YieldTermStructure> discountingCurve;
        discountingCurve.linkTo(discountingTermStructure);
        
        // Discounting 엔진 생성 (채권 가격 계산용)
        auto bondEngine = ext::make_shared<DiscountingBondEngine>(discountingCurve);
        
        // 쿠폰 일정 생성
        std::vector<Date> couponSch_;
        couponSch_.emplace_back(realStartDatesSerial[0]);
        for (Size schNum = 0; schNum < couponCnt; ++schNum) {
            couponSch_.emplace_back(realEndDatesSerial[schNum]);
        }

        // Schedule 객체 생성
        Schedule fixedBondSchedule_(couponSch_);
        
        // Fixed Rate Bond 생성
        FixedRateBond fixedRateBond(
            settlementDays,
            notional,
            fixedBondSchedule_,
            couponRate_,
            couponDayCounter,
            ModifiedFollowing,
            100.0);

        // Fixed Rate Bond 엔진 설정
        fixedRateBond.setPricingEngine(bondEngine);

        // 채권의 Net PV 계산
        Real npv = fixedRateBond.NPV();

        Real girrBump = 0.0001;
        std::vector<Real> disCountingGirr;

        // GIRR Delta 계산
        for (Size bumpNum = 1; bumpNum < girrRates_.size(); ++bumpNum) {
            // GIRR 커브의 금리를 bumping (1bp 상승)
            std::vector<Rate> bumpGirrRates = girrRates_;
            bumpGirrRates[bumpNum] += girrBump;

            // bump된 금리로 새로운 ZeroCurve 생성
            ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates,
                girrDayCounter, girrInterpolator_, girrCompounding_, girrFrequency_);
            
            // RelinkableHandle에 bump된 커브 연결
            RelinkableHandle<YieldTermStructure> bumpGirrCurve;
            bumpGirrCurve.linkTo(bumpGirrTermstructure);
            
            // CSR spread를 적용한 bump GIRR 기반 할인 커브 생성
            ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure =
                ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);
            
            // 할인 커브를 RelinkableHandle로 wrapping
            RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
            bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
            bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

            // discountingCurve로 새로운 pricing engine 생성
            auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve);
            
			// Fixed Rate Bond에 연결
            fixedRateBond.setPricingEngine(bumpBondEngine);
            
            // bump 후 NPV - 원래 NPV = 금리 민감도 (delta)
            Real tmpGirr = fixedRateBond.NPV() - npv;
            
            // delta 저장
            disCountingGirr.emplace_back(tmpGirr);
        }

        // CSR 델타 계산
        Real csrBump = 0.0001;
        std::vector<Real> discountingCsr;
        for (Size bumpNum = 1; bumpNum < csrSpreads_.size(); ++bumpNum) { 
           // bump 된 CSR spread 벡터 초기화
            std::vector<Handle<Quote>> bumpCsrSpreads_;
            
            //첫번째 spread 항목은 조건부로 bump 적용
            if (bumpNum == 1) {
                bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[0]->value() + csrBump));
            }
            else {
                bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[0]->value()));
            }

            // 나머지 spread 항목들도 bumpNum 위치에만 bump 적용
            for (Size i = 1; i < csrSpreads_.size(); ++i) {
                Real bump = (i == bumpNum) ? csrBump : 0.0;
                bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[i]->value() + bump));
            }

            // GIRR 커브에 bump된 CSR 스프레드 적용해 새로운 할인 커브 생성
            ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure =
                ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, bumpCsrSpreads_, csrDates_);
            
            // 새로운 할인 커브를 RelinkableHandle로 wrapping
            RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
            bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
            bumpDiscountingCurve->enableExtrapolation();
            
			// discountingCurve로 새로운 pricing engine 생성
            auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve);
            
            // fixedRateBond에 연결
            fixedRateBond.setPricingEngine(bumpBondEngine);
            
			// bump 후 NPV - 원래 NPV = spread 민감도 (delta)
            Real tmpCsr = fixedRateBond.NPV() - npv;
            discountingCsr.emplace_back(tmpCsr);
            std::cout << "Csr[" << bumpNum << "]:" << tmpCsr * 10000.0 << std::endl;
        }
        Real cleanPrice = fixedRateBond.cleanPrice() / 100.0 * notional;
        Real dirtyPrice = fixedRateBond.dirtyPrice() / 100.0 * notional;
        Real accruedInterest = fixedRateBond.accruedAmount() / 100.0 * notional;
        return npv;
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
