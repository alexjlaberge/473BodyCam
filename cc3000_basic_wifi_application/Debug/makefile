################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./cc3000_basic_wifi_application.obj" \
"./startup_ccs.obj" \
"./utils/cmdline.obj" \
"./utils/uartstdio.obj" \
"./utils/ustdlib.obj" \
"./ff11a/src/option/syscall.obj" \
"./ff11a/src/option/unicode.obj" \
"./ff11a/src/diskio.obj" \
"./ff11a/src/ff.obj" \
"./cc3000/src/cc3000lib.obj" \
"./cc3000/drivers/ek-tm4c129/board.obj" \
"../cc3000_basic_wifi_application_ccs.cmd" \
$(GEN_CMDS__FLAG) \
-l"libc.a" \
-l"C:/ti/TivaWare_C_Series-2.1.1.71/examples/boards/ek-tm4c1294xl-boost-cc3000/cc3000_basic_wifi_application/ccs/../../../../../driverlib/ccs/Debug/driverlib.lib" \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include utils/subdir_vars.mk
-include ff11a/src/option/subdir_vars.mk
-include ff11a/src/subdir_vars.mk
-include cc3000/src/subdir_vars.mk
-include cc3000/drivers/ek-tm4c129/subdir_vars.mk
-include subdir_rules.mk
-include utils/subdir_rules.mk
-include ff11a/src/option/subdir_rules.mk
-include ff11a/src/subdir_rules.mk
-include cc3000/src/subdir_rules.mk
-include cc3000/drivers/ek-tm4c129/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
cc3000_basic_wifi_application.out \

EXE_OUTPUTS__QUOTED += \
"cc3000_basic_wifi_application.out" \

BIN_OUTPUTS += \
cc3000_basic_wifi_application.bin \

BIN_OUTPUTS__QUOTED += \
"cc3000_basic_wifi_application.bin" \


# All Target
all: cc3000_basic_wifi_application.out

# Tool invocations
cc3000_basic_wifi_application.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Linker'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O2 -g --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=CC3000_USE_BOOSTERPACK2 --define=_POSIX_SOURCE --define=TARGET_IS_TM4C129_RA0 --define=CC3000_TM4C129_SPI --define=UART_BUFFERED --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --ual -z -m"cc3000_basic_wifi_application_ccs.map" --heap_size=0 --stack_size=16384 -i"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/lib" -i"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/include" --reread_libs --warn_sections --display_error_number --diag_wrap=off --xml_link_info="cc3000_basic_wifi_application_linkInfo.xml" --rom_model -o "cc3000_basic_wifi_application.out" $(ORDERED_OBJS)
	@echo 'Finished building target: $@'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

cc3000_basic_wifi_application.bin: $(EXE_OUTPUTS)
	@echo 'Invoking: ARM Hex Utility'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/bin/armhex"  -o "cc3000_basic_wifi_application.bin" $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building: $@'
	@echo ' '
	@$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXE_OUTPUTS__QUOTED)$(BIN_OUTPUTS__QUOTED)
	-$(RM) "cc3000_basic_wifi_application.pp" "startup_ccs.pp" "utils\cmdline.pp" "utils\uartstdio.pp" "utils\ustdlib.pp" "ff11a\src\option\syscall.pp" "ff11a\src\option\unicode.pp" "ff11a\src\diskio.pp" "ff11a\src\ff.pp" "cc3000\src\cc3000lib.pp" "cc3000\drivers\ek-tm4c129\board.pp" 
	-$(RM) "cc3000_basic_wifi_application.obj" "startup_ccs.obj" "utils\cmdline.obj" "utils\uartstdio.obj" "utils\ustdlib.obj" "ff11a\src\option\syscall.obj" "ff11a\src\option\unicode.obj" "ff11a\src\diskio.obj" "ff11a\src\ff.obj" "cc3000\src\cc3000lib.obj" "cc3000\drivers\ek-tm4c129\board.obj" 
	-@echo 'Finished clean'
	-@echo ' '

post-build:
	-"C:/ti/ccsv6/utils/tiobj2bin/tiobj2bin" "cc3000_basic_wifi_application.out" "cc3000_basic_wifi_application.bin" "C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/bin/armofd" "C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/bin/armhex" "C:/ti/ccsv6/utils/tiobj2bin/mkhex4bin"
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets

