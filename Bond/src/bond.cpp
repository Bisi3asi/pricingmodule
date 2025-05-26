#include "bond.h"
#include "logger.h"

#include <spdlog/spdlog.h>

// namespace
using namespace QuantLib;
using namespace std;
using namespace spdlog;

extern "C" double EXPORT pricing(
    // ===================================================================================================
    const int evaluationDate                // INPUT 1. 평가일 (serial number)
    , const int settlementDays              // INPUT 2. 결제일 offset
    , const int issueDate                   // INPUT 3. 발행일 (serial number)
    , const int maturityDate                // INPUT 4. 만기일 (serial number)
    , const double notional                 // INPUT 5. 채권 원금
    , const double couponRate               // INPUT 6. 쿠폰 이율
    , const int couponDayCounter            // INPUT 7. DayCounter code (TODO)

    , const int numberOfCoupons             // INPUT 8. 쿠폰 개수
    , const int* paymentDates               // INPUT 9. 지급일 배열
    , const int* realStartDates             // INPUT 10. 각 구간 시작일
    , const int* realEndDates               // INPUT 11. 각 구간 종료일

    , const int numberOfGirrTenors          // INPUT 12. GIRR 만기 수
    , const int* girrTenorDays              // INPUT 13. GIRR 만기 (startDate로부터의 일수)
    , const double* girrRates               // INPUT 14. GIRR 금리

    , const int girrDayCounter              // INPUT 15. GIRR DayCountern (TODO)
    , const int girrInterpolator            // INPUT 16. 보간법 (TODO)
    , const int girrCompounding             // INPUT 17. 이자 계산 방식 (TODO)
    , const int girrFrequency               // INPUT 18. 이자 빈도 (TODO)

    , const double spreadOverYield          // INPUT 19. 채권의 종목 Credit Spread
    , const int spreadOverYieldCompounding  // INPUT 20. 이자 계산 방식 (TODO)
    , const int spreadOverYieldDayCounter   // INPUT 21. DCB (TODO)

    , const int numberOfCsrTenors           // INPUT 22. CSR 만기 수
    , const int* csrTenorDays               // INPUT 23. CSR 만기 (startDate로부터의 일수)
    , const double* csrRates                // INPUT 24. CSR 스프레드 (금리 차이)

    , const int calType			            // INPUT 25. 계산 타입 (1: Price, 2. BASEL 2 Delta, 3. BASEL 3 GIRR / CSR)    
    , const int logYn                       // INPUT 26. 로그 파일 생성 여부 (0: No, 1: Yes)

                                            // OUTPUT 1. Net PV (리턴값)
	, double* resultGirrDelta               // OUTPUT 2. GIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultCsrDelta			    // OUTPUT 3. CSR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    // ===================================================================================================
) {
    /* TODO / NOTE */
    // 1. DayCounter, Frequency 등 QuantLib Class Get 함수 정의 필요

    /* 로거 초기화 */
    if (logYn == 1) {
        initLogger("bond.log"); // 생성 파일명 지정
    }

    info("==============[bond Logging Started!]==============");
    // INPUT 데이터 로깅
    printAllInputData(evaluationDate,
        settlementDays,
        issueDate,
        maturityDate,
        notional,
        couponRate,
        couponDayCounter,
        numberOfCoupons,
        paymentDates,
        realStartDates,
        realEndDates,
        numberOfGirrTenors,
        girrTenorDays,
        girrRates,
        girrDayCounter,
        girrInterpolator,
        girrCompounding,
        girrFrequency,
        spreadOverYield,
        spreadOverYieldCompounding,
        spreadOverYieldDayCounter,
        numberOfCsrTenors,
        csrTenorDays,
        csrRates,
        calType
    );

	if (calType != 1 && calType != 3) {
		error("[bond]: Invalid calculation type. Only 1 and 3 are supported.");
		return -1; // Invalid calculation type
	}

    // 결과 데이터 초기화
	initResult(resultGirrDelta, 50);
    initResult(resultCsrDelta, 50);

    // revaluationDateSerial -> revaluationDate
    Date asOfDate_ = Date(evaluationDate);

    // 전역 Settings에 평가일을 설정 (이후 모든 계산에 이 날짜 기준 적용)
    Settings::instance().evaluationDate() = asOfDate_;
    Size settlementDays_ = settlementDays;
    
    Real notional_ = notional;
    
    // 고정 쿠폰율 벡터 생성
    std::vector<Rate> couponRate_ = std::vector<Rate>(1, couponRate);
    
    // 쿠폰 이자 계산을 위한 일수계산 방식 설정
    DayCounter couponDayCounter_ = ActualActual(ActualActual::ISDA); // TODO 변환 함수 적용 (DayCounter)
    
    // GIRR 커브 구성용 날짜 및 금리 벡터
    std::vector<Date> girrDates_;
    std::vector<Real> girrRates_;

    // GIRR 커브의 주요 기간 설정 (시장 표준 테너)
    std::vector<Period> girrPeriod = { Period(3, Months), Period(6, Months), Period(1, Years), Period(2, Years),
                                      Period(3, Years), Period(5, Years), Period(10, Years), Period(15, Years),
                                      Period(20, Years), Period(30, Years) };
    
    // GIRR 커브 시작점 (revaluationDate 기준) 입력
    girrDates_.emplace_back(asOfDate_);
    girrRates_.emplace_back(girrRates[0]);
    
    // 나머지 GIRR 커브 구성 요소 입력
    for (Size dateNum = 0; dateNum < numberOfGirrTenors; ++dateNum) {
        girrDates_.emplace_back(asOfDate_ + girrPeriod[dateNum]);
        girrRates_.emplace_back(girrRates[dateNum] + spreadOverYield);
    }
    
    // GIRR 커브 계산 사용 추가 요소
    DayCounter girrDayCounter_ = Actual365Fixed(); // DCB, TODO 변환 함수 적용
    Linear girrInterpolator_ = Linear(); // 보간 방식, TODO 변환 함수 적용 (Interpolator)
    Compounding girrCompounding_ = Compounding::Continuous; // 이자 계산 방식, TODO 변환 함수 적용 (Compounding)
	Frequency girrFrequency_ = Frequency::Annual; // 이자 지급 빈도, TODO 변환 함수 적용 (Frequency)

    // GIRR 커브 생성
    ext::shared_ptr<YieldTermStructure> girrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, girrRates_,
        girrDayCounter_, girrInterpolator_, girrCompounding_, girrFrequency_);
    
	// GIRR 커브를 RelinkableHandle에 연결
    RelinkableHandle<YieldTermStructure> girrCurve;
    girrCurve.linkTo(girrTermstructure);

    // spreadOverYiled 값을 interest Rate 객체로 래핑 (CSR 계산용)
    double tmpSpreadOverYield = spreadOverYield;
    Compounding spreadOverYieldCompounding_ = Compounding::Continuous; // 이자 계산 방식, TODO 변환 함수 적용 (Compounding)
    DayCounter spreadOverYieldDayCounter_ = Actual365Fixed();  // DCB, TODO 변환 함수 적용 (DayCounter)
    InterestRate tempRate(tmpSpreadOverYield, spreadOverYieldDayCounter_, spreadOverYieldCompounding_, Frequency::Annual);
    
	// CSR 커브 구성용 날짜 및 금리 벡터
    std::vector<Date> csrDates_;
    std::vector<Period> csrPeriod = { Period(6, Months), Period(1, Years), Period(3, Years), Period(5, Years), Period(10, Years) };
    
	// CSR 커브 시작점 (revaluationDate 기준) 입력
    csrDates_.emplace_back(asOfDate_);
    
    // CSR 스프레드를 담을 핸들 벡터
    std::vector<Handle<Quote>> csrSpreads_;

	// 첫번째 CSR 스프레드에 spreadOverYield 값을 설정
    double spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_, girrDayCounter_.yearFraction(asOfDate_, asOfDate_));
    csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrRates[0]));
    
	// 나머지 CSR 커브 기간별 스프레드 입력
    for (Size dateNum = 0; dateNum < numberOfCsrTenors; ++dateNum) {
        csrDates_.emplace_back(asOfDate_ + csrPeriod[dateNum]);
        spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_, girrDayCounter_.yearFraction(asOfDate_, csrDates_.back()));
        csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrRates[dateNum]));
    }

    // GIRR + CSR 스프레드 커브 생성
    ext::shared_ptr<ZeroYieldStructure> discountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, csrSpreads_, csrDates_);
    
    // Discounting 커브 연결
    RelinkableHandle<YieldTermStructure> discountingCurve;
    discountingCurve.linkTo(discountingTermStructure);

	// Discounting 엔진 생성 (채권 가격 계산용)
    auto bondEngine = ext::make_shared<DiscountingBondEngine>(discountingCurve);

    // Schedule 객체 생성 (지급일)
    Schedule fixedBondSchedule_;

    // Coupon Schedule 생성
	if (numberOfCoupons > 0) { // 쿠폰 스케줄이 인자로 들어오는 경우        
        info("[Coupon Schedule]: PARAMETER INPUT");

        std::vector<Date> couponSch_;
        couponSch_.emplace_back(realStartDates[0]);
        for (Size schNum = 0; schNum < numberOfCoupons; ++schNum) {
            couponSch_.emplace_back(realEndDates[schNum]);
        }
        fixedBondSchedule_ = Schedule(couponSch_);
    }

    else {  // 쿠폰 스케줄이 인자로 들어오지 않는 경우, 스케줄을 직접 생성
        info("[Coupon Schedule]: GENERATED BY MODULE");

        Date effectiveDate = Date(realStartDates[0]); // 쿠폰 첫번째 시작일로 수정
        //Date effectiveDate = Date(issueDate); // 기존 코드
        Frequency couponFrequency = Frequency::Semiannual; // Period(Tenor)형태도 가능
        Calendar couponCalendar = SouthKorea();
        BusinessDayConvention couponBDC = Following;
        DateGeneration::Rule genRule = DateGeneration::Backward;

        fixedBondSchedule_ = MakeSchedule().from(effectiveDate)
            .to(Date(maturityDate))
            .withFrequency(couponFrequency)
            .withCalendar(couponCalendar)
            .withConvention(couponBDC)
            .withRule(genRule);
    }

    /* FOR DEBUG */
    printAllData(fixedBondSchedule_);


	// FixedRateBond 객체 생성
    FixedRateBond fixedRateBond(
        settlementDays_,
        notional_,
        fixedBondSchedule_,
        couponRate_,
        couponDayCounter_,
        ModifiedFollowing, // Business Day Convention, TODO 변환 함수 적용 (DayConvention)
        100.0);

	// Fixed Rate Bond에 Discounting 엔진 연결
    fixedRateBond.setPricingEngine(bondEngine);

	// 채권 가격 Net PV 계산
    Real npv = fixedRateBond.NPV();

    // 이론가 산출의 경우 GIRR Delta 산출을 하지 않음
	if (calType == 1) {
        // OUTPUT 데이터 로깅
		printAllOutputData(npv, resultGirrDelta, resultCsrDelta);
        /* OUTPUT 1. Net PV 리턴 */
        info("==============[bond Logging Ended!]==============");
        return npv;
	}
   
    // GIRR Bump Rate 설정
    Real girrBump = 0.0001;

    // GIRR Delta 적재용 벡터 생성
    std::vector<Real> disCountingGirr;

    // GIRR Delta 계산
    for (Size bumpNum = 1; bumpNum < girrRates_.size(); ++bumpNum) {
        // GIRR 커브의 금리를 bumping (1bp 상승)
        std::vector<Rate> bumpGirrRates = girrRates_;
        bumpGirrRates[bumpNum] += girrBump;

        // bump된 금리로 새로운 ZeroCurve 생성
        ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates, girrDayCounter_, girrInterpolator_, girrCompounding_, girrFrequency_);
        
        // RelinkableHandle에 bump된 커브 연결
        RelinkableHandle<YieldTermStructure> bumpGirrCurve;
        bumpGirrCurve.linkTo(bumpGirrTermstructure);

        // CSR Spread를 적용한 bump GIRR 기반 할인 커브 생성
        ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);
        
        // 할인 커브를 RelinkableHandle로 wrapping
        RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
        bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
        bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용
        
        // discountingCurve로 새로운 pricing engine 생성
        auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve);
        
		// FixedRateBond에 bump된 pricing engine 연결
        fixedRateBond.setPricingEngine(bumpBondEngine);
        
		// 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
        Real tmpGirr = (fixedRateBond.NPV() - npv) * 10000;

		// 산출된 Girr Delta 값을 벡터에 추가   
        disCountingGirr.emplace_back(tmpGirr);
    }

    double girrTenor[10] = { 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 15.0, 20.0, 30.0 };
	Size girrDataSize = sizeof(girrTenor) / sizeof(girrTenor[0]);

    /* OUTPUT 1. GIRR Delta 결과 적재 */
    resultGirrDelta[0] = girrDataSize; // index 0 : size
    for (Size i = 0; i < girrDataSize; ++i) {
		resultGirrDelta[i + 1] = girrTenor[i]; // index 1 ~ size : girrTenorDays
	}
	for (Size i = 0; i < girrDataSize; ++i) {
		resultGirrDelta[i + 1 + girrDataSize] = disCountingGirr[i]; // index size + 1 ~ size + size : GIRR Delta
	}

    // CSR Bump Rate 설정
    Real csrBump = 0.0001;

    // CSR Delta 적재용 벡터 생성
    std::vector<Real> disCountingCsr;

	// CSR Delta 계산
    for (Size bumpNum = 1; bumpNum < csrSpreads_.size(); ++bumpNum) {
        // bump된 CSR Spread 벡터 초기화
        std::vector<Handle<Quote>> bumpCsrSpreads_;
        
        // 첫번째 spread 항목은 조건부로 bump 적용 (벤치마크 sparead curve에 대해 하나의 bump만 적용)
        if (bumpNum == 1) {
            bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[0]->value() + csrBump));
        }
        else {
            bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[0]->value()));
        }

		// 나머지 CSR Spread 항목도 bumpNum 위치에만 bump 적용
        for (Size i = 1; i < csrSpreads_.size(); ++i) {
            Real bump = (i == bumpNum) ? csrBump : 0.0;
            bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[i]->value() + bump));            
        }

		// GIRR 커브에 bump된 CSR Spread를 적용한 할인 커브 생성
        ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, bumpCsrSpreads_, csrDates_);
        
		// 새로운 할인 커브를 RelinkableHandle로 wrapping
        RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
        bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
        bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용
        
		// discountingCurve로 새로운 pricing engine 생성
        auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve);
        
		// FixedRateBond에 bump된 pricing engine 연결
        fixedRateBond.setPricingEngine(bumpBondEngine);
        
		// 기존 Net PV - bump된 Net PV 계산 (CSR Delta)
        Real tmpCsr = (fixedRateBond.NPV() - npv) * 10000;
        
		// 산출된 CSR Delta 값을 벡터에 추가
        disCountingCsr.emplace_back(tmpCsr);
    }

    double csrTenor[5] = { 0.5, 1.0, 3.0, 5.0, 10.0 };
    Size csrDataSize = sizeof(csrTenor) / sizeof(csrTenor[0]);

	/* OUTPUT 3. CSR Delta 결과 적재 */
    resultCsrDelta[0] = csrDataSize; // index 0 : size
    for (Size i = 0; i < csrDataSize; ++i) {
		resultCsrDelta[i + 1] = csrTenor[i]; // index 1 ~ size : csrTenorDays
    }
    for (Size i = 0; i < csrDataSize; ++i) {
        resultCsrDelta[i + 1 + csrDataSize] = disCountingCsr[i]; // index size + 1 ~ size + size : CSR Delta
    }

    // NPV : clean, dirty, accured Interest
    Real cleanPrice = fixedRateBond.cleanPrice() / 100.0 * notional;
    Real dirtyPrice = fixedRateBond.dirtyPrice() / 100.0 * notional;
    Real accruedInterest = fixedRateBond.accruedAmount() / 100.0 * notional;
    
    printAllOutputData(npv, resultGirrDelta, resultCsrDelta);
    info("==============[bond Logging Ended!]==============");

    /* OUTPUT 1. Net PV 리턴 */
	return npv; 
}

void initResult(double* result, const int size) {

	fill_n(result, size, 0.0);
}

/* FOR DEBUG */
string qDateToString(const Date& date) {

    ostringstream oss;
    oss << date;
    return oss.str();
}

void printAllInputData(
    const int evaluationDate,
    const int settlementDays,
    const int issueDate,
    const int maturityDate,
    const double notional,
    const double couponRate,
    const int couponDayCounter,
    const int numberOfCoupons,
    const int* paymentDates,
    const int* realStartDates,
    const int* realEndDates,
    const int numberOfGirrTenors,
    const int* girrTenorDays,
    const double* girrRates,
    const int girrDayCounter,
    const int girrInterpolator,
    const int girrCompounding,
    const int girrFrequency,
    double spreadOverYield,
    const int spreadOverYieldCompounding,
    const int spreadOverYieldDayCounter,
    const int numberOfCsrTenors,
    const int* csrTenorDays,
    const double* csrRates,
    const int calType
) {

    info("------------------------------------------------------------");
    info("[Print All - Input Data]");

    info("evaluationDate: {}", evaluationDate);
    info("settlementDays: {}", settlementDays);
    info("issueDate: {}", issueDate);
    info("maturityDate: {}", maturityDate);
    info("notional: {:0.5f}", notional);
    info("couponRate: {:0.5f}", couponRate);
    info("couponDayCounter: {}", couponDayCounter);
    info("");
    
    info("numberOfCoupons: {}", numberOfCoupons);
    logArrayLine("paymentDates", paymentDates, numberOfCoupons);
    logArrayLine("realStartDates", realStartDates, numberOfCoupons);
	logArrayLine("realEndDates", realEndDates, numberOfCoupons);
    info(""); 

    info("numberOfGirrTenors: {}", numberOfGirrTenors);
	logArrayLine("girrTenorDays", girrTenorDays, numberOfGirrTenors);
	logArrayLine("girrRates", girrRates, numberOfGirrTenors, 10);
    info(""); 

    info("girrDayCounter: {}", girrDayCounter);
    info("girrInterpolator: {}", girrInterpolator);
    info("girrCompounding: {}", girrCompounding);
    info("girrFrequency: {}", girrFrequency);
    info("");

    info("spreadOverYield: {:0.5f}", spreadOverYield);
    info("spreadOverYieldCompounding: {}", spreadOverYieldCompounding);
    info("spreadOverYieldDayCounter: {}", spreadOverYieldDayCounter);
    info("");

    info("numberOfCsrTenors: {}", numberOfCsrTenors);
	logArrayLine("csrTenorDays", csrTenorDays, numberOfCsrTenors);
	logArrayLine("csrRates", csrRates, numberOfCsrTenors, 10);
    info("");

	info("calType: {}", calType);
    info("------------------------------------------------------------");
    info("");
}

void printAllOutputData(
    const double resultNetPV,
    const double* resultGirrDelta,
    const double* resultCsrDelta
) {

    info("[Print All - Output Data]");

    // Net PV
    info("[Net PV]");
    info("Net Present Value: {:0.10f}", resultNetPV);
    info("");

    // GIRR Delta
    int girrSize = static_cast<int>(resultGirrDelta[0]);
    info("[Result GIRR Delta Size]");
    info("INDEX 0. Size: {}", girrSize);
    info("");

    info("[Result GIRR Delta Tenor]");
    for (int i = 0; i < girrSize; ++i) {
        info("INDEX {}. Tenor: {:0.2f}", i + 1, static_cast<double>(resultGirrDelta[i + 1]));
    }
    info("");

    info("[Result GIRR Delta Sensitivity]");
    for (int i = 0; i < girrSize; ++i) {
        info("INDEX {}. Sensitivity: {:0.10f}", i + 1 + girrSize, resultGirrDelta[i + 1 + girrSize]);
    }
    //for (int i = girrSize * 2 + 1; i < 50; ++i) {
    //    info("INDEX {}. Empty: {}", i, resultGirrDelta[i]);
    //}
    info("");

    // CSR Delta
    int csrSize = static_cast<int>(resultCsrDelta[0]);
    info("[Result CSR Delta Size]");
    info("INDEX 0. Size: {}", csrSize);
    info("");

    info("[Result CSR Delta Tenor]");
    for (int i = 0; i < csrSize; ++i) {
        info("INDEX {}. Tenor: {:0.2f}", i + 1, static_cast<double>(resultCsrDelta[i + 1]));
    }
    info("");

    info("[Result CSR Delta Sensitivity]");
    for (int i = 0; i < csrSize; ++i) {
        info("INDEX {}. Sensitivity: {:0.10f}", i + 1 + csrSize, resultCsrDelta[i + 1 + csrSize]);
    }
    //for (int i = csrSize * 2 + 1; i < 50; ++i) {
    //    info("{}. Empty: {}", i, resultCsrDelta[i]);
    //}
    info("------------------------------------------------------------");
    info("");
}

/* FOR DEBUG */
void printAllData(const Schedule& schedule) {

    info("[Print All - Coupon Schedule]");

    // Coupon Schedule 로깅용 벡터 생성
    std::vector<std::string> logStartDatesStr, logEndDatesStr, logPaymentDatesStr;

    for (Size i = 0; i < schedule.size() - 1; ++i) {
        Date startDate = schedule.date(i);
        Date endDate = schedule.date(i + 1);
        Date paymentDate = schedule.calendar().adjust(endDate, schedule.businessDayConvention());

        logStartDatesStr.push_back(qDateToString(startDate));
        logEndDatesStr.push_back(qDateToString(endDate));
        logPaymentDatesStr.push_back(qDateToString(paymentDate));
    }
    logArrayLine("Start Dates", logStartDatesStr);
    logArrayLine("End Dates", logEndDatesStr);
    logArrayLine("Payment Dates", logPaymentDatesStr);

    info("------------------------------------------------------------");
    info("");
}