#include "../Configuration.hpp"
#include "Utility.hpp"
#include "Longitude.hpp"
//////////////////////////////////////////////////////////////////////////////////////
//
// -180..180 range, 0 is at the prime meridian (through Greenwich), negative going west, positive going east

#define FULL_CIRCLE_SECONDS (360L * 3600L)

Longitude::Longitude(const Longitude &other) : DayTime(other)
{
}

Longitude::Longitude(int h, int m, int s) : DayTime(h, m, s)
{
}

Longitude::Longitude(float inDegrees) : DayTime(inDegrees)
{
}

static long toCircleSeconds(const long seconds) {
    long result = -(FULL_CIRCLE_SECONDS - ((seconds + FULL_CIRCLE_SECONDS) % (FULL_CIRCLE_SECONDS)));
    if(result < -FULL_CIRCLE_SECONDS/2) 
    {
        result +=  FULL_CIRCLE_SECONDS;
    }
    if(result < -FULL_CIRCLE_SECONDS/2) 
    {
        result -=  FULL_CIRCLE_SECONDS;
    }

    return result;
}

void Longitude::checkHours()
{
#ifdef INDI //:Gg# :Sg-11*34# :Sg11*34#
    totalSeconds = toCircleSeconds(totalSeconds);
#else
    while (totalSeconds > 180L * 3600L)
    {
        LOGV1(DEBUG_GENERAL, F("[LONGITUDE]: CheckHours: Degrees is more than 180, wrapping"));
        totalSeconds -= 360L * 3600L;
    }
    while (totalSeconds < (-180L * 3600L))
    {
        LOGV1(DEBUG_GENERAL, F("[LONGITUDE]: CheckHours: Degrees is less than -180, wrapping"));
        totalSeconds += 360L * 3600L;
    }
#endif
}

Longitude Longitude::ParseFromMeade(String const &s)
{
    Longitude result(0.0);
    LOGV2(DEBUG_GENERAL, F("[LONGITUDE]: Parse(%s)"), s.c_str());

    // Use the DayTime code to parse it.
    DayTime dt = DayTime::ParseFromMeade(s);
#ifdef INDI
    result.totalSeconds = toCircleSeconds(dt.getTotalSeconds());
#else    //from indilib driver:  Meade defines longitude as 0 to 360 WESTWARD (https://github.com/indilib/indi/blob/1b2f462b9c9b0f75629b635d77dc626b9d4b74a3/drivers/telescope/lx200driver.cpp#L1019)
    result.totalSeconds = 180L * 3600L - dt.getTotalSeconds();
#endif
    result.checkHours();

    LOGV4(DEBUG_GENERAL, F("[LONGITUDE]: Parse(%s) -> %s = %ls"), s.c_str(), result.ToString(), result.getTotalSeconds());
    return result;
}

char achBufLong[32];

const char *Longitude::ToString() const
{
    long secs = toCircleSeconds(totalSeconds);

    String totalDegs = String(1.0f * abs(totalSeconds) / 3600.0f, 2);
    String degs      = String(1.0f * secs / 3600.0f, 2);
    strcpy(achBufLong, degs.c_str());
    strcat(achBufLong, " (");
    strcat(achBufLong, totalDegs.c_str());
    strcat(achBufLong, (totalSeconds < 0) ? "W)" : "E)");
    return achBufLong;
}

const char *Longitude::formatString(char *targetBuffer, const char *format, long *) const
{
    long secs = totalSeconds;
#ifdef INDI
    secs = toCircleSeconds(totalSeconds);
#else
    // Map to 0..360 westwards
    secs = 180L * 3600L - secs;
#endif

    long degs = secs / 3600;
    secs      = secs - degs * 3600;
    long mins = secs / 60;
    secs      = secs - mins * 60;

    return formatStringImpl(targetBuffer, format, '\0', degs, mins, secs);
}
