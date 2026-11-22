################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/IfxCcu6_PwmHl.c 

COMPILED_SRCS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/IfxCcu6_PwmHl.src 

C_DEPS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/IfxCcu6_PwmHl.d 

OBJS += \
./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/IfxCcu6_PwmHl.o 


# Each subdirectory must supply rules for building sources it contributes
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/IfxCcu6_PwmHl.src: ../_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/IfxCcu6_PwmHl.c _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/ADSworkplace/subject3/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/IfxCcu6_PwmHl.o: ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/IfxCcu6_PwmHl.src _reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-infineon_libraries-2f-iLLD-2f-TC38A-2f-Tricore-2f-Ccu6-2f-PwmHl

clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-infineon_libraries-2f-iLLD-2f-TC38A-2f-Tricore-2f-Ccu6-2f-PwmHl:
	-$(RM) ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/IfxCcu6_PwmHl.d ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/IfxCcu6_PwmHl.o ./_reference_tld7002_compare_20260818/TLD7002_TC387_Demo/libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/PwmHl/IfxCcu6_PwmHl.src

.PHONY: clean-_reference_tld7002_compare_20260818-2f-TLD7002_TC387_Demo-2f-libraries-2f-infineon_libraries-2f-iLLD-2f-TC38A-2f-Tricore-2f-Ccu6-2f-PwmHl

