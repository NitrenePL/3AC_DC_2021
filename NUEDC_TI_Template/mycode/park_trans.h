#ifndef PARK_TRANS_H_
#define PARK_TRANS_H_

#include <math.h>
#include <stdint.h>
#include "driverlib.h"

#ifndef PI
#define PI 3.14159265358979f
#endif

// 函数声明
void ab_to_dq(float32_t Alpha, float32_t Beta, float32_t wt, float32_t *d, float32_t *q);
void dq_to_AlphaBeta(float32_t *Alpha, float32_t *Beta, float32_t wt, float32_t d, float32_t q);
void abc_to_abz(float32_t a, float32_t b, float32_t c, float32_t *Alpha, float32_t *Beta, float32_t *z);
void abz_to_abc(float32_t *a, float32_t *b, float32_t *c, float32_t Alpha, float32_t Beta, float32_t z);
void abc_to_dqz(float32_t a, float32_t b, float32_t c, float32_t wt, float32_t *d, float32_t *q, float32_t *z);
void dqz_to_abc(float32_t *a, float32_t *b, float32_t *c, float32_t wt, float32_t d, float32_t q, float32_t z);

#endif /* PARK_TRANS_H_ */
