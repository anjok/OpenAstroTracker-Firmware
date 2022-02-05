#include <math.h>

#include "../Configuration.hpp"
#include "Utility.hpp"

#if DEBUG_LEVEL > 0
    #include <stdarg.h>
#endif

#if DEBUG_LEVEL > 0
unsigned long RealTime::_pausedTime   = 0;
unsigned long RealTime::_startTime    = micros();
unsigned long RealTime::_suspendStart = 0;
int RealTime::_suspended              = 0;
#endif

static u16 debugMask = DEBUG_LEVEL;

#if BUFFER_LOGS == true
    #define LOG_BUFFER_SIZE 512
char logBuffer[LOG_BUFFER_SIZE];
int bufferWritePos = 0;
int bufferStartPos = 0;

int scanForNextNewLine(int bufPos)
{
    for (int i = bufPos; i < LOG_BUFFER_SIZE; i++)
    {
        if (logBuffer[i] == '\n')
        {
            return i;
        }
    }

    for (int i = 0; i < LOG_BUFFER_SIZE; i++)
    {
        if (logBuffer[i] == '\n')
        {
            return i;
        }
    }
    // WTF?
    return 0;
}

void addToLogBuffer(String s)
{
    s += '\n';
    int charsToWrite = s.length();
    if (bufferWritePos + charsToWrite > LOG_BUFFER_SIZE)
    {
        int charsToWriteAtEndOfBuffer = LOG_BUFFER_SIZE - bufferWritePos;
        memcpy(logBuffer + bufferWritePos, s.c_str(), charsToWriteAtEndOfBuffer);
        charsToWrite -= charsToWriteAtEndOfBuffer;
        memcpy(logBuffer, s.c_str() + charsToWriteAtEndOfBuffer, charsToWrite);
        bufferWritePos            = charsToWrite;
        logBuffer[bufferWritePos] = '\0';
        bufferStartPos            = scanForNextNewLine(bufferWritePos);
    }
    else
    {
        memcpy(logBuffer + bufferWritePos, s.c_str(), charsToWrite);
        if (bufferStartPos > bufferWritePos)
        {
            bufferWritePos += charsToWrite;
            bufferStartPos = scanForNextNewLine(bufferWritePos);
        }
        else
        {
            bufferWritePos += charsToWrite;
        }
    }
}

class MyString : public String
{
  public:
    void setLen(unsigned int newLen)
    {
        len = newLen;
    }
};

String getLogBuffer()
{
    MyString result;
    unsigned int len = (bufferStartPos > bufferWritePos) ? LOG_BUFFER_SIZE - bufferStartPos : 0;
    len += bufferWritePos;
    result.reserve(len + 2);
    char *buffer = result.begin();

    if (bufferStartPos > bufferWritePos)
    {
        for (int i = bufferStartPos; i < LOG_BUFFER_SIZE; i++)
        {
            *buffer++ = (logBuffer[i] == '#') ? '%' : logBuffer[i];
        }
    }

    for (int i = 0; i < bufferWritePos; i++)
    {
        *buffer++ = (logBuffer[i] == '#') ? '%' : logBuffer[i];
    }

    *buffer++ = '#';
    *buffer   = '\0';
    result.setLen(buffer - result.begin());
    return result;
}
#else
String getLogBuffer()
{
    return "Debugging disabled.#";
}
#endif

// Adjust the given number by the given adjustment, wrap around the limits.
// Limits are inclusive, so they represent the lowest and highest valid number.
int adjustWrap(int current, int adjustBy, int minVal, int maxVal)
{
    current += adjustBy;
    if (current > maxVal)
    {
        current -= (maxVal + 1 - minVal);
    }
    if (current < minVal)
    {
        current += (maxVal + 1 - minVal);
    }
    return current;
}

// Adjust the given number by the given adjustment, clamping to the limits.
// Limits are inclusive, so they represent the lowest and highest valid number.
int adjustClamp(int current, int adjustBy, int minVal, int maxVal)
{
    current += adjustBy;
    if (current > maxVal)
    {
        current = maxVal;
    }
    if (current < minVal)
    {
        current = minVal;
    }
    return current;
}

// Clamp the given number to the limits.
// Limits are inclusive, so they represent the lowest and highest valid number.
long clamp(long current, long minVal, long maxVal)
{
    if (current > maxVal)
    {
        current = maxVal;
    }
    if (current < minVal)
    {
        current = minVal;
    }
    return current;
}

// Clamp the given number to the limits.
// Limits are inclusive, so they represent the lowest and highest valid number.
int clamp(int current, int minVal, int maxVal)
{
    if (current > maxVal)
    {
        current = maxVal;
    }
    if (current < minVal)
    {
        current = minVal;
    }
    return current;
}

// Clamp the given number to the limits.
// Limits are inclusive, so they represent the lowest and highest valid number.
float clamp(float current, float minVal, float maxVal)
{
    if (current > maxVal)
    {
        current = maxVal;
    }
    if (current < minVal)
    {
        current = minVal;
    }
    return current;
}

// Return -1 if the given number is less than zero, 1 if not.
int sign(long num)
{
    if (num < 0)
    {
        return -1;
    }
    return 1;
}

// Return -1 if the given number is less than zero, 1 if not.
int fsign(float num)
{
    if (num < 0)
    {
        return -1;
    }
    return 1;
}

// float-type point implementation of fabs
float fabsf(float x)
{
    return static_cast<float>(fabs(static_cast<double>(x)));
}

// float-type point implementation of round
float roundf(float x)
{
    return static_cast<float>(round(static_cast<double>(x)));
}

// float-type point implementation of atan
float atanf(float x)
{
    return static_cast<float>(atan(static_cast<double>(x)));
}

#if defined(ESP32)
int freeMemory()
{
    return ESP.getFreeHeap();
}
#else

    #ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char *sbrk(int incr);
    #else   // __ARM__
extern char *__brkval;
    #endif  // __arm__

int freeMemory()
{
    char top;
    #ifdef __arm__
    return &top - reinterpret_cast<char *>(sbrk(0));
    #elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
    return &top - __brkval;
    #else   // __arm__
    return __brkval ? &top - __brkval : &top - __malloc_heap_start;
    #endif  // __arm__
}
#endif

#if DEBUG_LEVEL > 0

String formatArg(const char *input, va_list args)
{
    const char *nibble = "0123456789ABCDEF";
    char achBuffer[255];
    char *p = achBuffer;

    for (const char *i = input; *i != 0; i++)
    {
        if (*i != '%')
        {
            *p++ = *i;
            continue;
        }
        i++;
        switch (*i)
        {
            case '%':
                {
                    *p++ = '%';
                }
                break;

            case 'c':
                {
                    char ch = (char) va_arg(args, int);
                    *p++    = ch;
                }
                break;

            case 's':
                {
                    char *s = va_arg(args, char *);
                    strcpy(p, s);
                    p += strlen(s);
                }
                break;

            case 'd':
                {
                    String s = String((int) va_arg(args, int));
                    strcpy(p, s.c_str());
                    p += s.length();
                }
                break;

            case 'x':
                {
                    int n             = (int) va_arg(args, int);
                    int shift         = 12;
                    unsigned int mask = 0xF000;
                    *p++              = '0';
                    *p++              = 'x';
                    while (shift >= 0)
                    {
                        int d = (n & mask) >> shift;
                        *p++  = *(nibble + d);
                        mask  = mask >> 4;
                        shift -= 4;
                    }

                    *p = 0;
                }
                break;

            case 'l':
                {
                    String s = String((long) va_arg(args, long));
                    strcpy(p, s.c_str());
                    p += s.length();
                }
                break;

            case 'f':
                {
                    float num = (float) va_arg(args, double);
                    String s  = String(num, 4);
                    strcpy(p, s.c_str());
                    p += s.length();
                }
                break;

            default:
                {
                    *p++ = *i;
                }
                break;
        }
    }

    *p = '\0';
    return String(achBuffer);
}

String format(const char *input, ...)
{
    va_list argp;
    va_start(argp, input);
    String ret = formatArg(input, argp);
    va_end(argp);
    return ret;
}

void logv(int levelFlags, String input, ...)
{
    if(!(debugMask & levelFlags)) {
        return;
    }

    if ((levelFlags & DEBUG_LEVEL) != 0)
    {
        unsigned long now = millis();
        va_list argp;
        va_start(argp, input);
    #if BUFFER_LOGS == true
        addToLogBuffer(formatArg(input.c_str(), argp));
    #else
        Serial.print("@[");
        Serial.print(String(now));
        Serial.print("]:");
        Serial.print(String(freeMemory()));
        Serial.print(": ");
        Serial.println(formatArg(input.c_str(), argp));
        Serial.flush();
    #endif
        va_end(argp);
    }
}
#endif

static const u16 enabledDebugMask      = DEBUG_ANY;
static const String debugMaskStrings[] = {String("INFO"),
                                          String("SERIAL"),
                                          String("WIFI"),
                                          String("MOUNT"),
                                          String("MOUNT_VERBOSE"),
                                          String("GENERAL"),
                                          String("MEADE"),
                                          String("VERBOSE"),
                                          String("STEPPERS"),
                                          String("EEPROM"),
                                          String("GYRO"),
                                          String("GPS"),
                                          String("FOCUS")};
static const String NONE               = String("NONE");
static const String ANY                = String("ANY");

static String getLevelStringForMask(u16 mask)
{
    String result = String();
    for (size_t i = 0; i < sizeof(debugMaskStrings) / sizeof(debugMaskStrings[0]); i++)
    {
        if (mask & (1 << i))
        {
            result.concat(debugMaskStrings[i]);
            result.concat("|");
        }
    }
    return result.charAt(result.length() - 1) == '|' ? result.substring(0, result.length() - 1) : result;
}

void Debug::setCurrentDebugMask(String maskString)
{
    if (maskString == NONE || maskString.length() == 0)
    {
        debugMask = DEBUG_NONE;
    }
    else if (maskString == ANY)
    {
        debugMask = DEBUG_ANY;
    }
    else
    {
        int index = 0, start = 0, len = maskString.length();
        u16 level = 0;
        while (index <= len)
        {
            if (index == len || maskString.charAt(index) == '|')
            {
                String substring = maskString.substring(start, index);
                int bit          = -1;
                for (size_t i = 0; i < sizeof(debugMaskStrings) / sizeof(debugMaskStrings[0]); i++)
                {
                    //LOGV3(DEBUG_ANY, "sub: %s mask: %s", substring.c_str(), debugMaskStrings[i].c_str());
                    if (debugMaskStrings[i] == substring)
                    {
                        bit = i;
                        break;
                    }
                }
                if (bit >= 0)
                {
                    level |= (1 << bit);
                }
                //LOGV6(DEBUG_ANY, "sub: %s bit: %d start: %d index: %d level: %x", substring.c_str(), bit, start, index, level);
                start = index + 1;
            }
            index++;
        }
        debugMask = level;
    }
}

String Debug::getCurrentDebugMask()
{
    if (debugMask == DEBUG_NONE)
    {
        return NONE;
    }
    else if (debugMask == DEBUG_ANY)
    {
        return ANY;
    }
    else
    {
        return getLevelStringForMask(debugMask);
    }
}

String Debug::getEnabledDebugMask()
{
    return getLevelStringForMask(enabledDebugMask);
}
