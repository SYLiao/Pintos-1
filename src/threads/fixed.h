#ifndef THREADS_FIXED_H
#define THREADS_FIXED_H

/* Type definition of fixed point number */
typedef int fixed_point;
/* Define fractional part */
#define F_SHIFT 14
/* Convert integer to fixed point */
#define FX_convert(X) ((fixed_point)(X << F_SHIFT))
/* Convert fixed point back to integer */
#define FX_back_int(X) (X >> F_SHIFT)
#define FX_back(X) (X >= 0 ? ((X + (1 << (F_SHIFT - 1))) >> F_SHIFT) : ((X - (1 << (F_SHIFT - 1))) >> F_SHIFT))
/* Add */
#define FX_plus(X,Y) (X + Y)
#define FX_mplus(X, Y) (X + (Y << F_SHIFT))
/* Substarct */
#define FX_min(X, Y) (X - Y)
#define FX_mmin(X, Y) (X - (Y << F_SHIFT))
/* Mutiple */
#define FX_mmut(X, Y) (X * Y)
#define FX_mut(X, Y) ((fixed_point)(((int64_t) X) * Y >> F_SHIFT))
/* Divide */
#define FX_mdiv(X, Y) (X / Y)
#define FX_div(X, Y) ((fixed_point)((((int64_t) X) << F_SHIFT) / Y))


#endif /* threads/fixed.h */
