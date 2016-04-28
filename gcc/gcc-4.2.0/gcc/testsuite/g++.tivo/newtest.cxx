#include "recording.h"
#include "ref.h"

ref<Recording> var1;
nonRef<Recording> var2;
refConst<Recording> var3;
referenceNot<Recording> var4;
abRefthis<Recording> var5;

#ifndef __TIVO_VERSION__
#define __TIVO_VERSION__ 0
#endif

#if __TIVO_VERSION__ >= 1
Recording *var6;
#endif

#if __TIVO_VERSION__ == 0
Recording *var7;
#endif
