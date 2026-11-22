################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu0_main.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu1_main.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu2_main.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu3_main.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/isr.c 

COMPILED_SRCS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu0_main.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu1_main.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu2_main.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu3_main.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/isr.src 

C_DEPS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu0_main.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu1_main.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu2_main.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu3_main.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/isr.d 

OBJS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu0_main.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu1_main.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu2_main.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu3_main.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/isr.o 


# Each subdirectory must supply rules for building sources it contributes
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu0_main.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu0_main.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu0_main.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu0_main.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu1_main.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu1_main.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu1_main.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu1_main.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu2_main.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu2_main.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu2_main.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu2_main.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu3_main.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu3_main.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu3_main.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu3_main.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/isr.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/isr.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/isr.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/isr.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-user

clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-user:
	-$(RM) ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu0_main.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu0_main.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu0_main.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu1_main.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu1_main.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu1_main.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu2_main.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu2_main.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu2_main.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu3_main.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu3_main.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/cpu3_main.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/isr.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/isr.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/user/isr.src

.PHONY: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-user

