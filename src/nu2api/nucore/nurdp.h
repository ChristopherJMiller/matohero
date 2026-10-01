#ifndef NURDP_H
#define NURDP_H

/* Recursive-descent expression evaluators (nurdpf.c for floats, nurdpi.c for ints).
 * getvar resolves a name to a float (nurdpf) or an int (nurdpi) value. */
typedef int nurdpgetvarfn(char *name, float *fval, int *ival);

float NuRDPF(char *txt);
float NuRDPFVar(char *txt, nurdpgetvarfn *getvar);
int NuRDPI(char *txt);

#endif
