################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
cc3000_basic_wifi_application.obj: ../cc3000_basic_wifi_application.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O2 --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/include" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/examples/boards/ek-tm4c1294xl-boost-cc3000" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/drivers/ek-tm4c129" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/ff11a" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/src/host_driver/core_driver" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/src/host_driver/include" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/src/host_driver" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/src" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/src/spi" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/src/uart" -g --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=CC3000_USE_BOOSTERPACK2 --define=_POSIX_SOURCE --define=TARGET_IS_TM4C129_RA0 --define=CC3000_TM4C129_SPI --define=UART_BUFFERED --define=SEND_NON_BLOCKING --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --ual --preproc_with_compile --preproc_dependency="cc3000_basic_wifi_application.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

startup_ccs.obj: ../startup_ccs.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O2 --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/include" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/examples/boards/ek-tm4c1294xl-boost-cc3000" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/drivers/ek-tm4c129" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/ff11a" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/src/host_driver/core_driver" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/src/host_driver/include" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/src/host_driver" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/src" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/src/spi" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/cc3000/src/uart" -g --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=CC3000_USE_BOOSTERPACK2 --define=_POSIX_SOURCE --define=TARGET_IS_TM4C129_RA0 --define=CC3000_TM4C129_SPI --define=UART_BUFFERED --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --ual --preproc_with_compile --preproc_dependency="startup_ccs.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


