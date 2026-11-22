################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant_interface.c 

COMPILED_SRCS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant_interface.src 

C_DEPS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant_interface.d 

OBJS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant_interface.o 


# Each subdirectory must supply rules for building sources it contributes
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant_interface.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant_interface.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant_interface.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant_interface.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-zf_components

clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-zf_components:
	-$(RM) ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant_interface.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant_interface.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/zf_components/seekfree_assistant_interface.src

.PHONY: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-zf_components

