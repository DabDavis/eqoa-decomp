// VITimeOfDay -- SUPPORT 0x003E8508 .. 0x003E8998.
#include "vi.h"

// The eight-byte BCD clock the CD subsystem fills in; every field below is
// read out of GetLocalTime's own `lbu` offsets off the 16-byte-aligned local.
struct sceCdCLOCK
{
    unsigned char stat;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char pad;
    unsigned char day;
    unsigned char month;
    unsigned char year;
};

extern "C" int sceCdReadClock(sceCdCLOCK *c);
extern "C" int sceScfGetLocalTimefromRTC(sceCdCLOCK *c);

// OPEN at 9 of 80: every register agrees and the one difference is whether
// `move t0,v0` (the GetMonth result) is emitted right after its call or ends up
// in the snprintf `jal`'s delay slot.  Two spellings with the results in locals
// do not move it.
int VITimeOfDay::GetLocalTime(char *out, int n)
{
    sceCdCLOCK c;

    sceCdReadClock(&c);
    sceScfGetLocalTimefromRTC(&c);

    int month  = (c.month  >> 4) * 10 + (c.month  & 15);
    int day    = (c.day    >> 4) * 10 + (c.day    & 15);
    int year   = (c.year   >> 4) * 10 + (c.year   & 15);
    int hour   = (c.hour   >> 4) * 10 + (c.hour   & 15);
    int minute = (c.minute >> 4) * 10 + (c.minute & 15);
    int second = (c.second >> 4) * 10 + (c.second & 15);

    VIString::snprintf(out, n, "%s %s %d %02d:%02d:%02d 20%02d",
                       GetDayOfWeek(month, day, year), GetMonth(month),
                       day, hour, minute, second, year);

    return 0;
}

int VITimeOfDay::GetLocalTime(unsigned short *out, int n)
{
    char buf[64];

    GetLocalTime(buf, 64);

    VIString::str8to16(out, n, buf);

    return 0;
}

const char *VITimeOfDay::GetMonth(int month)
{
    switch (month)
    {
    case 1:  return "January";
    case 2:  return "February";
    case 3:  return "March";
    case 4:  return "April";
    case 5:  return "May";
    case 6:  return "June";
    case 7:  return "July";
    case 8:  return "August";
    case 9:  return "September";
    case 10: return "October";
    case 11: return "November";
    case 12: return "December";
    }

    return "";
}

const char *VITimeOfDay::GetDayOfWeek(int month, int day, int year)
{
    int dow;

    GetDayOfWeek(month, day, year + 2000, dow);

    switch (dow)
    {
    case 0: return "Sunday";
    case 1: return "Monday";
    case 2: return "Tuesday";
    case 3: return "Wednesday";
    case 4: return "Thursday";
    case 5: return "Friday";
    case 6: return "Saturday";
    }

    return "";
}

// The month table is {6,2,2,5,0,3,5,1,4,6,2,4}, read out of the JUMP TABLE in
// the relocated dump: May's entry lands on the join (offset 0, no block at all)
// and September and December share one block, which is why ten case bodies
// answer for twelve months.
void VITimeOfDay::GetDayOfWeek(int month, int day, int year, int &dow)
{
    int leap = 0;

    if ((year & 3) == 0)
    {
        if (year % 100 != 0)
            leap = 1;
        else
            leap = (year % 400 == 0);
    }

    int c = year % 100;

    dow = c + c / 4;

    switch (month)
    {
    case 1:  dow += 6; break;
    case 2:  dow += 2; break;
    case 3:  dow += 2; break;
    case 4:  dow += 5; break;
    case 5:  break;
    case 6:  dow += 3; break;
    case 7:  dow += 5; break;
    case 8:  dow += 1; break;
    case 9:  dow += 4; break;
    case 10: dow += 6; break;
    case 11: dow += 2; break;
    case 12: dow += 4; break;
    }

    dow += day;

    if (year < 2000)
        dow += 1;

    if (leap && (unsigned int)month < 2)
        dow -= 1;

    dow = dow % 7;

    if (dow <= 0)
        dow += 7;

    dow -= 1;
}
