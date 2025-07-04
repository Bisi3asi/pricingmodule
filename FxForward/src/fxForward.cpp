#include "fxForward.h"
#include "logger.h"

#include <spdlog/spdlog.h>

using namespace QuantLib;
using namespace std;
using namespace spdlog;

extern "C" {
    void EXPORT_API pricingFFW(
        // ===================================================================================================
        const int evaluationDate                    // INPUT 1.  평가일 (Revaluation Date)

        , const char* buySideCurrency               // INPUT 2.  매입 통화 (Buy Side Currency)
        , const double buySideNotional              // INPUT 3.  매입 명목금액 (Buy Side Notional)
        , const int buySidePayDate                  // INPUT 4.  매입 명목금액 지급일 (Buy Side Payment Date)
        , const int buySideDcb                      // INPUT 5.  매입 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        , const int buyCurveDataSize                // INPUT 6.  매입 커브 데이터 사이즈
        , const int* buyCurveTenorDays              // INPUT 7.  매입 커브 만기 기간 (Buy Curve Term)
        , const double* buyCurveRates               // INPUT 8.  매입 커브 마켓 데이터 (Buy Curve Market Data)

        , const char* sellSideCurrency              // INPUT 9.  매도 통화 (Sell Side Currency)
        , const double sellSideNotional             // INPUT 10. 매도 명목금액 (Sell Side Notional)
        , const int sellSidePayDate                 // INPUT 11. 매도 명목금액 지급일 (Sell Side Payment Date)
        , const int sellSideDcb                     // INPUT 12. 매도 기준 Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]

        , const int sellCurveDataSize               // INPUT 13. 매도 커브 데이터 사이즈
        , const int* sellCurveTenorDays		        // INPUT 14. 매도 커브 만기 기간 (Sell Curve Term)
        , const double* sellCurveRates              // INPUT 15. 매도 커브 마켓 데이터 (Sell Curve Market Data)

		, const double buyGirrRiskWeight            // INPUT 16. Buy GIRR 리스크요소 위험 가중치
		, const double sellGirrRiskWeight           // INPUT 17. Sell GIRR 리스크요소 위험 가중치

        , const int calType                         // INPUT 18. 계산 타입 (1: Theo Price, 2. BASEL 2 Sensitivity, 3. BASEL 3 Sensitivity)
        , const int logYn                           // INPUT 19. 로그 파일 생성 여부 (0: No, 1: Yes)

        , double* resultNetPv                       // OUTPUT 1, 2. PV [index 0: Buy PV, index 1: Sell PV]
        , double* resultBuySideBasel2               // OUTPUT 3  Buy Side Basel2 Results(Delta, Gamma, Duration, Convexity, PV01)
        , double* resultSellSideBasel2              // OUTPUT 4  Sell Side Basel2 Results(Delta, Gamma, Duration, Convexity, PV01)
        , double* resultBuySideGirrDelta            // OUTPUT 5. Buy Side GIRR Delta [index 0: size, index 1 ~ end: 순서대로 buy Side tenor, buy Side Sensitivity, cs basis Sensitivity]
        , double* resultSellSideGirrDelta           // OUTPUT 6. Sell Side GIRR Delta [index 0: size, index 1 ~ end: 순서대로 sell Side tenor, sell Side Sensitivity, cs basis Sensitivity]
        , double* resultBuySideGirrCvr              // OUTPUT 7. Buy Side GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
        , double* resultSellSideGirrCvr             // OUTPUT 8. Sell Side GIRR Curvature [BumpUp Curvature, BumpDownCurvature]
        // ===================================================================================================
    )
    {
        /* TODO */
        // 1. 로깅 로직 추가
        // 2. 공통 유틸 함수로 리팩토링

        if (calType != 1 && calType != 2 && calType != 3) {
            error("[princingFXF]: Invalid calculation type. Only 1, 2, 3 are supported.");
            return; // Invalid calculation type
        }

        // 결과 데이터 초기화
        initResult(resultNetPv, 2);
        initResult(resultBuySideBasel2, 50);
        initResult(resultSellSideBasel2, 50);
        initResult(resultBuySideGirrDelta, 50);
        initResult(resultSellSideGirrDelta, 50);
        initResult(resultBuySideGirrCvr, 50);
        initResult(resultSellSideGirrCvr, 50);

        Date asOfDate_ = Date(evaluationDate);
		const int settlementDays = 0; // 결제일수 (기본값 0, 필요시 수정 가능)

        // GIRR 커브의 주요 기간 설정 (시장 표준 테너)
        std::vector<Period> girrPeriod = { Period(3, Months), Period(6, Months), Period(1, Years), Period(2, Years),
                                           Period(3, Years), Period(5, Years), Period(10, Years), Period(15, Years),
                                           Period(20, Years), Period(30, Years) };

        // GIRR 커브 구성용 날짜 및 금리 벡터
        std::vector<Date> buyGirrDates_;
        std::vector<Real> buyGirrRates_;


        // GIRR 커브 시작점 (revaluationDate 기준) 입력
        buyGirrDates_.emplace_back(asOfDate_);
        buyGirrRates_.emplace_back(buyCurveRates[0]);

        // 나머지 GIRR 커브 구성 요소 입력
        for (Size dateNum = 0; dateNum < buyCurveDataSize; ++dateNum) {
            buyGirrDates_.emplace_back(asOfDate_ + girrPeriod[dateNum]);
            buyGirrRates_.emplace_back(buyCurveRates[dateNum]);
        }

        // GIRR 커브 계산 사용 추가 요소
        DayCounter buyGirrDayCounter_ = Actual365Fixed(); // DCB, TODO 변환 함수 적용
        Linear buyGirrInterpolator_ = Linear(); // 보간 방식, TODO 변환 함수 적용 (Interpolator)
        Compounding buyGirrCompounding_ = Compounding::Continuous; // 이자 계산 방식, TODO 변환 함수 적용 (Compounding)
        Frequency buyGirrFrequency_ = Frequency::Annual; // 이자 지급 빈도, TODO 변환 함수 적용 (Frequency)

        // GIRR 커브 생성
        ext::shared_ptr<YieldTermStructure> buyGirrTermstructure = ext::make_shared<ZeroCurve>(buyGirrDates_, buyGirrRates_,
            buyGirrDayCounter_, buyGirrInterpolator_,
            buyGirrCompounding_, buyGirrFrequency_);

        // GIRR 커브를 RelinkableHandle에 연결
        RelinkableHandle<YieldTermStructure> buyGirrCurve;
        buyGirrCurve.linkTo(buyGirrTermstructure);


        // GIRR 커브 구성용 날짜 및 금리 벡터
        std::vector<Date> sellGirrDates_;
        std::vector<Real> sellGirrRates_;


        // GIRR 커브 시작점 (revaluationDate 기준) 입력
        sellGirrDates_.emplace_back(asOfDate_);
        sellGirrRates_.emplace_back(sellCurveRates[0]);

        // 나머지 GIRR 커브 구성 요소 입력
        for (Size dateNum = 0; dateNum < sellCurveDataSize; ++dateNum) {
            sellGirrDates_.emplace_back(asOfDate_ + girrPeriod[dateNum]);
            sellGirrRates_.emplace_back(sellCurveRates[dateNum]);
        }

        // GIRR 커브 계산 사용 추가 요소
        DayCounter sellGirrDayCounter_ = Actual365Fixed(); // DCB, TODO 변환 함수 적용
        Linear sellGirrInterpolator_ = Linear(); // 보간 방식, TODO 변환 함수 적용 (Interpolator)
        Compounding sellGirrCompounding_ = Compounding::Continuous; // 이자 계산 방식, TODO 변환 함수 적용 (Compounding)
        Frequency sellGirrFrequency_ = Frequency::Annual; // 이자 지급 빈도, TODO 변환 함수 적용 (Frequency)

        // GIRR 커브 생성
        ext::shared_ptr<YieldTermStructure> sellGirrTermstructure = ext::make_shared<ZeroCurve>(sellGirrDates_, sellGirrRates_,
            sellGirrDayCounter_, sellGirrInterpolator_,
            sellGirrCompounding_, sellGirrFrequency_);

        // GIRR 커브를 RelinkableHandle에 연결
        RelinkableHandle<YieldTermStructure> sellGirrCurve;
        sellGirrCurve.linkTo(sellGirrTermstructure);

        Real spotFXRate = 1288.0;
        // Assign FX spot rate for Reference Currency
        Real buySpotFXRate = 1.0;
        Real sellSpotFXRate = 1.0;
        //    if( isRCVCcyForeign == true )
        //    {
        //        rcvSpotFXRate = spotFXRate;
        //    } else {
        //        paySpotFXRate = spotFXRate;
        //    }

            // Calculate Present value of Far Leg CF
        Calendar settleCalendar = NullCalendar();
        Date settleDate = settleCalendar.advance(asOfDate_, Period(settlementDays, Days));
        Date buyFarDate = Date(buySidePayDate);
        Date sellFarDate = Date(sellSidePayDate);
        Real buyFarAmt = buySideNotional;
        Real sellFarAmt = sellSideNotional;
        DiscountFactor rcvSettleDF = buyGirrCurve->discount(settleDate);
        DiscountFactor rcvPaymentDF = buyGirrCurve->discount(buyFarDate);
        DiscountFactor rcvForwardDF = rcvPaymentDF / rcvSettleDF;
        DiscountFactor paySettleDF = sellGirrCurve->discount(settleDate);
        DiscountFactor payPaymentDF = sellGirrCurve->discount(sellFarDate);
        DiscountFactor payForwardDF = payPaymentDF / paySettleDF;
        Real buyLegNPV = buySpotFXRate * buyFarAmt * rcvForwardDF;
        Real sellLegNPV = sellSpotFXRate * sellFarAmt * payForwardDF;

        resultNetPv[0] = buyLegNPV;
        resultNetPv[1] = sellLegNPV;

        if (calType == 1) {
            return;
        }

        if (calType == 2) {
            // Delta 계산
            Real bumpSize = 0.0001; // bumpSize를 0.0001 이외의 값으로 적용 시, PV01 산출을 독립적으로 구현해줘야 함
            std::vector<Real> bumpGearings{ 1.0, -1.0 };
            std::vector<Real> buyBumpedNpv(bumpGearings.size(), 0.0);
            std::vector<Real> sellBumpedNpv(bumpGearings.size(), 0.0);
            for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                // buy GIRR 커브의 금리를 bumping (1bp 상승)
                std::vector<Rate> buyBumpGirrRates = buyGirrRates_;
                for (Size bumpTenorNum = 0; bumpTenorNum < buyBumpGirrRates.size(); ++bumpTenorNum) {
                    buyBumpGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * bumpSize;
                }

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> buyBumpGirrTermstructure = ext::make_shared<ZeroCurve>(buyGirrDates_,
                    buyBumpGirrRates,
                    buyGirrDayCounter_,
                    buyGirrInterpolator_,
                    buyGirrCompounding_,
                    buyGirrFrequency_);
                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> buyBumpGirrCurve;
                buyBumpGirrCurve.linkTo(buyBumpGirrTermstructure);
                buyBumpGirrCurve->enableExtrapolation(); // 외삽 허용

                // Buy Bumped PV
                DiscountFactor buyBumpSettleDF = buyBumpGirrCurve->discount(settleDate);
                DiscountFactor buyBumpPaymentDF = buyBumpGirrCurve->discount(buyFarDate);
                DiscountFactor buyBumpForwardDF = buyBumpPaymentDF / buyBumpSettleDF;
                buyBumpedNpv[bumpNo] = buySpotFXRate * buyFarAmt * buyBumpForwardDF;

                // sell GIRR 커브의 금리를 bumping (1bp 상승)
                std::vector<Rate> sellBumpGirrRates = sellGirrRates_;
                for (Size bumpTenorNum = 0; bumpTenorNum < sellBumpGirrRates.size(); ++bumpTenorNum) {
                    sellBumpGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * bumpSize;
                }

                ext::shared_ptr<YieldTermStructure> sellBumpGirrTermstructure = ext::make_shared<ZeroCurve>(sellGirrDates_,
                    sellBumpGirrRates,
                    sellGirrDayCounter_,
                    sellGirrInterpolator_,
                    sellGirrCompounding_,
                    sellGirrFrequency_);

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> sellBumpGirrCurve;
                sellBumpGirrCurve.linkTo(sellBumpGirrTermstructure);
                sellBumpGirrCurve->enableExtrapolation(); // 외삽 허용

                // Sell Bumped PV
                DiscountFactor sellBumpSettleDF = sellBumpGirrCurve->discount(settleDate);
                DiscountFactor sellBumpPaymentDF = sellBumpGirrCurve->discount(sellFarDate);
                DiscountFactor sellBumpForwardDF = sellBumpPaymentDF / sellBumpSettleDF;
                sellBumpedNpv[bumpNo] = sellSpotFXRate * sellFarAmt * sellBumpForwardDF;

            }

            // buy leg Basel2 result 계산
            QL_REQUIRE(buyBumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            Real buyDelta = (buyBumpedNpv[0] - buyLegNPV) / bumpSize;
            Real buyGamma = (buyBumpedNpv[0] - 2.0 * buyLegNPV + buyBumpedNpv[1]) / (bumpSize * bumpSize);

            // buy Leg 생성
            ZeroCouponBond buyCoupon(settlementDays, settleCalendar, buyFarAmt, buyFarDate);
            const DayCounter& buyYtmDayCounter = Actual365Fixed();
            Compounding buyYtmCompounding = Continuous;
            Frequency buyYtmFrequency = Annual;
            Rate buyYtmValue = CashFlows::yield(buyCoupon.cashflows(), buyLegNPV, buyYtmDayCounter, buyYtmCompounding, buyYtmFrequency,
                false, settleCalendar.advance(asOfDate_, Period(settlementDays, Days)), asOfDate_,
                1.0e-15, 100, 0.005);
            InterestRate buyYtm = InterestRate(buyYtmValue, buyYtmDayCounter, buyYtmCompounding, buyYtmFrequency);

            // Duration 계산
            Duration::Type durationType = Duration::Modified;
            Real buyDuration = CashFlows::duration(buyCoupon.cashflows(), buyYtm, durationType, false,
                settleCalendar.advance(asOfDate_, Period(settlementDays, Days)), asOfDate_);

            Real buyConvexity = CashFlows::convexity(buyCoupon.cashflows(), buyYtm, false,
                settleCalendar.advance(asOfDate_, Period(settlementDays, Days)), asOfDate_);

            Real buyPV01 = buyDelta * bumpSize;

            resultBuySideBasel2[0] = buyDelta;
            resultBuySideBasel2[1] = buyGamma;
            resultBuySideBasel2[2] = buyDuration;
            resultBuySideBasel2[3] = buyConvexity;
            resultBuySideBasel2[4] = buyPV01;

            // sell leg Basel2 result 계산
            QL_REQUIRE(sellBumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            Real sellDelta = (sellBumpedNpv[0] - sellLegNPV) / bumpSize;
            Real sellGamma = (sellBumpedNpv[0] - 2.0 * sellLegNPV + sellBumpedNpv[1]) / (bumpSize * bumpSize);

            // sell Leg 생성
            ZeroCouponBond sellCoupon(settlementDays, settleCalendar, sellFarAmt, sellFarDate);
            const DayCounter& sellYtmDayCounter = Actual365Fixed();
            Compounding sellYtmCompounding = Continuous;
            Frequency sellYtmFrequency = Annual;
            Rate sellYtmValue = CashFlows::yield(sellCoupon.cashflows(), sellLegNPV, sellYtmDayCounter, sellYtmCompounding, sellYtmFrequency,
                false, settleCalendar.advance(asOfDate_, Period(settlementDays, Days)), asOfDate_,
                1.0e-15, 100, 0.005);
            InterestRate sellYtm = InterestRate(sellYtmValue, sellYtmDayCounter, sellYtmCompounding, sellYtmFrequency);

            // Duration 계산
            durationType = Duration::Modified;
            Real sellDuration = CashFlows::duration(sellCoupon.cashflows(), sellYtm, durationType, false,
                settleCalendar.advance(asOfDate_, Period(settlementDays, Days)), asOfDate_);

            Real sellConvexity = CashFlows::convexity(sellCoupon.cashflows(), sellYtm, false,
                settleCalendar.advance(asOfDate_, Period(settlementDays, Days)), asOfDate_);

            Real sellPV01 = sellDelta * bumpSize;

            resultSellSideBasel2[0] = sellDelta;
            resultSellSideBasel2[1] = sellGamma;
            resultSellSideBasel2[2] = sellDuration;
            resultSellSideBasel2[3] = sellConvexity;
            resultSellSideBasel2[4] = sellPV01;

            return;
        }

        if (calType == 3) {
            // GIRR Bump Rate 설정
            Real girrBump = 0.0001;

            // GIRR Delta 적재용 벡터 생성
            std::vector<Real> buyGirr;
            std::vector<Real> sellGirr;
            std::vector<Real> buyGirrTenor = { 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 15.0, 20.0, 30.0 };
            std::vector<Real> sellGirrTenor = { 0.25, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0, 15.0, 20.0, 30.0 };

            // Buy GIRR Delta 계산
            Real totalBuyGirr = 0;
            for (Size bumpNum = 1; bumpNum < buyGirrRates_.size(); ++bumpNum) {
                // GIRR 커브의 금리를 bumping (1bp 상승)
                std::vector<Rate> bumpGirrRates = buyGirrRates_;
                bumpGirrRates[bumpNum] += girrBump;

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(buyGirrDates_, bumpGirrRates, buyGirrDayCounter_,
                    buyGirrInterpolator_, buyGirrCompounding_,
                    buyGirrFrequency_);

                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation(); // 외삽 허용

                // Buy Bumped PV
                DiscountFactor buyBumpSettleDF = bumpGirrCurve->discount(settleDate);
                DiscountFactor buyBumpPaymentDF = bumpGirrCurve->discount(buyFarDate);
                DiscountFactor buyBumpForwardDF = buyBumpPaymentDF / buyBumpSettleDF;
                Real buyBumpNpv = buySpotFXRate * buyFarAmt * buyBumpForwardDF;


                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                Real tmpGirr = (buyBumpNpv - buyLegNPV) * 10000;

                // 산출된 Girr Delta 값을 벡터에 추가
                buyGirr.emplace_back(tmpGirr);
                totalBuyGirr += tmpGirr;
            }

            // Currency basis risk
            if (strcmp(buySideCurrency, "USD") != 0) {
                // buy GIRR 커브의 금리를 bumping (1bp 상승)
                std::vector<Rate> buyBumpGirrRates = buyGirrRates_;
                for (Size bumpTenorNum = 0; bumpTenorNum < buyBumpGirrRates.size(); ++bumpTenorNum) {
                    buyBumpGirrRates[bumpTenorNum] += girrBump;
                }

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(buyGirrDates_,
                    buyBumpGirrRates,
                    buyGirrDayCounter_,
                    buyGirrInterpolator_,
                    buyGirrCompounding_,
                    buyGirrFrequency_);
                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation(); // 외삽 허용

                // Buy Bumped PV
                DiscountFactor buyBumpSettleDF = bumpGirrCurve->discount(settleDate);
                DiscountFactor buyBumpPaymentDF = bumpGirrCurve->discount(buyFarDate);
                DiscountFactor buyBumpForwardDF = buyBumpPaymentDF / buyBumpSettleDF;
                Real buyBumpNpv = buySpotFXRate * buyFarAmt * buyBumpForwardDF;


                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                Real tmpGirr = (buyBumpNpv - buyLegNPV) * 10000;

                buyGirrTenor.insert(buyGirrTenor.begin(), 0.0);
                buyGirr.insert(buyGirr.begin(), tmpGirr);
            }


            /* OUTPUT 2. GIRR Delta 결과 적재 */
            Size buyGirrDataSize = buyGirrTenor.size();
            // 0인 민감도를 제외하고 적재
            processResultArray(buyGirrTenor, buyGirr, buyGirrDataSize, resultBuySideGirrDelta);


            // Sell GIRR Delta 계산
            Real totalSellGirr = 0;
            for (Size bumpNum = 1; bumpNum < sellGirrRates_.size(); ++bumpNum) {
                // GIRR 커브의 금리를 bumping (1bp 상승)
                std::vector<Rate> bumpGirrRates = sellGirrRates_;
                bumpGirrRates[bumpNum] += girrBump;

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(sellGirrDates_, bumpGirrRates, sellGirrDayCounter_,
                    sellGirrInterpolator_, sellGirrCompounding_,
                    sellGirrFrequency_);

                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation(); // 외삽 허용

                // Buy Bumped PV
                DiscountFactor sellBumpSettleDF = bumpGirrCurve->discount(settleDate);
                DiscountFactor sellBumpPaymentDF = bumpGirrCurve->discount(sellFarDate);
                DiscountFactor sellBumpForwardDF = sellBumpPaymentDF / sellBumpSettleDF;
                Real sellBumpNpv = sellSpotFXRate * sellFarAmt * sellBumpForwardDF;


                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                Real tmpGirr = (sellBumpNpv - sellLegNPV) * 10000;

                // 산출된 Girr Delta 값을 벡터에 추가
                sellGirr.emplace_back(tmpGirr);
                totalSellGirr += tmpGirr;
            }

            // Currency basis risk
            if (strcmp(sellSideCurrency, "USD") != 0) {
                // buy GIRR 커브의 금리를 bumping (1bp 상승)
                std::vector<Rate> sellBumpGirrRates = sellGirrRates_;
                for (Size bumpTenorNum = 0; bumpTenorNum < sellBumpGirrRates.size(); ++bumpTenorNum) {
                    sellBumpGirrRates[bumpTenorNum] += girrBump;
                }

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(sellGirrDates_,
                    sellBumpGirrRates,
                    sellGirrDayCounter_,
                    sellGirrInterpolator_,
                    sellGirrCompounding_,
                    sellGirrFrequency_);
                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> bumpGirrCurve;
                bumpGirrCurve.linkTo(bumpGirrTermstructure);
                bumpGirrCurve->enableExtrapolation(); // 외삽 허용

                // Buy Bumped PV
                DiscountFactor sellBumpSettleDF = bumpGirrCurve->discount(settleDate);
                DiscountFactor sellBumpPaymentDF = bumpGirrCurve->discount(buyFarDate);
                DiscountFactor sellBumpForwardDF = sellBumpPaymentDF / sellBumpSettleDF;
                Real sellBumpNpv = sellSpotFXRate * sellFarAmt * sellBumpForwardDF;


                // 기존 Net PV - bump된 Net PV 계산 (GIRR Delta)
                Real tmpGirr = (sellBumpNpv - sellLegNPV) * 10000;

                sellGirrTenor.insert(sellGirrTenor.begin(), 0.0);
                sellGirr.insert(sellGirr.begin(), tmpGirr);
            }


            /* OUTPUT 2. GIRR Delta 결과 적재 */
            Size sellGirrDataSize = sellGirrTenor.size();
            // 0인 민감도를 제외하고 적재
            processResultArray(sellGirrTenor, sellGirr, sellGirrDataSize, resultSellSideGirrDelta);

            // Curvature 계산
            Real curvatureRW = 0.017; // bumpSize를 FRTB 기준서의 Girr Curvature RiskWeight로 설정
            std::vector<Real> bumpGearings{ 1.0, -1.0 };
            std::vector<Real> buyBumpedNpv(bumpGearings.size(), 0.0);
            std::vector<Real> sellBumpedNpv(bumpGearings.size(), 0.0);
            for (Size bumpNo = 0; bumpNo < bumpGearings.size(); ++bumpNo) {
                // buy GIRR 커브의 금리를 bumping (1bp 상승)
                std::vector<Rate> buyBumpGirrRates = buyGirrRates_;
                for (Size bumpTenorNum = 0; bumpTenorNum < buyBumpGirrRates.size(); ++bumpTenorNum) {
                    buyBumpGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * curvatureRW;
                }

                // bump된 금리로 새로운 ZeroCurve 생성
                ext::shared_ptr<YieldTermStructure> buyBumpGirrTermstructure = ext::make_shared<ZeroCurve>(buyGirrDates_,
                    buyBumpGirrRates,
                    buyGirrDayCounter_,
                    buyGirrInterpolator_,
                    buyGirrCompounding_,
                    buyGirrFrequency_);
                // RelinkableHandle에 bump된 커브 연결
                RelinkableHandle<YieldTermStructure> buyBumpGirrCurve;
                buyBumpGirrCurve.linkTo(buyBumpGirrTermstructure);
                buyBumpGirrCurve->enableExtrapolation(); // 외삽 허용

                // Buy Bumped PV
                DiscountFactor buyBumpSettleDF = buyBumpGirrCurve->discount(settleDate);
                DiscountFactor buyBumpPaymentDF = buyBumpGirrCurve->discount(buyFarDate);
                DiscountFactor buyBumpForwardDF = buyBumpPaymentDF / buyBumpSettleDF;
                buyBumpedNpv[bumpNo] = buySpotFXRate * buyFarAmt * buyBumpForwardDF;

                // sell GIRR 커브의 금리를 bumping (1bp 상승)
                std::vector<Rate> sellBumpGirrRates = sellGirrRates_;
                for (Size bumpTenorNum = 0; bumpTenorNum < sellBumpGirrRates.size(); ++bumpTenorNum) {
                    sellBumpGirrRates[bumpTenorNum] += bumpGearings[bumpNo] * curvatureRW;
                }

                ext::shared_ptr<YieldTermStructure> sellBumpGirrTermstructure = ext::make_shared<ZeroCurve>(sellGirrDates_,
                    sellBumpGirrRates,
                    sellGirrDayCounter_,
                    sellGirrInterpolator_,
                    sellGirrCompounding_,
                    sellGirrFrequency_);

                // 할인 커브를 RelinkableHandle로 wrapping
                RelinkableHandle<YieldTermStructure> sellBumpGirrCurve;
                sellBumpGirrCurve.linkTo(sellBumpGirrTermstructure);
                sellBumpGirrCurve->enableExtrapolation(); // 외삽 허용

                // Sell Bumped PV
                DiscountFactor sellBumpSettleDF = sellBumpGirrCurve->discount(settleDate);
                DiscountFactor sellBumpPaymentDF = sellBumpGirrCurve->discount(sellFarDate);
                DiscountFactor sellBumpForwardDF = sellBumpPaymentDF / sellBumpSettleDF;
                sellBumpedNpv[bumpNo] = sellSpotFXRate * sellFarAmt * sellBumpForwardDF;

            }

            QL_REQUIRE(buyBumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            QL_REQUIRE(sellBumpedNpv.size() > 1, "Failed to calculate bumpedNPV.");
            resultBuySideGirrCvr[0] = (buyBumpedNpv[0] - buyLegNPV);
            resultBuySideGirrCvr[1] = (buyBumpedNpv[1] - buyLegNPV);

            resultSellSideGirrCvr[0] = (sellBumpedNpv[0] - sellLegNPV);
            resultSellSideGirrCvr[1] = (sellBumpedNpv[1] - sellLegNPV);

            return;
        }

    }
}

// 결과값 초기화
void initResult(double* result, const int size) {
    fill_n(result, size, 0.0);
}

double roundToDecimals(const double value, const int n) {
	double factor = pow(10.0, n);
	return round(value * factor) / factor;
}

void processResultArray(vector<Real> tenors, vector<Real> sensitivities, Size originalSize, double* resultArray) {
    vector<double> filteredTenor;
    vector<double> filteredSensitivity;

    // 1. 전처리: sensitivity가 0이 아닌 것만 걸러냄
    for (Size i = 0; i < originalSize; ++i) {
        if (sensitivities[i] != 0.0) {
            filteredTenor.push_back(tenors[i]);
            filteredSensitivity.push_back(sensitivities[i]);
        }
    }

    Size filteredSize = filteredTenor.size();

    // 2. resultArray 구성
    resultArray[0] = filteredSize;  // 유효 데이터 개수

    // 3. Tenor 입력 (index 1 ~ filteredSize)
    for (Size i = 0; i < filteredSize; ++i) {
        resultArray[i + 1] = filteredTenor[i];
    }

    // 4. girrSensitivity 입력 (index 1 + filteredSize ~ 2 + 2 * filteredSize)
    for (Size i = 0; i < filteredSize; ++i) {
        resultArray[i + 1 + filteredSize] = filteredSensitivity[i];
    }
}