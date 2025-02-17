#ifndef CMOS_H
#define CMOS_H

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

#define CMOS_RTC_SECONDS 0x00
#define CMOS_RTC_MINUTES 0x02
#define CMOS_RTC_HOURS 0x04
#define CMOS_RTC_WEEKDAY 0x06
#define CMOS_RTC_DAY 0x07
#define CMOS_RTC_MONTH 0x08
#define CMOS_RTC_YEAR 0x09

#define CMOS_MAYBE_CENTURY 0x32

#define CMOS_SELECT_RTCA 0x8A           // Select register A and disable NMI
#define CMOS_SELECT_RTCB 0x8B           // Select register B and disable NMI
#define CMOS_WRITE_RAM 0x20

#endif // CMOS_H