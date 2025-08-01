// common.cpp
#include "common.hpp"

// Date 변환 함수
QuantLib::Date makeDateFromInt(int intDate) {
    int year = intDate / 10000;
    int month = (intDate / 100) % 100;
    int day = intDate % 100;
    return Date(day, static_cast<QuantLib::Month>(month), year);
}

// Calendar 변환 함수
QuantLib::Calendar makeCalendarFromInt(int intCalendar) {
    switch (intCalendar) {
    case 0: return QuantLib::SouthKorea();
    case 1: return QuantLib::UnitedStates(QuantLib::UnitedStates::Settlement);
    case 2: return QuantLib::UnitedStates(QuantLib::UnitedStates::NYSE);
    case 3: return QuantLib::UnitedStates(QuantLib::UnitedStates::GovernmentBond);
    case 4: return QuantLib::UnitedStates(QuantLib::UnitedStates::NERC);
    case 5: return QuantLib::UnitedStates(QuantLib::UnitedStates::LiborImpact);
    case 6: return QuantLib::UnitedStates(QuantLib::UnitedStates::FederalReserve);
    case 7: return QuantLib::UnitedStates(QuantLib::UnitedStates::SOFR);
    case 8: return QuantLib::TARGET();
    case 9: return QuantLib::UnitedKingdom();
    case 10: return QuantLib::Japan();
    case 11: return QuantLib::Canada();
    case 12: return QuantLib::Switzerland();
    case 13: return QuantLib::Australia();
    case 14: return QuantLib::NewZealand();
    case 15: return QuantLib::Singapore();
    case 16: return QuantLib::HongKong(); // CNH는 없음. 동일하게 처리.
    case 17: return QuantLib::China();
    case 18: return QuantLib::HongKong(); // CNH도 마찬가지로 HongKong 달력 사용
    case 19: return QuantLib::India();
    case 20: return QuantLib::Thailand();
    case 21: return QuantLib::Turkey();
    case 22: return QuantLib::Taiwan();
    case 23: return QuantLib::Norway();
    case 24: return QuantLib::Denmark();
    case 25: return QuantLib::Sweden();
    case 26: return QuantLib::Russia();
    case 27: return QuantLib::Brazil();
    case 28: return QuantLib::Hungary();
    case 29: return QuantLib::Indonesia();
    case 30: return QuantLib::Mexico();
    case 31: return QuantLib::Poland();
    case 32: return QuantLib::Austria();
    case 38: return QuantLib::Chile();
    case 39: return QuantLib::CzechRepublic();
    case 40: return QuantLib::Germany();
    case 43: return QuantLib::Finland();
    case 45: return QuantLib::France();
    case 46: return QuantLib::Israel();
    case 47: return QuantLib::Italy();
    case 62: return QuantLib::SaudiArabia();
    case 66: return QuantLib::SouthAfrica();
    default: return QuantLib::NullCalendar();
    }
}


// DayCounter 변환 함수
QuantLib::DayCounter makeDayCounterFromInt(int dayCounterCode) {
    switch (dayCounterCode) {
    case 0: return QuantLib::Actual365Fixed();
    case 1: return QuantLib::Actual360();
    case 2: return QuantLib::Actual364();
    case 3: return QuantLib::Actual36525();
    case 4: return QuantLib::Actual366();
    case 5: return QuantLib::ActualActual(QuantLib::ActualActual::Actual365);
    case 6: return QuantLib::Business252();
    case 7: return QuantLib::OneDayCounter();
    case 8: return QuantLib::SimpleDayCounter();
    case 9: return QuantLib::Thirty360(QuantLib::Thirty360::USA);
    case 10: return QuantLib::Thirty360(QuantLib::Thirty360::BondBasis);
    case 11: return QuantLib::Thirty360(QuantLib::Thirty360::European);
    case 12: return QuantLib::Thirty360(QuantLib::Thirty360::EurobondBasis);
    case 13: return QuantLib::Thirty360(QuantLib::Thirty360::Italian);
    case 14: return QuantLib::Thirty360(QuantLib::Thirty360::German);
    case 15: return QuantLib::Thirty360(QuantLib::Thirty360::ISMA);
    case 16: return QuantLib::Thirty360(QuantLib::Thirty360::ISDA);
    case 17: return QuantLib::Thirty360(QuantLib::Thirty360::NASD);
    case 18: return QuantLib::Thirty365();
    default: return QuantLib::Actual365Fixed();
    }
}

// Compounding 변환 함수
QuantLib::Compounding makeCompoundingFromInt(int compoundingCode) {
    switch (compoundingCode) {
    case 0: return QuantLib::Continuous;
    case 1: return QuantLib::Simple;
    case 2: return QuantLib::Compounded;
    case 3: return QuantLib::SimpleThenCompounded;
    case 4: return QuantLib::CompoundedThenSimple;
    default: return QuantLib::Continuous;
    }
}

// Frequency 변환 함수
QuantLib::Frequency makeFrequencyFromInt(int frequencyCode) {
    switch (frequencyCode) {
    case 0: return QuantLib::Annual;
    case 1: return QuantLib::Semiannual;
    case 2: return QuantLib::Quarterly;
    case 3: return QuantLib::Monthly;
    case 4: return QuantLib::Bimonthly;
    case 5: return QuantLib::Weekly;
    case 6: return QuantLib::Biweekly;
    case 7: return QuantLib::Daily;
    case 8: return QuantLib::NoFrequency;
    case 9: return QuantLib::Once;
    case 10: return QuantLib::EveryFourthMonth;
    case 11: return QuantLib::EveryFourthWeek;
    default: return QuantLib::Annual;
    }
}

// BusinessDayConvention 변환 함수
QuantLib::BusinessDayConvention makeBDCFromInt(int bdcCode) {
    switch (bdcCode) {
    case 0: return QuantLib::ModifiedFollowing;
    case 1: return QuantLib::Following;
    case 2: return QuantLib::Preceding;
    case 3: return QuantLib::ModifiedPreceding;
    case 4: return QuantLib::Unadjusted;
    case 5: return QuantLib::HalfMonthModifiedFollowing;
    case 6: return QuantLib::Nearest;
    default: return QuantLib::Following;
    }
}

// Currency 변환 함수
QuantLib::Currency makeCurrencyFromInt(int currencyCode) {
    switch (currencyCode) {
    case 0: return QuantLib::KRWCurrency();
    default: return QuantLib::KRWCurrency();
    }
}

// Boolean 변환 함수
bool makeBoolFromInt(int boolCode) {
    return boolCode == 1;
}

// VolatilityType 변환 함수
QuantLib::VolatilityType makeVolatilityTypeFromInt(int volTypeCode) {
    switch (volTypeCode) {
    case 0: return QuantLib::ShiftedLognormal;
    case 1: return QuantLib::Normal;
    default: return QuantLib::Normal;
    }
}

// RateAveraging 변환 함수
ext::optional<QuantLib::RateAveraging::Type> makeRateAveragingFromInt(int rateTypeCode) {
    switch (rateTypeCode) {
    case 0: return QuantLib::RateAveraging::Type::Simple;
    case 1: return QuantLib::RateAveraging::Type::Compound;
    default: return ext::nullopt;
    }
}

// SwapType 변환 함수
QuantLib::Swap::Type makeSwapTypeFromInt(int swapTypeCode) {
    switch (swapTypeCode) {
    case 0: return QuantLib::Swap::Payer;
    case 1: return QuantLib::Swap::Receiver;
    default: return QuantLib::Swap::Payer;
    }
}

QuantLib::Option::Type makeOptionTypeFromInt(int code) {
    switch (code) {
    case 0: return QuantLib::Option::Call;
    case 1: return QuantLib::Option::Put;
    default: return QuantLib::Option::Call; // 기본값
    }
}

// Strike::Type makeStrikeTypeFromInt(int code) {
//     switch (code) {
//         case 0: return Strike::Type::Fixed;
//         case 1: return Strike::Type::Floating;
//         default: return Strike::Type::Fixed;
//     }
// }

QuantLib::Average::Type makeAverageTypeFromInt(int code) {
    switch (code) {
    case 0: return QuantLib::Average::Arithmetic;
    case 1: return QuantLib::Average::Geometric;
    default: return QuantLib::Average::Arithmetic;
    }
}


// OptionOwner 변환 함수
int makeOptionOwnerFromInt(int ownerCode) {
    switch (ownerCode) {
    case 0: return 0;  // Us
    case 1: return 1;  // Them
    default: return 0;
    }
}

QuantLib::Period makePeriodFromIntParts(int numberPart, int letterPart) {
    switch (letterPart) {
    case 0: return QuantLib::Period(numberPart, QuantLib::Days);
    case 1: return QuantLib::Period(numberPart, QuantLib::Months);
    case 2: return QuantLib::Period(numberPart, QuantLib::Years);
    default: return QuantLib::Period(0, QuantLib::Days); // 기본값
    }
}



std::vector<Date> makeDatesVectorFromIntArray(int arrSize, int intDates[]) {
    std::vector<Date> dates(arrSize);
    for (size_t i = 0; i < static_cast<Size>(arrSize); ++i)
    {
        dates[i] = makeDateFromInt(intDates[i]);
    }
    return dates;
}

std::vector<Rate> makeRatesVectorFromDoubleArray(int arrSize, double doubleRates[]) {
    std::vector<Rate> rates;
    for (size_t i = 0; i < static_cast<Size>(arrSize); ++i)
    {
        rates.push_back(doubleRates[i]);
    }
    return rates;
}

// TargetRedemptionForward::AccumulationDirection makeAccumulationDirectionFromInt(int code) {
//     switch (code) {
//         case 0: return TargetRedemptionForward::ProfitOnly;
//         case 1: return TargetRedemptionForward::LossOnly;
//         default: return TargetRedemptionForward::ProfitOnly;
//     }
// }

// TargetRedemptionForward::FinalAmtType makeFinalAmtTypeFromInt(int code) {
//     switch (code) {
//         case 0: return TargetRedemptionForward::NoPayment;
//         case 1: return TargetRedemptionForward::UntilTarget;
//         case 2: return TargetRedemptionForward::FullPayment;
//         default: return TargetRedemptionForward::UntilTarget;
//     }
// }

ext::shared_ptr<StrikedTypePayoff> makeStrikeTypePayoff(int strikeType, int optionType,
    QuantLib::Real strike,
    ext::optional<QuantLib::Real> secondStrike,
    ext::optional<QuantLib::Real> cashPayoff) {
    QuantLib::Option::Type type = makeOptionTypeFromInt(optionType);
    using namespace QuantLib;

    switch (strikeType) {
    case 0: return ext::make_shared<PlainVanillaPayoff>(type, strike);

    case 1: return ext::make_shared<PercentageStrikePayoff>(type, strike);

    case 2: return ext::make_shared<AssetOrNothingPayoff>(type, strike);

    case 3: QL_REQUIRE(cashPayoff.has_value(), "CashOrNothingPayoff requires a valid cashPayoff");
        return ext::make_shared<CashOrNothingPayoff>(type, strike, *cashPayoff);

    case 4: QL_REQUIRE(secondStrike.has_value(), "GapPayoff requires a valid secondStrike");
        return ext::make_shared<GapPayoff>(type, strike, *secondStrike);

    case 5: QL_REQUIRE(secondStrike.has_value(), "SuperFundPayoff requires a valid secondStrike");
        return ext::make_shared<SuperFundPayoff>(strike, *secondStrike);

    case 6: QL_REQUIRE(secondStrike.has_value(), "SuperSharePayoff requires a valid secondStrike");
        QL_REQUIRE(cashPayoff.has_value(), "SuperSharePayoff requires a valid cashPayoff");
        return ext::make_shared<SuperSharePayoff>(strike, *secondStrike, *cashPayoff);

    default: QL_FAIL("Unknown StrikedTypePayoff: " << strikeType);
    }
}

QuantLib::Barrier::Type makeBarrierTypeFromInt(int code) {
    switch (code) {
    case 0: return QuantLib::Barrier::DownIn;
    case 1: return QuantLib::Barrier::UpIn;
    case 2: return QuantLib::Barrier::DownOut;
    case 3: return QuantLib::Barrier::UpOut;
    default: return QuantLib::Barrier::UpIn;
    }
}


// 일수를 Period로 근사 변환해주는 함수 (단순한 일수 기준)
Period makePeriodFromDays(int days) {
    if (days < 30) {
        return Period(days, Days);
    }

    double months = days / 30.44;  // 평균 월 길이 기준
    int roundedMonths = static_cast<int>(std::round(months));

    if (roundedMonths % 12 == 0) {
        int years = roundedMonths / 12;
        return Period(years, Years);
    }
    else {
        return Period(roundedMonths, Months);
    }
}

std::vector<Period> makePeriodArrayFromTenorDaysArray(const int* girrTenorDays, const int numberOfGirrTenors) {
    std::vector<Period> result;
    for (int i = 0; i < numberOfGirrTenors; ++i) {
        result.push_back(makePeriodFromDays(girrTenorDays[i]));
    }
    return result;
}

DateGeneration::Rule makeScheduleGenRuleFromInt(int code) {
    switch (code) {
    case 0: return DateGeneration::Backward;
    case 1: return DateGeneration::Forward;
    case 2: return DateGeneration::Zero;
    case 3: return DateGeneration::ThirdWednesday;
    case 4: return DateGeneration::Twentieth;
    case 5: return DateGeneration::TwentiethIMM;
    case 6: return DateGeneration::OldCDS;
    case 7: return DateGeneration::CDS;
    case 8: return DateGeneration::CDS2015;
    default: return DateGeneration::Backward; // 기본값
    }
}

void initResult(double* result, const int size) {

    std::fill_n(result, size, 0.0);
}

void processResultArray(std::vector<Real> tenors, std::vector<Real> sensitivities, Size originalSize, double* resultArray) {
    std::vector<double> filteredTenor;
    std::vector<double> filteredSensitivity;

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

void freeArray(double* arr) {
    if (arr != nullptr) {
        free(arr);
    }
}

/* FOR DEBUG */
std::string qDateToString(const Date& date) {

    std::ostringstream oss;
    oss << date;
    return oss.str();
}