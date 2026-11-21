#include "test_util.h"

int g_checks = 0;
int g_failures = 0;

int main(void)
{
    printf("USB-C PD smart charger - host test suite\n");
    printf("=========================================\n");

    RUN_SUITE(test_battery_run);
    RUN_SUITE(test_ntc_run);
    RUN_SUITE(test_soc_run);
    RUN_SUITE(test_ina226_run);
    RUN_SUITE(test_thermal_run);
    RUN_SUITE(test_balance_run);
    RUN_SUITE(test_pd_run);
    RUN_SUITE(test_fault_run);
    RUN_SUITE(test_display_run);
    RUN_SUITE(test_ssd1306_run);
    RUN_SUITE(test_scenarios_run);

    printf("=========================================\n");
    printf("checks: %d, failures: %d\n", g_checks, g_failures);
    if (g_failures == 0) {
        printf("RESULT: PASS\n");
        return 0;
    }
    printf("RESULT: FAIL\n");
    return 1;
}
