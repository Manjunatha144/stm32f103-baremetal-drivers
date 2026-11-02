#include "madgwick.h"
#include <math.h>

static float s_beta = 0.08f;            // Filter gain balancing gyro drift vs accel noise
static float s_inv_sample_freq = 0.02f; // dt = 20 ms (50 Hz)
static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f; // Unit quaternion orientation

/* Fast inverse square root */
static float inv_sqrt(float x) {
    return 1.0f / sqrtf(x);
}

void madgwick_init(float sample_freq, float beta) {
    s_beta = beta;
    s_inv_sample_freq = 1.0f / sample_freq;
    q0 = 1.0f;
    q1 = 0.0f;
    q2 = 0.0f;
    q3 = 0.0f;
}

void madgwick_set_beta(float beta) {
    s_beta = beta;
}

void madgwick_reset(void) {
    q0 = 1.0f;
    q1 = 0.0f;
    q2 = 0.0f;
    q3 = 0.0f;
}

void madgwick_update_6dof(float gx, float gy, float gz, float ax, float ay, float az) {
    float recip_norm;
    float s0, s1, s2, s3;
    float q_dot1, q_dot2, q_dot3, q_dot4;
    float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2, _8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

    /* Rate of change of quaternion from gyroscope */
    q_dot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    q_dot2 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy);
    q_dot3 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx);
    q_dot4 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx);

    /* Compute feedback only if accelerometer measurement is valid */
    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
        /* Normalize accelerometer measurement */
        recip_norm = inv_sqrt(ax * ax + ay * ay + az * az);
        ax *= recip_norm;
        ay *= recip_norm;
        az *= recip_norm;

        /* Auxiliary variables to avoid repeated arithmetic */
        _2q0 = 2.0f * q0;
        _2q1 = 2.0f * q1;
        _2q2 = 2.0f * q2;
        _2q3 = 2.0f * q3;
        _4q0 = 4.0f * q0;
        _4q1 = 4.0f * q1;
        _4q2 = 4.0f * q2;
        _8q1 = 8.0f * q1;
        _8q2 = 8.0f * q2;
        q0q0 = q0 * q0;
        q1q1 = q1 * q1;
        q2q2 = q2 * q2;
        q3q3 = q3 * q3;

        /* Gradient descent algorithm corrective step */
        s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
        s1 = _8q1 * q1q1 + _4q1 * q3q3 + _2q3 * ax + _4q1 * (1.0f - 2.0f * q1q1 - 2.0f * q2q2) - _2q0 * ay - _4q1 + _8q1 * q2q2;
        s2 = _4q2 * (1.0f - 2.0f * q1q1 - 2.0f * q2q2) + _2q0 * ax + _4q2 * q0q0 - _2q3 * ay + _8q2 * q2q2;
        s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;

        /* Normalize step magnitude */
        recip_norm = inv_sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
        s0 *= recip_norm;
        s1 *= recip_norm;
        s2 *= recip_norm;
        s3 *= recip_norm;

        /* Apply feedback step */
        q_dot1 -= s_beta * s0;
        q_dot2 -= s_beta * s1;
        q_dot3 -= s_beta * s2;
        q_dot4 -= s_beta * s3;
    }

    /* Integrate rate of change of quaternion to yield quaternion */
    q0 += q_dot1 * s_inv_sample_freq;
    q1 += q_dot2 * s_inv_sample_freq;
    q2 += q_dot3 * s_inv_sample_freq;
    q3 += q_dot4 * s_inv_sample_freq;

    /* Normalize quaternion */
    recip_norm = inv_sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recip_norm;
    q1 *= recip_norm;
    q2 *= recip_norm;
    q3 *= recip_norm;
}

void madgwick_get_quaternion(float *out_q0, float *out_q1, float *out_q2, float *out_q3) {
    *out_q0 = q0;
    *out_q1 = q1;
    *out_q2 = q2;
    *out_q3 = q3;
}
