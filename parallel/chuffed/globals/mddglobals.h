#ifndef __MDDGLOBALS_H__
#define __MDDGLOBALS_H__

#include <chuffed/support/vec.h>

void mdd_table(vec<IntVar*>& x, vec<vec<int> >& t);

void mdd_regular(vec<IntVar*>& x, int q, int s, vec<vec<int> >& d, int q0, vec<int>& f);

#endif
