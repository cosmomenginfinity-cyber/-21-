################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Console.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Shell.c 

COMPILED_SRCS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Console.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Shell.src 

C_DEPS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Console.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Shell.d 

OBJS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Console.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Shell.o 


# Each subdirectory must supply rules for building sources it contributes
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Console.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Console.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Console.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Console.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Shell.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Shell.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Shell.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Shell.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-infineon_libraries-2f-Service-2f-CpuGeneric-2f-SysSe-2f-Comm

clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-infineon_libraries-2f-Service-2f-CpuGeneric-2f-SysSe-2f-Comm:
	-$(RM) ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Console.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Console.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Console.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Shell.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Shell.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Service/CpuGeneric/SysSe/Comm/Ifx_Shell.src

.PHONY: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-infineon_libraries-2f-Service-2f-CpuGeneric-2f-SysSe-2f-Comm

