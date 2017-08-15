#include <gtest/gtest.h>
#include <emulator.h>

TEST(TemperatureSensor, ReadWrite) {
    const char *file_name = "_em_temperature_sensor.t";
    int ok;

    ASSERT_TRUE(temperature_sensor_write(file_name, 12, &ok));
    ASSERT_TRUE(ok);
    ASSERT_EQ(temperature_sensor_read(file_name, &ok), 12);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(temperature_sensor_write(file_name, -12, &ok));
    ASSERT_TRUE(ok);
    ASSERT_EQ(temperature_sensor_read(file_name, &ok), -12);
    ASSERT_TRUE(ok);
}
