#pragma once

#include "unity.h"
#include "Sidereal.hpp"

namespace test {
    namespace sidereal {

        void test_ha(float hours, int expectedHours, int expectedMinutes, int expectedSeconds) 
        {
            DayTime ha_zero = Sidereal::calculateHa(hours);
            //Serial.println(hours);
            //Serial.println(ha_zero.ToString());
            TEST_ASSERT_EQUAL_INT16(expectedHours, ha_zero.getHours());
            TEST_ASSERT_EQUAL_INT16(expectedMinutes, ha_zero.getMinutes());
            TEST_ASSERT_EQUAL_INT16(expectedSeconds, ha_zero.getSeconds());
        }

        void test_calculate_ha_from_lst()
        {
            // https://www.eso.org/observing/etc/bin/gen/form?INS.MODE=swspectr+INS.NAME=SKYCALC
            // at 27/01/2022 20:13:16 -> LST = 0-> HA = 20:00:00 in whereever they are
            test_ha(0.0, 21, 01, 26);  // 21:04 in PA Pro
            test_ha(12.2, 9, 13, 26);  // 09:09 in PA Pro
            test_ha(16.2, 13, 13, 26); // 13:14 in PA Pro
            test_ha(21.5, 18, 31, 26); // 18:37 in PA Pro 
        }

        void test_lst(double longitude, int year, int month, int day, DayTime *timeUTC, DayTime expected) 
        {
            DayTime lst = Sidereal::calculateByDateAndTime(longitude, year, month, day, timeUTC);
            //Serial.println(lst.ToString());
            //Serial.println(expected.ToString());
            //return ;
            TEST_ASSERT_EQUAL_INT16(expected.getHours(), lst.getHours());
            TEST_ASSERT_EQUAL_INT16(expected.getMinutes(), lst.getMinutes());
            TEST_ASSERT_EQUAL_INT16(expected.getSeconds(), lst.getSeconds());
        }

        void test_calculate_lst()
        {
            test_lst(0.0, 2000, 1, 1, new DayTime(0, 0, 0), DayTime(6, 39, 47));
            test_lst(11.1834, 2022, 1, 27, new DayTime(19, 30, 32), DayTime(4, 43, 24));
        }

        void run() {
            RUN_TEST(test_calculate_ha_from_lst);
            RUN_TEST(test_calculate_lst);
        }
    }
}
