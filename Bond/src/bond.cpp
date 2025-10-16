#include "bond.h"
#include "logger_data.hpp"
#include "logger_messages.hpp"
#include "common.hpp"

// namespace
using namespace QuantLib;
using namespace std;
using namespace logger;

extern "C" double EXPORT pricingFRB(
    // ===================================================================================================
    const int evaluationDate                // INPUT 1. 평가일 (serial number)
    , const int issueDate                   // INPUT 2. 발행일 (serial number)
    , const int maturityDate                // INPUT 3. 만기일 (serial number)
    , const double notional                 // INPUT 4. 채권 원금
    , const double couponRate               // INPUT 5. 쿠폰 이율
    , const int couponDayCounter            // INPUT 6. DayCounter code (TODO)
    , const int couponCalendar              // INPUT 7. (추가) Calendar code (TODO)
    , const int couponFrequency             // INPUT 8. (추가) Frequency code (TODO)
    , const int scheduleGenRule             // INPUT 9. 스케쥴 생성 기준(Forward/Backward) (TODO)
    , const int paymentBDC                  // INPUT 10. 지급일 휴일 적용 기준 (TODO)
    , const int paymentLag                  // INPUT 11. 지급일 지연 일수

    , const int numberOfCoupons             // INPUT 12. 쿠폰 개수
    , const int* paymentDates               // INPUT 13. 지급일 배열
    , const int* realStartDates             // INPUT 14. 각 구간 시작일
    , const int* realEndDates               // INPUT 15. 각 구간 종료일

    , const int numberOfGirrTenors          // INPUT 16. GIRR 만기 수
    , const int* girrTenorDays              // INPUT 17. GIRR 만기 (startDate로부터의 일수)
    , const double* girrRates               // INPUT 18. GIRR 금리
    , const int* girrConvention             // INPUT 19. (추가) GIRR 컨벤션 [index 0 ~ 4: GIRR DayCounter, 보간법, 이자 계산 방식, 이자 빈도] (TODO)

    , const double spreadOverYield          // INPUT 20. 채권의 종목 Credit Spread

    , const int numberOfCsrTenors           // INPUT 21. CSR 만기 수
    , const int* csrTenorDays               // INPUT 22. CSR 만기 (startDate로부터의 일수)
    , const double* csrRates                // INPUT 23. CSR 스프레드 (금리 차이)

    , const double marketPrice              // INPUT 24. (추가) 시장가격(Spread Over Yield 산출 시 사용)
    , const double girrRiskWeight           // INPUT 25. (추가) girr 리스크요소 버킷의 위험 가중치(Curvature 산출 시 사용) (TODO)
    , const double csrRiskWeight            // INPUT 26. (추가) csr 리스크요소 버킷의 위험 가중치(Curvature 산출 시 사용) (TODO)

    , const int calType			            // INPUT 27. 계산 타입 (1: Price, 2. BASEL 2 민감도, 3. BASEL 3 민감도, 9: SOY)
    , const int logYn                       // INPUT 28. 로그 파일 생성 여부 (0: No, 1: Yes)

                                            // OUTPUT 1. Net PV (리턴값)
    , double* resultBasel2                  // OUTPUT 2. Basel 2 Result [index 0 ~ 4: Delta, Gamma, Duration, Convexity, PV01]
    , double* resultGirrDelta               // OUTPUT 3. GIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultCsrDelta			    // OUTPUT 4. CSR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultGirrCvr			        // OUTPUT 5. GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultCsrCvr			        // OUTPUT 6. CSR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultCashFlow                // OUTPUT 7. CF(index 0: size, index cfNum * 7 + 1 ~ cfNum * 7 + 7: 
                                            //              startDate, endDate, notional, rate, payDate, CF, DF)
// ===================================================================================================
) {
    double result = -1.0; // 결과값 리턴 변수
    
    FINALLY({
        /* Output Result 로그 출력 */
        LOG_OUTPUT(
            FIELD_VAR(result),
            FIELD_ARR(resultBasel2, 5), 
            FIELD_ARR(resultGirrDelta, 23), FIELD_ARR(resultCsrDelta, 13),
            FIELD_ARR(resultGirrCvr, 2), FIELD_ARR(resultCsrCvr, 2), 
            FIELD_ARR(resultCashFlow, 1000)
        );
        
        /* 로그 종료 */
        LOG_END(result);
    });

    try {
        /* 로거 초기화 */
        disableConsoleLogging();
        if (logYn == 1) {
            LOG_START("bond");
        }

        /* Input Parameter 로그 출력 */
        LOG_INPUT(
            FIELD_VAR(evaluationDate), FIELD_VAR(issueDate), FIELD_VAR(maturityDate), FIELD_VAR(notional),
            FIELD_VAR(couponRate), FIELD_VAR(couponDayCounter), FIELD_VAR(couponCalendar), FIELD_VAR(couponFrequency),
            FIELD_VAR(scheduleGenRule), FIELD_VAR(paymentBDC), FIELD_VAR(paymentLag),
            FIELD_VAR(numberOfCoupons), FIELD_ARR(paymentDates, numberOfCoupons), FIELD_ARR(realStartDates, numberOfCoupons), FIELD_ARR(realEndDates, numberOfCoupons),
            FIELD_VAR(numberOfGirrTenors), FIELD_ARR(girrTenorDays, numberOfGirrTenors), FIELD_ARR(girrRates, numberOfGirrTenors), FIELD_ARR(girrConvention, 4),
            FIELD_VAR(spreadOverYield),
            FIELD_VAR(numberOfCsrTenors), FIELD_ARR(csrTenorDays, numberOfCsrTenors), FIELD_ARR(csrRates, numberOfCsrTenors),
            FIELD_VAR(marketPrice), FIELD_VAR(girrRiskWeight), FIELD_VAR(csrRiskWeight),
            FIELD_VAR(calType), FIELD_VAR(logYn)
        );

        if (calType != 1 && calType != 2 && calType != 3 && calType != 4 && calType != 9) {
            error("Invalid calculation type. Only 1, 2, 3, 4, 9 are supported.");
            return result = -1.0; // Invalid calculation type
        }

        // Input Data Check
        // Maturity Date >= evaluation Date
        if (maturityDate < evaluationDate) {
            error("Maturity Date is less than evaluation Date.");
            return result = -1.0;
        }
        // Maturity Date >= issue Date
        if (maturityDate < issueDate) {
            error("Maturity Date is less than issue Date.");
            return result = -1.0;
        }
        // Last Payment date >= evaluation Date
        if ((numberOfCoupons > 0) && (paymentDates[numberOfCoupons - 1] < evaluationDate)) {
            error("PaymentDate Date is less than evaluation Date.");
            return result = -1.0;
        }

        // 결과 데이터 초기화
        initResult(resultBasel2, 5);
        initResult(resultGirrDelta, 23);
        initResult(resultCsrDelta, 13);
        initResult(resultGirrCvr, 2);
        initResult(resultCsrCvr, 2);
        initResult(resultCashFlow, 1000);

        /* 평가 로직 시작 */
        LOG_MESSAGE_ENTER_PRICING();

        // revaluationDateSerial -> revaluationDate
        Date asOfDate_ = Date(evaluationDate);
        const Date issueDate_ = Date(issueDate);
        const Date maturityDate_ = Date(maturityDate);

        // 전역 Settings에 평가일을 설정 (이후 모든 계산에 이 날짜 기준 적용)
        Settings::instance().evaluationDate() = asOfDate_;
        Size settlementDays_ = 0;
        bool includeSettlementDateFlows_ = true;

        Real notional_ = notional;

        // 고정 쿠폰율 벡터 생성
        std::vector<Rate> couponRate_ = std::vector<Rate>(1, couponRate);

        // GIRR 커브 구성용 날짜 및 금리 벡터
        std::vector<Date> girrDates_;
        std::vector<Real> girrRates_;

        // GIRR 커브의 주요 기간 설정 (시장 표준 테너)
        std::vector<Period> girrPeriod =
            makePeriodArrayFromTenorDaysArray(girrTenorDays, numberOfGirrTenors);

        // GIRR 커브 시작점 (revaluationDate 기준) 입력
        girrDates_.emplace_back(asOfDate_);
        girrRates_.emplace_back(girrRates[0]);

        // 나머지 GIRR 커브 구성 요소 입력
        for (Size dateNum = 0; dateNum < numberOfGirrTenors; ++dateNum) {
            girrDates_.emplace_back(asOfDate_ + girrPeriod[dateNum]);
            girrRates_.emplace_back(girrRates[dateNum]);
        }

        // GIRR 커브 계산 사용 추가 요소
        DayCounter girrDayCounter_ = makeDayCounterFromInt(girrConvention[0]); // DCB
        Linear girrInterpolator_ = Linear(); // 보간 방식, TODO 변환 함수 적용 (Interpolator)
        Compounding girrCompounding_ = makeCompoundingFromInt(girrConvention[2]); // 이자 계산 방식(Compounding)
        Frequency girrFrequency_ = makeFrequencyFromInt(girrConvention[3]); // 이자 지급 빈도(Frequency)


        // GIRR 커브 생성
        ext::shared_ptr<YieldTermStructure> girrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, girrRates_,
            girrDayCounter_, girrInterpolator_, girrCompounding_, girrFrequency_);

        // GIRR 커브를 RelinkableHandle에 연결
        RelinkableHandle<YieldTermStructure> girrCurve;
        girrCurve.linkTo(girrTermstructure);
        girrCurve->enableExtrapolation();

        // spreadOverYiled 값을 interest Rate 객체로 래핑 (CSR 계산용)
        double tmpSpreadOverYield = spreadOverYield;
        Compounding spreadOverYieldCompounding_ = Compounding::Continuous; // 이자 계산 방식, Continuous만 지원
        DayCounter spreadOverYieldDayCounter_ = Actual365Fixed();  // DCB, DayCounter, Actual/365 Fixed만 지원
        InterestRate tempRate(tmpSpreadOverYield, spreadOverYieldDayCounter_, spreadOverYieldCompounding_, Frequency::Annual);

        // CSR 커브 구성용 날짜 및 금리 벡터
        std::vector<Date> csrDates_;
        std::vector<Period> csrPeriod =
            makePeriodArrayFromTenorDaysArray(csrTenorDays, numberOfCsrTenors);

        // CSR 커브 시작점 (revaluationDate 기준) 입력
        csrDates_.emplace_back(asOfDate_);

        // CSR 스프레드를 담을 핸들 벡터
        std::vector<Handle<Quote>> csrSpreads_;

        // 첫번째 CSR 스프레드에 spreadOverYield 값을 설정
        QL_REQUIRE(!csrPeriod.empty(), "csrPeriod is empty.");
        double spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_,
            girrDayCounter_.yearFraction(asOfDate_, asOfDate_ + 1));
        csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(spreadOverYield_));

        // 나머지 CSR 커브 기간별 스프레드 입력
        for (Size dateNum = 0; dateNum < numberOfCsrTenors; ++dateNum) {
            csrDates_.emplace_back(asOfDate_ + csrPeriod[dateNum]);
            spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_, girrDayCounter_.yearFraction(asOfDate_, csrDates_.back()));
            csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrRates[dateNum] + spreadOverYield_));
        }

        // GIRR + CSR 스프레드 커브 생성
        ext::shared_ptr<ZeroYieldStructure> discountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, csrSpreads_, csrDates_);

        // Discounting 커브 연결
        RelinkableHandle<YieldTermStructure> discountingCurve;
        discountingCurve.linkTo(discountingTermStructure);
        discountingCurve->enableExtrapolation(); // 외삽 허용

        // Discounting 엔진 생성 (채권 가격 계산용)
        auto bondEngine = ext::make_shared<DiscountingBondEngine>(discountingCurve, includeSettlementDateFlows_);

        // Schedule 객체 생성 (지급일)
        Schedule fixedBondSchedule_;
        BusinessDayConvention paymentBDC_ = makeBDCFromInt(paymentBDC); // 지급일 휴일 적용 기준  
        DayCounter couponDayCounter_ = makeDayCounterFromInt(couponDayCounter); // 쿠폰 이자 계산을 위한 일수계산 방식 설정
        Calendar couponCalendar_ = makeCalendarFromInt(couponCalendar);

        // Coupon Schedule 생성
        if (numberOfCoupons > 0) { // 쿠폰 스케줄이 인자로 들어오는 경우
            LOG_MESSAGE_COUPON_SCHEDULE_INPUT();

            // Schedule Data 유효성 점검(1. 발행일 & 만기일 유효성 점검. 2. 개별 Coupon Data의 유효성 점검)
            if (realStartDates[0] < issueDate   // 첫 번째 시작일이 발행일보다 이전인 경우(종료일 정보는 개별 Coupon 유효성 로직에서 점검)
                || paymentDates[0] < issueDate  // 첫 번째 지급일이 발행일보다 이전인 경우
                || paymentDates[numberOfCoupons - 1] > maturityDate  // 마지막 지급일이 만기일보다 이후인 경우
                ) {
                error("Invalid Coupon Schedule Data. Check the schedule period is in the trading period.");
                return result = -1.0;
            }

            for (int i = 0; i < numberOfCoupons; i++) {
                // 개별 Coupon의 시작일, 종료일, 지급일 유효성(종료일 > 지급일은 가능)
                if (realEndDates[i] < realStartDates[i] // 쿠폰 종료일이 시작일보다 이전인 경우
                    || paymentDates[i] < realStartDates[i]  // 지급일이 시작일보다 이전인 경우
                    ) {
                    error("Invalid Coupon Schedule Data. Check the start date, end date, and payment date.");
                    return result = -1.0;
                }

                // 스케쥴 배열의 Sorting 여부
                if (i != numberOfCoupons - 1) {
                    if (realStartDates[i + 1] < realStartDates[i]   // StartDates의 정렬 여부 점검
                        || realEndDates[i + 1] < realEndDates[i]    // EndDates의 정렬 여부 점검
                        || paymentDates[i + 1] < paymentDates[i]    // PaymentDates의 정렬 여부 점검
                        ) {
                        error("Invalid Coupon Schedule Data. Check the schedule is sorted.");
                        return result = -1.0;
                    }
                }
            }

            std::vector<Date> couponSch_;
            couponSch_.emplace_back(realStartDates[0]);
            for (Size schNum = 0; schNum < numberOfCoupons; ++schNum) {
                couponSch_.emplace_back(realEndDates[schNum]);
            }
            fixedBondSchedule_ = Schedule(couponSch_);
        }
        else {  // 쿠폰 스케줄이 인자로 들어오지 않는 경우, 스케줄을 직접 생성
            LOG_MESSAGE_COUPON_SCHEDULE_GENERATE();

            //Date effectiveDate = Date(realStartDates[0]); // 쿠폰 첫번째 시작일로 수정
            Date effectiveDate = Date(issueDate); // 기존 코드
            Calendar couponCalendar_ = makeCalendarFromInt(couponCalendar);
            Frequency couponFrequency_ = makeFrequencyFromInt(couponFrequency); // Period(Tenor)형태도 가능
            DateGeneration::Rule genRule = makeScheduleGenRuleFromInt(scheduleGenRule); // Forward, Backward 등
 
            fixedBondSchedule_ = MakeSchedule().from(effectiveDate)
                .to(maturityDate_)
                .withFrequency(couponFrequency_)
                .withCalendar(couponCalendar_)
                .withConvention(paymentBDC_)
                .withRule(genRule); // payment Lag는 Bond 스케쥴 생성 시 적용
        }

		// 평가일 기준 유효한 현금흐름만 포함하는 Schedule 생성
		Date schStartDate = fixedBondSchedule_.previousDate(asOfDate_);
		std::vector<Date> futureScheduleDates = fixedBondSchedule_.after(schStartDate).dates();
		Schedule futureFixedBondSchedule_ = Schedule(futureScheduleDates);

        /* 스케쥴 로그 */
        LOG_COUPON_SCHEDULE(fixedBondSchedule_);

        Integer paymentLag_ = paymentLag;

        Real redemptionRatio = 100.0;
        // FixedRateBond 객체 생성
        FixedRateBondCustom fixedRateBond(
            settlementDays_,
            notional_,
            futureFixedBondSchedule_,
            couponRate_,
            couponDayCounter_,
            paymentBDC_, // Business Day Convention, TODO 변환 함수 적용 (DayConvention)
            paymentLag_,
            redemptionRatio,
            issueDate_,
            couponCalendar_);

        if (calType == 9) {
            // Calc Spread Over Yield
            std::vector<Handle<Quote>> tmpCsrSpreads_;
            tmpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(0.0));
            for (Size dateNum = 0; dateNum < numberOfCsrTenors; ++dateNum) {
                tmpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrRates[dateNum]));
            }
            ext::shared_ptr<ZeroYieldStructure> tmpDiscountingTermStructure =
                ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, tmpCsrSpreads_, csrDates_);
            RelinkableHandle<YieldTermStructure> tmpDiscountingCurve;
            tmpDiscountingCurve.linkTo(tmpDiscountingTermStructure);
            tmpDiscountingCurve->enableExtrapolation();

            auto tmpBondEngine = ext::make_shared<DiscountingBondEngine>(tmpDiscountingCurve, includeSettlementDateFlows_);
            fixedRateBond.setPricingEngine(tmpBondEngine);
            // SettlementDays 관행 무시(Algo
            Real soy = CashFlows::zSpread(fixedRateBond.cashflows(), marketPrice, *tmpDiscountingCurve, Actual365Fixed(), Continuous, Annual,
                includeSettlementDateFlows_, asOfDate_, asOfDate_, 1.0e-10, 100, 0.005);  // settlementDate 산출 로직 제거(20250822, jwlee)
            //        Real soy = CashFlows::zSpread(fixedRateBond.cashflows(), marketPrice, *tmpDiscountingCurve, Actual365Fixed(), Continuous, Annual,
            //                                      false, asOfDate_, couponCalendar.advance(asOfDate_, Period(settlementDays, Days)), 1.0e-10, 100, 0.005);

            /* OUTPUT SpreadOverYield 리턴 */
            return result = soy;
        }
        // Fixed Rate Bond에 Discounting 엔진 연결
        fixedRateBond.setPricingEngine(bondEngine);

        // 채권 가격 Net PV 계산
        Real npv = fixedRateBond.NPV();

        // 이론가 산출의 경우 GIRR Delta 산출을 하지 않음
        if (calType == 1) {
            /* OUTPUT 1. Net PV 리턴 */
            return result = npv;
        }

        if (calType == 2) {
            // Delta 계산
            Real bumpSize = 0.0001; // bumpSize를 0.0001 이외의 값으로 적용 시, PV01 산출을 독립적으로 구현해줘야 함
            std::vector<Real> bumpGearings{ 1.0, -1.0 };
            std::vector<Real> bumpedNpv(bumpGearings.size(), 0.0);
            for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                std::vector<Rate> bumpGirrRates = girrRates_;
                for (Size bumpTenorNum = 0; bumpTenorNum < girrRates_.size(); ++bumpTenorNum) {
                    // GIRR 커브의 금리를 bumping (1bp 상승)
                    bumpGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * bumpSize;
                }

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates, girrDayCounter_, girrInterpolator_,
                    girrCompounding_, girrFrequency_);

                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation();

                // CSR Spread를 적용한 bump GIRR 기반 할인 커브 생성
                ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                fixedRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                bumpedNpv[bumpNo] = fixedRateBond.NPV();
            }

            QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            Real delta = (bumpedNpv[0] - npv) / bumpSize;
            Real gamma = (bumpedNpv[0] - 2.0 * npv + bumpedNpv[1]) / (bumpSize * bumpSize);

            const DayCounter& ytmDayCounter = Actual365Fixed();
            Compounding ytmCompounding = Compounded;//Continuous;
            Frequency couponFrequency_ = makeFrequencyFromInt(couponFrequency); // Period(Tenor)형태도 가능
            Frequency ytmFrequency = couponFrequency_; //Semiannual;//Annual;
            Rate ytmValue = 0.00000000000001;
            if (asOfDate_ < maturityDate_) {
                ytmValue = CashFlows::yield(fixedRateBond.cashflows(), npv, ytmDayCounter, ytmCompounding, ytmFrequency, false,
                    couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), asOfDate_,
                    1.0e-15, 100, 0.005);
            }
            InterestRate ytm = InterestRate(ytmValue, ytmDayCounter, ytmCompounding, ytmFrequency);

            // Duration 계산
            Duration::Type durationType = Duration::Macaulay;//Duration::Modified;
            Real duration = CashFlows::duration(fixedRateBond.cashflows(), ytm, durationType, false,
                couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), asOfDate_);

            Real convexity = CashFlows::convexity(fixedRateBond.cashflows(), ytm, false,
                couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), asOfDate_);

            Real PV01 = delta * bumpSize;

            resultBasel2[0] = delta;
            resultBasel2[1] = gamma;
            resultBasel2[2] = duration;
            resultBasel2[3] = convexity;
            resultBasel2[4] = PV01;

            /* OUTPUT 1. Net PV 리턴 */
            return result = npv;
        }

        if (calType == 3) {
            // GIRR Bump Rate 설정
            Real girrBump = 0.0001;

            // GIRR Delta 적재용 벡터 생성
            std::vector<Real> disCountingGirr;

            // GIRR Delta 계산
            for (Size bumpNum = 1; bumpNum < girrRates_.size(); ++bumpNum) {
                // GIRR 커브의 금리를 bumping (1bp 상승)
                std::vector<Rate> bumpGirrRates = girrRates_;
                if (bumpNum == 1) {
                    bumpGirrRates[0] += girrBump; // 0번째 tenor도 같이 bump 적용
                }
                bumpGirrRates[bumpNum] += girrBump;

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates, girrDayCounter_, girrInterpolator_, girrCompounding_, girrFrequency_);

                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation();

                // CSR Spread를 적용한 bump GIRR 기반 할인 커브 생성
                ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                fixedRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                Real tmpGirr = (fixedRateBond.NPV() - npv) * 10000;

                // 산출된 Girr Delta 값을 벡터에 추가
                disCountingGirr.emplace_back(tmpGirr);
            }

            /* OUTPUT 2. GIRR Delta 결과 적재 */
            std::vector<Real> girrTenor = { 0.0, 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 15.0, 20.0, 30.0 };
            Size girrDataSize = girrTenor.size();

            // Parallel 민감도 추가
            double tmpCcyDelta = std::accumulate(disCountingGirr.begin(), disCountingGirr.end(), 0.0);
            disCountingGirr.insert(disCountingGirr.begin(), tmpCcyDelta);
            QL_REQUIRE(girrDataSize == disCountingGirr.size(), "Girr result Size mismatch.");

            // 0인 민감도를 제외하고 적재
            processResultArray(girrTenor, disCountingGirr, girrDataSize, resultGirrDelta);

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
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                fixedRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (CSR Delta)
                Real tmpCsr = (fixedRateBond.NPV() - npv) * 10000;

                // 산출된 CSR Delta 값을 벡터에 추가
                disCountingCsr.emplace_back(tmpCsr);
            }

            /* OUTPUT 3. CSR Delta 결과 적재 */
            std::vector<Real> csrTenor = { 0.5, 1.0, 3.0, 5.0, 10.0 };
            Size csrDataSize = csrTenor.size();
            // 0인 민감도를 제외하고 적재
            processResultArray(csrTenor, disCountingCsr, csrDataSize, resultCsrDelta);

            Real totalGirr = 0;
            for (const auto& girr : disCountingGirr) {
                totalGirr += girr;
            }

            // Curvature 계산
            Real curvatureRW = girrRiskWeight; // bumpSize를 FRTB 기준서의 Girr Curvature RiskWeight로 설정
            std::vector<Real> bumpGearings{ 1.0, -1.0 };
            std::vector<Real> bumpedNpv(bumpGearings.size(), 0.0);
            for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                std::vector<Rate> bumpGirrRates = girrRates_;
                for (Size bumpTenorNum = 0; bumpTenorNum < girrRates_.size(); ++bumpTenorNum) {
                    // GIRR 커브의 금리를 bumping (RiskWeight 만큼 상승)
                    bumpGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * curvatureRW;
                }

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates, girrDayCounter_, girrInterpolator_,
                    girrCompounding_, girrFrequency_);

                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation();

                // CSR Spread를 적용한 bump GIRR 기반 할인 커브 생성
                ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                fixedRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                bumpedNpv[bumpNo] = fixedRateBond.NPV();
            }

            QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            resultGirrCvr[0] = (bumpedNpv[0] - npv);
            resultGirrCvr[1] = (bumpedNpv[1] - npv);

            Real totalCsr = 0;
            for (const auto& csr : disCountingCsr) {
                totalCsr += csr;
            }

            // Curvature 계산
            curvatureRW = csrRiskWeight; // bumpSize를 FRTB 기준서의 CSR Bucket의 Curvature RiskWeight로 설정
            for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                std::vector<Handle<Quote>> bumpCsrSpreads_;
                for (Size bumpTenorNum = 0; bumpTenorNum < csrSpreads_.size(); ++bumpTenorNum) {
                    // Csr 커브의 금리를 bumping (RiskWeight 만큼 상승)
                    bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[bumpTenorNum]->value() +
                        bumpGearings[bumpNo] * curvatureRW));
                }

                // GIRR 커브에 bump된 CSR Spread를 적용한 할인 커브 생성
                ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure =
                    ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, bumpCsrSpreads_, csrDates_);

                // 새로운 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                fixedRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                bumpedNpv[bumpNo] = fixedRateBond.NPV();
            }

            QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            resultCsrCvr[0] = (bumpedNpv[0] - npv);
            resultCsrCvr[1] = (bumpedNpv[1] - npv);

            /* OUTPUT 1. Net PV 리턴 */
            return result = npv;
        }

        if (calType == 4) {
            const Leg& bondCFs = fixedRateBond.cashflows();
            Size numberOfCoupons = bondCFs.size();
            Size numberOfFields = 7;
            Size n_startDateField = 1;
            Size n_endDateField = 2;
            Size n_notionalField = 3;
            Size n_rateField = 4;
            Size n_payDateField = 5;
            Size n_CFField = 6;
            Size n_DFField = 7;

            resultCashFlow[0] = static_cast<double>(numberOfCoupons);
            for (Size couponNum = 0; couponNum < numberOfCoupons; ++couponNum) {
                const auto& cp = ext::dynamic_pointer_cast<FixedRateCoupon>(bondCFs[couponNum]);
                if (cp != nullptr) {
                    resultCashFlow[couponNum * numberOfFields + n_startDateField] = static_cast<double>(cp->accrualStartDate().serialNumber());
                    resultCashFlow[couponNum * numberOfFields + n_endDateField] = static_cast<double>(cp->accrualEndDate().serialNumber());
                    resultCashFlow[couponNum * numberOfFields + n_notionalField] = cp->nominal();
                    resultCashFlow[couponNum * numberOfFields + n_rateField] = cp->rate();
                    resultCashFlow[couponNum * numberOfFields + n_payDateField] = static_cast<double>(cp->date().serialNumber());
                    resultCashFlow[couponNum * numberOfFields + n_CFField] = cp->amount();
					Real tmpDF = 0.0;
					if (!cp->hasOccurred(asOfDate_, includeSettlementDateFlows_) &&
						!cp->tradingExCoupon(asOfDate_)) {
						tmpDF = discountingCurve->discount(cp->date());
					}
					resultCashFlow[couponNum * numberOfFields + n_DFField] = tmpDF;
                }
                else {
                    const auto& redemption = ext::dynamic_pointer_cast<Redemption>(bondCFs[couponNum]);
                    if (redemption != nullptr) {
                        resultCashFlow[couponNum * numberOfFields + n_startDateField] = -1.0;
                        resultCashFlow[couponNum * numberOfFields + n_endDateField] = -1.0;
                        resultCashFlow[couponNum * numberOfFields + n_notionalField] = redemption->amount();
                        resultCashFlow[couponNum * numberOfFields + n_rateField] = -1.0;
                        resultCashFlow[couponNum * numberOfFields + n_payDateField] = static_cast<double>(redemption->date().serialNumber());
                        resultCashFlow[couponNum * numberOfFields + n_CFField] = redemption->amount();
						if (redemption->date() < asOfDate_) {
							resultCashFlow[couponNum * numberOfFields + n_DFField] = 0.0;
						}
						else {
							resultCashFlow[couponNum * numberOfFields + n_DFField] = discountingCurve->discount(redemption->date());
						}
                    }
                    else {
                        QL_FAIL("Coupon is not a FixedRateCoupon.");
                    }
                }
            }
            return result = npv;
        }

        // NPV : clean, dirty, accured Interest
        Real cleanPrice = fixedRateBond.cleanPrice() / 100.0 * notional;
        Real dirtyPrice = fixedRateBond.dirtyPrice() / 100.0 * notional;
        Real accruedInterest = fixedRateBond.accruedAmount() / 100.0 * notional;

        /* OUTPUT 1. Net PV 리턴 */
        return result = npv;
    }
    catch (...) {
        try {
            std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& e) {
			LOG_ERROR_KNOWN_EXCEPTION(std::string(e.what()));
			return result = -1.0;
        }
        catch (...) {
			LOG_ERROR_UNKNOWN_EXCEPTION();
			return result = -1.0;
        }
    }
}

 /* FRB Custom Class */
 FixedRateBondCustom::FixedRateBondCustom(Natural settlementDays,
     Real faceAmount,
     Schedule schedule,
     const std::vector<Rate>& coupons,
     const DayCounter& accrualDayCounter,
     BusinessDayConvention paymentConvention,
     Integer paymentLag,
     Real redemption,
     const Date& issueDate,
     const Calendar& paymentCalendar,
     const Period& exCouponPeriod,
     const Calendar& exCouponCalendar,
     const BusinessDayConvention exCouponConvention,
     bool exCouponEndOfMonth,
     const DayCounter& firstPeriodDayCounter)
     : Bond(settlementDays,
         paymentCalendar == Calendar() ? schedule.calendar() : paymentCalendar,
         issueDate),
     frequency_(schedule.hasTenor() ? schedule.tenor().frequency() : NoFrequency),
     dayCounter_(accrualDayCounter),
     firstPeriodDayCounter_(firstPeriodDayCounter) {

     maturityDate_ = schedule.endDate();

     cashflows_ = FixedRateLeg(std::move(schedule))
         .withNotionals(faceAmount)
         .withCouponRates(coupons, accrualDayCounter)
         .withFirstPeriodDayCounter(firstPeriodDayCounter)
         .withPaymentCalendar(calendar_)
         .withPaymentAdjustment(paymentConvention)
         .withPaymentLag(paymentLag)
         .withExCouponPeriod(exCouponPeriod,
             exCouponCalendar,
             exCouponConvention,
             exCouponEndOfMonth);

     addRedemptionsToCashflows(std::vector<Real>(1, redemption));

     QL_ENSURE(!cashflows().empty(), "bond with no cashflows!");
     QL_ENSURE(redemptions_.size() == 1, "multiple redemptions created.");
 }


extern "C" double EXPORT pricingFRN(
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

    , const int fixingDays                  // INPUT 11. 금리 확정일 수
    , const double gearing                  // INPUT 12. 참여율
    , const double spread                   // INPUT 13. 스프레드
    , const double lastResetRate            // INPUT 14. 직전 확정 금리
    , const double nextResetRate            // INPUT 15. 차기 확정 금리

    , const int numberOfCoupons             // INPUT 16. 쿠폰 개수
    , const int* paymentDates               // INPUT 17. 지급일 배열
    , const int* realStartDates             // INPUT 18. 각 구간 시작일
    , const int* realEndDates               // INPUT 19. 각 구간 종료일

    , const double spreadOverYield          // INPUT 20. 채권의 종목 Credit Spread

    , const int numberOfGirrTenors          // INPUT 21. GIRR 만기 수
    , const int* girrTenorDays              // INPUT 22. GIRR 만기 (startDate로부터의 일수)
    , const double* girrRates               // INPUT 23. GIRR 금리
    , const int* girrConvention             // INPUT 24. GIRR 컨벤션 [index 0 ~ 3: GIRR DayCounter, 보간법, 이자 계산 방식, 이자 빈도] (TODO)

    , const int numberOfCsrTenors           // INPUT 25. CSR 만기 수
    , const int* csrTenorDays               // INPUT 26. CSR 만기 (startDate로부터의 일수)
    , const double* csrRates                // INPUT 27. CSR 스프레드 (금리 차이)

    , const int numberOfIndexGirrTenors     // INPUT 28. GIRR 만기 수
    , const int* indexGirrTenorDays         // INPUT 29. GIRR 만기 (startDate로부터의 일수)
    , const double* indexGirrRates          // INPUT 30. GIRR 금리
    , const int* indexGirrConvention        // INPUT 31. GIRR 컨벤션 [index 0 ~ 3: GIRR DayCounter, 보간법, 이자 계산 방식, 이자 빈도] (TODO)
    , const int isSameCurve                 // INPUT 32. Discounting Curve와 Index Curve의 일치 여부(0: False, others: true)

    , const int indexTenor                  // INPUT 33. 금리 인덱스 만기의 날짜수(1 Month = 30 기준)
    , const int indexFixingDays             // INPUT 34. 금리 인덱스의 고시 확정일 수
    , const int indexCurrency               // INPUT 35. 금리 인덱스의 표시 통화
    , const int indexCalendar               // INPUT 36. 금리 인덱스의 휴일 기준 달력
    , const int indexBDC                    // INPUT 37. 금리 인덱스의 휴일 적용 기준
    , const int indexEOM                    // INPUT 38. 금리 인덱스의 월말 여부
    , const int indexDayCounter             // INPUT 39. 금리 인덱스의 날짜 계산 기준

    , const double marketPrice              // INPUT 40. (추가) 시장가격(Spread Over Yield 산출 시 사용)
    , const double girrRiskWeight           // INPUT 41. (추가) girr 리스크요소 버킷의 위험 가중치(Curvature 산출 시 사용) (TODO)
    , const double csrRiskWeight            // INPUT 42. (추가) csr 리스크요소 버킷의 위험 가중치(Curvature 산출 시 사용) (TODO)

    , const int calType			            // INPUT 43. 계산 타입 (1: Price, 2. BASEL 2 Delta, 3. BASEL 3 GIRR / CSR, 9. SOY)
    , const int logYn                       // INPUT 44. 로그 파일 생성 여부 (0: No, 1: Yes)

                                            // OUTPUT 1. Net PV (리턴값)
    , double* resultGirrBasel2              // OUTPUT 2. Basel 2 Result [index 0 ~ 4: Delta, Gamma, Duration, Convexity, PV01]
    , double* resultIndexGirrBasel2         // OUTPUT 3. Basel 2 Result [index 0 ~ 4: Delta, Gamma, Duration, Convexity, PV01]
    , double* resultGirrDelta               // OUTPUT 4. GIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultIndexGirrDelta          // OUTPUT 5. IndexGIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultCsrDelta			    // OUTPUT 6. CSR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultGirrCvr			        // OUTPUT 7. GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultIndexGirrCvr			// OUTPUT 8. GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultCsrCvr			        // OUTPUT 9. CSR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultCashFlow                // OUTPUT 7. CF(index 0: size, index cfNum * 7 + 1 ~ cfNum * 7 + 7: 
                                            //              startDate, endDate, notional, rate, payDate, CF, DF)
// ===================================================================================================
) {
    double result = -1.0; // 결과값 리턴 변수

    FINALLY({
        /* Output Result 로그 출력 */
        LOG_OUTPUT(
            FIELD_VAR(result),
            FIELD_ARR(resultGirrBasel2, 5), FIELD_ARR(resultIndexGirrBasel2, 5), 
            FIELD_ARR(resultGirrDelta, 23), FIELD_ARR(resultIndexGirrDelta, 23), 
            FIELD_ARR(resultCsrDelta, 13),
            FIELD_ARR(resultGirrCvr, 2), FIELD_ARR(resultIndexGirrCvr, 2), 
            FIELD_ARR(resultCsrCvr, 2), 
            FIELD_ARR(resultCashFlow, 1000)
        );
        
        /* 로그 종료 */
        LOG_END(result);
    });

    try {
        /* 로거 초기화 */
        disableConsoleLogging();
        if (logYn == 1) {
            LOG_START("bond");
        }

        /* Input Parameter 로그 출력 */
        LOG_INPUT(
            FIELD_VAR(evaluationDate), FIELD_VAR(issueDate), FIELD_VAR(maturityDate), FIELD_VAR(notional),
            FIELD_VAR(couponDayCounter), FIELD_VAR(couponCalendar), FIELD_VAR(couponFrequency), FIELD_VAR(scheduleGenRule), FIELD_VAR(paymentBDC), FIELD_VAR(paymentLag),
            FIELD_VAR(fixingDays), FIELD_VAR(gearing), FIELD_VAR(spread), FIELD_VAR(lastResetRate), FIELD_VAR(nextResetRate),
            FIELD_VAR(numberOfCoupons), FIELD_ARR(paymentDates, numberOfCoupons), FIELD_ARR(realStartDates, numberOfCoupons), FIELD_ARR(realEndDates, numberOfCoupons),
            FIELD_VAR(spreadOverYield),
            FIELD_VAR(numberOfGirrTenors), FIELD_ARR(girrTenorDays, numberOfGirrTenors), FIELD_ARR(girrRates, numberOfGirrTenors), FIELD_ARR(girrConvention, 4),
            FIELD_VAR(numberOfCsrTenors), FIELD_ARR(csrTenorDays, numberOfCsrTenors), FIELD_ARR(csrRates, numberOfCsrTenors), 
            FIELD_VAR(numberOfIndexGirrTenors), FIELD_ARR(indexGirrTenorDays, numberOfIndexGirrTenors), FIELD_ARR(indexGirrRates, numberOfIndexGirrTenors), FIELD_ARR(indexGirrConvention, 4),
            FIELD_VAR(isSameCurve),
            FIELD_VAR(indexTenor), FIELD_VAR(indexFixingDays), FIELD_VAR(indexCurrency), FIELD_VAR(indexCalendar), FIELD_VAR(indexBDC), FIELD_VAR(indexEOM), FIELD_VAR(indexDayCounter),
            FIELD_VAR(marketPrice), FIELD_VAR(girrRiskWeight), FIELD_VAR(csrRiskWeight),
            FIELD_VAR(calType), FIELD_VAR(logYn)
        );

        if (calType != 1 && calType != 2 && calType != 3 && calType != 4 && calType != 9) {
            error("Invalid calculation type. Only 1, 2, 3, 4, 9 are supported.");
            return result = -1.0; // Invalid calculation type
        }

        // Input Data Check
        // Maturity Date >= evaluation Date
        if (maturityDate < evaluationDate) {
            error("Maturity Date is less than evaluation Date.");
            return result = -1.0;
        }
        // Maturity Date >= issue Date
        if (maturityDate < issueDate) {
            error("Maturity Date is less than issue Date.");
            return result = -1.0;
        }
        // Last Payment date >= evaluation Date
        if ((numberOfCoupons > 0) && (paymentDates[numberOfCoupons - 1] < evaluationDate)) {
            error("PaymentDate Date is less than evaluation Date.");
            return result = -1.0;
        }

        // 결과 데이터 초기화
        initResult(resultGirrBasel2, 5);
        initResult(resultIndexGirrBasel2, 5);
        initResult(resultGirrDelta, 23);
        initResult(resultIndexGirrDelta, 23);
        initResult(resultCsrDelta, 13);
        initResult(resultGirrCvr, 2);
        initResult(resultIndexGirrCvr, 2);
        initResult(resultCsrCvr, 2);
        initResult(resultCashFlow, 1000);

        // revaluationDateSerial -> revaluationDate
        Date asOfDate_ = Date(evaluationDate);
        const Date maturityDate_ = Date(maturityDate);

        // 전역 Settings에 평가일을 설정 (이후 모든 계산에 이 날짜 기준 적용)
        Settings::instance().evaluationDate() = asOfDate_;
        Size settlementDays_ = 0;
        bool includeSettlementDateFlows_ = true;

        Real notional_ = notional;

        DayCounter couponDayCounter_ = makeDayCounterFromInt(couponDayCounter);
        Calendar couponCalendar_ = makeCalendarFromInt(couponCalendar);
        Frequency couponFrequency_ = makeFrequencyFromInt(couponFrequency);
        BusinessDayConvention paymentBDC_ = makeBDCFromInt(paymentBDC);

        // index Reference 커브 구성용 날짜 및 금리 벡터
        std::vector<Date> indexGirrDates_;
        std::vector<Real> indexGirrRates_;

        // GIRR 커브의 주요 기간 설정 (시장 표준 테너)
        std::vector<Period> indexGirrPeriod =
            makePeriodArrayFromTenorDaysArray(indexGirrTenorDays, numberOfIndexGirrTenors);

        // GIRR 커브 시작점 (revaluationDate 기준) 입력
        indexGirrDates_.emplace_back(asOfDate_);
        indexGirrRates_.emplace_back(indexGirrRates[0]);

        // 나머지 GIRR 커브 구성 요소 입력
        for (Size dateNum = 0; dateNum < numberOfIndexGirrTenors; ++dateNum) {
            indexGirrDates_.emplace_back(asOfDate_ + indexGirrPeriod[dateNum]);
            indexGirrRates_.emplace_back(indexGirrRates[dateNum]);
        }

        // GIRR 커브 계산 사용 추가 요소
        DayCounter indexGirrDayCounter_ = makeDayCounterFromInt(indexGirrConvention[0]); // DCB, TODO 변환 함수 적용
        Linear indexGirrInterpolator_ = Linear(); // 보간 방식, TODO 변환 함수 적용 (Interpolator)
        Compounding indexGirrCompounding_ = makeCompoundingFromInt(indexGirrConvention[2]); // 이자 계산 방식, TODO 변환 함수 적용 (Compounding)
        Frequency indexGirrFrequency_ = makeFrequencyFromInt(indexGirrConvention[3]); // 이자 지급 빈도, TODO 변환 함수 적용 (Frequency)

        // GIRR 커브 생성
        ext::shared_ptr<YieldTermStructure> indexGirrTermstructure = ext::make_shared<ZeroCurve>(indexGirrDates_, indexGirrRates_,
            indexGirrDayCounter_, indexGirrInterpolator_,
            indexGirrCompounding_, indexGirrFrequency_);

        // GIRR 커브를 RelinkableHandle에 연결
        RelinkableHandle<YieldTermStructure> indexGirrCurve;
        indexGirrCurve.linkTo(indexGirrTermstructure);
        indexGirrCurve->enableExtrapolation(); // 외삽 허용

        // Index 클래스 생성
        Period index1Tenor_ = makePeriodFromDays(indexTenor); // 금리 인덱스 만기 설정 (1 Month = 30 기준)
        Calendar index1FixingCalendar_ = makeCalendarFromInt(indexCalendar); // 금리 인덱스의 휴일 적용 기준 달력 
        DayCounter index1DayCounter_ = makeDayCounterFromInt(indexDayCounter); // 금리 인덱스의 날짜 계산 기준 

        // Define default data -  index1
        std::string indexFamilyName_ = "CD";
        Natural index1FixingDays_ = fixingDays;
        Currency index1Currency_ = makeCurrencyFromInt(indexCurrency); // 금리 인덱스의 표시 통화
        BusinessDayConvention index1BusinessDayConvention_ = makeBDCFromInt(indexBDC); //
        bool index1EndOfMonth_ = makeBoolFromInt(indexEOM); // 금리 인덱스의 월말 여부 

        // Make index instance
        ext::shared_ptr<IborIndex> refIndex = ext::make_shared<IborIndex>(indexFamilyName_, index1Tenor_, index1FixingDays_
            , index1Currency_, index1FixingCalendar_, index1BusinessDayConvention_
            , index1EndOfMonth_, index1DayCounter_, indexGirrCurve);


        // GIRR 커브 구성용 날짜 및 금리 벡터
        std::vector<Date> girrDates_;
        std::vector<Real> girrRates_;

        // GIRR 커브의 주요 기간 설정 (시장 표준 테너)
        std::vector<Period> girrPeriod =
            makePeriodArrayFromTenorDaysArray(girrTenorDays, numberOfGirrTenors);

        // GIRR 커브 시작점 (revaluationDate 기준) 입력
        girrDates_.emplace_back(asOfDate_);
        girrRates_.emplace_back(girrRates[0]);

        // 나머지 GIRR 커브 구성 요소 입력
        for (Size dateNum = 0; dateNum < numberOfGirrTenors; ++dateNum) {
            girrDates_.emplace_back(asOfDate_ + girrPeriod[dateNum]);
            girrRates_.emplace_back(girrRates[dateNum]);
        }

        // GIRR 커브 계산 사용 추가 요소
        DayCounter girrDayCounter_ = makeDayCounterFromInt(girrConvention[0]); // DCB
        Linear girrInterpolator_ = Linear(); // 보간 방식, TODO 변환 함수 적용 (Interpolator)
        Compounding girrCompounding_ = makeCompoundingFromInt(girrConvention[2]); // 이자 계산 방식(Compounding)
        Frequency girrFrequency_ = makeFrequencyFromInt(girrConvention[3]); // 이자 지급 빈도(Frequency)

        // GIRR 커브 생성
        ext::shared_ptr<YieldTermStructure> girrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, girrRates_,
            girrDayCounter_, girrInterpolator_, girrCompounding_, girrFrequency_);

        // GIRR 커브를 RelinkableHandle에 연결
        RelinkableHandle<YieldTermStructure> girrCurve;
        girrCurve.linkTo(girrTermstructure);
        girrCurve->enableExtrapolation(); // 외삽 허용

        // spreadOverYiled 값을 interest Rate 객체로 래핑 (CSR 계산용)
        double tmpSpreadOverYield = spreadOverYield;
        Compounding spreadOverYieldCompounding_ = Compounding::Continuous; // 이자 계산 방식, Continuous로 설정
        DayCounter spreadOverYieldDayCounter_ = Actual365Fixed();  // DayCounter Actual/365 Fixed로 설정
        InterestRate tempRate(tmpSpreadOverYield, spreadOverYieldDayCounter_, spreadOverYieldCompounding_, Frequency::Annual);

        // CSR 커브 구성용 날짜 및 금리 벡터
        std::vector<Date> csrDates_;
        std::vector<Period> csrPeriod =
            makePeriodArrayFromTenorDaysArray(csrTenorDays, numberOfCsrTenors);

        // CSR 커브 시작점 (revaluationDate 기준) 입력
        csrDates_.emplace_back(asOfDate_);

        // CSR 스프레드를 담을 핸들 벡터
        std::vector<Handle<Quote>> csrSpreads_;

        // 첫번째 CSR 스프레드에 spreadOverYield 값을 설정
        QL_REQUIRE(!csrPeriod.empty(), "csrPeriod is empty.");
        double spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_,
            girrDayCounter_.yearFraction(asOfDate_, asOfDate_ + 1));
        csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(spreadOverYield_));

        // 나머지 CSR 커브 기간별 스프레드 입력
        for (Size dateNum = 0; dateNum < numberOfCsrTenors; ++dateNum) {
            csrDates_.emplace_back(asOfDate_ + csrPeriod[dateNum]);
            spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_, girrDayCounter_.yearFraction(asOfDate_, csrDates_.back()));
            csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrRates[dateNum] + spreadOverYield_));
        }

        // GIRR + CSR 스프레드 커브 생성
        ext::shared_ptr<ZeroYieldStructure> discountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, csrSpreads_, csrDates_);

        // Discounting 커브 연결
        RelinkableHandle<YieldTermStructure> discountingCurve;
        discountingCurve.linkTo(discountingTermStructure);
        discountingCurve->enableExtrapolation(); // 외삽 허용

        // Discounting 엔진 생성 (채권 가격 계산용)
        auto bondEngine = ext::make_shared<DiscountingBondEngine>(discountingCurve, includeSettlementDateFlows_);

        // Schedule 객체 생성 (지급일)
        Schedule FRNSchedule_;
        BusinessDayConvention couponBDC_ = makeBDCFromInt(paymentBDC); // 지급일 휴일 적용 기준

        // Coupon Schedule 생성
        if (numberOfCoupons > 0) { // 쿠폰 스케줄이 인자로 들어오는 경우
            LOG_MESSAGE_COUPON_SCHEDULE_INPUT();

            // Schedule Data 유효성 점검(1. 발행일 & 만기일 유효성 점검. 2. 개별 Coupon Data의 유효성 점검)
            if (realStartDates[0] < issueDate   // 첫 번째 시작일이 발행일보다 이전인 경우(종료일 정보는 개별 Coupon 유효성 로직에서 점검)
                || paymentDates[0] < issueDate  // 첫 번째 지급일이 발행일보다 이전인 경우
                || paymentDates[numberOfCoupons - 1] > maturityDate  // 마지막 지급일이 만기일보다 이후인 경우
                ) {
                error("Invalid Coupon Schedule Data. Check the schedule period is in the trading period.");
                return -1;
            }

            for (int i = 0; i < numberOfCoupons; i++) {
                // 개별 Coupon의 시작일, 종료일, 지급일 유효성(종료일 > 지급일은 가능)
                if (realEndDates[i] < realStartDates[i] // 쿠폰 종료일이 시작일보다 이전인 경우
                    || paymentDates[i] < realStartDates[i]  // 지급일이 시작일보다 이전인 경우
                        ) {
                    error("Invalid Coupon Schedule Data. Check the start date, end date, and payment date.");
                    return -1;
                }

                // 스케쥴 배열의 Sorting 여부
                if (i != numberOfCoupons - 1) {
                    if (realStartDates[i + 1] < realStartDates[i]   // StartDates의 정렬 여부 점검
                        || realEndDates[i + 1] < realEndDates[i]    // EndDates의 정렬 여부 점검
                            || paymentDates[i + 1] < paymentDates[i]    // PaymentDates의 정렬 여부 점검
                                ) {
                        error("Invalid Coupon Schedule Data. Check the schedule is sorted.");
                        return -1;
                    }
                }
            }

            std::vector<Date> couponSch_;
            couponSch_.emplace_back(realStartDates[0]);
            for (Size schNum = 0; schNum < numberOfCoupons; ++schNum) {
                couponSch_.emplace_back(realEndDates[schNum]);
            }
            FRNSchedule_ = Schedule(couponSch_);
        }
        else {  // 쿠폰 스케줄이 인자로 들어오지 않는 경우, 스케줄을 직접 생성
            LOG_MESSAGE_COUPON_SCHEDULE_GENERATE();

            //Date effectiveDate = Date(realStartDates[0]); // 쿠폰 첫번째 시작일로 수정
            Date effectiveDate = Date(issueDate); // 기존 코드
            DateGeneration::Rule genRule = makeScheduleGenRuleFromInt(scheduleGenRule); // Forward, Backward 등

            FRNSchedule_ = MakeSchedule().from(effectiveDate)
                .to(maturityDate_)
                .withFrequency(couponFrequency_)
                .withCalendar(couponCalendar_)
                .withConvention(couponBDC_)
                .withRule(genRule);
        }

		// 평가일 기준 유효한 현금흐름만 포함하는 Schedule 생성
		Date schStartDate = FRNSchedule_.previousDate(asOfDate_);
		std::vector<Date> futureScheduleDates = FRNSchedule_.after(schStartDate).dates();
		Schedule futureFRNSchedule_ = Schedule(futureScheduleDates);

        /* 스케쥴 로그 */
        LOG_COUPON_SCHEDULE(futureFRNSchedule_);

        // fixing data 입력
        Date lastRefDate = futureFRNSchedule_.previousDate(asOfDate_);
        Date lastFixingDate1 = refIndex->fixingCalendar().advance(lastRefDate, -static_cast<Integer>(fixingDays)
            , Days, Preceding);
        Date nextRefDate = futureFRNSchedule_.nextDate(asOfDate_);
        Date nextFixingDate1 = refIndex->fixingCalendar().advance(nextRefDate, -static_cast<Integer>(fixingDays)
            , Days, Preceding);
        // Date lastFixingDate1 = refIndex->fixingDate(FRNSchedule_.previousDate(asOfDate_));
        // Date nextFixingDate1 = refIndex->fixingDate(FRNSchedule_.nextDate(asOfDate_));
        refIndex->addFixing(lastFixingDate1, lastResetRate, true);
        refIndex->addFixing(nextFixingDate1, nextResetRate, true);

        // FixedRateBond 객체 생성
        FloatingRateBondCustom floatingRateBond(
            settlementDays_,
            notional_,
            futureFRNSchedule_,
            refIndex,
            couponDayCounter_,
            couponBDC_, // Business Day Convention, TODO 변환 함수 적용 (DayConvention)
            fixingDays,
            paymentLag,
            { gearing },
            { spread });

        // Fixed Rate Bond에 Discounting 엔진 연결
        floatingRateBond.setPricingEngine(bondEngine);

        // 채권 가격 Net PV 계산
        Real npv = floatingRateBond.NPV();

        // 이론가 산출의 경우 GIRR Delta 산출을 하지 않음
        if (calType == 1) {
            /* OUTPUT 1. Net PV 리턴 */
            return result = npv;
        }

        if (calType == 9) {
            // Calc Spread Over Yield
            std::vector<Handle<Quote>> tmpCsrSpreads_;
            tmpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(0.0));
            for (Size dateNum = 0; dateNum < numberOfCsrTenors; ++dateNum) {
                tmpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrRates[dateNum]));
            }
            ext::shared_ptr<ZeroYieldStructure> tmpDiscountingTermStructure =
                ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, tmpCsrSpreads_, csrDates_);
            RelinkableHandle<YieldTermStructure> tmpDiscountingCurve;
            tmpDiscountingCurve.linkTo(tmpDiscountingTermStructure);
            tmpDiscountingCurve->enableExtrapolation();

            auto tmpBondEngine = ext::make_shared<DiscountingBondEngine>(tmpDiscountingCurve, includeSettlementDateFlows_);
            floatingRateBond.setPricingEngine(tmpBondEngine);
            // SettlementDays 관행 무시
            Real soy = CashFlows::zSpread(floatingRateBond.cashflows(), marketPrice, *tmpDiscountingCurve, Actual365Fixed(), Continuous, Annual,
                includeSettlementDateFlows_, asOfDate_, asOfDate_, 1.0e-10, 100, 0.005);  // settlementDate 산출 로직 제거(20250822, jwlee)            
            //        Real soy = CashFlows::zSpread(fixedRateBond.cashflows(), marketPrice, *tmpDiscountingCurve, Actual365Fixed(), Continuous, Annual,
            //                                      false, asOfDate_, couponCalendar.advance(asOfDate_, Period(settlementDays, Days)), 1.0e-10, 100, 0.005);
            return result = soy;
        }

        if (calType == 2) {
            // girrDelta 계산
            Real bumpSize = 0.0001; // bumpSize를 0.0001 이외의 값으로 적용 시, PV01 산출을 독립적으로 구현해줘야 함
            std::vector<Real> bumpGearings{ 1.0, -1.0 };
            std::vector<Real> bumpedNpv(bumpGearings.size(), 0.0);
            bool isSameCurve_ = true;
            // if (isSameCurve == 0) {
            //     isSameCurve_ = false;
            // }

            for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                std::vector<Rate> bumpGirrRates = girrRates_;
                std::vector<Rate> bumpIndexGirrRates = indexGirrRates_;
                for (Size bumpTenorNum = 0; bumpTenorNum < girrRates_.size(); ++bumpTenorNum) {
                    // GIRR 커브의 금리를 bumping (1bp 상승)
                    bumpGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * bumpSize;
                    bumpIndexGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * bumpSize;
                }

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates, girrDayCounter_, girrInterpolator_,
                    girrCompounding_, girrFrequency_);

                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation();

                // CSR Spread를 적용한 bump GIRR 기반 할인 커브 생성
                ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                // Index 커브와 할인커브가 달라도 동시에 bump된 금리커브적용
                ext::shared_ptr<YieldTermStructure> bumpIndexGirrTermstructure = ext::make_shared<ZeroCurve>(indexGirrDates_, bumpIndexGirrRates,
                    indexGirrDayCounter_, indexGirrInterpolator_,
                    indexGirrCompounding_, indexGirrFrequency_);

                if (isSameCurve_) {
                    indexGirrCurve.linkTo(bumpIndexGirrTermstructure); // indexGirrCurve.linkTo(bumpGirrTermstructure);
                }

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                floatingRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                bumpedNpv[bumpNo] = floatingRateBond.NPV();

                // 방어 코드 추가
                indexGirrCurve.linkTo(indexGirrTermstructure);
                floatingRateBond.setPricingEngine(bondEngine);
            }

            QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            Real delta = (bumpedNpv[0] - npv) / bumpSize;
            Real gamma = (bumpedNpv[0] - 2.0 * npv + bumpedNpv[1]) / (bumpSize * bumpSize);

            //        const DayCounter& ytmDayCounter = Actual365Fixed();
            //        Compounding ytmCompounding = Continuous;
            //        Frequency ytmFrequency = couponFrequency_;
            //        Rate ytmValue = CashFlows::yield(floatingRateBond.cashflows(), npv, ytmDayCounter, ytmCompounding, ytmFrequency, false,
            //                                         couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), asOfDate_,
            //                                         1.0e-15, 100, 0.005);
            //        InterestRate ytm = InterestRate(ytmValue, ytmDayCounter, ytmCompounding, ytmFrequency);
            //
            //        // Duration 계산
            //        Duration::Type durationType = Duration::Modified;
            //        Real duration = CashFlows::duration(floatingRateBond.cashflows(), ytm, durationType, false,
            //                                            couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), asOfDate_);
            //
            //        Real convexity = CashFlows::convexity(floatingRateBond.cashflows(), ytm, false,
            //                                              couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), asOfDate_);

            // Calculate Effective Duration
            Real duration = (bumpedNpv[1] - bumpedNpv[0]) / (bumpSize * npv * 2.0);
            Real convexity = gamma / npv;
            Real PV01 = delta * bumpSize;

            resultGirrBasel2[0] = delta;
            resultGirrBasel2[1] = gamma;
            resultGirrBasel2[2] = duration;
            resultGirrBasel2[3] = convexity;
            resultGirrBasel2[4] = PV01;

            if (!isSameCurve_) {
                floatingRateBond.setPricingEngine(bondEngine);
                for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                    std::vector<Rate> bumpIndexGirrRates = indexGirrRates_;
                    for (Size bumpTenorNum = 0; bumpTenorNum < indexGirrRates_.size(); ++bumpTenorNum) {
                        // GIRR 커브의 금리를 bumping (1bp 상승)
                        bumpIndexGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * bumpSize;
                    }

                    // bump된 금리로 새로운 ZeroCurve 생성
                    ext::shared_ptr<YieldTermStructure> bumpIndexGirrTermstructure = ext::make_shared<ZeroCurve>(indexGirrDates_, bumpIndexGirrRates, indexGirrDayCounter_,
                        indexGirrInterpolator_, indexGirrCompounding_,
                        indexGirrFrequency_);

                    // RelinkableHandle에 bump된 커브 연결
                    indexGirrCurve.linkTo(bumpIndexGirrTermstructure);

                    // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                    bumpedNpv[bumpNo] = floatingRateBond.NPV();

                }
                indexGirrCurve.linkTo(indexGirrTermstructure);

                QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
                delta = (bumpedNpv[0] - npv) / bumpSize;
                gamma = (bumpedNpv[0] - 2.0 * npv + bumpedNpv[1]) / (bumpSize * bumpSize);

                // Calculate Effective Duration
                duration = (bumpedNpv[1] - bumpedNpv[0]) / (bumpSize * npv * 2.0);
                convexity = gamma / npv;
                PV01 = delta * bumpSize;

                resultIndexGirrBasel2[0] = delta;
                resultIndexGirrBasel2[1] = gamma;
                resultIndexGirrBasel2[2] = duration;
                resultIndexGirrBasel2[3] = convexity;
                resultIndexGirrBasel2[4] = PV01;
            }
            return npv;

        }

        if (calType == 3) {
            // GIRR Bump Rate 설정
            Real girrBump = 0.0001;

            // (Discounting Curve) GIRR Delta 적재용 벡터 생성
            std::vector<Real> disCountingGirr;
            bool isSameCurve_ = true;
            if (isSameCurve == 0) {
                isSameCurve_ = false;
            }

            // (Discounting Curve) GIRR Delta 계산
            for (Size bumpNum = 1; bumpNum < girrRates_.size(); ++bumpNum) {
                // GIRR 커브의 금리를 bumping (1bp 상승)
                std::vector<Rate> bumpGirrRates = girrRates_;
                if (bumpNum == 1) {
                    bumpGirrRates[0] += girrBump; // 0번째 tenor도 같이 bump 적용
                }
                bumpGirrRates[bumpNum] += girrBump;

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates, girrDayCounter_, girrInterpolator_, girrCompounding_, girrFrequency_);

                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation();

                // CSR Spread를 적용한 bump GIRR 기반 할인 커브 생성
                ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                if (isSameCurve_) {
                    indexGirrCurve.linkTo(bumpGirrTermstructure);
                }

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                floatingRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                Real tmpGirr = (floatingRateBond.NPV() - npv) * 10000;

                // 산출된 Girr Delta 값을 벡터에 추가
                disCountingGirr.emplace_back(tmpGirr);
                indexGirrCurve.linkTo(indexGirrTermstructure);
            }


            /* OUTPUT 2. (Discounting Curve) GIRR Delta 결과 적재 */
            std::vector<Real> girrTenor = { 0.0, 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 15.0, 20.0, 30.0 };
            Size girrDataSize = girrTenor.size();

            // Parallel 민감도 추가
            double tmpCcyDelta = std::accumulate(disCountingGirr.begin(), disCountingGirr.end(), 0.0);
            disCountingGirr.insert(disCountingGirr.begin(), tmpCcyDelta);
            QL_REQUIRE(girrDataSize == disCountingGirr.size(), "Girr result Size mismatch.");
            // 0인 민감도를 제외하고 적재
            processResultArray(girrTenor, disCountingGirr, girrDataSize, resultGirrDelta);

            // (Index Reference Curve) GIRR Delta 적재용 벡터 생성
            if (!isSameCurve_) {
                floatingRateBond.setPricingEngine(bondEngine);
                std::vector<Real> indexGirr;
                // (Index Reference Curve) GIRR Delta 계산
                for (Size bumpNum = 1; bumpNum < indexGirrRates_.size(); ++bumpNum) {
                    // GIRR 커브의 금리를 bumping (1bp 상승)
                    std::vector<Rate> bumpGirrRates = indexGirrRates_;
                    if (bumpNum == 1) {
                        bumpGirrRates[0] += girrBump; // 0번째 tenor도 같이 bump 적용
                    }
                    bumpGirrRates[bumpNum] += girrBump;

                    // bump된 금리로 새로운 ZeroCurve 생성
                    ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(indexGirrDates_, bumpGirrRates, indexGirrDayCounter_
                        , indexGirrInterpolator_, indexGirrCompounding_, indexGirrFrequency_);

                    // RelinkableHandle에 bump된 커브 연결
                    RelinkableHandle<YieldTermStructure> bumpGirrCurve; // 미사용 변수 확인 필요
                    indexGirrCurve.linkTo(bumpGirrTermstructure);
                    
                    // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                    Real tmpGirr = (floatingRateBond.NPV() - npv) * 10000;

                    // 산출된 Girr Delta 값을 벡터에 추가
                    indexGirr.emplace_back(tmpGirr);
                }
                indexGirrCurve.linkTo(indexGirrTermstructure);

                /* OUTPUT 3. GIRR Delta 결과 적재 */
                std::vector<Real> indexGirrTenor = { 0.0, 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 15.0, 20.0, 30.0 };
                Size indexGirrDataSize = indexGirrTenor.size();

                // Parallel 민감도 추가
                double tmpCcyDelta = std::accumulate(indexGirr.begin(), indexGirr.end(), 0.0);
                indexGirr.insert(indexGirr.begin(), tmpCcyDelta);
                QL_REQUIRE(indexGirrDataSize == indexGirr.size(), "Girr result Size mismatch.");

                // 0인 민감도를 제외하고 적재
                processResultArray(indexGirrTenor, indexGirr, indexGirrDataSize, resultIndexGirrDelta);
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
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                floatingRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (CSR Delta)
                Real tmpCsr = (floatingRateBond.NPV() - npv) * 10000;

                // 산출된 CSR Delta 값을 벡터에 추가
                disCountingCsr.emplace_back(tmpCsr);
            }

            /* OUTPUT 4. CSR Delta 결과 적재 */
            std::vector<Real> csrTenor = { 0.5, 1.0, 3.0, 5.0, 10.0 };
            Size csrDataSize = csrTenor.size();
            // 0인 민감도를 제외하고 적재
            processResultArray(csrTenor, disCountingCsr, csrDataSize, resultCsrDelta);

            // Girr Curvature 계산
            Real curvatureRW = girrRiskWeight; // bumpSize를 FRTB 기준서의 Girr Curvature RiskWeight로 설정
            std::vector<Real> bumpGearings{ 1.0, -1.0 };
            std::vector<Real> bumpedNpv(bumpGearings.size(), 0.0);
            for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                std::vector<Rate> bumpGirrRates = girrRates_;
                for (Size bumpTenorNum = 0; bumpTenorNum < girrRates_.size(); ++bumpTenorNum) {
                    // GIRR 커브의 금리를 bumping (RiskWeight 만큼 상승)
                    bumpGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * curvatureRW;
                }

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates, girrDayCounter_, girrInterpolator_,
                    girrCompounding_, girrFrequency_);

                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation();

                // CSR Spread를 적용한 bump GIRR 기반 할인 커브 생성
                ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                if (isSameCurve_) {
                    indexGirrCurve.linkTo(bumpGirrTermstructure);
                }

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                floatingRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                bumpedNpv[bumpNo] = floatingRateBond.NPV();
                indexGirrCurve.linkTo(indexGirrTermstructure);
            }

            QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            resultGirrCvr[0] = (bumpedNpv[0] - npv);
            resultGirrCvr[1] = (bumpedNpv[1] - npv);

            // Index Girr Curvature 계산
            if (!isSameCurve_) {
                floatingRateBond.setPricingEngine(bondEngine);
                for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                    std::vector<Rate> bumpIndexGirrRates = indexGirrRates_;
                    for (Size bumpTenorNum = 0; bumpTenorNum < indexGirrRates_.size(); ++bumpTenorNum) {
                        // GIRR 커브의 금리를 bumping (RiskWeight 만큼 상승)
                        bumpIndexGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * curvatureRW;
                    }

                    // bump된 금리로 새로운 ZeroCurve 생성
                    ext::shared_ptr<YieldTermStructure> bumpIndexGirrTermstructure = ext::make_shared<ZeroCurve>(indexGirrDates_, bumpIndexGirrRates, indexGirrDayCounter_,
                        indexGirrInterpolator_, indexGirrCompounding_,
                        indexGirrFrequency_);

                    // RelinkableHandle에 bump된 커브 연결
                    indexGirrCurve.linkTo(bumpIndexGirrTermstructure);

                    // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                    bumpedNpv[bumpNo] = floatingRateBond.NPV();
                }

                QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
                resultIndexGirrCvr[0] = (bumpedNpv[0] - npv);
                resultIndexGirrCvr[1] = (bumpedNpv[1] - npv);
                indexGirrCurve.linkTo(indexGirrTermstructure);
            }

            // Curvature 계산
            curvatureRW = csrRiskWeight; // bumpSize를 FRTB 기준서의 CSR Bucket의 Curvature RiskWeight로 설정
            for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                std::vector<Handle<Quote>> bumpCsrSpreads_;
                for (Size bumpTenorNum = 0; bumpTenorNum < csrSpreads_.size(); ++bumpTenorNum) {
                    // Csr 커브의 금리를 bumping (RiskWeight 만큼 상승)
                    bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[bumpTenorNum]->value() +
                        bumpGearings[bumpNo] * curvatureRW));
                }

                // GIRR 커브에 bump된 CSR Spread를 적용한 할인 커브 생성
                ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure =
                    ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, bumpCsrSpreads_, csrDates_);

                // 새로운 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용


                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                floatingRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                bumpedNpv[bumpNo] = floatingRateBond.NPV();
            }

            QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            resultCsrCvr[0] = (bumpedNpv[0] - npv);
            resultCsrCvr[1] = (bumpedNpv[1] - npv);

            // NPV : clean, dirty, accured Interest
            Real cleanPrice = floatingRateBond.cleanPrice() / 100.0 * notional;
            Real dirtyPrice = floatingRateBond.dirtyPrice() / 100.0 * notional;
            Real accruedInterest = floatingRateBond.accruedAmount() / 100.0 * notional;

            /* OUTPUT 1. Net PV 리턴 */
            return result = npv;
        }

        if (calType == 4) {
            const Leg& bondCFs = floatingRateBond.cashflows();
            Size numberOfCoupons = bondCFs.size();
            Size numberOfFields = 7;
            Size n_startDateField = 1;
            Size n_endDateField = 2;
            Size n_notionalField = 3;
            Size n_rateField = 4;
            Size n_payDateField = 5;
            Size n_CFField = 6;
            Size n_DFField = 7;

            resultCashFlow[0] = static_cast<double>(numberOfCoupons);
            for (Size couponNum = 0; couponNum < numberOfCoupons; ++couponNum) {
                const auto& cp = ext::dynamic_pointer_cast<FloatingRateCoupon>(bondCFs[couponNum]);
                if (cp != nullptr) {
                    resultCashFlow[couponNum * numberOfFields + n_startDateField] = static_cast<double>(cp->accrualStartDate().serialNumber());
                    resultCashFlow[couponNum * numberOfFields + n_endDateField] = static_cast<double>(cp->accrualEndDate().serialNumber());
                    resultCashFlow[couponNum * numberOfFields + n_notionalField] = cp->nominal();
                    resultCashFlow[couponNum * numberOfFields + n_rateField] = cp->rate();
                    resultCashFlow[couponNum * numberOfFields + n_payDateField] = static_cast<double>(cp->date().serialNumber());
                    resultCashFlow[couponNum * numberOfFields + n_CFField] = cp->amount();
					Real tmpDF = 0.0;
					if (!cp->hasOccurred(asOfDate_, includeSettlementDateFlows_) &&
						!cp->tradingExCoupon(asOfDate_)) {
						tmpDF = discountingCurve->discount(cp->date());
					}
					resultCashFlow[couponNum * numberOfFields + n_DFField] = tmpDF;
                }
                else {
                    const auto& redemption = ext::dynamic_pointer_cast<Redemption>(bondCFs[couponNum]);
                    if (redemption != nullptr) {
                        resultCashFlow[couponNum * numberOfFields + n_startDateField] = -1.0;
                        resultCashFlow[couponNum * numberOfFields + n_endDateField] = -1.0;
                        resultCashFlow[couponNum * numberOfFields + n_notionalField] = redemption->amount();
                        resultCashFlow[couponNum * numberOfFields + n_rateField] = -1.0;
                        resultCashFlow[couponNum * numberOfFields + n_payDateField] = static_cast<double>(redemption->date().serialNumber());
                        resultCashFlow[couponNum * numberOfFields + n_CFField] = redemption->amount();
						if (redemption->date() < asOfDate_) {
							resultCashFlow[couponNum * numberOfFields + n_DFField] = 0.0;
						}
						else {
							resultCashFlow[couponNum * numberOfFields + n_DFField] = discountingCurve->discount(redemption->date());
						}
                    }
                    else {
                        QL_FAIL("Coupon is not a FloatingRateCoupon");
                    }
                }
            }
            return result = npv;
        }

        return result = npv;
    }
    catch (...) {
        try {
            std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& e) {
            LOG_ERROR_KNOWN_EXCEPTION(std::string(e.what()));
            return result = -1.0;
        }
        catch (...) {
			LOG_ERROR_UNKNOWN_EXCEPTION();
            return result = -1.0;
        }
    }
}

 FloatingRateBondCustom::FloatingRateBondCustom(
     Natural settlementDays,
     Real faceAmount,
     Schedule schedule,
     const ext::shared_ptr<IborIndex>& iborIndex,
     const DayCounter& paymentDayCounter,
     BusinessDayConvention paymentConvention,
     Natural fixingDays,
     Integer paymentLag,
     const std::vector<Real>& gearings,
     const std::vector<Spread>& spreads,
     const std::vector<Rate>& caps,
     const std::vector<Rate>& floors,
     bool inArrears,
     Real redemption,
     const Date& issueDate,
     const Period& exCouponPeriod,
     const Calendar& exCouponCalendar,
     const BusinessDayConvention exCouponConvention,
     bool exCouponEndOfMonth)
     : Bond(settlementDays, schedule.calendar(), issueDate) {

     maturityDate_ = schedule.endDate();

     cashflows_ = IborLeg(std::move(schedule), iborIndex)
         .withNotionals(faceAmount)
         .withPaymentDayCounter(paymentDayCounter)
         .withPaymentAdjustment(paymentConvention)
         .withFixingDays(fixingDays)
         .withPaymentLag(paymentLag)
         .withGearings(gearings)
         .withSpreads(spreads)
         .withCaps(caps)
         .withFloors(floors)
         .inArrears(inArrears)
         .withExCouponPeriod(exCouponPeriod, exCouponCalendar, exCouponConvention, exCouponEndOfMonth);

     addRedemptionsToCashflows(std::vector<Real>(1, redemption));

     QL_ENSURE(!cashflows().empty(), "bond with no cashflows!");
     QL_ENSURE(redemptions_.size() == 1, "multiple redemptions created.");

     registerWith(iborIndex);
 }


extern "C" double EXPORT pricingZCB(
    // ===================================================================================================
    const int evaluationDate                // INPUT 1. 평가일 (serial number)
    , const int issueDate                   // INPUT 2. 발행일 (serial number)
    , const int maturityDate                // INPUT 3. 만기일 (serial number)
    , const double notional                 // INPUT 4. 채권 원금

    , const int numberOfGirrTenors          // INPUT 5. GIRR 만기 수
    , const int* girrTenorDays              // INPUT 6. GIRR 만기 (startDate로부터의 일수)
    , const double* girrRates               // INPUT 7. GIRR 금리
    , const int* girrConvention             // INPUT 8. GIRR 컨벤션 [index 0 ~ 4: GIRR DayCounter, 보간법, 이자 계산 방식, 이자 빈도] (TODO)

    , const double spreadOverYield          // INPUT 9. 채권의 종목 Credit Spread

    , const int numberOfCsrTenors           // INPUT 10. CSR 만기 수
    , const int* csrTenorDays               // INPUT 11. CSR 만기 (startDate로부터의 일수)
    , const double* csrRates                // INPUT 12. CSR 스프레드 (금리 차이)

    , const double marketPrice              // INPUT 13. (추가) 시장가격(Spread Over Yield 산출 시 사용)
    , const double girrRiskWeight           // INPUT 14. (추가) girr 리스크요소 버킷의 위험 가중치(Curvature 산출 시 사용)
    , const double csrRiskWeight            // INPUT 15. (추가) csr 리스크요소 버킷의 위험 가중치(Curvature 산출 시 사용)

    , const int calType			            // INPUT 16. 계산 타입 (1: Price, 2. BASEL 2 민감도, 3. BASEL 3 민감도, 9: SOY)
    , const int logYn                       // INPUT 17. 로그 파일 생성 여부 (0: No, 1: Yes)

                                            // OUTPUT 1. Net PV (리턴값)
    , double* resultBasel2                  // OUTPUT 2. (추가)Basel 2 Result(Delta, Gamma, Duration, Convexity, PV01)
    , double* resultGirrDelta               // OUTPUT 3. GIRR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultCsrDelta			    // OUTPUT 4. CSR Delta [index 0: size, index 1 ~ size + 1: tenor, index size + 2 ~ 2 * size + 1: sensitivity]
    , double* resultGirrCvr			        // OUTPUT 5. (추가)GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultCsrCvr			        // OUTPUT 6. (추가)CSR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultCashFlow                // OUTPUT 7. CF(index 0: size, index cfNum * 7 + 1 ~ cfNum * 7 + 7: 
    //              startDate, endDate, notional, rate, payDate, CF, DF)
// ===================================================================================================
) {
    double result = -1.0;

    FINALLY({
        /* Output Result 로그 출력 */
        LOG_OUTPUT(
            FIELD_VAR(result), 
            FIELD_ARR(resultBasel2, 5), 
            FIELD_ARR(resultGirrDelta, 23), FIELD_ARR(resultCsrDelta, 13),
            FIELD_ARR(resultGirrCvr, 2), FIELD_ARR(resultCsrCvr, 2), 
            FIELD_ARR(resultCashFlow, 1000)
        );
        /* 로그 종료 */
        LOG_END(result);
    });

    try {
        /* 로거 초기화 */
        disableConsoleLogging();
        if (logYn == 1) {
            LOG_START("bond");
        }

        /* Input Parameter 로그 출력 */
        LOG_INPUT(
            FIELD_VAR(evaluationDate), FIELD_VAR(issueDate), FIELD_VAR(maturityDate), FIELD_VAR(notional),
            FIELD_VAR(numberOfGirrTenors), FIELD_ARR(girrTenorDays, numberOfGirrTenors), FIELD_ARR(girrRates, numberOfGirrTenors), FIELD_ARR(girrConvention, 4),
            FIELD_VAR(spreadOverYield), 
            FIELD_VAR(numberOfCsrTenors), FIELD_ARR(csrTenorDays, numberOfCsrTenors), FIELD_ARR(csrRates, numberOfCsrTenors),
            FIELD_VAR(marketPrice), FIELD_VAR(girrRiskWeight), FIELD_VAR(csrRiskWeight), 
            FIELD_VAR(calType), FIELD_VAR(logYn)
        );

        if (calType != 1 && calType != 2 && calType != 3 && calType != 4 && calType != 9) {
            error("Invalid calculation type. Only 1, 2, 3, 4, 9 are supported.");
            return result = -1.0; // Invalid calculation type
        }

        // Input Data Check
        // Maturity Date >= evaluation Date
        if (maturityDate < evaluationDate) {
            error("Maturity Date is less than evaluation Date.");
            return result = -1.0;
        }
        // Maturity Date >= issue Date
        if (maturityDate < issueDate) {
            error("Maturity Date is less than issue Date.");
            return result = -1.0;
        }

        // 결과 데이터 초기화
        initResult(resultBasel2, 5);
        initResult(resultGirrDelta, 23);
        initResult(resultCsrDelta, 13);
        initResult(resultGirrCvr, 2);
        initResult(resultCsrCvr, 2);
        initResult(resultCashFlow, 1000);

        LOG_MESSAGE_ENTER_PRICING();

        // revaluationDateSerial -> revaluationDate
        Date asOfDate_ = Date(evaluationDate);
        const Date issueDate_ = Date(issueDate);
        const Date maturityDate_ = Date(maturityDate);

        // 전역 Settings에 평가일을 설정 (이후 모든 계산에 이 날짜 기준 적용)
        Settings::instance().evaluationDate() = asOfDate_;
        Size settlementDays_ = 0;
        bool includeSettlementDateFlows_ = true;

        Real notional_ = notional;

        // GIRR 커브 구성용 날짜 및 금리 벡터
        std::vector<Date> girrDates_;
        std::vector<Real> girrRates_;

        // GIRR 커브의 주요 기간 설정 (시장 표준 테너)
        std::vector<Period> girrPeriod =
            makePeriodArrayFromTenorDaysArray(girrTenorDays, numberOfGirrTenors);

        // GIRR 커브 시작점 (revaluationDate 기준) 입력
        girrDates_.emplace_back(asOfDate_);
        girrRates_.emplace_back(girrRates[0]);

        // 나머지 GIRR 커브 구성 요소 입력
        for (Size dateNum = 0; dateNum < numberOfGirrTenors; ++dateNum) {
            girrDates_.emplace_back(asOfDate_ + girrPeriod[dateNum]);
            girrRates_.emplace_back(girrRates[dateNum]);
        }

        // GIRR 커브 계산 사용 추가 요소
        DayCounter girrDayCounter_ = makeDayCounterFromInt(girrConvention[0]); // DCB
        Linear girrInterpolator_ = Linear(); // 보간 방식, TODO 변환 함수 적용 (Interpolator)
        Compounding girrCompounding_ = makeCompoundingFromInt(girrConvention[2]); // 이자 계산 방식(Compounding)
        Frequency girrFrequency_ = makeFrequencyFromInt(girrConvention[3]); // 이자 지급 빈도(Frequency)

        // GIRR 커브 생성
        ext::shared_ptr<YieldTermStructure> girrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, girrRates_,
            girrDayCounter_, girrInterpolator_, girrCompounding_, girrFrequency_);

        // GIRR 커브를 RelinkableHandle에 연결
        RelinkableHandle<YieldTermStructure> girrCurve;
        girrCurve.linkTo(girrTermstructure);
        girrCurve->enableExtrapolation();

        // spreadOverYield 값을 interest Rate 객체로 래핑 (CSR 계산용)
        double tmpSpreadOverYield = spreadOverYield;
        Compounding spreadOverYieldCompounding_ = Compounding::Continuous; // 이자 계산 방식, Continuous만 지원
        DayCounter spreadOverYieldDayCounter_ = Actual365Fixed();  // DayCounter Actual/365 Fixed만 지원
        InterestRate tempRate(tmpSpreadOverYield, spreadOverYieldDayCounter_, spreadOverYieldCompounding_, Frequency::Annual);

        // CSR 커브 구성용 날짜 및 금리 벡터
        std::vector<Date> csrDates_;
        std::vector<Period> csrPeriod =
            makePeriodArrayFromTenorDaysArray(csrTenorDays, numberOfCsrTenors);

        // CSR 커브 시작점 (revaluationDate 기준) 입력
        csrDates_.emplace_back(asOfDate_);

        // CSR 스프레드를 담을 핸들 벡터
        std::vector<Handle<Quote>> csrSpreads_;

        // 첫번째 CSR 스프레드에 spreadOverYield 값을 설정
        QL_REQUIRE(!csrPeriod.empty(), "csrPeriod is empty.");
        double spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_,
            girrDayCounter_.yearFraction(asOfDate_, asOfDate_ + 1));
        csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(spreadOverYield_));

        // 나머지 CSR 커브 기간별 스프레드 입력
        for (Size dateNum = 0; dateNum < numberOfCsrTenors; ++dateNum) {
            csrDates_.emplace_back(asOfDate_ + csrPeriod[dateNum]);
            spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_, girrDayCounter_.yearFraction(asOfDate_, csrDates_.back()));
            csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrRates[dateNum] + spreadOverYield_));
        }

        // GIRR + CSR 스프레드 커브 생성
        ext::shared_ptr<ZeroYieldStructure> discountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, csrSpreads_, csrDates_);

        // Discounting 커브 연결
        RelinkableHandle<YieldTermStructure> discountingCurve;
        discountingCurve.linkTo(discountingTermStructure);
        discountingCurve->enableExtrapolation(); // 외삽 허용

        // Discounting 엔진 생성 (채권 가격 계산용)
        auto bondEngine = ext::make_shared<DiscountingBondEngine>(discountingCurve, includeSettlementDateFlows_);

        // ZeroCouponBond 객체 생성
        Calendar couponCalendar_ = NullCalendar(); // TODO 변환 함수 적용(Calendar)
        ZeroCouponBond zeroCouponBond(
            settlementDays_,
            couponCalendar_,
            notional_,
            maturityDate_,
            ModifiedFollowing, // Business Day Convention
            100.0,
            issueDate_);

        if (calType == 9) {
            // Calc Spread Over Yield
            std::vector<Handle<Quote>> tmpCsrSpreads_;
            tmpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(0.0));
            for (Size dateNum = 0; dateNum < numberOfCsrTenors; ++dateNum) {
                tmpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrRates[dateNum]));
            }
            ext::shared_ptr<ZeroYieldStructure> tmpDiscountingTermStructure =
                ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, tmpCsrSpreads_, csrDates_);
            RelinkableHandle<YieldTermStructure> tmpDiscountingCurve;
            tmpDiscountingCurve.linkTo(tmpDiscountingTermStructure);
            tmpDiscountingCurve->enableExtrapolation();

            auto tmpBondEngine = ext::make_shared<DiscountingBondEngine>(tmpDiscountingCurve, includeSettlementDateFlows_);
            zeroCouponBond.setPricingEngine(tmpBondEngine);
            // SettlementDays 관행 무시(Algo
            Real soy = CashFlows::zSpread(zeroCouponBond.cashflows(), marketPrice, *tmpDiscountingCurve, Actual365Fixed(), Continuous, Annual,
                includeSettlementDateFlows_, asOfDate_, asOfDate_, 1.0e-10, 100, 0.005);  // settlementDate 산출 로직 제거(20250822, jwlee)
            //        Real soy = CashFlows::zSpread(fixedRateBond.cashflows(), marketPrice, *tmpDiscountingCurve, Actual365Fixed(), Continuous, Annual,
            //                                      false, asOfDate_, couponCalendar.advance(asOfDate_, Period(settlementDays, Days)), 1.0e-10, 100, 0.005);
            return result = soy;
        }
        // Fixed Rate Bond에 Discounting 엔진 연결
        zeroCouponBond.setPricingEngine(bondEngine);

        // 채권 가격 Net PV 계산
        Real npv = zeroCouponBond.NPV();

        // 이론가 산출의 경우 GIRR Delta 산출을 하지 않음
        if (calType == 1) {
            return result = npv;
        }

        if (calType == 2) {
            // Delta 계산
            Real bumpSize = 0.0001; // bumpSize를 0.0001 이외의 값으로 적용 시, PV01 산출을 독립적으로 구현해줘야 함
            std::vector<Real> bumpGearings{ 1.0, -1.0 };
            std::vector<Real> bumpedNpv(bumpGearings.size(), 0.0);
            for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                std::vector<Rate> bumpGirrRates = girrRates_;
                for (Size bumpTenorNum = 0; bumpTenorNum < girrRates_.size(); ++bumpTenorNum) {
                    // GIRR 커브의 금리를 bumping (1bp 상승)
                    bumpGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * bumpSize;
                }

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates, girrDayCounter_, girrInterpolator_,
                    girrCompounding_, girrFrequency_);

                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation();

                // CSR Spread를 적용한 bump GIRR 기반 할인 커브 생성
                ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                zeroCouponBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                bumpedNpv[bumpNo] = zeroCouponBond.NPV();

            }

            QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            Real delta = (bumpedNpv[0] - npv) / bumpSize;
            Real gamma = (bumpedNpv[0] - 2.0 * npv + bumpedNpv[1]) / (bumpSize * bumpSize);

            const DayCounter& ytmDayCounter = Actual365Fixed();
            Compounding ytmCompounding = Compounded;//Continuous;
            Frequency ytmFrequency = Annual;
            Rate ytmValue = 0.00000000000001;
            if (asOfDate_ < maturityDate_) {
                ytmValue = CashFlows::yield(zeroCouponBond.cashflows(), npv, ytmDayCounter, ytmCompounding, ytmFrequency, false,
                    couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), asOfDate_,
                    1.0e-15, 100, 0.005);
            }
            InterestRate ytm = InterestRate(ytmValue, ytmDayCounter, ytmCompounding, ytmFrequency);

            // Duration 계산
            Duration::Type durationType = Duration::Macaulay;//Duration::Modified;
            Real duration = CashFlows::duration(zeroCouponBond.cashflows(), ytm, durationType, false,
                couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), asOfDate_);

            Real convexity = CashFlows::convexity(zeroCouponBond.cashflows(), ytm, false,
                couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), asOfDate_);

            Real PV01 = delta * bumpSize;

            resultBasel2[0] = delta;
            resultBasel2[1] = gamma;
            resultBasel2[2] = duration;
            resultBasel2[3] = convexity;
            resultBasel2[4] = PV01;

            return result = npv;
        }

        if (calType == 3) {
            // GIRR Bump Rate 설정
            Real girrBump = 0.0001;

            // GIRR Delta 적재용 벡터 생성
            std::vector<Real> disCountingGirr;

            // GIRR Delta 계산
            for (Size bumpNum = 1; bumpNum < girrRates_.size(); ++bumpNum) {
                // GIRR 커브의 금리를 bumping (1bp 상승)
                std::vector<Rate> bumpGirrRates = girrRates_;
                if (bumpNum == 1) {
                    bumpGirrRates[0] += girrBump; // 0번째 tenor도 같이 bump 적용
                }
                bumpGirrRates[bumpNum] += girrBump;

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates, girrDayCounter_, girrInterpolator_, girrCompounding_, girrFrequency_);

                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation();

                // CSR Spread를 적용한 bump GIRR 기반 할인 커브 생성
                ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                zeroCouponBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                Real tmpGirr = (zeroCouponBond.NPV() - npv) * 10000;

                // 산출된 Girr Delta 값을 벡터에 추가
                disCountingGirr.emplace_back(tmpGirr);
            }

            /* OUTPUT 2. GIRR Delta 결과 적재 */
            std::vector<Real> girrTenor = { 0.0, 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 15.0, 20.0, 30.0 };
            Size girrDataSize = girrTenor.size();

            // Parallel 민감도 추가
            double tmpCcyDelta = std::accumulate(disCountingGirr.begin(), disCountingGirr.end(), 0.0);
            disCountingGirr.insert(disCountingGirr.begin(), tmpCcyDelta);
            QL_REQUIRE(girrDataSize == disCountingGirr.size(), "Girr result Size mismatch.");

            // 0인 민감도를 제외하고 적재
            processResultArray(girrTenor, disCountingGirr, girrDataSize, resultGirrDelta);

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
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                zeroCouponBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (CSR Delta)
                Real tmpCsr = (zeroCouponBond.NPV() - npv) * 10000;

                // 산출된 CSR Delta 값을 벡터에 추가
                disCountingCsr.emplace_back(tmpCsr);
            }

            /* OUTPUT 3. CSR Delta 결과 적재 */
            std::vector<Real> csrTenor = { 0.5, 1.0, 3.0, 5.0, 10.0 };
            Size csrDataSize = csrTenor.size();
            // 0인 민감도를 제외하고 적재
            processResultArray(csrTenor, disCountingCsr, csrDataSize, resultCsrDelta);

            Real totalGirr = 0;
            for (const auto& girr : disCountingGirr) {
                totalGirr += girr;
            }

            // Curvature 계산
            Real curvatureRW = girrRiskWeight; // bumpSize를 FRTB 기준서의 Girr Curvature RiskWeight로 설정
            std::vector<Real> bumpGearings{ 1.0, -1.0 };
            std::vector<Real> bumpedNpv(bumpGearings.size(), 0.0);
            for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                std::vector<Rate> bumpGirrRates = girrRates_;
                for (Size bumpTenorNum = 0; bumpTenorNum < girrRates_.size(); ++bumpTenorNum) {
                    // GIRR 커브의 금리를 bumping (RiskWeight 만큼 상승)
                    bumpGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * curvatureRW;
                }

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates, girrDayCounter_, girrInterpolator_,
                    girrCompounding_, girrFrequency_);

                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation();

                // CSR Spread를 적용한 bump GIRR 기반 할인 커브 생성
                ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                zeroCouponBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                bumpedNpv[bumpNo] = zeroCouponBond.NPV();
            }

            QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            resultGirrCvr[0] = (bumpedNpv[0] - npv);
            resultGirrCvr[1] = (bumpedNpv[1] - npv);

            Real totalCsr = 0;
            for (const auto& csr : disCountingCsr) {
                totalCsr += csr;
            }

            // Curvature 계산
            curvatureRW = csrRiskWeight; // bumpSize를 FRTB 기준서의 CSR Bucket의 Curvature RiskWeight로 설정
            for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                std::vector<Handle<Quote>> bumpCsrSpreads_;
                for (Size bumpTenorNum = 0; bumpTenorNum < csrSpreads_.size(); ++bumpTenorNum) {
                    // Csr 커브의 금리를 bumping (RiskWeight 만큼 상승)
                    bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[bumpTenorNum]->value() +
                        bumpGearings[bumpNo] * curvatureRW));
                }

                // GIRR 커브에 bump된 CSR Spread를 적용한 할인 커브 생성
                ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure =
                    ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, bumpCsrSpreads_, csrDates_);

                // 새로운 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용


                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve, includeSettlementDateFlows_);

                // FixedRateBond에 bump된 pricing engine 연결
                zeroCouponBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                bumpedNpv[bumpNo] = zeroCouponBond.NPV();
            }

            QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            resultCsrCvr[0] = (bumpedNpv[0] - npv);
            resultCsrCvr[1] = (bumpedNpv[1] - npv);

            /* OUTPUT 1. Net PV 리턴 */
            return result = npv;
        }

        if (calType == 4) {
            const Leg& bondCFs = zeroCouponBond.cashflows();
            Size numberOfCoupons = bondCFs.size();
            Size numberOfFields = 7;
            Size n_startDateField = 1;
            Size n_endDateField = 2;
            Size n_notionalField = 3;
            Size n_rateField = 4;
            Size n_payDateField = 5;
            Size n_CFField = 6;
            Size n_DFField = 7;

            resultCashFlow[0] = static_cast<double>(numberOfCoupons);
            for (Size couponNum = 0; couponNum < numberOfCoupons; ++couponNum) {
                const auto& redemption = ext::dynamic_pointer_cast<Redemption>(bondCFs[couponNum]);
                if (redemption != nullptr) {
                    // redemption is the last cashflow
                    resultCashFlow[couponNum * numberOfFields + n_startDateField] = -1.0;
                    resultCashFlow[couponNum * numberOfFields + n_endDateField] = -1.0;
                    resultCashFlow[couponNum * numberOfFields + n_notionalField] = redemption->amount();
                    resultCashFlow[couponNum * numberOfFields + n_rateField] = -1.0;
                    resultCashFlow[couponNum * numberOfFields + n_payDateField] = static_cast<double>(redemption->date().serialNumber());
                    resultCashFlow[couponNum * numberOfFields + n_CFField] = redemption->amount();
					if (asOfDate_ < redemption->date()) {
						resultCashFlow[couponNum * numberOfFields + n_DFField] = 0.0;
					}
					else {
						resultCashFlow[couponNum * numberOfFields + n_DFField] = discountingCurve->discount(redemption->date());
					}
                }
                else {
                    QL_FAIL("Coupon is not a Redemption.");
                }
            }
            return result = npv;
        }

        // NPV : clean, dirty, accured Interest
        Real cleanPrice = zeroCouponBond.cleanPrice() / 100.0 * notional;
        Real dirtyPrice = zeroCouponBond.dirtyPrice() / 100.0 * notional;
        Real accruedInterest = zeroCouponBond.accruedAmount() / 100.0 * notional;

        /* OUTPUT 1. Net PV 리턴 */
        return result = npv;
    }
    catch (...) {
        try {
            std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& e) {
			LOG_ERROR_KNOWN_EXCEPTION(std::string(e.what()));
            return result = -1.0;
        }
        catch (...) {
			LOG_ERROR_UNKNOWN_EXCEPTION();
            return result = -1.0;
        }
    }
}