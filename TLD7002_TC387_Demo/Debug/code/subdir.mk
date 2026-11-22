################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../code/PID.c \
../code/encoder.c \
../code/encoder_motor_ctl.c \
../code/flash.c \
../code/global_vars.c \
../code/imu_ahrs.c \
../code/imu_datalogger.c \
../code/initialize.c \
../code/ins.c \
../code/key_mode.c \
../code/menu.c \
../code/motor.c \
../code/pathrecorder.c \
../code/remote_control.c \
../code/sub2_globals.c \
../code/sub2_menu.c \
../code/sub2_navigation.c \
../code/sub2_waypoint_store.c \
../code/zf_device_dot_matrix_screen.c \
../code/zf_device_tld7002.c 

COMPILED_SRCS += \
code/PID.src \
code/encoder.src \
code/encoder_motor_ctl.src \
code/flash.src \
code/global_vars.src \
code/imu_ahrs.src \
code/imu_datalogger.src \
code/initialize.src \
code/ins.src \
code/key_mode.src \
code/menu.src \
code/motor.src \
code/pathrecorder.src \
code/remote_control.src \
code/sub2_globals.src \
code/sub2_menu.src \
code/sub2_navigation.src \
code/sub2_waypoint_store.src \
code/zf_device_dot_matrix_screen.src \
code/zf_device_tld7002.src 

C_DEPS += \
code/PID.d \
code/encoder.d \
code/encoder_motor_ctl.d \
code/flash.d \
code/global_vars.d \
code/imu_ahrs.d \
code/imu_datalogger.d \
code/initialize.d \
code/ins.d \
code/key_mode.d \
code/menu.d \
code/motor.d \
code/pathrecorder.d \
code/remote_control.d \
code/sub2_globals.d \
code/sub2_menu.d \
code/sub2_navigation.d \
code/sub2_waypoint_store.d \
code/zf_device_dot_matrix_screen.d \
code/zf_device_tld7002.d 

OBJS += \
code/PID.o \
code/encoder.o \
code/encoder_motor_ctl.o \
code/flash.o \
code/global_vars.o \
code/imu_ahrs.o \
code/imu_datalogger.o \
code/initialize.o \
code/ins.o \
code/key_mode.o \
code/menu.o \
code/motor.o \
code/pathrecorder.o \
code/remote_control.o \
code/sub2_globals.o \
code/sub2_menu.o \
code/sub2_navigation.o \
code/sub2_waypoint_store.o \
code/zf_device_dot_matrix_screen.o \
code/zf_device_tld7002.o 


# Each subdirectory must supply rules for building sources it contributes
code/PID.src: ../code/PID.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/PID.o: code/PID.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/encoder.src: ../code/encoder.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/encoder.o: code/encoder.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/encoder_motor_ctl.src: ../code/encoder_motor_ctl.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/encoder_motor_ctl.o: code/encoder_motor_ctl.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/flash.src: ../code/flash.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/flash.o: code/flash.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/global_vars.src: ../code/global_vars.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/global_vars.o: code/global_vars.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/imu_ahrs.src: ../code/imu_ahrs.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/imu_ahrs.o: code/imu_ahrs.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/imu_datalogger.src: ../code/imu_datalogger.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/imu_datalogger.o: code/imu_datalogger.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/initialize.src: ../code/initialize.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/initialize.o: code/initialize.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/ins.src: ../code/ins.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/ins.o: code/ins.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/key_mode.src: ../code/key_mode.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/key_mode.o: code/key_mode.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/menu.src: ../code/menu.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/menu.o: code/menu.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/motor.src: ../code/motor.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/motor.o: code/motor.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/pathrecorder.src: ../code/pathrecorder.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/pathrecorder.o: code/pathrecorder.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/remote_control.src: ../code/remote_control.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/remote_control.o: code/remote_control.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/sub2_globals.src: ../code/sub2_globals.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/sub2_globals.o: code/sub2_globals.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/sub2_menu.src: ../code/sub2_menu.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/sub2_menu.o: code/sub2_menu.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/sub2_navigation.src: ../code/sub2_navigation.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/sub2_navigation.o: code/sub2_navigation.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/sub2_waypoint_store.src: ../code/sub2_waypoint_store.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/sub2_waypoint_store.o: code/sub2_waypoint_store.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/zf_device_dot_matrix_screen.src: ../code/zf_device_dot_matrix_screen.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/zf_device_dot_matrix_screen.o: code/zf_device_dot_matrix_screen.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
code/zf_device_tld7002.src: ../code/zf_device_tld7002.c code/subdir.mk
	cctc -cs --dep-file="$(*F).d" --misrac-version=2004 "-fD:/tc387/TC387_Library-master/Example/Example_dual_camera/Motherboard_Demo/TLD7002_TC387_Demo/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=2 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
code/zf_device_tld7002.o: code/zf_device_tld7002.src code/subdir.mk
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-code

clean-code:
	-$(RM) code/PID.d code/PID.o code/PID.src code/encoder.d code/encoder.o code/encoder.src code/encoder_motor_ctl.d code/encoder_motor_ctl.o code/encoder_motor_ctl.src code/flash.d code/flash.o code/flash.src code/global_vars.d code/global_vars.o code/global_vars.src code/imu_ahrs.d code/imu_ahrs.o code/imu_ahrs.src code/imu_datalogger.d code/imu_datalogger.o code/imu_datalogger.src code/initialize.d code/initialize.o code/initialize.src code/ins.d code/ins.o code/ins.src code/key_mode.d code/key_mode.o code/key_mode.src code/menu.d code/menu.o code/menu.src code/motor.d code/motor.o code/motor.src code/pathrecorder.d code/pathrecorder.o code/pathrecorder.src code/remote_control.d code/remote_control.o code/remote_control.src code/sub2_globals.d code/sub2_globals.o code/sub2_globals.src code/sub2_menu.d code/sub2_menu.o code/sub2_menu.src code/sub2_navigation.d code/sub2_navigation.o code/sub2_navigation.src code/sub2_waypoint_store.d code/sub2_waypoint_store.o code/sub2_waypoint_store.src code/zf_device_dot_matrix_screen.d code/zf_device_dot_matrix_screen.o code/zf_device_dot_matrix_screen.src code/zf_device_tld7002.d code/zf_device_tld7002.o code/zf_device_tld7002.src

.PHONY: clean-code

