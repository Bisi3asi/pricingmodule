#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <memory>
#include <ql/time/date.hpp>
#include <ql/types.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/frequency.hpp>
#include <ql/time/businessdayconvention.hpp>
#include <ql/compounding.hpp>
#include <ql/time/daycounters/all.hpp>
#include <ql/time/date.hpp>
#include <ql/time/frequency.hpp>
#include <ql/time/calendars/all.hpp>
#include <ql/time/calendars/unitedstates.hpp>
#include <ql/utilities/dataformatters.hpp>
#include <ql/time/schedule.hpp>
#include <ql/handle.hpp>
#include <ql/math/interpolations/all.hpp>
#include <ql/termstructures/yield/zerocurve.hpp>
#include <ql/termstructures/volatility/volatilitytype.hpp>
#include <ql/termstructures/volatility/swaption/swaptionvolstructure.hpp>
#include <ql/instruments/swap.hpp>
#include <ql/time/dategenerationrule.hpp>
#include <ql/currencies/all.hpp>
#include <ql/indexes/all.hpp>
#include <ql/instruments/averagetype.hpp>
#include <ql/instruments/barriertype.hpp>

using namespace QuantLib;

Date makeDateFromInt(int intDate);
Calendar makeCalendarFromInt(int intCalendar);
DayCounter makeDayCounterFromInt(int dayCounterCode);
Compounding makeCompoundingFromInt(int compoundingCode);
Frequency makeFrequencyFromInt(int frequencyCode);
BusinessDayConvention makeBDCFromInt(int bdcCode);
Currency makeCurrencyFromInt(int currencyCode);
bool makeBoolFromInt(int boolCode);
VolatilityType makeVolatilityTypeFromInt(int volTypeCode);
ext::optional<RateAveraging::Type> makeRateAveragingFromInt(int rateTypeCode);
Swap::Type makeSwapTypeFromInt(int swapTypeCode);
Option::Type makeOptionTypeFromInt(int code);
Average::Type makeAverageTypeFromInt(int code);
int makeOptionOwnerFromInt(int ownerCode);
Period makePeriodFromIntParts(int numberPart, int letterPart);
std::vector<Date> makeDatesVectorFromIntArray(int arrSize, int intDates[]);
std::vector<Rate> makeRatesVectorFromDoubleArray(int arrSize, double doubleRates[]);
ext::shared_ptr<StrikedTypePayoff> makeStrikeTypePayoff(int strikeType, int optionType,
                                                        Real strike = 0.0,
                                                        ext::optional<Real> secondStrike = ext::nullopt,
                                                        ext::optional<Real> cashPayoff = ext::nullopt);
Barrier::Type makeBarrierTypeFromInt(int code);
Period makePeriodFromDays(int days);
std::vector<Period> makePeriodArrayFromTenorDaysArray(
    const int* girrTenorDays, const int numberOfGirrTenors);

DateGeneration::Rule makeScheduleGenRuleFromInt(int code);

void initResult(double* result, const int size);
void processResultArray(std::vector<QuantLib::Real> tenors, std::vector<QuantLib::Real> sensitivities, QuantLib::Size originalSize, double* resultArray);
void freeArray(double* arr);
std::string qDateToString(const QuantLib::Date& date);

// System

#include <functional>

class ScopeGuard {
public:
    explicit ScopeGuard(std::function<void()> f) : func_(std::move(f)), active_(true) {}
    ~ScopeGuard() { if (active_) func_(); }
    void dismiss() { active_ = false; }

private:
    std::function<void()> func_;
    bool active_;
};

#define CONCAT_INNER(x,y) x##y
#define CONCAT(x,y) CONCAT_INNER(x,y)
#define FINALLY(code) ScopeGuard CONCAT(_guard_, __LINE__)([&](){code;})
