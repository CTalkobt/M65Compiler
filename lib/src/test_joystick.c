/* test_joystick.c — Comprehensive Joystick Input Tests */

#include "test_harness.h"
#include "joystick.h"
#include <stdlib.h>

static sprite_joystick_manager_t test_manager = NULL;
static sprite_joystick_t test_joy_atari = NULL;
static sprite_joystick_t test_joy_sega = NULL;

/* Test functions */

void test_manager_create(void) {
    current_test.test_name = "manager_create";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    /* Create persistent test manager */
    test_manager = sprite_joystick_manager_create(4);
    ASSERT_NOT_NULL(test_manager);

    test_harness_record_result();
}

void test_init_atari(void) {
    current_test.test_name = "init_atari";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    test_joy_atari = sprite_joystick_init(test_manager, 1, JOYSTICK_PROTOCOL_ATARI);
    ASSERT_NOT_NULL(test_joy_atari);
    ASSERT_EQ(sprite_joystick_get_count(test_manager), 1);

    test_harness_record_result();
}

void test_init_sega(void) {
    current_test.test_name = "init_sega";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    test_joy_sega = sprite_joystick_init(test_manager, 2, JOYSTICK_PROTOCOL_SEGA);
    ASSERT_NOT_NULL(test_joy_sega);
    ASSERT_EQ(sprite_joystick_get_count(test_manager), 2);

    test_harness_record_result();
}

void test_init_sega_full(void) {
    current_test.test_name = "init_sega_full";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    sprite_joystick_t joy = sprite_joystick_init(test_manager, 3, JOYSTICK_PROTOCOL_SEGA_FULL);
    ASSERT_NOT_NULL(joy);

    test_harness_record_result();
}

void test_read_buttons(void) {
    current_test.test_name = "read_buttons";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    int buttons = sprite_joystick_read_buttons(test_joy_atari);
    ASSERT_EQ(buttons, 0);

    test_harness_record_result();
}

void test_is_pressed(void) {
    current_test.test_name = "is_pressed";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    int pressed = sprite_joystick_is_pressed(test_joy_atari, JOYSTICK_UP);
    ASSERT_FALSE(pressed);

    test_harness_record_result();
}

void test_detect(void) {
    current_test.test_name = "detect";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    int detected = sprite_joystick_detect(test_manager);
    ASSERT_GT(detected, 0);

    test_harness_record_result();
}

void test_get_direction(void) {
    current_test.test_name = "get_direction";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    int x = 0, y = 0;
    sprite_joystick_get_direction(test_joy_atari, &x, &y);
    ASSERT_EQ(x, 0);
    ASSERT_EQ(y, 0);

    test_harness_record_result();
}

void test_debounce(void) {
    current_test.test_name = "debounce";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    sprite_joystick_set_debounce_time(test_joy_atari, 50);
    sprite_joystick_update(test_joy_atari, 5);
    int buttons = sprite_joystick_read_buttons(test_joy_atari);
    ASSERT_EQ(buttons, 0);

    test_harness_record_result();
}

void test_callbacks(void) {
    current_test.test_name = "callbacks";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    int result = sprite_joystick_set_callback(test_joy_atari, JOYSTICK_UP, NULL, NULL);
    ASSERT_TRUE(result);

    test_harness_record_result();
}

void test_remap(void) {
    current_test.test_name = "remap";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    int result = sprite_joystick_remap_button(test_joy_atari, 0, JOYSTICK_BUTTON_A);
    ASSERT_TRUE(result);

    test_harness_record_result();
}

void test_is_connected(void) {
    current_test.test_name = "is_connected";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    int connected = sprite_joystick_is_connected(test_joy_atari);
    ASSERT_TRUE(connected);

    test_harness_record_result();
}

void test_get_name(void) {
    current_test.test_name = "get_name";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    const char *name = sprite_joystick_get_name(test_joy_atari);
    ASSERT_NOT_NULL(name);
    ASSERT_STR_EQ(name, "Atari 2600");

    test_harness_record_result();
}

void test_button_count(void) {
    current_test.test_name = "button_count";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    int count = sprite_joystick_get_button_count(test_joy_atari);
    ASSERT_EQ(count, 5);

    test_harness_record_result();
}

void test_multiple_devices(void) {
    current_test.test_name = "multiple_devices";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    /* We already have atari and sega joysticks from earlier tests */
    int count = sprite_joystick_get_count(test_manager);
    ASSERT_GT(count, 1);

    test_harness_record_result();
}

void test_cleanup(void) {
    current_test.test_name = "cleanup";
    current_test.suite_name = "joystick";
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    if (test_manager) {
        sprite_joystick_manager_destroy(test_manager);
        test_manager = NULL;
    }
    ASSERT_NULL(test_manager);

    test_harness_record_result();
}

int main(void) {
    test_harness_init();

    printf("=== JOYSTICK INPUT TESTS ===\n\n");

    test_manager_create();
    test_init_atari();
    test_init_sega();
    test_init_sega_full();
    test_read_buttons();
    test_is_pressed();
    test_detect();
    test_get_direction();
    test_debounce();
    test_callbacks();
    test_remap();
    test_is_connected();
    test_get_name();
    test_button_count();
    test_multiple_devices();
    test_cleanup();

    test_harness_print_summary();

    test_stats_t stats = test_harness_get_stats();
    return stats.failed > 0 ? 1 : 0;
}
