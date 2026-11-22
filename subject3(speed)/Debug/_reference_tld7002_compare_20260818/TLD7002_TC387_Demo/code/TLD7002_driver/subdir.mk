################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002FuncLayer.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ControlLayer.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ServiceLayer.c 

COMPILED_SRCS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002FuncLayer.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ControlLayer.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ServiceLayer.src 

C_DEPS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002FuncLayer.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ControlLayer.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ServiceLayer.d 

OBJS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002FuncLayer.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ControlLayer.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ServiceLayer.o 


# Each subdirectory must supply rules for building sources it contributes
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002FuncLayer.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002FuncLayer.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002FuncLayer.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002FuncLayer.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ControlLayer.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ControlLayer.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ControlLayer.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ControlLayer.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ServiceLayer.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ServiceLayer.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ServiceLayer.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ServiceLayer.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-code-2f-TLD7002_driver

clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-code-2f-TLD7002_driver:
	-$(RM) ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002FuncLayer.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002FuncLayer.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002FuncLayer.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ControlLayer.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ControlLayer.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ControlLayer.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ServiceLayer.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ServiceLayer.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/code/TLD7002_driver/TLD7002_ServiceLayer.src

.PHONY: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-code-2f-TLD7002_driver

