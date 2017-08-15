#include <gtest/gtest.h>
#include <conditioner.h>
#include <emulator.h>

TEST(ExternalController, ReadWrite) {
    const char *file_name = "_em_external_controller.t";
    int ok;

    external_controller_write(file_name, _PowerOff, &ok);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(external_controller_read(file_name, &ok) == _PowerOff);
    ASSERT_TRUE(ok);
    external_controller_write(file_name, _PowerOn, &ok);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(external_controller_read(file_name, &ok) == _PowerOn);
    ASSERT_TRUE(ok);
}
