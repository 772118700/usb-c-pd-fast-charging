#include "board_port.h"

/* Host-side board stubs. */
void board_init(void) {}
uint32_t board_millis(void) { return 0U; }
void board_delay_ms(uint32_t ms) { (void)ms; }
void board_set_fault_led(bool on) { (void)on; }
void board_set_pd_inhibit(bool inhibit) { (void)inhibit; }
void board_critical_enter(void) {}
void board_critical_exit(void) {}
bool board_read_user_key(void) { return false; }
