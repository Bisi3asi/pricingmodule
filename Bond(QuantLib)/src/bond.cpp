#include <iostream>

#include "ql/time/calendars/southkorea.hpp"
#include "ql/termstructures/yield/piecewisezerospreadedtermstructure.hpp"
#include "ql/termstructures/yield/zerocurve.hpp"
#include "ql/quotes/simplequote.hpp"
#include "ql/pricingengines/bond/discountingbondengine.hpp"
#include "ql/instruments/bonds/zerocouponbond.hpp"
#include "ql/instruments/bonds/fixedratebond.hpp"
#include "ql/time/schedule.hpp"
#include "ql/time/daycounters/actualactual.hpp"

#include "bond.h"

// namespace
using namespace QuantLib;
using namespace std;

extern "C" void EXPORT printSettlementDate(
	long tradeDate,
	long settlementDays
) {
	// space where to create business logic using QuantLib
	cout << "Settlement Date: " << Date(tradeDate + settlementDays) << endl;
}

extern "C" double EXPORT ZeroBondTest(
    long evaluationDate,            // ���� (serial number, ��: 46164)
    long settlementDays,            // ������ offset (���� 2��)
    long issueDate,                 // ������
    long maturityDate,              // ������
    double notional,                // ä�� ����
    double couponRate,              // ���� ����
    int couponDayCounter,           // DayCounter code (��: 5 = Actual/Actual(Bond))
    int numberOfCoupons,            // ���� ����
    const long* paymentDates,       // ������ �迭
    const long* realStartDates,     // �� ���� ������
    const long* realEndDates,       // �� ���� ������
    int numberOfGirrTenors,         // GIRR ���� ��
    const long* girrTenorDays,      // GIRR ���� (startDate�κ����� �ϼ�)
    const double* girrRates,        // GIRR �ݸ�
    int girrDayCounter,             // GIRR DayCounter (��: 1 = Actual/365)
    int girrInterpolator,           // ������ (��: 1 = Linear)
    int girrCompounding,            // ���� ��� ��� (��: 1 = Continuous)
    int girrFrequency,              // ���� �� (��: 1 = Annual)
    double spreadOverYield,         // ä���� ���� Credit Spread
    int spreadOverYieldCompounding, // Continuous
    int spreadOverYieldDayCounter,  // Actual/365
    int numberOfCsrTenors,          // CSR ���� ��
    const long* csrTenorDays,       // CSR ���� (startDate�κ����� �ϼ�)
    const double* csrSpreads        // CSR �������� (�ݸ� ����)

) {

    std::cout.precision(15);
    Date asOfDate_ = Date(evaluationDate);
    Settings::instance().evaluationDate() = asOfDate_;
    Size settlementDays_ = settlementDays;
    Real notional_ = notional;
    std::vector<Rate> couponRate_ = std::vector<Rate>(1, couponRate);
    DayCounter couponDayCounter_ = ActualActual(ActualActual::ISDA); // TODO ��ȯ �Լ� ���� (DayCounter)
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
    // TODO ��ȯ �Լ� ����
    DayCounter girrDayCounter_ = Actual365Fixed();
    Linear girrInterpolator_ = Linear(); // TODO ��ȯ �Լ� ���� (Interpolator)
    Compounding girrCompounding_ = Compounding::Continuous; // TODO ��ȯ �Լ� ���� (Compounding)
    Frequency girrFrequency_ = Frequency::Annual;
    ext::shared_ptr<YieldTermStructure> girrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, girrRates_,
        girrDayCounter_, girrInterpolator_, girrCompounding_, girrFrequency_);
    RelinkableHandle<YieldTermStructure> girrCurve;
    girrCurve.linkTo(girrTermstructure);
    double tmpSpreadOverYield = spreadOverYield;
    Compounding spreadOverYieldCompounding_ = Compounding::Continuous; // TODO ��ȯ �Լ� ���� (Compounding)
    DayCounter spreadOverYieldDayCounter_ = Actual365Fixed();  // TODO ��ȯ �Լ� ���� (DayCounter)
    InterestRate tempRate(tmpSpreadOverYield, spreadOverYieldDayCounter_, spreadOverYieldCompounding_, Frequency::Annual);
    std::vector<Date> csrDates_;
    std::vector<Period> csrPeriod = { Period(6, Months), Period(1, Years), Period(3, Years), Period(5, Years), Period(10, Years) };
    csrDates_.emplace_back(asOfDate_);
    std::vector<Handle<Quote>> csrSpreads_;
    double spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_, girrDayCounter_.yearFraction(asOfDate_, asOfDate_));
    csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads[0]));
    //    csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads[0]+spreadOverYield_));
    for (Size dateNum = 0; dateNum < numberOfCsrTenors; ++dateNum) {
        //        csrDates_.emplace_back(asOfDate_ + csrTenorDays[dateNum]);
        csrDates_.emplace_back(asOfDate_ + csrPeriod[dateNum]);
        spreadOverYield_ = tempRate.equivalentRate(girrCompounding_, girrFrequency_, girrDayCounter_.yearFraction(asOfDate_, csrDates_.back()));
        csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads[dateNum]));
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
        ModifiedFollowing, // TODO ��ȯ �Լ� ���� (DayConvention)
        100.0);
    fixedRateBond.setPricingEngine(bondEngine);
    //    //������ �迭
    //    const Leg& tmpLeg = fixedRateBond.cashflows();
    //    std::vector<Real> tmpCf;
    //    std::vector<DiscountFactor> tmpDf;
    //    for (const auto& cf : tmpLeg) {
    //        tmpCf.emplace_back(cf->amount());
    //        tmpDf.emplace_back(discountingCurve->discount(cf->date()));
    //    }
    Real npv = fixedRateBond.NPV();
    std::cout << "NPV: " << npv << std::endl;
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
        disCountingGirr.emplace_back(tmpGirr);
        std::cout << "Girr[" << bumpNum << "]:" << tmpGirr * 10000.0 << std::endl;
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
        disCountingCsr.emplace_back(tmpCsr);
        std::cout << "Csr[" << bumpNum << "]:" << tmpCsr * 10000.0 << std::endl;
    }
    Real cleanPrice = fixedRateBond.cleanPrice() / 100.0 * notional;
    Real dirtyPrice = fixedRateBond.dirtyPrice() / 100.0 * notional;
    Real accruedInterest = fixedRateBond.accruedAmount() / 100.0 * notional;
    return npv;
}

