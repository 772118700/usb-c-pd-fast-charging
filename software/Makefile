CC       := gcc
CFLAGS   ?= -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-format-truncation
CPPFLAGS ?= -Iinclude -Itests
LDFLAGS  ?= -lm

BUILD   := build
TARGET  := $(BUILD)/host_tests.exe

ifeq ($(OS),Windows_NT)
MKDIR_P = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
RM_RF   = if exist "$(subst /,\,$(1))" rmdir /S /Q "$(subst /,\,$(1))"
else
MKDIR_P = mkdir -p "$(1)"
RM_RF   = rm -rf "$(1)"
endif

APP_SRCS := \
    src/services/battery_math.c \
    src/services/ntc.c \
    src/services/soc.c \
    src/services/thermal.c \
    src/services/balance.c \
    src/services/fault_diag.c \
    src/services/pd_status.c \
    src/services/display_model.c \
    src/drivers/ina226.c \
    src/drivers/ssd1306.c \
    src/drivers/ssd1306_font.c \
    src/app/app_state.c

TEST_SRCS := \
    tests/test_main.c \
    tests/test_battery.c \
    tests/test_ntc.c \
    tests/test_soc.c \
    tests/test_ina226.c \
    tests/test_thermal.c \
    tests/test_balance.c \
    tests/test_pd.c \
    tests/test_fault.c \
    tests/test_display.c \
    tests/test_ssd1306.c \
    tests/test_scenarios.c \
    tests/mock_port.c

SRCS := $(APP_SRCS) $(TEST_SRCS)
OBJS := $(patsubst %.c,$(BUILD)/%.o,$(SRCS))

.PHONY: all test syntax syntax-platform clean

all: $(TARGET)

test: $(TARGET)
	@$(TARGET)

# 无需 FreeRTOS/STM32 HAL 源文件, 仅对应用任务层做类型/语法检查。
syntax:
	$(CC) $(CPPFLAGS) -Itests/stubs $(CFLAGS) -fsyntax-only \
	    src/app/app_tasks.c src/app/app_main.c

# 用 HAL 桩检查 STM32G0 适配层; 不验证真实寄存器语义。
syntax-platform:
	$(CC) $(CPPFLAGS) -Itests/stubs $(CFLAGS) -fsyntax-only \
	    platform/stm32g0/bsp_adc_stm32g0.c \
	    platform/stm32g0/bsp_i2c_stm32g0.c \
	    platform/stm32g0/board_port_stm32g0.c

$(TARGET): $(OBJS)
	@$(call MKDIR_P,$(dir $@))
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD)/%.o: %.c
	@$(call MKDIR_P,$(dir $@))
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	@$(call RM_RF,$(BUILD))
