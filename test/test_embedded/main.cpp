#include <unity.h>
#include <Arduino.h>
PUSH_NO_WARNINGS
#include "test_sidereal.h"
#include "test_longitude.h"
POP_NO_WARNINGS

// void setup()
// {

// }

// void loop()
// {
    
// }

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    test::sidereal::run();
    test::longitude::run();
    UNITY_END();

    return 0;
}