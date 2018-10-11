#ifndef THREADS_FIXED_H
#define THREADS_FIXED_H

/* Type definition of fixed point number */
typedef int fixed_point;
/* Define fractional part */
#define F_SHIFT 14 
/* Convert integer to fixed point */
#define FX_convert(X) ((fixed_point)(X << F_SHIFT))
/* Convert fixed point back to integer */
#define FX_back(X) (X >= 0 ? ((X + (1 << (F_SHIFT - 1))) >> F_SHIFT) : ((X - (1 << (F_SHIFT - 1))) >> F_SHIFT))
/* Add two fixed point number */
#define FX_add(X,Y) (X + Y)
#define FX_madd(X, Y) (X + (Y << F_SHIFT))
/* Substarct */
#define FX_min(X, Y) (X - Y)
#define FX_mmin(X, Y) (X - (Y << F_SHIFT))
/* Mutiple */
#define FX_mut(X, Y) (X * Y)
#define FX_mmut(X, Y) ((fixed_point)(((int64_t) X) * Y >> FP_SHIFT_AMOUNT))
/* Divide */
#define FX_div(X, Y) (X / Y)
#define FX_mdiv(X, Y) ((fixed_t)((((int64_t) X) << FP_SHIFT_AMOUNT) / Y))


#endif /* threads/fixed.h */
