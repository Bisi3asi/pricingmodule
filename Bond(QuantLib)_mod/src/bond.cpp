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
        double notional,                    // ä�� ���� ���ݾ�
        long issueDateSerial,               // ������ (serial number)
        long maturityDateSerial,            // ������ (serial number)
        long revaluationDateSerial,         // ���� (serial number)
        long settlementDays,                // ������ offset (���� 2��)
        
        int couponCnt,                      // ���� ����
        double couponRate,                  // ���� ����
        int couponDcb,                      // ���� ��� Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        int businessDayConventionCode,          // ������ ���� Convention [Following = 0, Modified Following = 1, Preceding = 2, Modified Preceding = 3] 
        const long* realStartDatesSerial,   // �� ���� ������
        const long* realEndDatesSerial,     // �� ���� ������
        const long* paymentDatesSerial,     // ������ �迭
        
        int girrCnt,                        // GIRR ���� ��
        const double* girrRates,            // GIRR �ݸ�
        int girrDcb,                        // GIRR ��� Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        int girrInterpolatorCode,               // ������ (Linear) (����)
        int girrCompoundingCode,                // ���� ��� ��� (Continuous) (����)
        int girrFrequencyCode,                  // ���� �� (Annual) (����)
        
        double spreadOverYield,             // ä���� ���� Credit Spread
        int spreadOverYieldCompoundingCode,     // Continuous (����)
        int spreadOverYieldDcb,             // Credit Spread Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        
        int csrCnt,                         // CSR ���� ��
		const long* csrDates,               // CSR ���� (startDate�κ����� �ϼ�)
        const double* csrSpreads,           // CSR �������� (�ݸ� ����)

		unsigned short logYn,               // �α� ���� ���� ���� (0: No, 1: Yes)
       
                                            // OUTPUT 1. NETPV (���ϰ�)
        double* resultGirrDelta,            // OUTPUT 2. GIRR ����� ([index 0] array size, [index 1 ~ size -1] Girr Delta tenor, [size ~ end] Girr Delta Sensitivity)    
		double* resultCsrDelta  	        // OUTPUT 3. CSR ����� ([index 0] array size, [index 1 ~ size -1] Csr Delta tenor, [size ~ end] Csr Delta Sensitivity)   
        // ===================================================================================================
    ) {
        /* �ΰ� �ʱ�ȭ */
        // ����׿� �޼ҵ�� �Ʒ� for debug �޼ҵ带 ����
        if (logYn == 1) {
            initLogger("Bond.log"); // ���� ���ϸ� ����
        }

        info("==============[Bond Logging Started!]==============");
        /*
        printAllInputData( // Input ������ �α�
            maturityDate, revaluationDate, exchangeRate,
            buySideCurrency, notionalForeign, buySideDCB, buySideDcCurve,
            buyCurveDataSize, buyCurveYearFrac, buyMarketData,
            sellSideCurrency, notionalDomestic, sellSideDCB, sellSideDcCurve,
            sellCurveDataSize, sellCurveYearFrac, sellMarketData,
            calType, logYn
        );
        */

        // ��� ������ �ʱ�ȭ
        initResult(resultGirrDelta, 25);
        initResult(resultCsrDelta, 25);
        
        // DayCounter ������ (ĳ�ÿ� ����) ����
        vector<DayCounter> dayCounters{};
        vector<BusinessDayConvention> dayConventions{};
		vector<Frequency> frequencies{};

        initDayCounters(dayCounters);
        initDayConventions(dayConventions);
		initFrequencies(frequencies);


        // revaluationDateSerial -> revaluationDate
        Date revaluationDate = Date(revaluationDateSerial);
        
        // ���� Settings�� ������ ���� (���� ��� ��꿡 �� ��¥ ���� ����)
        Settings::instance().evaluationDate() = revaluationDate;

        // ���� ������ ���� ����
        vector<Rate> couponRate_ = vector<Rate>(1, couponRate);
        
        // ���� ���� ����� ���� �ϼ���� ��� ����
        DayCounter couponDayCounter = getDayCounterByCode(couponDcb, dayCounters); 

        // GIRR Ŀ�� ������ ��¥ �� �ݸ� ����
        vector<Date> girrDates_;
        vector<Real> girrRates_;

        // GIRR Ŀ���� �ֿ� �Ⱓ ���� (���� ǥ�� �׳�)
        vector<Period> girrPeriod = { 
            Period(3, Months), Period(6, Months), 
            Period(1, Years), Period(2, Years), Period(3, Years), Period(5, Years),
            Period(10, Years), Period(15, Years), Period(20, Years), Period(30, Years) 
        };

        // GIRR Ŀ�� ������ (revaluationDate ����) �Է�
        girrDates_.emplace_back(revaluationDate);
        girrRates_.emplace_back(girrRates[0]);
        
        // ������ GIRR Ŀ�� ���� ��� �Է�
        for (int dateNum = 0; dateNum < girrCnt; ++dateNum) {
            girrDates_.emplace_back(revaluationDate + girrPeriod[dateNum]);
            girrRates_.emplace_back(girrRates[dateNum] + spreadOverYield);
        }
        
        // GIRR Ŀ�� ��� ��� �߰� ���
        DayCounter girrDayCounter = getDayCounterByCode(girrDcb, dayCounters);   // DCB
        Linear girrInterpolator = Linear();                                     // ���� ����
        Compounding girrCompounding = Compounding::Continuous;                  // ���� ����
		Frequency girrFrequency = Frequency::Annual;                            // �� ���� ��

        // GIRR Ŀ�� ����
        ext::shared_ptr<YieldTermStructure> girrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, girrRates_, girrDayCounter, girrInterpolatorCode, girrCompoundingCode, girrFrequencyCode);

        // GIRR Ŀ�긦 RelinkableHandle�� ����
        RelinkableHandle<YieldTermStructure> girrCurve;
        girrCurve.linkTo(girrTermstructure);

        // spreadOverYield ���� Interest Rate ��ü�� ���� (CSR ����)
        double tmpSpreadOverYield = spreadOverYield;
        Compounding spreadOverYieldCompounding = Compounding::Continuous;       // ���� ����
        Frequency spreadOverYieldFrequency = Frequency::Annual;                 // �� ���� ��
        DayCounter spreadOverYieldDayCounter = getDayCounterByCode(spreadOverYieldDcb, dayCounters);
        InterestRate tempRate(tmpSpreadOverYield, spreadOverYieldDayCounter, spreadOverYieldCompoundingCode, spreadOverYieldFrequency);

        // CSR Ŀ�� ������ ��¥ �� �ݸ� ����
        vector<Date> csrDates_;
        vector<Period> csrPeriod = { Period(6, Months), Period(1, Years), Period(3, Years), Period(5, Years), Period(10, Years) };

        // ���� CSR ��¥ ����
        csrDates_.emplace_back(revaluationDate);

        // CSR �������带 ���� �ڵ� ����
        vector<Handle<Quote>> csrSpreads_;

        // ù��° CSR �������忡 spreadOverYield ����
        double spreadOverYield_ = tempRate.equivalentRate(girrCompoundingCode, girrFrequencyCode, girrDayCounter.yearFraction(revaluationDate, revaluationDate));
        csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads[0]));

        // ������ CSR �Ⱓ�� �������� ����
        for (Size dateNum = 0; dateNum < csrCnt; ++dateNum) {
            csrDates_.emplace_back(revaluationDate + csrPeriod[dateNum]);
            spreadOverYield_ = tempRate.equivalentRate(girrCompoundingCode, girrFrequencyCode, girrDayCounter.yearFraction(revaluationDate, csrDates_.back()));
            csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads[dateNum]));
        }

        // GIRR + CSR �������� Ŀ�� ����
        ext::shared_ptr<ZeroYieldStructure> discountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, csrSpreads_, csrDates_);
        
        // Discounting Ŀ�� ����
        RelinkableHandle<YieldTermStructure> discountingCurve;
        discountingCurve.linkTo(discountingTermStructure);
        
        // Discounting ���� ���� (ä�� ���� ����)
        auto bondEngine = ext::make_shared<DiscountingBondEngine>(discountingCurve);
        
        // ���� ���� ����
        vector<Date> couponSch_;
        couponSch_.emplace_back(realStartDatesSerial[0]);
        for (Size schNum = 0; schNum < couponCnt; ++schNum) {
            couponSch_.emplace_back(realEndDatesSerial[schNum]);
        }

        // Schedule ��ü ����
        Schedule fixedBondSchedule_(couponSch_);
        
        // Fixed Rate Bond ����
        FixedRateBond fixedRateBond(
            settlementDays, 
            notional, 
            fixedBondSchedule_,
            couponRate_, 
            couponDayCounter, 
            getBusinessDayConventionByCode(businessDayConventionCode, bdcs),
            100.0
        );

        // Fixed Rate Bond ���� ����
        fixedRateBond.setPricingEngine(bondEngine);

        // ä���� Net PV ���
        Real npv = fixedRateBond.NPV();

        //Real girrBump = 0.0001;
        Real girrBump = 0.01;
        vector<Real> discountingGirr;

        // GIRR Delta ���
        for (int bumpNum = 1; bumpNum < girrRates_.size(); ++bumpNum) {
            // GIRR Ŀ���� �ݸ��� bumping (1% ���)
            vector<Rate> bumpGirrRates = girrRates_;
            bumpGirrRates[bumpNum] += girrBump;

            // bump�� �ݸ��� ���ο� ZeroCurve ����
            ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates, girrDayCounter, girrInterpolatorCode, girrCompoundingCode, girrFrequencyCode);
            
            // RelinkableHandle�� bump�� Ŀ�� ����
            RelinkableHandle<YieldTermStructure> bumpGirrCurve;
            bumpGirrCurve.linkTo(bumpGirrTermstructure);
            
            // CSR spread�� ������ bump GIRR ��� ���� Ŀ�� ����
            ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure = ext::make_shared<PiecewiseZeroSpreadedTermStructure>(bumpGirrCurve, csrSpreads_, csrDates_);
            
            // ���� Ŀ�긦 RelinkableHandle�� wrapping
            RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
            bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
            bumpDiscountingCurve->enableExtrapolation(); // �ܻ� ���

            // discountingCurve�� ���ο� pricing engine ����
            auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve);
            
			// Fixed Rate Bond�� ����
            fixedRateBond.setPricingEngine(bumpBondEngine);
            
            // bump �� NPV - ���� NPV = �ݸ� �ΰ��� (delta)
            Real tmpGirr = (fixedRateBond.NPV() - npv) / 0.0001;
            
            // delta ����
            discountingGirr.emplace_back(tmpGirr);
        }

        // CSR ��Ÿ ���
        Real csrBump = 0.01;
        vector<Real> discountingCsr;
        for (int bumpNum = 1; bumpNum < csrSpreads_.size(); ++bumpNum) { 
           // bump �� CSR spread ���� �ʱ�ȭ
            vector<Handle<Quote>> bumpCsrSpreads_;
            
            //ù��° spread �׸��� ���Ǻη� bump ����
            if (bumpNum == 1) {
                bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[0]->value() + csrBump));
            }
            else {
                bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[0]->value()));
            }

            // ������ spread �׸�鵵 bumpNum ��ġ���� bump ����
            for (Size i = 1; i < csrSpreads_.size(); ++i) {
                Real bump = (i == bumpNum) ? csrBump : 0.0;
                bumpCsrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads_[i]->value() + bump));
            }

            // GIRR Ŀ�꿡 bump�� CSR �������� ������ ���ο� ���� Ŀ�� ����
            ext::shared_ptr<ZeroYieldStructure> bumpDiscountingTermStructure =
                ext::make_shared<PiecewiseZeroSpreadedTermStructure>(girrCurve, bumpCsrSpreads_, csrDates_);
            
            // ���ο� ���� Ŀ�긦 RelinkableHandle�� wrapping
            RelinkableHandle<YieldTermStructure> bumpDiscountingCurve;
            bumpDiscountingCurve.linkTo(bumpDiscountingTermStructure);
            bumpDiscountingCurve->enableExtrapolation();
            
			// discountingCurve�� ���ο� pricing engine ����
            auto bumpBondEngine = ext::make_shared<DiscountingBondEngine>(bumpDiscountingCurve);
            
            // fixedRateBond�� ����
            fixedRateBond.setPricingEngine(bumpBondEngine);
            
			// bump �� NPV - ���� NPV = spread �ΰ��� (delta)
            Real tmpCsr = (fixedRateBond.NPV() - npv) / 0.0001;
            discountingCsr.emplace_back(tmpCsr);
            // cout << "Csr[" << bumpNum << "]:" << tmpCsr * 10000.0 << endl;
        }
        
        /* for Debug (bond Price) */
        //Real cleanPrice = fixedRateBond.cleanPrice() / 100.0 * notional;
        //Real dirtyPrice = fixedRateBond.dirtyPrice() / 100.0 * notional;
        //Real accruedInterest = fixedRateBond.accruedAmount() / 100.0 * notional;
        
        //cout << "cleanPrice(NPV): " << fixed << setprecision(10) << cleanPrice << endl;
        //cout << "dirtyPrice(NPV): " << fixed << setprecision(10) << dirtyPrice << endl;
        //cout << "accuredInterest(NPV): " << fixed << setprecision(10) << accruedInterest << endl;

        // ����� (Girr Delta Sensitivity)
        for (int i = 0; i <= discountingGirr.size(); ++i) {
            if (i == 0) {
                resultGirrDelta[i] = discountingGirr.size();
            }
            else resultGirrDelta[i] = discountingGirr[i-1];
        }

        // ����� (Csr Delta Sensitivitity)
        for (int i = 0; i <= discountingCsr.size(); ++i) {
            if (i == 0) {
                resultCsrDelta[i] = discountingCsr.size();
            }
            else resultCsrDelta[i] = discountingCsr[i-1];
        }

        return npv;
    }
}

// ����� �ʱ�ȭ
void initResult(double* result, int size) {
    
    fill_n(result, size, 0.0);
}

// Day Counter ��ü ����
void initDayCounters(vector<DayCounter>& dayCounters) {

    dayCounters.clear();
    dayCounters.reserve(18);

    dayCounters.emplace_back(Actual365Fixed());                     // 0: Actual365
    dayCounters.emplace_back(Actual360());                          // 1: Act/360
    // dayCounters.emplace_back(Actual364());                       // 2: Act/364
    // dayCounters.emplace_back(Actual36525());                     // 3: Act/36525
    // dayCounters.emplace_back(Actual366());                       // 4: Act/366
    dayCounters.emplace_back(ActualActual(ActualActual::ISDA));     // 5: Act/Act
    dayCounters.emplace_back(Business252());                        // 6: Business252
    dayCounters.emplace_back(OneDayCounter());                      // 7: OneDayCounter
    dayCounters.emplace_back(SimpleDayCounter());                   // 8: SimpleDayCounter
    dayCounters.emplace_back(Thirty360(Thirty360::USA));            // 9: 30U/360 USA
    dayCounters.emplace_back(Thirty360(Thirty360::BondBasis));      // 10: 30U/360 BondBasis
    dayCounters.emplace_back(Thirty360(Thirty360::European));       // 11: 30U/360 European
	dayCounters.emplace_back(Thirty360(Thirty360::EurobondBasis));  // 12: 30U/360 EurobondBasis
	dayCounters.emplace_back(Thirty360(Thirty360::Italian));        // 13: 30U/360 Italian
	dayCounters.emplace_back(Thirty360(Thirty360::German));         // 14: 30U/360 German
	dayCounters.emplace_back(Thirty360(Thirty360::ISMA));           // 15: 30U/360 ISMA
	dayCounters.emplace_back(Thirty360(Thirty360::ISDA));           // 16: 30U/360 ISDA
	dayCounters.emplace_back(Thirty360(Thirty360::NASD));           // 17: 30U/360 NASD
    dayCounters.emplace_back();                                     // 18: ERR (Default None)
}

// Business Day Convention ��ü ����
void initDayConventions(vector<BusinessDayConvention>& dayConventions) {

    dayConventions.clear();
    dayConventions.reserve(5);

    dayConventions.emplace_back(ModifiedFollowing);             // 0: Modified Following
    dayConventions.emplace_back(Following);                     // 1: Following
    dayConventions.emplace_back(Preceding);                     // 2: Preceding
    dayConventions.emplace_back(ModifiedPreceding);             // 3: Modified Preceding
    dayConventions.emplace_back(Unadjusted);                    // 4: Unadjusted
	dayConventions.emplace_back(HalfMonthModifiedFollowing);    // 5: HalfMonthModifiedFollowing
	dayConventions.emplace_back(Nearest);                       // 6: Nearest
	dayConventions.emplace_back();                              // 7: ERR (Default Following)
}

// Frequency ��ü ����
void initFrequencies(vector<Frequency>& frequencies) {
    frequencies.clear();
    frequencies.reserve(13);

    frequencies.emplace_back(Annual);           // 0: Annual (����)
    frequencies.emplace_back(Semiannual);       // 1: Semiannual (�ݱ�)
    frequencies.emplace_back(Quarterly);        // 2: Quarterly (�б�)
    frequencies.emplace_back(Monthly);          // 3: Monthly (�Ŵ�)
    frequencies.emplace_back(Bimonthly);        // 4: Bimonthly (2��)
    frequencies.emplace_back(Weekly);           // 5: Weekly (����)
    frequencies.emplace_back(Biweekly);         // 6: Biweekly (2��)
    frequencies.emplace_back(Daily);            // 7: Daily (����)
    frequencies.emplace_back(NoFrequency);      // 8: No Frequency (�ֱ� ����)
    frequencies.emplace_back(Once);             // 9: Once  (���� 1ȸ)
    frequencies.emplace_back(EveryFourthMonth); // 10: Every Fourth Month (4��)
    frequencies.emplace_back(EveryFourthWeek);  // 11: Every Fourth Week  (4��)
    frequencies.emplace_back(OtherFrequency);   // 12: ERR (Default Other Frequency)
}

// Interpolator ��ü ����
void initInterpolators(vector<Linear>& interpolators) {
    interpolators.clear();
    interpolators.reserve(1);

    interpolators.emplace_back(Linear());   // 0: Linear
	interpolators.emplace_back();           // 1: ERR (Default None)
}

// Compounding ��ü ����
void initCompoundings(vector<Compounding>& compoundings) {
    compoundings.clear();
    compoundings.reserve(4);

    compoundings.emplace_back(Compounding::Continuous);             // 0: Continuous
	compoundings.emplace_back(Compounding::Simple);                 // 1: Simple
	compoundings.emplace_back(Compounding::Compounded);             // 2: Compounded
	compoundings.emplace_back(Compounding::SimpleThenCompounded);   // 3: SimpleThenCompounded
	compoundings.emplace_back(Compounding::CompoundedThenSimple);   // 4: CompoundedThenSimple
	compoundings.emplace_back();                                    // 5: ERR (Default None)
}

DayCounter getDayCounterByCode(const int code, const vector<DayCounter>& dayCounters) {

    if (code >= 0 && code < static_cast<int>(dayCounters.size())) {
        return dayCounters[code];
    }

    warn("[getDayCounterByCode] Unknown day count code: default DayCounter applied.");
    return dayCounters[18]; // 18: ERR (Default None)
}

// �ڵ忡 ���� QuantLib BusinessDayConvention ��ü�� ����
BusinessDayConvention getBusinessDayConventionByCode(const int code, const vector<BusinessDayConvention>& dayConventions) {

    if (code >= 0 && code < static_cast<int>(dayConventions.size())) {
        return dayConventions[code];
    }

    warn("[getBusinessDayConventionByCode] Unknown business day convention code: default applied.");
    return dayConventions[7]; // 7: ERR (Default Following)
}

// �ڵ忡 ���� QuantLib Frequency ��ü�� ����
Frequency getFrequencyByCode(const int code, const vector<Frequency>& frequencies) {

    switch (code) {
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
