#ifndef SUB3_BUILD_MODE_H
#define SUB3_BUILD_MODE_H

/*
 * Car-follow build: enable encoders, absolute steering sensor, IMU and the
 * 10 ms chassis control ISR. Keep this at 1 only for camera/IPS display-only
 * testing; that mode intentionally skips all closed-loop vehicle hardware.
 */
#define SUB3_CAMERA_ONLY_MODE (0U)

#endif
