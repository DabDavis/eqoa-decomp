#ifndef _MATH_H
#define _MATH_H
#ifdef __cplusplus
extern "C" {
#endif
/* newlib's: the image's isinf loads HUGE_VAL from the import __infinity. */
union __dmath { unsigned long i[2]; double d; };
extern const union __dmath __infinity[];
#define HUGE_VAL (__infinity[0].d)
double pow(double, double);
double floor(double);
double ceil(double);
double fabs(double);
double log10(double);
double fmod(double, double);
int isnan(double);
int finite(double);
#ifdef __cplusplus
}
#endif
#endif
