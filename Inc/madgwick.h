#ifndef MADGWICK_H_
#define MADGWICK_H_

#include <stdint.h>

void madgwick_init(float sample_freq, float beta);
void madgwick_set_beta(float beta);
void madgwick_reset(void);
void madgwick_update_6dof(float gx_rad, float gy_rad, float gz_rad,
                          float ax,     float ay,     float az);
void madgwick_get_quaternion(float *q0, float *q1, float *q2, float *q3);

#endif /* MADGWICK_H_ */
