#include "leg.h"
#include "logger.hpp"
#include "common.hpp"

using namespace QuantLib;
using namespace std;
using namespace logger;

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
    , double* resultGirrCvr			        // OUTPUT 5. (추가)GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
    , double* resultCashFlow                // OUTPUT 5. CF(index 0: size, index cfNum * 7 + 1 ~ cfNum * 7 + 7: 
    //              startDate, endDate, notional, rate, payDate, CF, DF)
// ===================================================================================================

) {
    try {
        /* 로거 초기화 */
        disableConsoleLogging();    // 로깅여부 N일시 콘촐 입출력 비활성화
        if (logYn == 1) {
            initLogger("leg.log"); // 생성 파일명 지정
        }

        if (calType != 1 && calType != 2 && calType != 3 && calType != 4) {
            error("[pricingZCL]: Invalid calculation type. Only 1, 2, 3, 4 are supported.");
            return -1; // Invalid calculation type
        }

        // 결과 데이터 초기화
        initResult(resultBasel2, 5);
        initResult(resultGirrDelta, 23);
        initResult(resultGirrCvr, 2);
        initResult(resultCashFlow, 1000);

        // revaluationDateSerial -> revaluationDate
        Date asOfDate_ = Date(evaluationDate);
        const Date issueDate_ = Date(issueDate);
        const Date maturityDate_ = Date(maturityDate);

        // 전역 Settings에 평가일을 설정 (이후 모든 계산에 이 날짜 기준 적용)
        Settings::instance().evaluationDate() = asOfDate_;
        Integer settlementDays_ = 0;

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


        // Discounting 엔진 생성 (채권 가격 계산용)
        auto bondEngine = ext::make_shared<DiscountingBondEngine>(girrCurve);


        // ZeroCouponBond 객체 생성
        Calendar couponCalendar_ = NullCalendar(); // CF가 하나이므로 Calendar는 NullCalendar로 설정
        ZeroCouponBond zeroCouponBond(
            settlementDays_,
            couponCalendar_,
            notional_,
            maturityDate_,
            ModifiedFollowing, // Business Day Convention, CF가 하나이므로 영향도 없음
            100.0,
            issueDate_);

        // Fixed Rate Bond에 Discounting 엔진 연결
        zeroCouponBond.setPricingEngine(bondEngine);

        // 채권 가격 Net PV 계산
        Real npv = zeroCouponBond.NPV();

        // 이론가 산출의 경우 GIRR Delta 산출을 하지 않음
        if (calType == 1) {
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
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpGirrTermstructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve);

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
            Duration::Type durationType = Duration::Macaulay; //Duration::Modified;
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

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpGirrTermstructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve);

                // FixedRateBond에 bump된 pricing engine 연결
                zeroCouponBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                Real tmpGirr = (zeroCouponBond.NPV() - npv) * 10000;

                // 산출된 Girr Delta 값을 벡터에 추가
                disCountingGirr.emplace_back(tmpGirr);
            }

            /* OUTPUT 2. GIRR Delta 결과 적재 */
            std::vector<Real> girrTenor = { 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 15.0, 20.0, 30.0 };
            Size girrDataSize = girrTenor.size();
            // 0인 민감도를 제외하고 적재
            processResultArray(girrTenor, disCountingGirr, girrDataSize, resultGirrDelta);


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

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
                bumpDiscountingCurve.linkTo(bumpGirrTermstructure);
                bumpDiscountingCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve);

                // FixedRateBond에 bump된 pricing engine 연결
                zeroCouponBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                bumpedNpv[bumpNo] = zeroCouponBond.NPV();
            }

            QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            resultGirrCvr[0] = (bumpedNpv[0] - npv);
            resultGirrCvr[1] = (bumpedNpv[1] - npv);


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
                    resultCashFlow[couponNum * numberOfFields + n_DFField] = girrCurve->discount(redemption->date());

                }
                else {
                    QL_FAIL("[pricingZCL]Coupon is not a Redemption");
                }
            }
            return npv;
        }

        // NPV : clean, dirty, accured Interest
        Real cleanPrice = zeroCouponBond.cleanPrice() / 100.0 * notional;
        Real dirtyPrice = zeroCouponBond.dirtyPrice() / 100.0 * notional;
        Real accruedInterest = zeroCouponBond.accruedAmount() / 100.0 * notional;

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
            error("[pricingZCL]: Unknown exception occurred.");
            return -1;
        }
    }
}


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
)
{
    try {
        /* 로거 초기화 */
        disableConsoleLogging();    // 로깅여부 N일시 콘촐 입출력 비활성화
        if (logYn == 1) {
            initLogger("leg.log"); // 생성 파일명 지정
        }

        if (calType != 1 && calType != 2 && calType != 3 && calType != 4 && calType != 9) {
            error("[pricingFDL]: Invalid calculation type. Only 1, 2, 3, 4, 9 are supported.");
            return -1; // Invalid calculation type
        }

        // 결과 데이터 초기화
        initResult(resultBasel2, 5);
        initResult(resultGirrDelta, 23);
        initResult(resultGirrCvr, 2);
        initResult(resultCashFlow, 1000);

        // revaluationDateSerial -> revaluationDate
        Date asOfDate_ = Date(evaluationDate);
        const Date issueDate_ = Date(issueDate);

        // 전역 Settings에 평가일을 설정 (이후 모든 계산에 이 날짜 기준 적용)
        Settings::instance().evaluationDate() = asOfDate_;
        Size settlementDays_ = 0;

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

        // Discounting 엔진 생성 (채권 가격 계산용)
        auto bondEngine = ext::make_shared<DiscountingBondEngine>(girrCurve);

        // Schedule 객체 생성 (지급일)
        Schedule fixedBondSchedule_;
        BusinessDayConvention couponBDC = makeBDCFromInt(paymentBDC); // 지급일 휴일 적용 기준  
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
            Frequency couponFrequency_ = makeFrequencyFromInt(couponFrequency); // Period(Tenor)형태도 가능
            DateGeneration::Rule genRule = makeScheduleGenRuleFromInt(scheduleGenRule); // Forward, Backward 등

            fixedBondSchedule_ = MakeSchedule().from(effectiveDate)
                .to(Date(maturityDate))
                .withFrequency(couponFrequency_)
                .withCalendar(couponCalendar_)
                .withConvention(couponBDC)
                .withRule(genRule); // payment Lag는 Bond 스케쥴 생성 시 적용
        }

        /* FOR DEBUG */
        //    printAllData(fixedBondSchedule_);

        Integer paymentLag_ = paymentLag;
        BusinessDayConvention paymentBDC_ = makeBDCFromInt(paymentBDC);;
        Real redemptionRatio = 100.0;
        // bool isNotionalExchange_ = true;
        if (isNotionalExchange == 0) {
            // isNotionalExchange_ = false;
            redemptionRatio = 0.0;
        }

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

        // Fixed Rate Bond에 Discounting 엔진 연결
        fixedRateBond.setPricingEngine(bondEngine);

        // 채권 가격 Net PV 계산
        Real npv = fixedRateBond.NPV();

        // 이론가 산출의 경우 GIRR Delta 산출을 하지 않음
        if (calType == 1) {
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
                bumpGirrCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpGirrCurve);

                // FixedRateBond에 bump된 pricing engine 연결
                fixedRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                bumpedNpv[bumpNo] = fixedRateBond.NPV();

            }

            fixedRateBond.setPricingEngine(bondEngine);

            QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            Real delta = (bumpedNpv[0] - npv) / bumpSize;
            Real gamma = (bumpedNpv[0] - 2.0 * npv + bumpedNpv[1]) / (bumpSize * bumpSize);

            const DayCounter& ytmDayCounter = Actual365Fixed();
            Compounding ytmCompounding = Compounded;//Continuous;
            Frequency ytmFrequency = Semiannual;//Annual
            Rate ytmValue = CashFlows::yield(fixedRateBond.cashflows(), npv, ytmDayCounter, ytmCompounding, ytmFrequency, false,
                couponCalendar_.advance(asOfDate_, Period(settlementDays_, Days)), asOfDate_,
                1.0e-15, 100, 0.005);
            InterestRate ytm = InterestRate(ytmValue, ytmDayCounter, ytmCompounding, ytmFrequency);

            // Duration 계산
            Duration::Type durationType = Duration::Macaulay; //Duration::Modified;
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
                bumpGirrCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpGirrCurve);

                // FixedRateBond에 bump된 pricing engine 연결
                fixedRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                Real tmpGirr = (fixedRateBond.NPV() - npv) * 10000;

                // 산출된 Girr Delta 값을 벡터에 추가
                disCountingGirr.emplace_back(tmpGirr);
            }
            fixedRateBond.setPricingEngine(bondEngine);

            /* OUTPUT 2. GIRR Delta 결과 적재 */
            std::vector<Real> girrTenor = { 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 15.0, 20.0, 30.0 };
            Size girrDataSize = girrTenor.size();
            // 0인 민감도를 제외하고 적재
            processResultArray(girrTenor, disCountingGirr, girrDataSize, resultGirrDelta);


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
                bumpGirrCurve->enableExtrapolation(); // 외삽 허용

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpGirrCurve);

                // FixedRateBond에 bump된 pricing engine 연결
                fixedRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                bumpedNpv[bumpNo] = fixedRateBond.NPV();
            }

            QL_REQUIRE(bumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            resultGirrCvr[0] = (bumpedNpv[0] - npv);
            resultGirrCvr[1] = (bumpedNpv[1] - npv);

            /* OUTPUT 1. Net PV 리턴 */
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
                    resultCashFlow[couponNum * numberOfFields + n_DFField] = girrCurve->discount(cp->date());

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
                        resultCashFlow[couponNum * numberOfFields + n_DFField] = girrCurve->discount(redemption->date());

                    }
                    else {
                        QL_FAIL("[pricingFDL]Coupon is not a FixedRateCoupon");
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
            error("[pricingFDL]: Unknown exception occurred.");
            return -1;
        }
    }
}


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
)
{
    try {
        /* 로거 초기화 */
        disableConsoleLogging();    // 로깅여부 N일시 콘촐 입출력 
        if (logYn == 1) {
            initLogger("leg.log"); // 생성 파일명 지정
        }

        if (calType != 1 && calType != 2 && calType != 3 && calType != 4) {
            error("[pricingFLL]: Invalid calculation type. Only 1, 2, 3, 4 are supported.");
            return -1; // Invalid calculation type
        }

        // 결과 데이터 초기화
        initResult(resultGirrBasel2, 5);
        initResult(resultIndexGirrBasel2, 5);
        initResult(resultGirrDelta, 23);
        initResult(resultIndexGirrDelta, 23);
        initResult(resultGirrCvr, 2);
        initResult(resultIndexGirrCvr, 2);
        initResult(resultCashFlow, 1000);

        // revaluationDateSerial -> revaluationDate
        Date asOfDate_ = Date(evaluationDate);

        // 전역 Settings에 평가일을 설정 (이후 모든 계산에 이 날짜 기준 적용)
        Settings::instance().evaluationDate() = asOfDate_;
        Size settlementDays_ = 0;

        Real notional_ = notional;

        DayCounter couponDayCounter_ = makeDayCounterFromInt(couponDayCounter); // 쿠폰 이자 계산을 위한 일수계산 방식 설정
        Calendar couponCalendar_ = makeCalendarFromInt(couponCalendar); // 쿠폰 지급일 휴일 적용 기준 달력
        Frequency couponFrequency_ = makeFrequencyFromInt(couponFrequency); // 쿠폰 이자 지급 빈도
        BusinessDayConvention couponBDC_ = makeBDCFromInt(paymentBDC);

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
        Natural indexFixingDays_ = indexFixingDays;
        Currency indexCurrency_ = makeCurrencyFromInt(indexCurrency); // 금리 인덱스의 표시 통화
        BusinessDayConvention index1BusinessDayConvention_ = makeBDCFromInt(indexBDC); //
        bool index1EndOfMonth_ = makeBoolFromInt(indexEOM); // 금리 인덱스의 월말 여부 

        // Make index instance
        ext::shared_ptr<IborIndex> refIndex = ext::make_shared<IborIndex>(indexFamilyName_, index1Tenor_, indexFixingDays_
            , indexCurrency_, index1FixingCalendar_, index1BusinessDayConvention_
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

        // Discounting 엔진 생성 (채권 가격 계산용)
        auto bondEngine = ext::make_shared<DiscountingBondEngine>(girrCurve);

        // Schedule 객체 생성 (지급일)
        Schedule FRNSchedule_;

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

        const std::vector<QuantLib::Rate>& caps_ = {};
        const std::vector<QuantLib::Rate>& floors_ = {};
        bool inArrears_ = false;
        QuantLib::Real redemptionRatio = 100.0;
        //bool isNotionalExchange_ = true;
        if (isNotionalExchange == 0) {
            //isNotionalExchange_ = false;/
            redemptionRatio = 0.0;
        }
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
            { spread },
            caps_,
            floors_,
            inArrears_,
            redemptionRatio);

        // Fixed Rate Bond에 Discounting 엔진 연결
        floatingRateBond.setPricingEngine(bondEngine);

        // 채권 가격 Net PV 계산
        Real npv = floatingRateBond.NPV();

        // 이론가 산출의 경우 GIRR Delta 산출을 하지 않음
        if (calType == 1) {
            return npv;
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
                bumpGirrCurve->enableExtrapolation(); // 외삽 허용


                if (isSameCurve_) {
                    indexGirrCurve.linkTo(bumpGirrTermstructure);
                }

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpGirrCurve);

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
                bumpGirrCurve->enableExtrapolation(); // 외삽 허용

                if (isSameCurve_) {
                    indexGirrCurve.linkTo(bumpGirrTermstructure);
                }

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpGirrCurve);

                // FixedRateBond에 bump된 pricing engine 연결
                floatingRateBond.setPricingEngine(bumpBondEngine);

                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                Real tmpGirr = (floatingRateBond.NPV() - npv) * 10000;

                // 산출된 Girr Delta 값을 벡터에 추가
                disCountingGirr.emplace_back(tmpGirr);
                indexGirrCurve.linkTo(indexGirrTermstructure);
            }

            /* OUTPUT 2. (Discounting Curve) GIRR Delta 결과 적재 */
            std::vector<Real> girrTenor = { 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 15.0, 20.0, 30.0 };
            Size girrDataSize = girrTenor.size();
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
                std::vector<Real> indexGirrTenor = { 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 15.0, 20.0, 30.0 };
                Size indexGirrDataSize = indexGirrTenor.size();
                // 0인 민감도를 제외하고 적재
                processResultArray(indexGirrTenor, indexGirr, indexGirrDataSize, resultIndexGirrDelta);
            }

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
                bumpGirrCurve->enableExtrapolation(); // 외삽 허용

                if (isSameCurve_) {
                    indexGirrCurve.linkTo(bumpGirrTermstructure);
                }

                // discountingCurve로 새로운 pricing engine 생성
                auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpGirrCurve);

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

            // NPV : clean, dirty, accured Interest
            Real cleanPrice = floatingRateBond.cleanPrice() / 100.0 * notional;
            Real dirtyPrice = floatingRateBond.dirtyPrice() / 100.0 * notional;
            Real accruedInterest = floatingRateBond.accruedAmount() / 100.0 * notional;

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
                    resultCashFlow[couponNum * numberOfFields + n_DFField] = girrCurve->discount(cp->date());

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
                        resultCashFlow[couponNum * numberOfFields + n_DFField] = girrCurve->discount(redemption->date());

                    }
                    else {
                        QL_FAIL("[pricingFLL]Coupon is not a FloatingRateCoupon");
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
            error("[pricingFLL]: Unknown exception occurred.");
            return -1;
        }
    }
}

// /* FRB Custom Class */
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