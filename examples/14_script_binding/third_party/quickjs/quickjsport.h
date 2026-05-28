#ifndef QUICKJSPORT_H
#define QUICKJSPORT_H
#include <math.h>
static double js_math_log2(double x) { return log2(x); }
#define log2 js_math_log2
#endif
