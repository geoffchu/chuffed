#include <chuffed/core/propagator.h>

// y = |x|

template <int U = 0, int V = 0>
class Abs : public Propagator, public Checker {
public:
	IntView<U> x;
	IntView<V> y;

	Abs(IntView<U> _x, IntView<V> _y) : x(_x), y(_y) {
		priority = 1;
		x.attach(this, 0, EVENT_LU);
		y.attach(this, 1, EVENT_U);
	}

	bool propagate() {

		int64_t l = x.getMin();
		int64_t u = x.getMax();

		if (l >= 0) {
			setDom(y, setMin, l, x.getMinLit());
			setDom(y, setMax, u, x.getMinLit(), x.getMaxLit());
		} else if (u <= 0) {
			setDom(y, setMin, -u, x.getMaxLit());
			setDom(y, setMax, -l, x.getMaxLit(), x.getMinLit());
		} else {
			// Finesse stronger bound
			int64_t t = (-l > u ? -l : u);
			setDom(y, setMax, t, x.getMaxLit(), x.getMinLit());
//			setDom(y, setMax, t, x.getFMaxLit(t), x.getFMinLit(-t));
//			setDom(y, setMax, t, x.getLit(t+1, 2), x.getLit(-t-1, 3));
		}


		setDom(x, setMax, y.getMax(), y.getMaxLit());
		setDom(x, setMin, -y.getMax(), y.getMaxLit());

/*
		if (x.isFixed()) {
			if (x.getVal() < 0) {
				setDom(y, setMin, -x.getVal(), x.getMaxLit());
				setDom(y, setMax, -x.getVal(), x.getMinLit());
			} else if (x.getVal() > 0) {
				setDom(y, setMin, x.getVal(), x.getMinLit());
				setDom(y, setMax, x.getVal(), x.getMaxLit());
			} else {
				setDom(y, setVal, 0, x.getMinLit(), x.getMaxLit());
			}
		}
*/

		return true;
	}

	bool check() {
		return ((x.getShadowVal() == y.getShadowVal()) || (x.getShadowVal() == -y.getShadowVal()));
	}

};

// y = |x|

void int_abs(IntVar* x, IntVar* y) {
	int_rel(y, IRT_GE, 0);
	new Abs<>(IntView<>(x), IntView<>(y));
}

//-----

// x * y = z     x, y, z >= 0 

template <int U = 0, int V = 0, int W = 0>
class Times : public Propagator, public Checker {
public:
	IntView<U> x;
	IntView<V> y;
	IntView<W> z;

	Times(IntView<U> _x, IntView<V> _y, IntView<W> _z) :
		x(_x), y(_y), z(_z) {
		priority = 1;
		assert(x.getMin() >= 0 && y.getMin() >= 0 && z.getMin() >= 0);
		x.attach(this, 0, EVENT_LU);
		y.attach(this, 1, EVENT_LU);
		z.attach(this, 2, EVENT_LU);
	}

	bool propagate() {
		int64_t x_min = x.getMin(), x_max = x.getMax();
		int64_t y_min = y.getMin(), y_max = y.getMax();
		int64_t z_min = z.getMin(), z_max = z.getMax();
		
		// z >= x.min * y.min
		setDom(z, setMin, x_min*y_min, x.getMinLit(), y.getMinLit());
		// z <= x.max * y.max
		if (x_max * y_max < IntVar::max_limit)
		setDom(z, setMax, x_max*y_max, x.getMaxLit(), y.getMaxLit());

		// x >= ceil(z.min / y.max)
		if (y_max >= 1) {
			setDom(x, setMin, (z_min+y_max-1)/y_max, y.getMaxLit(), z.getMinLit());
		}

		// x <= floor(z.max / y.min)
		if (y_min >= 1) {
			setDom(x, setMax, z_max/y_min, y.getMinLit(), z.getMaxLit());
		}

		// y >= ceil(z.min / x.max)
		if (x_max >= 1) {
			setDom(y, setMin, (z_min+x_max-1)/x_max, x.getMaxLit(), z.getMinLit());
		}

		// y <= floor(z.max / x.min)
		if (x_min >= 1) {
			setDom(y, setMax, z_max/x_min, x.getMinLit(), z.getMaxLit());
		}

		return true;
	}

	bool check() {
		return (x.getShadowVal() * y.getShadowVal() == z.getShadowVal());
	}

};

int get_sign(IntVar* x) {
	if (x->getMin() >= 0) return 1;
	if (x->getMax() <= 0) return -1;
	return 0;
}

// z = x * y

void int_times(IntVar* x, IntVar* y, IntVar* z) {
	if (!get_sign(x) || !get_sign(y) || !get_sign(z)) {
		ERROR("Cannot handle non-sign-fixed vars\n");
	}
	bool x_flip = (get_sign(x) == -1);
	bool y_flip = (get_sign(y) == -1);
	bool z_flip = (get_sign(z) == -1);

	if (!x_flip && !y_flip && !z_flip) {
		new Times<0,0,0>(IntView<>(x), IntView<>(y), IntView<>(z));
	} else if (!x_flip && y_flip && z_flip) {
		new Times<0,1,1>(IntView<>(x), IntView<>(y), IntView<>(z));
	} else if (x_flip && !y_flip && z_flip) {
		new Times<1,0,1>(IntView<>(x), IntView<>(y), IntView<>(z));
	} else if (x_flip && y_flip && !z_flip) {
		new Times<1,1,0>(IntView<>(x), IntView<>(y), IntView<>(z));
	} else {
		ERROR("Cannot handle this case\n");
	}
}

//-----

// ceil(x / y) = z     x, y, z >= 0

// x / y <= z
// x / y > z - 1

// floor(x / y) = z  <=>  ceil(x+1 / y) = z+1

template <int U = 0, int V = 0, int W = 0>
class Divide : public Propagator, public Checker {
public:
	IntView<U> x;
	IntView<V> y;
	IntView<W> z;

	Divide(IntView<U> _x, IntView<V> _y, IntView<W> _z) :
		x(_x), y(_y), z(_z) {
		priority = 1;
		assert(x.getMin() >= 0 && y.getMin() >= 1 && z.getMin() >= 0);
		x.attach(this, 0, EVENT_LU);
		y.attach(this, 1, EVENT_LU);
		z.attach(this, 2, EVENT_LU);
	}

	bool propagate() {
		int64_t x_min = x.getMin(), x_max = x.getMax();
		int64_t y_min = y.getMin(), y_max = y.getMax();
		int64_t z_min = z.getMin(), z_max = z.getMax();
		
		// z >= ceil(x.min / y.max)
		setDom(z, setMin, (x_min+y_max-1)/y_max, x.getMinLit(), y.getMaxLit());
		// z <= ceil(x.max / y.min)
		setDom(z, setMax, (x_max+y_min-1)/y_min, x.getMaxLit(), y.getMinLit());

		// x >= y.min * (z.min - 1) + 1
		setDom(x, setMin, y_min*(z_min-1)+1, y.getMinLit(), z.getMinLit());
		// x <= y.max * z.max
		setDom(x, setMax, y_max*z_max, y.getMaxLit(), z.getMaxLit());

		// y >= ceil(x.min / z.max)
		if (z_max >= 1) {
			setDom(y, setMin, (x_min+z_max-1)/z_max, x.getMinLit(), z.getMaxLit());
		}

		// y <= ceil(x.max / z.min-1) - 1
		if (z_min >= 2) {
			setDom(y, setMax, (x_max+z_min-2)/(z_min-1)-1, x.getMaxLit(), z.getMinLit());
		}

		return true;
	}

	bool check() {
		return ((int) ceil(x.getShadowVal() / y.getShadowVal()) == z.getShadowVal());
	}

};

// z = floor(x / y)

void int_div(IntVar* x, IntVar* y, IntVar* z) {
	if (!get_sign(x) || !get_sign(y) || !get_sign(z)) {
		ERROR("Cannot handle non-sign-fixed vars\n");
	}
	bool x_flip = (get_sign(x) == -1);
	bool y_flip = (get_sign(y) == -1);
	bool z_flip = (get_sign(z) == -1);

	if (!x_flip && !y_flip && !z_flip) {
		// ceil(x+1 / y) = z+1
		new Divide<4,0,4>(IntView<>(x,1,1), IntView<>(y), IntView<>(z,1,1));
	} else if (!x_flip && y_flip && z_flip) {
		// ceil(x / -y) = -z
		new Divide<0,1,1>(IntView<>(x), IntView<>(y), IntView<>(z));
	} else if (x_flip && !y_flip && z_flip) {
		// ceil(-x / y) = -z
		new Divide<1,0,1>(IntView<>(x), IntView<>(y), IntView<>(z));
	} else if (x_flip && y_flip && !z_flip) {
		// ceil(-x+1 / -y) = z+1
		new Divide<5,1,4>(IntView<>(x,1,1), IntView<>(y), IntView<>(z,1,1));
	} else {
		ERROR("Cannot handle this case\n");
	}
}

void int_mod(IntVar* x, IntVar* y, IntVar* z) {
	ERROR("Not yet supported\n");
}

// z = min(x, y)

template <int U>
class Min2 : public Propagator, public Checker {
public:
	IntView<U> x, y, z;

	Min2(IntView<U> _x, IntView<U> _y, IntView<U> _z) :
		x(_x), y(_y), z(_z) {
		priority = 1;
		x.attach(this, 0, EVENT_LU);
		y.attach(this, 1, EVENT_LU);
		z.attach(this, 2, EVENT_L);
	}

	bool propagate() {
		// make a less than or equal to min(max(b_i))

		setDom(z, setMax, x.getMax(), x.getMaxLit());
		setDom(z, setMax, y.getMax(), y.getMaxLit());

		int64_t m = (x.getMin() < y.getMin() ? x.getMin() : y.getMin());
		setDom(z, setMin, m, x.getFMinLit(m), y.getFMinLit(m));

		setDom(x, setMin, z.getMin(), z.getMinLit());
		setDom(y, setMin, z.getMin(), z.getMinLit());

		if (z.getMin() == x.getMax() || z.getMin() == y.getMax()) satisfied = true;

		return true;
	}

	bool check() {
		return ((int) std::min(x.getShadowVal(), y.getShadowVal()) == z.getShadowVal());
	}

	int checkSatisfied() {
		if (satisfied) return 1;
		if (z.getMin() == x.getMax() || z.getMin() == y.getMax()) satisfied = true;
		return 3;
	}

};


void int_min(IntVar* x, IntVar* y, IntVar* z) {
	new Min2<0>(IntView<0>(x), IntView<0>(y), IntView<0>(z));
}

void int_max(IntVar* x, IntVar* y, IntVar* z) {
	new Min2<1>(IntView<1>(x), IntView<1>(y), IntView<1>(z));
}


//-----

// z = x + y

void int_plus(IntVar* x, IntVar* y, IntVar* z) {
	vec<int> coeffs;
	vec<IntVar*> w;
	coeffs.push(1); w.push(x);
	coeffs.push(1); w.push(y);
	coeffs.push(-1); w.push(z);
	int_linear(coeffs, w, IRT_EQ, 0);
}

//-----

// z = x - y

void int_minus(IntVar* x, IntVar* y, IntVar* z) {
	int_plus(y, z, x);
}

//-----

// y = -x

void int_negate(IntVar* x, IntVar* y) {
	bin_linear(x, y, IRT_EQ, 0);
}


//-----

// y = bool2int(x)

void bool2int(BoolView x, IntVar* y) {
	int_rel(y, IRT_GE, 0);
	int_rel(y, IRT_LE, 1);
	y->specialiseToEL();
	bool_rel(x, BRT_EQ, BoolView(y->getLit(1,2)));
}

