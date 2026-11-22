################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include user/subdir.mk
-include libraries/zf_driver/subdir.mk
-include libraries/zf_device/subdir.mk
-include libraries/zf_components/subdir.mk
-include libraries/zf_common/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/_PinMap/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/_Lib/DataHandling/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/_Impl/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Stm/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Src/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Scu/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Qspi/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Qspi/SpiMaster/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Port/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Pms/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Mtu/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Gtm/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Gtm/Atom/Pwm/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Gpt12/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Gpt12/IncrEnc/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Flash/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Evadc/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Evadc/Adc/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Dma/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Dma/Dma/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Cpu/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Cpu/Irq/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/Timer/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Ccu6/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Asclin/Std/subdir.mk
-include libraries/infineon_libraries/iLLD/TC38A/Tricore/Asclin/Asc/subdir.mk
-include libraries/infineon_libraries/Service/CpuGeneric/SysSe/Math/subdir.mk
-include libraries/infineon_libraries/Service/CpuGeneric/SysSe/Bsp/subdir.mk
-include libraries/infineon_libraries/Service/CpuGeneric/StdIf/subdir.mk
-include libraries/infineon_libraries/Service/CpuGeneric/If/subdir.mk
-include libraries/infineon_libraries/Infra/Ssw/TC38A/Tricore/subdir.mk
-include libraries/infineon_libraries/Infra/Platform/Tricore/Compilers/subdir.mk
-include libraries/infineon_libraries/Configurations/subdir.mk
-include code/xf_asr/subdir.mk
-include code/TLD7002_driver/subdir.mk
-include code/subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := subject3_speed
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_SIZE += \
subject3_speed.siz \


# All Target
all: main-build

# Main-build Target
main-build: subject3_speed.elf secondary-outputs

# Tool invocations
subject3_speed.elf: $(OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: TASKING Linker'
	cctc --lsl-file="../Lcf_Tasking_Tricore_Tc.lsl" -Wl-Oc -Wl-OL -Wl-Ot -Wl-Ox -Wl-Oy "D:\tc387\TC387_Library-master\Example\Example_dual_camera\Motherboard_Demo\subject3(speed)\libraries\zf_device\zf_device_config.a" -Wl--map-file="subject3_speed.map" -Wl-mc -Wl-mf -Wl-mi -Wl-mk -Wl-ml -Wl-mm -Wl-md -Wl-mr -Wl-mu --no-warnings= -Wl--error-limit=42 --fp-model=3 -lrt -lcs_fpu --lsl-core=vtc --exceptions --strict --anachronisms --force-c++ -Ctc38x -o"subject3_speed.elf" -Wl-o"subject3_speed.hex:IHEX" $(OBJS)
	@echo 'Finished building target: $@'
	@echo ' '

subject3_speed.siz: subject3_speed.elf makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: Print Size'
	elfsize  "subject3_speed.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) subject3_speed.siz subject3_speed.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_SIZE)

.PHONY: all clean dependents main-build

-include ../makefile.targets
