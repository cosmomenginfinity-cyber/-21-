################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Infra.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc0.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc1.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc2.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc3.c 

COMPILED_SRCS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Infra.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc0.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc1.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc2.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc3.src 

C_DEPS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Infra.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc0.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc1.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc2.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc3.d 

OBJS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Infra.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc0.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc1.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc2.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc3.o 


# Each subdirectory must supply rules for building sources it contributes
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Infra.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Infra.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Infra.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Infra.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc0.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc0.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc0.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc0.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc1.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc1.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc1.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc1.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc2.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc2.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc2.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc2.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc3.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc3.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc3.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc3.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-infineon_libraries-2f-Infra-2f-Ssw-2f-TC38A-2f-Tricore

clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-infineon_libraries-2f-Infra-2f-Ssw-2f-TC38A-2f-Tricore:
	-$(RM) ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Infra.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Infra.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Infra.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc0.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc0.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc0.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc1.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc1.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc1.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc2.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc2.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc2.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc3.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc3.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/Ifx_Ssw_Tc3.src

.PHONY: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-infineon_libraries-2f-Infra-2f-Ssw-2f-TC38A-2f-Tricore

