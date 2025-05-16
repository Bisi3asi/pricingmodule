#include "bond.h"
#include "logger.h"

#include <spdlog/spdlog.h>

// namespace
using namespace QuantLib;
using namespace std;
using namespace spdlog;

extern "C" double EXPORT pricing(
    long evaluationDate,            // 평가일 (serial number, 예: 46164)
    long settlementDays,            // 결제일 offset (보통 2일)
    long issueDate,                 // 발행일
    long maturityDate,              // 만기일
    double notional,                // 채권 원금
    double couponRate,              // 쿠폰 이율
    int couponDayCounter,           // DayCounter code (예: 5 = Actual/Actual(Bond))
    int numberOfCoupons,            // 쿠폰 개수
    const long* paymentDates,       // 지급일 배열
    const long* realStartDates,     // 각 구간 시작일
    const long* realEndDates,       // 각 구간 종료일
    int numberOfGirrTenors,         // GIRR 만기 수
    const long* girrTenorDays,      // GIRR 만기 (startDate로부터의 일수)
    const double* girrRates,        // GIRR 금리
    int girrDayCounter,             // GIRR DayCounter (예: 1 = Actual/365)
    int girrInterpolator,           // 보간법 (예: 1 = Linear)
    int girrCompounding,            // 이자 계산 방식 (예: 1 = Continuous)
    int girrFrequency,              // 이자 빈도 (예: 1 = Annual)
    double spreadOverYield,         // 채권의 종목 Credit Spread
    int spreadOverYieldCompounding, // Continuous
    int spreadOverYieldDayCounter,  // Actual/365
    int numberOfCsrTenors,          // CSR 만기 수
    const long* csrTenorDays,       // CSR 만기 (startDate로부터의 일수)
    const double* csrRates,         // CSR 스프레드 (금리 차이)

    const int logYn,                // 로깅여부 (0: No, 1: Yes)

                                    // OUTPUT1. Net PV
    double* resultGirrDelta,        // OUTPUT2. GIRR Delta
	double* resultCsrDelta			// OUTPUT3. CSR Delta
) {
    /* 로거 초기화 */
    if (logYn == 1) {
        initLogger("bond.log"); // 생성 파일명 지정
    }
    info("==============[bond Logging Started!]==============");
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
        csrRates
    );


	initResult(resultGirrDelta, 11);
    initResult(resultCsrDelta, 11);

    std::cout.precision(15);
    Date asOfDate_ = Date(evaluationDate);
    Settings::instance().evaluationDate() = asOfDate_;
    Size settlementDays_ = settlementDays;
    Real notional_ = notional;
    std::vector<Rate> couponRate_ = std::vector<Rate>(1, couponRate);
    DayCounter couponDayCounter_ = ActualActual(ActualActual::ISDA); // TODO 변환 함수 적용 (DayCounter)
    std::vector<Date> girrDates_;
    std::vector<Real> girrRates_;
    std::vector<Period> girrPeriod = { Period(3, Months), Period(6, Months), Period(1, Years), Period(2, Years),
                                      Period(3, Years), Period(5, Years), Period(10, Years), Period(15, Years),
                                      Period(20, Years), Period(30, Years) };
    girrDates_.emplace_back(asOfDate_);
    girrRates_.emplace_back(girrRates[0]);
    for (Size dateNum = 0; dateNum < numberOfGirrTenors; ++dateNum) {
        //        girrDates_.emplace_back(asOfDate_ + girrTenorDays[dateNum]);
        girrDates_.emplace_back(asOfDate_ + girrPeriod[dateNum]);
        girrRates_.emplace_back(girrRates[dateNum] + spreadOverYield);
    }
    // TODO 변환 함수 적용
    DayCounter girrDayCounter_ = Actual365Fixed();
    Linear girrInterpolator_ = Linear(); // TODO 변환 함수 적용 (Interpolator)
    Compounding girrCompounding_ = Compounding::Continuous; // TODO 변환 함수 적용 (Compounding)
    Frequency girrFrequency_ = Frequency::Annual;
    ext::shared_ptr<YieldTermStructure> girrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, girrRates_,
        girrDayCounter_, girrInterpolator_, girrCompounding_, girrFrequency_);
    RelinkableHandle<YieldTermStructure> girrCurve;
    girrCurve.linkTo(girrTermstructure);
    double tmpSpreadOverYield = spreadOverYield;
    Compounding spreadOverYieldCompounding_ = Compounding::Continuous; // TODO 변환 함수 적용 (Compounding)
    DayCounter spreadOverYieldDayCounter_ = Actual365Fixed();  // TODO 변환 함수 적용 (DayCounter)
    InterestRate tempRate(tmpSpreadOverYield, spreadOverYieldDayCounter_, spreadOverYieldCompounding_, Frequency::Annual);
    std::vector<Date> csrDates_;
    std::vector<Period> csrPeriod = { Period(6, Months), Period(1, Years), Period(3, Years), Period(5, Years), Period(10, Years) };
    csrDates_.emplace_back(asOfDate_);
    std::vector<Handle<Quote>> csrSpreads_;
    double spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_, girrDayCounter_.yearFraction(asOfDate_, asOfDate_));
    csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrRates[0]));
    //    csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads[0]+spreadOverYield_));
    for (Size dateNum = 0; dateNum < numberOfCsrTenors; ++dateNum) {
        //        csrDates_.emplace_back(asOfDate_ + csrTenorDays[dateNum]);
        csrDates_.emplace_back(asOfDate_ + csrPeriod[dateNum]);
        spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_, girrDayCounter_.yearFraction(asOfDate_, csrDates_.back()));
        csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrRates[dateNum]));
        //        csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads[dateNum] + spreadOverYield_));
    }
    ext::shared_ptr<ZeroYieldStructure> discountingTermStructure =
        ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, csrSpreads_, csrDates_);
    RelinkableHandle<YieldTermStructure> discountingCurve;
    discountingCurve.linkTo(discountingTermStructure);
    auto bondEngine = ext::make_shared<DiscountingBondEngine>(discountingCurve);
    std::vector<Date> couponSch_;
    couponSch_.emplace_back(realStartDates[0]);
    for (Size schNum = 0; schNum < numberOfCoupons; ++schNum) {
        couponSch_.emplace_back(realEndDates[schNum]);
    }
    Schedule fixedBondSchedule_(couponSch_);
    FixedRateBond fixedRateBond(
        settlementDays_,
        notional_,
        fixedBondSchedule_,
        couponRate_,
        couponDayCounter_,
        ModifiedFollowing, // TODO 변환 함수 적용 (DayConvention)
        100.0);
    fixedRateBond.setPricingEngine(bondEngine);
    //    //디버깅용 배열
    //    const Leg& tmpLeg = fixedRateBond.cashflows();
    //    std::vector<Real> tmpCf;
    //    std::vector<DiscountFactor> tmpDf;
    //    for (const auto& cf : tmpLeg) {
    //        tmpCf.emplace_back(cf->amount());
    //        tmpDf.emplace_back(discountingCurve->discount(cf->date()));
    //    }
    Real npv = fixedRateBond.NPV();
    //std::cout << "NPV: " << npv << std::endl;
   
    Real girrBump = 0.0001;
    std::vector<Real> disCountingGirr;
    for (Size bumpNum = 1; bumpNum < girrRates_.size(); ++bumpNum) {
        std::vector<Rate> bumpGirrRates = girrRates_;
        bumpGirrRates[bumpNum] += girrBump;
        ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates,
            girrDayCounter_, girrInterpolator_, girrCompounding_, girrFrequency_);
        RelinkableHandle<YieldTermStructure> bumpGirrCurve;
        bumpGirrCurve.linkTo(bumpGirrTermstructure);
        ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure =
            ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);
        RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
        bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
        bumpDiscountingCurve->enableExtrapolation();
        auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve);
        fixedRateBond.setPricingEngine(bumpBondEngine);
        Real tmpGirr = fixedRateBond.NPV() - npv;

        if (tmpGirr != 0.0) {
            disCountingGirr.emplace_back(tmpGirr);
            //std::cout << "Girr[" << bumpNum << "]:" << tmpGirr * 10000.0 << std::endl;
        }
    }

	//OUTPUT2. GIRR Delta
    resultGirrDelta[0] = disCountingGirr.size(); // index 0 : size
    for (Size i = 0; i < disCountingGirr.size(); ++i) {
		resultGirrDelta[i + 1] = girrTenorDays[i]; // index 1 ~ size : girrTenorDays
	}
	for (Size i = 0; i < disCountingGirr.size(); ++i) {
		resultGirrDelta[i + 1 + disCountingGirr.size()] = disCountingGirr[i] * 10000.0; // index size + 1 ~ size + size : GIRR Delta
	}

    Real csrBump = 0.0001;
    std::vector<Real> disCountingCsr;
    for (Size bumpNum = 1; bumpNum < csrSpreads_.size(); ++bumpNum) {
        //        std::vector<Handle<Quote>> bumpCsrSpreads_ = csrSpreads_;
        //        bumpCsrSpreads_[bumpNum] = Handle<Quote>(ext::make_shared<SimpleQuote>(csrSpreads[bumpNum] + spreadOverYield_ + csrBump));
        std::vector<Handle<Quote>> bumpCsrSpreads_;
        if (bumpNum == 1) {
            bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[0]->value() + csrBump));
        }
        else {
            bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[0]->value()));
        }

        for (Size i = 1; i < csrSpreads_.size(); ++i) {
            Real bump = (i == bumpNum) ? csrBump : 0.0;
            bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[i]->value() + bump));            
            //            std::cout << "bumpNum, i: " << bumpNum << ", " << bumpCsrSpreads_[i]->value() << std::endl;
        }

        ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure =
            ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, bumpCsrSpreads_, csrDates_);
        RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
        bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
        bumpDiscountingCurve->enableExtrapolation();
        auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve);
        fixedRateBond.setPricingEngine(bumpBondEngine);
        Real tmpCsr = fixedRateBond.NPV() - npv;
        if (tmpCsr != 0.0) {
            disCountingCsr.emplace_back(tmpCsr);
            //std::cout << "Csr[" << bumpNum << "]:" << tmpCsr * 10000.0 << std::endl;
        }
    }

    resultCsrDelta[0] = disCountingCsr.size(); // index 0 : size
    for (Size i = 0; i < disCountingCsr.size(); ++i) {
		resultCsrDelta[i + 1] = csrTenorDays[i]; // index 1 ~ size : csrTenorDays
    }
    for (Size i = 0; i < disCountingCsr.size(); ++i) {
		resultCsrDelta[i + 1 + disCountingCsr.size()] = disCountingCsr[i] * 10000.0; // index size + 1 ~ size + size : CSR Delta
    }

    Real cleanPrice = fixedRateBond.cleanPrice() / 100.0 * notional;
    Real dirtyPrice = fixedRateBond.dirtyPrice() / 100.0 * notional;
    Real accruedInterest = fixedRateBond.accruedAmount() / 100.0 * notional;
    
    printAllOutputData(npv, resultGirrDelta, resultCsrDelta);

	return npv; // OUTPUT1. Net PV
}

void initResult(double* result, const int size) {
	fill_n(result, size, 0.0);
}

void printAllInputData(
    long evaluationDate,
    long settlementDays,
    long issueDate,
    long maturityDate,
    double notional,
    double couponRate,
    int couponDayCounter,
    int numberOfCoupons,
    const long* paymentDates,
    const long* realStartDates,
    const long* realEndDates,
    int numberOfGirrTenors,
    const long* girrTenorDays,
    const double* girrRates,
    int girrDayCounter,
    int girrInterpolator,
    int girrCompounding,
    int girrFrequency,
    double spreadOverYield,
    int spreadOverYieldCompounding,
    int spreadOverYieldDayCounter,
    int numberOfCsrTenors,
    const long* csrTenorDays,
    const double* csrRates
) {
    info("------------------------------------------------------------");
    info("[Print All: Input Data]");

    info("evaluationDate : {}", evaluationDate);
    info("settlementDays : {}", settlementDays);
    info("issueDate : {}", issueDate);
    info("maturityDate : {}", maturityDate);
    info("notional : {:0.4f}", notional);
    info("couponRate : {:0.6f}", couponRate);
    info("couponDayCounter : {}", couponDayCounter);
    info("numberOfCoupons : {}", numberOfCoupons);

    for (int i = 0; i < numberOfCoupons; ++i) {
        info("paymentDates[{}] : {}", i, paymentDates[i]);
        info("realStartDates[{}] : {}", i, realStartDates[i]);
        info("realEndDates[{}] : {}", i, realEndDates[i]);
    }

    info("numberOfGirrTenors : {}", numberOfGirrTenors);
    for (int i = 0; i < numberOfGirrTenors; ++i) {
        info("girrTenorDays[{}] : {}", i, girrTenorDays[i]);
        info("girrRates[{}] : {:0.8f}", i, girrRates[i]);
    }

    info("girrDayCounter : {}", girrDayCounter);
    info("girrInterpolator : {}", girrInterpolator);
    info("girrCompounding : {}", girrCompounding);
    info("girrFrequency : {}", girrFrequency);

    info("spreadOverYield : {:0.8f}", spreadOverYield);
    info("spreadOverYieldCompounding : {}", spreadOverYieldCompounding);
    info("spreadOverYieldDayCounter : {}", spreadOverYieldDayCounter);

    info("numberOfCsrTenors : {}", numberOfCsrTenors);
    for (int i = 0; i < numberOfCsrTenors; ++i) {
        info("csrTenorDays[{}] : {}", i, csrTenorDays[i]);
        info("csrRates[{}] : {:0.8f}", i, csrRates[i]);
    }
    info("------------------------------------------------------------\n");
    info("");
}

void printAllOutputData(
    const double resultNetPV,
    const double* resultGirrDelta,
    const double* resultCsrDelta
) {
    info("[Print All: Output Data]");

    // Net PV
    info("[Net PV]");
    info("Net Present Value : {:.15f}", resultNetPV);
    info("");

    // GIRR Delta
    int girrSize = static_cast<int>(resultGirrDelta[0]);
    info("[GIRR Delta]");
    info("0. size : {}", girrSize);
    for (int i = 0; i < girrSize; ++i) {
        info("{}. tenorDay : {}", i + 1, static_cast<long>(resultGirrDelta[i + 1]));
    }
    for (int i = 0; i < girrSize; ++i) {
        info("{}. GIRR Delta : {:.15f}", i + 1 + girrSize, resultGirrDelta[i + 1 + girrSize]);
    }
    for (int i = girrSize * 2 + 1; i < 21; ++i) {
        info("{}. Empty : {}", i, resultGirrDelta[i]);
    }

    // CSR Delta
    int csrSize = static_cast<int>(resultCsrDelta[0]);
    info("");
    info("[CSR Delta]");
    info("0. size : {}", csrSize);
    for (int i = 0; i < csrSize; ++i) {
        info("{}. tenorDay : {}", i + 1, static_cast<long>(resultCsrDelta[i + 1]));
    }
    for (int i = 0; i < csrSize; ++i) {
        info("{}. CSR Delta : {:.15f}", i + 1 + csrSize, resultCsrDelta[i + 1 + csrSize]);
    }
    for (int i = csrSize * 2 + 1; i < 21; ++i) {
        info("{}. Empty : {}", i, resultCsrDelta[i]);
    }

    info("------------------------------------------------------------");
    info("");
}