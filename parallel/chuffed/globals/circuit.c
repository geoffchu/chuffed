#include <chuffed/core/propagator.h>
//#include "globals/alldiffbase.h"

bool circuitSCC(int sz, IntVar ** a) {
/*
	int root = 0;
//	int root = myrand()%sz;
	int gindex = 0;   // dfs depth
	int num_sccs = 0; // number of sccs
	int index[sz];    // depth at which it was first seen
	int lowlink[sz];  // earliest node it can link to
	int highlink[sz]; // latest scc it can link to
	bool in_s[sz];    // is it in stack s?
	int scc_num[sz];  // which scc is it in
	uint64_t aset[sz];     // allowed values indexed by scc_num
	vec<int> s;       // seen and not in a scc

	uint64_t cur_aset = bit[sz]-1;
	aset[0] = cur_aset;

	struct TarjanNode {
		int u; int alt; bool* val;
	};

	// search data
	TarjanNode ns[sz];
	TarjanNode *n = ns-1;
	int v;
	
	for (int i = 0; i < sz; i++) {
		index[i] = -1;
		scc_num[i] = -1;
		in_s[i] = false;
	}

	index[root] = gindex++;
	scc_num[root] = num_sccs++;
	cur_aset ^= bit[root];
	aset[num_sccs] = cur_aset;

	for (int i = 0; i < sz; i++) {
		v = i;
		if (index[v] != -1) continue;
		goto Expand;
		while (true) {
			while (n->alt == sz) {
				if (lowlink[n->u] == index[n->u]) {
					if (highlink[n->u] != num_sccs-1) {
						for (int i = 0; i < sz; i++) {
							for (int j = 0; j < sz; j++) printf("%d", getbit(j, a[i]->getVals())); printf("\n");
						}
						for (int i = 0; i < sz; i++) {
							printf("%d %d %d %d\n", index[i], lowlink[i], highlink[i], scc_num[i]);
						}
						printf("%d\n", n->u);
						exit(0);
						return false;
					}
					int x;
					do {
						x = s.last();	s.pop();	in_s[x] = false;
						highlink[x] = scc_num[x] = num_sccs;
						cur_aset ^= bit[x];
					} while (x != n->u);
					num_sccs++;
					aset[num_sccs] = cur_aset;
				}
				if (n-- == ns) goto Finish;
				if (lowlink[(n+1)->u] < lowlink[n->u]) lowlink[n->u] = lowlink[(n+1)->u];
				if (highlink[(n+1)->u] > highlink[n->u]) highlink[n->u] = highlink[(n+1)->u];
			}
			v = n->alt++;
			if (!n->val[v]) continue;
			if (index[v] == -1) {
				Expand:;
				n++;
				n->u = v;
				n->alt = 0;
				n->val = a[v]->getVals();
				lowlink[v] = index[v] = gindex++;
				highlink[v] = -1;
				s.push(v); in_s[v] = true;
				continue;
			}
			if (in_s[v]) {
				if (index[v] < lowlink[n->u]) lowlink[n->u] = index[v];
			} else {
				if (scc_num[v] > highlink[n->u]) highlink[n->u] = scc_num[v];
			}
		}
		Finish:;
	}
*/
/*
	for (int i = 0; i < sz; i++) {
		if (scc_num[i] > 0) {
			if (!a[i]->allowSet(aset[scc_num[i]-1])) return false;
		}
	}
*/
	return true;
}

//-----

// a[i] forms a circuit
// Value consistent propagator

class Circuit : public Propagator {
public:
	int const sz;
	IntView<> * const x;

	// Persistent state

//	AllDiffBase<offset> alldiff;
	Tchar *chains;
	Tint num_unfixed;

	// Intermediate state
	vec<int> new_fixed;

	Circuit(vec<IntView<> > _x) : sz(_x.size()), x(_x.release()), // alldiff(sz,x),
		num_unfixed(sz) {
		NOT_SUPPORTED;
		priority = 2;
		new_fixed.reserve(sz);
		chains = (Tchar*) malloc(sz * sizeof(Tchar));
		for (int i = 0; i < sz; i++) chains[i] = 0;
		for (int i = 0; i < sz; i++) x[i].attach(this, i, EVENT_F);
	}

	void wakeup(int i, int c) {
		if (c & EVENT_F) new_fixed.push(i);
		pushInQueue();
	}

	bool propagate() {
		for (int i = 0; i < new_fixed.size(); i++) {
			int a = new_fixed[i];
			int b = x[a].getVal();
			Clause* r = NULL;
			if (so.lazy) {
				r = Reason_new(2);
				(*r)[1] = x[a].getValLit();
			}
			for (int j = 0; j < sz; j++) {
				if (j == a) continue;
				if (x[j].remValNotR(b)) if (!x[j].remVal(b, r)) return false;
			}
			num_unfixed = num_unfixed - 1;

			assert(chains[a] <= 0);
			assert(chains[b] >= 0);

			if (chains[a] < 0) {
				int q = -chains[a]-1;
				assert(chains[q] == a+1);
				if (q == b && num_unfixed > 0) {
					// reason
					
					return false;
				} else {
					// what happens?
				}
				if (chains[b] > 0) {
					int r = chains[b]-1;
					assert(chains[r] == -b-1);
					// q -> a -> b -> r
					chains[q] = r+1;
					chains[r] = -q-1;
					chains[a] = 0;
					chains[b] = 0;
					if (num_unfixed > 1) {
						// reason
						if (!x[r].remVal(q)) return false;
					}
				} else {
					// q -> a -> b
					chains[q] = b+1;
					chains[b] = -q-1;
					chains[a] = 0;
					if (num_unfixed > 1) {
						// reason
						if (!x[b].remVal(q)) return false;
					}
				}
			} else {
				if (chains[b] > 0) {
					int r = chains[b]-1;
					assert(chains[r] == -b-1);
					// a -> b -> r
					chains[a] = r+1;
					chains[r] = -a-1;
					chains[b] = 0;
					if (num_unfixed > 1) {
						// reason
						if (!x[r].remVal(a)) return false;
					}
				} else {
					// a -> b
					chains[a] = b+1;
					chains[b] = -a-1;
					if (num_unfixed > 1) {
						// reason
						if (!x[b].remVal(a)) return false;
					}
				}
			}
		}

//		if (!alldiff.match()) return false;

//		alldiff.dc();

//		if (!circuitSCC(sz, x)) return false;

		return true;
	}

	void clearPropState() {
		in_queue = false;
		new_fixed.clear();
	}

};

void circuit(vec<IntVar*>& _x) {
	vec<IntView<> > x;
	for (int i = 0; i < _x.size(); i++) _x[i]->specialiseToEL();
	for (int i = 0; i < _x.size(); i++) int_rel(_x[i], IRT_NE, i);
	// Assuming offset of 0!!
	for (int i = 0; i < _x.size(); i++) x.push(IntView<>(_x[i]));
	new Circuit(x);
}
