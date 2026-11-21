#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdio.h>

extern int g_checks;
extern int g_failures;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(cond)) {                                                       \
            ++g_failures;                                                    \
            printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
        }                                                                    \
    } while (0)

#define CHECK_NEAR(actual, expected, tol)                                    \
    do {                                                                     \
        long _a = (long)(actual);                                            \
        long _e = (long)(expected);                                          \
        long _t = (long)(tol);                                               \
        ++g_checks;                                                          \
        if (_a < _e - _t || _a > _e + _t) {                                  \
            ++g_failures;                                                    \
            printf("  FAIL %s:%d: %s=%ld, expected %ld +/- %ld\n",           \
                   __FILE__, __LINE__, #actual, _a, _e, _t);                 \
        }                                                                    \
    } while (0)

#define RUN_SUITE(fn)                                                        \
    do {                                                                     \
        printf("[ RUN  ] %s\n", #fn);                                        \
        fn();                                                                \
        printf("[ DONE ] %s\n", #fn);                                        \
    } while (0)

void test_battery_run(void);
void test_ntc_run(void);
void test_soc_run(void);
void test_ina226_run(void);
void test_thermal_run(void);
void test_balance_run(void);
void test_pd_run(void);
void test_fault_run(void);
void test_display_run(void);
void test_ssd1306_run(void);
void test_scenarios_run(void);

#endif /* TEST_UTIL_H */
