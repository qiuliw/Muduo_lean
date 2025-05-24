#include "Timestamp.h"
#include <time.h>

Timestamp::Timestamp():microSecondsSinceEpoch_(0)
{}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    :microSecondsSinceEpoch_(microSecondsSinceEpoch)
{}

Timestamp Timestamp::now(){
     return Timestamp(time(NULL));
}


// struct tm
// {
//   int tm_sec;			/* Seconds.	[0-60] (1 leap second) */
//   int tm_min;			/* Minutes.	[0-59] */
//   int tm_hour;			/* Hours.	[0-23] */
//   int tm_mday;			/* Day.		[1-31] */
//   int tm_mon;			/* Month.	[0-11] */
//   int tm_year;			/* Year	- 1900.  */
//   int tm_wday;			/* Day of week.	[0-6] */
//   int tm_yday;			/* Days in year.[0-365]	*/
//   int tm_isdst;			/* DST.		[-1/0/1]*/
// }


// localtime() 是 C/C++ 标准库中的一个函数，用于将 ​Unix 时间戳（自 1970-01-01 以来的秒数）​​ 转换为 ​本地时区的 struct tm 结构体，便于按年、月、日、时、分、秒等格式处理时间。
std::string Timestamp::toString() const {
    char buf[128] = {0};
    // 转换标准时间到通用时间
    tm *tm_time = localtime(&microSecondsSinceEpoch_);
    snprintf(buf, sizeof(buf), 
             "%04d/%02d/%02d %02d:%02d:%02d",  // 注意 %04d 表示年份补零到4位
             tm_time->tm_year + 1900,         // 年份（如 2023）
             tm_time->tm_mon + 1,             // 月份（1-12）
             tm_time->tm_mday,                // 日（1-31）
             tm_time->tm_hour,                // 时（0-23）
             tm_time->tm_min,                 // 分（0-59）
             tm_time->tm_sec);                // 秒（0-60）
    return std::string(buf);
}


// int main(){
//     std::cout << Timestamp::now().toString() << std::endl;
//     return 0;
// }