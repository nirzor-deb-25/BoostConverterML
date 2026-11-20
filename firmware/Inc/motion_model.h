#ifndef MOTION_MODEL_H
#define MOTION_MODEL_H

#define MOTION_MODEL_ID "76317ad9a8605641"
#define MOTION_FEATURE_COUNT 6U
#define MOTION_CLASS_COUNT 4U

/* Feature order:
 * 0: mean_x_mg
 * 1: mean_y_mg
 * 2: mean_z_mg
 * 3: std_x_mg
 * 4: std_y_mg
 * 5: std_z_mg
 */

typedef enum {
    MOTION_NEUTRAL = 0,
    MOTION_SHAKE = 1,
    MOTION_TILT_X_NEGATIVE = 2,
    MOTION_TILT_X_POSITIVE = 3,
    MOTION_INVALID = -1
} motion_class_t;

static inline motion_class_t motion_predict(const float *f)
{
    if (f == 0) { return MOTION_INVALID; }
    for (unsigned int i = 0U; i < MOTION_FEATURE_COUNT; i++) {
        if (!(f[i] >= -3.402823466e38F &&
              f[i] <= 3.402823466e38F)) {
            return MOTION_INVALID;
        }
    }
    if ((double)f[3] <= 0x1.1c30ee0000000p+2) {
        return MOTION_NEUTRAL;
    } else {
        if ((double)f[0] <= -0x1.3d11130000000p+8) {
            return MOTION_TILT_X_NEGATIVE;
        } else {
            if ((double)f[0] <= 0x1.2b820c0000000p+9) {
                return MOTION_SHAKE;
            } else {
                return MOTION_TILT_X_POSITIVE;
            }
        }
    }
}

#endif
