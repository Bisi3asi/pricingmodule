#include "bond.h"
#include "logger.hpp"
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
    /* TODO / NOTE */
    // 1. DayCounter, Frequency 등 QuantLib Class Get 함수 정의 필요

    try {
        /* 로거 초기화 */
        disableConsoleLogging();    // 로깅여부 N일시 콘촐 입출력 비활성화
        if (logYn == 1) {
            initLogger("bond.log"); // 생성 파일명 지정
        }

        info("==============[Bond: pricingFRB Logging Started!]==============");
        // INPUT 데이터 로깅
        printAllInputDataFRB(
            evaluationDate, issueDate, maturityDate, notional,
            couponRate, couponDayCounter, couponCalendar, couponFrequency, scheduleGenRule, paymentBDC, paymentLag,
            numberOfCoupons, paymentDates, realStartDates, realEndDates,
            numberOfGirrTenors, girrTenorDays, girrRates, girrConvention,
            spreadOverYield, numberOfCsrTenors, csrTenorDays, csrRates,
            marketPrice, girrRiskWeight, csrRiskWeight,
            calType
        );
        if (calType != 1 && calType != 2 && calType != 3 && calType != 4 && calType != 9) {
            error("[princingFRB]: Invalid calculation type. Only 1, 2, 3, 4, 9 are supported.");
            return -1; // Invalid calculation type
        }

        // Input Data Check
        // Maturity date >= evaluation Date
        if (maturityDate < evaluationDate) {
            error("[princingFRB]: Maturity Date is less than evaluation Date");
            return -1;
        }
        // Maturity Date >= issue Date
        if (maturityDate < issueDate) {
            error("[pricingFRB]: Maturity Date is less than issue Date");
            return -1;
        }
        // Last Payment date >= evaluation Date
        if ((numberOfCoupons > 0) && (paymentDates[numberOfCoupons - 1] < evaluationDate)) {
            error("[princingFRB]: PaymentDate Date is less than evaluation Date");
            return -1;
        }

        // 결과 데이터 초기화
        initResult(resultBasel2, 5);
        initResult(resultGirrDelta, 23);
        initResult(resultCsrDelta, 13);
        initResult(resultGirrCvr, 2);
        initResult(resultCsrCvr, 2);
        initResult(resultCashFlow, 1000);

        // revaluationDateSerial -> revaluationDate
        Date asOfDate_ = Date(evaluationDate);
        const Date issueDate_ = Date(issueDate);

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
        QL_REQUIRE(!csrPeriod.empty(), "[pricingFRB]csrPeriod is empty");
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

        // Discounting 엔진 생성 (채권 가격 계산용)
        auto bondEngine = ext::make_shared<DiscountingBondEngine>(discountingCurve, includeSettlementDateFlows_);

        // Schedule 객체 생성 (지급일)
        Schedule fixedBondSchedule_;
        BusinessDayConvention paymentBDC_ = makeBDCFromInt(paymentBDC); // 지급일 휴일 적용 기준  
        DayCounter couponDayCounter_ = makeDayCounterFromInt(couponDayCounter); // 쿠폰 이자 계산을 위한 일수계산 방식 설정
        Calendar couponCalendar_ = makeCalendarFromInt(couponCalendar);

        // Coupon Schedule 생성
        if (numberOfCoupons > 0) { // 쿠폰 스케줄이 인자로 들어오는 경우
            //        info("[Coupon Schedule]: PARAMETER INPUT");

            std::vector<Date> couponSch_;
            couponSch_.emplace_back(realStartDates[0]);
            for (Size schNum = 0; schNum < numberOfCoupons; ++schNum) {
                couponSch_.emplace_back(realEndDates[schNum]);
            }
            fixedBondSchedule_ = Schedule(couponSch_);
        }
        else {  // 쿠폰 스케줄이 인자로 들어오지 않는 경우, 스케줄을 직접 생성
            //        info("[Coupon Schedule]: GENERATED BY MODULE");

            //Date effectiveDate = Date(realStartDates[0]); // 쿠폰 첫번째 시작일로 수정
            Date effectiveDate = Date(issueDate); // 기존 코드
            Calendar couponCalendar_ = makeCalendarFromInt(couponCalendar);
            Frequency couponFrequency_ = makeFrequencyFromInt(couponFrequency); // Period(Tenor)형태도 가능
            DateGeneration::Rule genRule = makeScheduleGenRuleFromInt(scheduleGenRule); // Forward, Backward 등


            fixedBondSchedule_ = MakeSchedule().from(effectiveDate)
                .to(Date(maturityDate))
                .withFrequency(couponFrequency_)
                .withCalendar(couponCalendar_)
                .withConvention(paymentBDC_)
                .withRule(genRule); // payment Lag는 Bond 스케쥴 생성 시 적용
        }

        /* FOR DEBUG */
        //    printAllData(fixedBondSchedule_);

        Integer paymentLag_ = paymentLag;

        Real redemptionRatio = 100.0;
        // FixedRateBond 객체 생성
        FixedRateBondCustom fixedRateBond(
            settlementDays_,
            notional_,
            fixedBondSchedule_,
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
            auto tmpBondEngine = ext::make_shared<DiscountingBondEngine>(tmpDiscountingCurve, includeSettlementDateFlows_);
            fixedRateBond.setPricingEngine(tmpBondEngine);
            // SettlementDays 관행 무시(Algo
            Real soy = CashFlows::zSpread(fixedRateBond.cashflows(), marketPrice, *tmpDiscountingCurve, Actual365Fixed(), Continuous, Annual,
                false, asOfDate_, couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), 1.0e-10, 100, 0.005);
            //        Real soy = CashFlows::zSpread(fixedRateBond.cashflows(), marketPrice, *tmpDiscountingCurve, Actual365Fixed(), Continuous, Annual,
            //                                      false, asOfDate_, couponCalendar.advance(asOfDate_, Period(settlementDays, Days)), 1.0e-10, 100, 0.005);

            // OUTPUT 데이터 로깅
            printAllOutputDataFRB(soy, resultBasel2, resultGirrDelta, resultCsrDelta, resultGirrCvr, resultCsrCvr, calType);
            /* OUTPUT SpreadOverYield 리턴 */
            info("==============[Bond: pricingFRB Logging Ended!]==============");
            return soy;
        }
        // Fixed Rate Bond에 Discounting 엔진 연결
        fixedRateBond.setPricingEngine(bondEngine);

        // 채권 가격 Net PV 계산
        Real npv = fixedRateBond.NPV();

        // 이론가 산출의 경우 GIRR Delta 산출을 하지 않음
        if (calType == 1) {
            // OUTPUT 데이터 로깅
            printAllOutputDataFRB(npv, resultBasel2, resultGirrDelta, resultCsrDelta, resultGirrCvr, resultCsrCvr, calType);
            /* OUTPUT 1. Net PV 리턴 */
            info("==============[Bond: pricingFRB Logging Ended!]==============");
            return npv;
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
            Frequency ytmFrequency = Semiannual;//Annual;
            Rate ytmValue = CashFlows::yield(fixedRateBond.cashflows(), npv, ytmDayCounter, ytmCompounding, ytmFrequency, false,
                couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), asOfDate_,
                1.0e-15, 100, 0.005);
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

            // OUTPUT 데이터 로깅
            printAllOutputDataFRB(npv, resultBasel2, resultGirrDelta, resultCsrDelta, resultGirrCvr, resultCsrCvr, calType);
            /* OUTPUT 1. Net PV 리턴 */
            info("==============[Bond: pricingFRB Logging Ended!]==============");
            return npv;
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
            QL_REQUIRE(girrDataSize == disCountingGirr.size(), "[pricingFRB]Girr result Size mismatch.");

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

            // OUTPUT 데이터 로깅
            printAllOutputDataFRB(npv, resultBasel2, resultGirrDelta, resultCsrDelta, resultGirrCvr, resultCsrCvr, calType);
            /* OUTPUT 1. Net PV 리턴 */
            info("==============[Bond: pricingFRB Logging Ended!]==============");
            return npv;
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
                    resultCashFlow[couponNum * numberOfFields + n_DFField] = discountingCurve->discount(cp->date());

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
                        resultCashFlow[couponNum * numberOfFields + n_DFField] = discountingCurve->discount(redemption->date());

                    }
                    else {
                        QL_FAIL("[pricingFRB]Coupon is not a FixedRateCoupon");
                    }
                }
            }
            return npv;
        }


        // NPV : clean, dirty, accured Interest
        Real cleanPrice = fixedRateBond.cleanPrice() / 100.0 * notional;
        Real dirtyPrice = fixedRateBond.dirtyPrice() / 100.0 * notional;
        Real accruedInterest = fixedRateBond.accruedAmount() / 100.0 * notional;

        //    printAllOutputDataFRB(npv, resultGirrDelta, resultCsrDelta);
        //    info("==============[Bond: pricingFRB Logging Ended!]==============");

        // OUTPUT 데이터 로깅
        printAllOutputDataFRB(npv, resultBasel2, resultGirrDelta, resultCsrDelta, resultGirrCvr, resultCsrCvr, calType);
        /* OUTPUT 1. Net PV 리턴 */
        info("==============[Bond: pricingFRB Logging Ended!]==============");
        return npv;
    }
    catch (...) {
        try {
            std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& e) {
            error("Exception occurred: {}", e.what());
            return -1;
        }
        catch (...) {
            error("[pricingFRB]: Unknown exception occurred.");
            return -1;
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
     QL_ENSURE(redemptions_.size() == 1, "multiple redemptions created");
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
    /* TODO / NOTE */
    // 1. DayCounter, Frequency 등 QuantLib Class Get 함수 정의 필요

    try {
        /* 로거 초기화 */
        disableConsoleLogging();    // 로깅여부 N일시 콘촐 입출력 비활성화
        if (logYn == 1) {
            initLogger("bond.log"); // 생성 파일명 지정
        }

        // info("==============[Bond: pricingFRN Logging Started!]==============");
        // INPUT 데이터 로깅
       // printAllInputDataFRN(
       //     evaluationDate,
       //     settlementDays,
       //     issueDate,
       //     maturityDate,
       //     notional,
       //     couponDayCounter,

       //     referenceIndex,
       //     fixingDays,
       //     gearing,
       //     spread,
       //     lastResetRate,
       //     nextResetRate,

       //     numberOfCoupons,
       //     paymentDates,
       //     realStartDates,
       //     realEndDates,

       //     spreadOverYield,
       //     spreadOverYieldCompounding,
       //     spreadOverYieldDayCounter,

       //     numberOfGirrTenors,
       //     girrTenorDays,
       //     girrRates,

       //     girrDayCounter,
       //     girrInterpolator,
       //     girrCompounding,
       //     girrFrequency,

       //     numberOfCsrTenors,
       //     csrTenorDays,
       //     csrRates,

       //     numberOfIndexGirrTenors,
       //     indexGirrTenorDays,
       //     indexGirrRates,

       //     indexGirrDayCounter,
       //     indexGirrInterpolator,
       //     indexGirrCompounding,
       //     indexGirrFrequency,

       //     indexTenorNumber,
       //     indexTenorUnit,
       //     indexFixingDays,
       //     indexCurrency,
       //     indexCalendar,
       //     indexBDC,
       //     indexEOM,
       //     indexDayCounter,

       //     calType
       // );

        if (calType != 1 && calType != 2 && calType != 3 && calType != 4 && calType != 9) {
            error("[pricingFRN]: Invalid calculation type. Only 1, 2, 3, 4, 9 are supported.");
            return -1; // Invalid calculation type
        }

        // Input Data Check
        // Maturity date >= evaluation Date
        if (maturityDate < evaluationDate) {
            error("[princingFRN]: Maturity Date is less than evaluation Date");
            return -1;
        }
        // Maturity Date >= issue Date
        if (maturityDate < issueDate) {
            error("[pricingFRN]: Maturity Date is less than issue Date");
            return -1;
        }
        // Last Payment date >= evaluation Date
        if ((numberOfCoupons > 0) && (paymentDates[numberOfCoupons - 1] < evaluationDate)) {
            error("[princingFRN]: PaymentDate Date is less than evaluation Date");
            return -1;
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
        for (Size dateNum = 0; dateNum < numberOfGirrTenors; ++dateNum) {
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
        QL_REQUIRE(!csrPeriod.empty(), "[pricingFRB]csrPeriod is empty");
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

        // Discounting 엔진 생성 (채권 가격 계산용)
        auto bondEngine = ext::make_shared<DiscountingBondEngine>(discountingCurve, includeSettlementDateFlows_);

        // Schedule 객체 생성 (지급일)
        Schedule FRNSchedule_;
        BusinessDayConvention couponBDC_ = makeBDCFromInt(paymentBDC); // 지급일 휴일 적용 기준

        // Coupon Schedule 생성
        if (numberOfCoupons > 0) { // 쿠폰 스케줄이 인자로 들어오는 경우
            //        info("[Coupon Schedule]: PARAMETER INPUT");

            std::vector<Date> couponSch_;
            couponSch_.emplace_back(realStartDates[0]);
            for (Size schNum = 0; schNum < numberOfCoupons; ++schNum) {
                couponSch_.emplace_back(realEndDates[schNum]);
            }
            FRNSchedule_ = Schedule(couponSch_);
        }
        else {  // 쿠폰 스케줄이 인자로 들어오지 않는 경우, 스케줄을 직접 생성
            //        info("[Coupon Schedule]: GENERATED BY MODULE");

                    //Date effectiveDate = Date(realStartDates[0]); // 쿠폰 첫번째 시작일로 수정
            Date effectiveDate = Date(issueDate); // 기존 코드
            DateGeneration::Rule genRule = makeScheduleGenRuleFromInt(scheduleGenRule); // Forward, Backward 등


            FRNSchedule_ = MakeSchedule().from(effectiveDate)
                .to(Date(maturityDate))
                .withFrequency(couponFrequency_)
                .withCalendar(couponCalendar_)
                .withConvention(couponBDC_)
                .withRule(genRule);
        }

        // fixing data 입력
        Date lastFixingDate1 = refIndex->fixingDate(Date(realStartDates[0]));
        Date nextFixingDate1 = refIndex->fixingDate(Date(realStartDates[1]));
        refIndex->addFixing(lastFixingDate1, lastResetRate);
        refIndex->addFixing(nextFixingDate1, nextResetRate);

        // FixedRateBond 객체 생성
        FloatingRateBondCustom floatingRateBond(
            settlementDays_,
            notional_,
            FRNSchedule_,
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
            // OUTPUT 데이터 로깅
            printAllOutputDataFRN(npv, resultGirrDelta, resultIndexGirrDelta, resultCsrDelta);
            /* OUTPUT 1. Net PV 리턴 */
            // info("==============[Bond: pricingFRN Logging Ended!]==============");
            return npv;
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
            auto tmpBondEngine = ext::make_shared<DiscountingBondEngine>(tmpDiscountingCurve, includeSettlementDateFlows_);
            floatingRateBond.setPricingEngine(tmpBondEngine);
            // SettlementDays 관행 무시
            Real soy = CashFlows::zSpread(floatingRateBond.cashflows(), marketPrice, *tmpDiscountingCurve, Actual365Fixed(), Continuous, Annual,
                false, asOfDate_, couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), 1.0e-10, 100, 0.005);
            //        Real soy = CashFlows::zSpread(fixedRateBond.cashflows(), marketPrice, *tmpDiscountingCurve, Actual365Fixed(), Continuous, Annual,
            //                                      false, asOfDate_, couponCalendar.advance(asOfDate_, Period(settlementDays, Days)), 1.0e-10, 100, 0.005);

            return soy;
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
            Real delta = (bumpedNpv[0] - npv) / bumpSize;
            Real gamma = (bumpedNpv[0] - 2.0 * npv + bumpedNpv[1]) / (bumpSize * bumpSize);

            //        const DayCounter& ytmDayCounter = Actual365Fixed();
            //        Compounding ytmCompounding = Continuous;
            //        Frequency ytmFrequency = Annual;
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
            QL_REQUIRE(girrDataSize == disCountingGirr.size(), "[pricingFRN]Girr result Size mismatch.");
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
                    bumpGirrRates[bumpNum] += girrBump;

                    // bump된 금리로 새로운 ZeroCurve 생성
                    ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(indexGirrDates_, bumpGirrRates, indexGirrDayCounter_
                        , indexGirrInterpolator_, indexGirrCompounding_, indexGirrFrequency_);

                    // RelinkableHandle에 bump된 커브 연결
                    RelinkableHandle<YieldTermStructure> bumpGirrCurve;
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
                QL_REQUIRE(indexGirrDataSize == indexGirr.size(), "[pricingFRN]Girr result Size mismatch.");

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

            //       printAllOutputDataFRN(npv, resultGirrDelta, resultIndexGirrDelta, resultCsrDelta);
            //       info("==============[Bond: pricingFRN Logging Ended!]==============");

                    /* OUTPUT 1. Net PV 리턴 */
            return npv;
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
                    resultCashFlow[couponNum * numberOfFields + n_DFField] = discountingCurve->discount(cp->date());

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
                        resultCashFlow[couponNum * numberOfFields + n_DFField] = discountingCurve->discount(redemption->date());

                    }
                    else {
                        QL_FAIL("[pricingFRN]Coupon is not a FloatingRateCoupon");
                    }
                }
            }

            return npv;
        }

        return npv;
    }
    catch (...) {
        try {
            std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& e) {
            error("Exception occurred: {}", e.what());
            return -1;
        }
        catch (...) {
            error("[pricingFRN]: Unknown exception occurred.");
            return -1;
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
     QL_ENSURE(redemptions_.size() == 1, "multiple redemptions created");

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
    /* TODO / NOTE */
    // 1. 로거, 로깅 로직 함수 정의 필요

    try {
        if (calType != 1 && calType != 2 && calType != 3 && calType != 4 && calType != 9) {
            error("[princingZCB]: Invalid calculation type. Only 1, 2, 3, 4, 9 are supported.");
            return -1; // Invalid calculation type
        }

        // Input Data Check
        // Maturity date >= evaluation Date
        if (maturityDate < evaluationDate) {
            error("[princingZCB]: Maturity Date is less than evaluation Date");
            return -1;
        }
        // Maturity Date >= issue Date
        if (maturityDate < issueDate) {
            error("[pricingZCB]: Maturity Date is less than issue Date");
            return -1;
        }


        // 결과 데이터 초기화
        initResult(resultBasel2, 5);
        initResult(resultGirrDelta, 23);
        initResult(resultCsrDelta, 13);
        initResult(resultGirrCvr, 2);
        initResult(resultCsrCvr, 2);
        initResult(resultCashFlow, 1000);

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

        // spreadOverYiled 값을 interest Rate 객체로 래핑 (CSR 계산용)
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
        QL_REQUIRE(!csrPeriod.empty(), "[pricingFRB]csrPeriod is empty");
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
            auto tmpBondEngine = ext::make_shared<DiscountingBondEngine>(tmpDiscountingCurve, includeSettlementDateFlows_);
            zeroCouponBond.setPricingEngine(tmpBondEngine);
            // SettlementDays 관행 무시(Algo
            Real soy = CashFlows::zSpread(zeroCouponBond.cashflows(), marketPrice, *tmpDiscountingCurve, Actual365Fixed(), Continuous, Annual,
                false, asOfDate_, couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), 1.0e-10, 100, 0.005);
            //        Real soy = CashFlows::zSpread(fixedRateBond.cashflows(), marketPrice, *tmpDiscountingCurve, Actual365Fixed(), Continuous, Annual,
            //                                      false, asOfDate_, couponCalendar.advance(asOfDate_, Period(settlementDays, Days)), 1.0e-10, 100, 0.005);

            return soy;
        }
        // Fixed Rate Bond에 Discounting 엔진 연결
        zeroCouponBond.setPricingEngine(bondEngine);

        // 채권 가격 Net PV 계산
        Real npv = zeroCouponBond.NPV();

        // 이론가 산출의 경우 GIRR Delta 산출을 하지 않음
        if (calType == 1) {
            // OUTPUT 데이터 로깅
    //        printAllOutputDataZCB(npv, resultGirrDelta, resultCsrDelta);
            /* OUTPUT 1. Net PV 리턴 */
    //        info("==============[Bond: pricingFRB Logging Ended!]==============");
            return npv;
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
            Frequency ytmFrequency = Semiannual;//Annual;
            Rate ytmValue = CashFlows::yield(zeroCouponBond.cashflows(), npv, ytmDayCounter, ytmCompounding, ytmFrequency, false,
                couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), asOfDate_,
                1.0e-15, 100, 0.005);
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

            return npv;
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
            QL_REQUIRE(girrDataSize == disCountingGirr.size(), "[PricingZCB]Girr result Size mismatch.");

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

            QL_REQUIRE(bumpedNpv.size() > 1, "[PricingZCB]Failed to calculate bumpedNPV.");
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

            QL_REQUIRE(bumpedNpv.size() > 1, "[PricingZCB]Failed to calculate bumpedNPV.");
            resultCsrCvr[0] = (bumpedNpv[0] - npv);
            resultCsrCvr[1] = (bumpedNpv[1] - npv);

            /* OUTPUT 1. Net PV 리턴 */
            return npv;
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
                    resultCashFlow[couponNum * numberOfFields + n_DFField] = discountingCurve->discount(redemption->date());

                }
                else {
                    QL_FAIL("[PricingZCB]Coupon is not a Redemption");
                }
            }
            return npv;
        }

        // NPV : clean, dirty, accured Interest
        Real cleanPrice = zeroCouponBond.cleanPrice() / 100.0 * notional;
        Real dirtyPrice = zeroCouponBond.dirtyPrice() / 100.0 * notional;
        Real accruedInterest = zeroCouponBond.accruedAmount() / 100.0 * notional;

        //    printAllOutputDataFRB(npv, resultGirrDelta, resultCsrDelta);
        //    info("==============[Bond: pricingFRB Logging Ended!]==============");

        /* OUTPUT 1. Net PV 리턴 */
        return npv;
    }
    catch (...) {
        try {
            std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& e) {
            error("Exception occurred: {}", e.what());
            return -1;
        }
        catch (...) {
            error("[pricingZCB]: Unknown exception occurred.");
            return -1;
        }
    }
}

void printAllInputDataFRB(
    const int evaluationDate, const int issueDate, const int maturityDate, const double notional,
    const double couponRate, const int couponDayCounter, const int couponCalendar, const int couponFrequency,
    const int scheduleGenRule, const int paymentBDC, const int paymentLag,
    const int numberOfCoupons, const int* paymentDates, const int* realStartDates, const int* realEndDates,
    const int numberOfGirrTenors, const int* girrTenorDays, const double* girrRates, const int* girrConvention,
    const double spreadOverYield, const int numberOfCsrTenors, const int* csrTenorDays, const double* csrRates,
    const double marketPrice, const double girrRiskWeight, const double csrRiskWeight,
    const int calType
) {
    info("------------------------------------------------------------");
    info("[Print All - FRB Input Data]");

    info("evaluationDate: {}", evaluationDate);
    info("issueDate: {}", issueDate);
    info("maturityDate: {}", maturityDate);
    info("notional: {:0.5f}", notional);
    info("couponRate: {:0.5f}", couponRate);
    info("couponDayCounter: {}", couponDayCounter);
    info("couponCalendar: {}", couponCalendar);
    info("couponFrequency: {}", couponFrequency);
    info("scheduleGenRule: {}", scheduleGenRule);
    info("paymentBDC: {}", paymentBDC);
    info("paymentLag: {}", paymentLag);
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

    info("girrDayCounter: {}", girrConvention[0]);
    info("girrInterpolator: {}", girrConvention[1]);
    info("girrCompounding: {}", girrConvention[2]);
    info("girrFrequency: {}", girrConvention[3]);
    info("");

    info("spreadOverYield: {:0.5f}", spreadOverYield);
    info("");

    info("numberOfCsrTenors: {}", numberOfCsrTenors);
    logArrayLine("csrTenorDays", csrTenorDays, numberOfCsrTenors);
    logArrayLine("csrRates", csrRates, numberOfCsrTenors, 5);
    info("");

    info("marketPrice: {:0.5f}", marketPrice);
    info("girrRiskWeight: {:0.5f}", girrRiskWeight);
    info("csrRiskWeight: {:0.5f}", csrRiskWeight);
    info("calType: {}", calType);
    info("------------------------------------------------------------");
    info("");
}

void printAllOutputDataFRB(
    const double result,
    const double* resultBasel2,
    const double* resultGirrDelta,
    const double* resultCsrDelta,
    const double* resultGirrCvr,
    const double* resultCsrCvr,
    const int calType
) {

    info("[Print All - FRB Output Data]");

    // Net PV / Spread Over Yield
    if (calType != 9) {
        info("[Net PV]");
        info("Net Present Value: {:0.10f}", result);
        info("");
    }
    else {
        info("[Spread Over Yield]");
        info("Spread Over Yield: {:0.10f}", result);
        info("");
    }

    // Basel 2
    info("[Basel 2]");
    info("INDEX 0. Basel 2 Delta: {:0.10f}", resultBasel2[0]);
    info("INDEX 1. Basel 2 Gamma: {:0.10f}", resultBasel2[1]);
    info("INDEX 2. Basel 2 Duration: {:0.10f}", resultBasel2[2]);
    info("INDEX 3. Basel 2 Convexity: {:0.10f}", resultBasel2[3]);
    info("INDEX 4. Basel 2 PV01: {:0.10f}", resultBasel2[4]);
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
    info("");

    info("[Result GIRR CVR]");
    info("INDEX 0. BUMPUP CVR: {:0.10f}", resultGirrCvr[0]);
    info("INDEX 1. BUMPDOWN CVR: {:0.10f}", resultGirrCvr[1]);
    info("");

    info("[Result CSR CVR]");
    info("INDEX 0. BUMPUP CVR: {:0.10f}", resultCsrCvr[0]);
    info("INDEX 1. BUMPDOWN CVR: {:0.10f}", resultCsrCvr[1]);

    info("------------------------------------------------------------");
    info("");
}

void printAllInputDataFRN(
    const int evaluationDate,
    const int settlementDays,
    const int issueDate,
    const int maturityDate,
    const double notional,
    const int couponDayCounter,

    const int referenceIndex,
    const int fixingDays,
    const double gearing,
    const double spread,
    const double lastResetRate,
    const double nextResetRate,

    const int numberOfCoupons,
    const int* paymentDates,
    const int* realStartDates,
    const int* realEndDates,

    const double spreadOverYield,
    const int spreadOverYieldCompounding,
    const int spreadOverYieldDayCounter,

    const int numberOfGirrTenors,
    const int* girrTenorDays,
    const double* girrRates,

    const int girrDayCounter,
    const int girrInterpolator,
    const int girrCompounding,
    const int girrFrequency,

    const int numberOfCsrTenors,
    const int* csrTenorDays,
    const double* csrRates,

    const int numberOfIndexGirrTenors,
    const int* indexGirrTenorDays,
    const double* indexGirrRates,

    const int indexGirrDayCounter,
    const int indexGirrInterpolator,
    const int indexGirrCompounding,
    const int indexGirrFrequency,

    const int indexTenorNumber,
    const int indexTenorUnit,
    const int indexFixingDays,
    const int indexCurrency,
    const int indexCalendar,
    const int indexBDC,
    const int indexEOM,
    const int indexDayCounter,

    const int calType
) {
    info("------------------------------------------------------------");
    info("[Print All - FRN Input Data]");

    info("evaluationDate: {}", evaluationDate);
    info("settlementDays: {}", settlementDays);
    info("issueDate: {}", issueDate);
    info("maturityDate: {}", maturityDate);
    info("notional: {:0.5f}", notional);
    info("couponDayCounter: {}", couponDayCounter);
    info("");

    info("referenceIndex: {}", referenceIndex);
    info("fixingDays: {}", fixingDays);
    info("gearing: {:0.5f}", gearing);
    info("spread: {:0.5f}", spread);
    info("lastResetRate: {:0.5f}", lastResetRate);
    info("nextResetRate: {:0.5f}", nextResetRate);
    info("");

    info("numberOfCoupons: {}", numberOfCoupons);
    logArrayLine("paymentDates", paymentDates, numberOfCoupons);
    logArrayLine("realStartDates", realStartDates, numberOfCoupons);
    logArrayLine("realEndDates", realEndDates, numberOfCoupons);
    info("");

    info("spreadOverYield: {:0.5f}", spreadOverYield);
    info("spreadOverYieldCompounding: {}", spreadOverYieldCompounding);
    info("spreadOverYieldDayCounter: {}", spreadOverYieldDayCounter);
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

    info("numberOfCsrTenors: {}", numberOfCsrTenors);
    logArrayLine("csrTenorDays", csrTenorDays, numberOfCsrTenors);
    logArrayLine("csrRates", csrRates, numberOfCsrTenors, 10);
    info("");

    info("numberOfIndexGirrTenors: {}", numberOfIndexGirrTenors);
    logArrayLine("indexGirrTenorDays", indexGirrTenorDays, numberOfIndexGirrTenors);
    logArrayLine("indexGirrRates", indexGirrRates, numberOfIndexGirrTenors, 10);
    info("");

    info("indexGirrDayCounter: {}", indexGirrDayCounter);
    info("indexGirrInterpolator: {}", indexGirrInterpolator);
    info("indexGirrCompounding: {}", indexGirrCompounding);
    info("indexGirrFrequency: {}", indexGirrFrequency);
    info("");

    info("indexTenorNumber: {}", indexTenorNumber);
    info("indexTenorUnit: {}", indexTenorUnit);
    info("indexFixingDays: {}", indexFixingDays);
    info("indexCurrency: {}", indexCurrency);
    info("indexCalendar: {}", indexCalendar);
    info("indexBDC: {}", indexBDC);
    info("indexEOM: {}", indexEOM);
    info("indexDayCounter: {}", indexDayCounter);
    info("");

    info("calType: {}", calType);

    info("------------------------------------------------------------");
    info("");
}

void printAllOutputDataFRN(
    const double resultNetPV,
    const double* resultGirrDelta,
    const double* resultIndexGirrDelta,
    const double* resultCsrDelta
) {

    info("[Print All - FRB Output Data]");

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

    // Index GIRR Delta
    int indexGirrSize = static_cast<int>(resultIndexGirrDelta[0]);
    info("[Result Index GIRR Delta Size]");
    info("INDEX 0. Size: {}", indexGirrSize);
    info("");

    info("[Result Index GIRR Delta Tenor]");
    for (int i = 0; i < indexGirrSize; ++i) {
        info("INDEX {}. Tenor: {:0.2f}", i + 1, static_cast<double>(resultIndexGirrDelta[i + 1]));
    }
    info("");

    info("[Result Index GIRR Delta Sensitivity]");
    for (int i = 0; i < indexGirrSize; ++i) {
        info("INDEX {}. Sensitivity: {:0.10f}", i + 1 + indexGirrSize, resultIndexGirrDelta[i + 1 + indexGirrSize]);
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