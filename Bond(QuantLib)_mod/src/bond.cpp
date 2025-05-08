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
        int couponDCB,                      // ���� ��� Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        int businessDayConvention,          // ������ ���� Convention [Following = 0, Modified Following = 1, Preceding = 2, Modified Preceding = 3] 
        const long* realStartDatesSerial,   // �� ���� ������
        const long* realEndDatesSerial,     // �� ���� ������
        const long* paymentDatesSerial,     // ������ �迭
        
        int girrCnt,                        // GIRR ���� ��
        const double* girrRates,            // GIRR �ݸ�
        int girrDCB,                        // GIRR ��� Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        // int girrInterpolator,               // ������ (Linear) (����)
        // int girrCompounding,                // ���� ��� ��� (Continuous) (����)
        // int girrFrequency,                  // ���� �� (Annual) (����)
        
        double spreadOverYield,             // ä���� ���� Credit Spread
        // int spreadOverYieldCompounding,     // Continuous (����)
        int spreadOverYieldDCB,             // Credit Spread Day Count Basis [30U/360 = 0, Act/Act = 1, Act/360 = 2, Act/365 = 3, 30E/360 = 4]
        
        int csrCnt,                         // CSR ���� ��
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
        }

        // ��� ������ �ʱ�ȭ
        initResult(resultGirrDelta, 25);
        initResult(resultCsrDelta, 25);
        
        // DayCounter ������ (ĳ�ÿ� ����) ����
        vector<DayCounter> dayCounters{};
        initDayCounters(dayCounters);

        vector<BusinessDayConvention> bdcs{};
        initBDCs(bdcs);

        // revaluationDateSerial -> revaluationDate
        Date revaluationDate = Date(revaluationDateSerial);
        
        // ���� Settings�� ������ ���� (���� ��� ��꿡 �� ��¥ ���� ����)
        Settings::instance().evaluationDate() = revaluationDate;

        // ���� ������ ���� ����
        vector<Rate> couponRate_ = vector<Rate>(1, couponRate);
        
        // ���� ���� ����� ���� �ϼ���� ��� ����
        DayCounter couponDayCounter = getDayCounterByDCB(couponDCB, dayCounters); 

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
        DayCounter girrDayCounter = getDayCounterByDCB(girrDCB, dayCounters);   // DCB
        Linear girrInterpolator = Linear();                                     // ���� ����
        Compounding girrCompounding = Compounding::Continuous;                  // ���� ����
		Frequency girrFrequency = Frequency::Annual;                            // �� ���� ��
        //Frequency girrFrequency= Frequency::Semiannual;

        // GIRR Ŀ�� ����
        ext::shared_ptr<YieldTermStructure> girrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, girrRates_, girrDayCounter, girrInterpolator, girrCompounding, girrFrequency);

        // GIRR Ŀ�긦 RelinkableHandle�� ����
        RelinkableHandle<YieldTermStructure> girrCurve;
        girrCurve.linkTo(girrTermstructure);

        // spreadOverYield ���� Interest Rate ��ü�� ���� (CSR ����)
        double tmpSpreadOverYield = spreadOverYield;
        Compounding spreadOverYieldCompounding = Compounding::Continuous;       // ���� ����
        Frequency spreadOverYieldFrequency = Frequency::Annual;                 // �� ���� ��
        DayCounter spreadOverYieldDayCounter = getDayCounterByDCB(spreadOverYieldDCB, dayCounters);
        InterestRate tempRate(tmpSpreadOverYield, spreadOverYieldDayCounter, spreadOverYieldCompounding, spreadOverYieldFrequency);

        // CSR Ŀ�� ������ ��¥ �� �ݸ� ����
        vector<Date> csrDates_;
        vector<Period> csrPeriod = { Period(6, Months), Period(1, Years), Period(3, Years), Period(5, Years), Period(10, Years) };

        // ���� CSR ��¥ ����
        csrDates_.emplace_back(revaluationDate);

        // CSR �������带 ���� �ڵ� ����
        vector<Handle<Quote>> csrSpreads_;

        // ù��° CSR �������忡 spreadOverYield ����
        double spreadOverYield_ = tempRate.equivalentRate(girrCompounding, girrFrequency, girrDayCounter.yearFraction(revaluationDate, revaluationDate));
        csrSpreads_.emplace_back(ext::make_shared<SimpleQuote>(csrSpreads[0]));

        // ������ CSR �Ⱓ�� �������� ����
        for (Size dateNum = 0; dateNum < csrCnt; ++dateNum) {
            csrDates_.emplace_back(revaluationDate + csrPeriod[dateNum]);
            spreadOverYield_ = tempRate.equivalentRate(girrCompounding, girrFrequency, girrDayCounter.yearFraction(revaluationDate, csrDates_.back()));
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
            getBusinessDayConventionByBDC(businessDayConvention, bdcs),
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
            ext::shared_ptr<YieldTermStructure> bumpGirrTermstructure = ext::make_shared<ZeroCurve>(girrDates_, bumpGirrRates, girrDayCounter, girrInterpolator, girrCompounding, girrFrequency);
            
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
            Real tmpGirr = fixedRateBond.NPV() - npv;
            
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
            Real tmpCsr = fixedRateBond.NPV() - npv;
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
    dayCounters.reserve(6);

    dayCounters.emplace_back(Thirty360(Thirty360::USA));         // 0: 30U/360
    dayCounters.emplace_back(ActualActual(ActualActual::ISDA));  // 1: Act/Act
    dayCounters.emplace_back(Actual360());                       // 2: Act/360
    dayCounters.emplace_back(Actual365Fixed());                  // 3: Act/365
    dayCounters.emplace_back(Thirty360(Thirty360::European));    // 4: 30E/360
    dayCounters.emplace_back();                                  // 5: ERR
}

// Business Day Convention ��ü ����
void initBDCs(vector<BusinessDayConvention>& bdcs) {
    
    bdcs.clear();
    bdcs.reserve(6);

    bdcs.emplace_back(Following);           // 0: Following
    bdcs.emplace_back(ModifiedFollowing);   // 1: Modified Following 
    bdcs.emplace_back(Preceding);           // 2: Preceding
    bdcs.emplace_back(ModifiedPreceding);   // 3: Modified Preceding
    bdcs.emplace_back();                    // 4: ERR
}

// dcb �ڵ忡 ���� QuantLib DayCounter ��ü�� ����
DayCounter getDayCounterByDCB(unsigned int dcb, vector<DayCounter>& dayCounters) {

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

// bdc �ڵ忡 ���� QuantLib BusinessDayConvention ��ü�� ����
BusinessDayConvention getBusinessDayConventionByBDC(unsigned int businessDayConvention, vector<BusinessDayConvention>& bdcs) {

    switch (businessDayConvention) {
    case 0: // Following
        return bdcs[0];
    case 1: // Modified Following
        return bdcs[1];
    case 2: // Preceding
        return bdcs[2];
    case 3: // Modified Preceding
        return bdcs[3];
    default:
        warn("[getBusinessDayConventionBybdc] Unknown business Day Convention: default Business Day Convention applied.");
        return bdcs[4];
    }
}
