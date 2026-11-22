################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.c \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.c 

COMPILED_SRCS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.src \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.src 

C_DEPS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.d \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.d 

OBJS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.o \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.o 


# Each subdirectory must supply rules for building sources it contributes
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-infineon_libraries-2f-Infra-2f-Platform-2f-Tricore-2f-Compilers

clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-infineon_libraries-2f-Infra-2f-Platform-2f-Tricore-2f-Compilers:
	-$(RM) ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerDcc.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGhs.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerGnuc.src ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/CompilerTasking.src

.PHONY: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-infineon_libraries-2f-Infra-2f-Platform-2f-Tricore-2f-Compilers

