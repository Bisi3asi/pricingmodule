#pragma once
#include "logger.hpp"
#include "common.hpp"

/* 데이터 출력 관련 로그 구현부 */
/* Data Macro */
#define LOG_OUTPUT(...) \
    do { \
        logger::info("| Output Results |"); \
        logger::info("---------------------------------------------"); \
        LOG_FIELDS(__VA_ARGS__); \
        logger::info("---------------------------------------------"); \
        logger::info(""); \
    } while(0)

#define LOG_INPUT(...) \
    do { \
        logger::info("| Input Parameters |"); \
        logger::info("---------------------------------------------"); \
        LOG_FIELDS(__VA_ARGS__); \
        logger::info("---------------------------------------------"); \
        logger::info(""); \
    } while(0)

#define LOG_COUPON_SCHEDULE(schedule) logger::data::logCouponSchedule(schedule)

/* Data Log Function */
namespace logger::data {
        inline void logCouponSchedule(const QuantLib::Schedule& schedule) {
            logger::info("");
            logger::info("| Coupon Schedule |");
            logger::info("---------------------------------------------");

            std::vector<std::string> logStartDatesStr, logEndDatesStr, logPaymentDatesStr;

            // Local YYYYMMDD formatter (avoid using external qDateToString implementations)
            auto toYYYYMMDD = [](const QuantLib::Date& d) -> std::string {
                std::ostringstream oss;
                oss << std::setw(4) << std::setfill('0') << d.year()
                    << std::setw(2) << std::setfill('0') << static_cast<int>(d.month())
                    << std::setw(2) << std::setfill('0') << d.dayOfMonth();
                return oss.str();
            };

            for (QuantLib::Size i = 0; i + 1 < schedule.size(); ++i) {
                QuantLib::Date startDate = schedule.date(i);
                QuantLib::Date endDate = schedule.date(i + 1);
                QuantLib::Date paymentDate = schedule.calendar().adjust(endDate, schedule.businessDayConvention());

                logStartDatesStr.push_back(toYYYYMMDD(startDate));
                logEndDatesStr.push_back(toYYYYMMDD(endDate));
                logPaymentDatesStr.push_back(toYYYYMMDD(paymentDate));
            }

            logger::logArrayLine("Start Dates", logStartDatesStr);
            logger::logArrayLine("End Dates", logEndDatesStr);
            logger::logArrayLine("Payment Dates", logPaymentDatesStr);

            logger::info("---------------------------------------------");
            logger::info("");
        }
}

