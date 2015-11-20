################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
ff11a/src/diskio.obj: ../ff11a/src/diskio.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_15.9.0.STS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me -O2 --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-arm_15.9.0.STS/include" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/examples/boards/ek-tm4c1294xl-boost-cc3000" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/drivers/ek-tm4c129" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/ff11a" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/usblib" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/src/host_driver/core_driver" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/src/host_driver/include" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/src/host_driver" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/src" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/src/spi" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/src/uart" -g --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=CC3000_USE_BOOSTERPACK2 --define=_POSIX_SOURCE --define=TARGET_IS_TM4C129_RA0 --define=CC3000_TM4C129_SPI --define=UART_BUFFERED --display_error_number --diag_wrap=off --diag_warning=225 --gen_func_subsections=on --abi=eabi --ual --preproc_with_compile --preproc_dependency="ff11a/src/diskio.pp" --obj_directory="ff11a/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

ff11a/src/ff.obj: ../ff11a/src/ff.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_15.9.0.STS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me -O2 --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-arm_15.9.0.STS/include" --include_path="C:/ti/TivaWare_C_Series-2.1.1.71/examples/boards/ek-tm4c1294xl-boost-cc3000" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/drivers/ek-tm4c129" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/ff11a" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/usblib" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/src/host_driver/core_driver" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/src/host_driver/include" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/src/host_driver" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/src" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/src/spi" --include_path="C:/Users/Al/workspace_v6_1/cc3000_basic_wifi_application/cc3000/src/uart" -g --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --define=CC3000_USE_BOOSTERPACK2 --define=_POSIX_SOURCE --define=TARGET_IS_TM4C129_RA0 --define=CC3000_TM4C129_SPI --define=UART_BUFFERED --display_error_number --diag_wrap=off --diag_warning=225 --gen_func_subsections=on --abi=eabi --ual --preproc_with_compile --preproc_dependency="ff11a/src/ff.pp" --obj_directory="ff11a/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


