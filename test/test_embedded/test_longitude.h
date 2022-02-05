#pragma once

#include "unity.h"
#include "Longitude.hpp"

namespace test {
    namespace longitude {

        void test_longitude(char const *meade, char const *expected) {
            Longitude longitude = Longitude::ParseFromMeade(String(meade));
            char buf[200];
            longitude.formatString(buf, "{d}*{m}");

            TEST_ASSERT_EQUAL_STRING(expected, buf);
        }

        void test_meade_parsing()
        {
            test_longitude("00*00",  "00*00");
            test_longitude("+00*00", "00*00");
            test_longitude("+11*41", "11*41");
            test_longitude("11*41",  "11*41");
            test_longitude("179*59", "179*59");
            test_longitude("180*01", "180*01");
            test_longitude("-11*41", "348*19");
            test_longitude("-480*41", "239*19");
        }

        void run() {
            RUN_TEST(test_meade_parsing);
        }
    }
}
