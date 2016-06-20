#include <cstdio>
#include <algorithm>
#include <cassert>
#include <chuffed/core/options.h>
#include <chuffed/core/engine.h>
#include <chuffed/core/sat.h>
#include <chuffed/core/propagator.h>
#include <chuffed/parallel/parallel.h>
#include <chuffed/ldsb/ldsb.h>

#define PRINT_ANALYSIS 0


//---------
// inline methods

inline void SAT::learntLenBumpActivity(int l) {
	if (l >= MAX_SHARE_LEN) return;
	if (engine.conflicts % 16 == 0) {
		double new_ll_time = wallClockTime();
		double factor = exp((new_ll_time-ll_time)/learnt_len_el);
		if ((ll_inc *= factor) > 1e100) {
			for (int i = 0; i < MAX_SHARE_LEN; i++) learnt_len_occ[i] *= 1e-100;
			ll_inc *= 1e-100;
		}
		ll_time = new_ll_time;
		confl_rate /= factor;
	}
	learnt_len_occ[l] += ll_inc;
	confl_rate += 1 / learnt_len_el;
}

inline void SAT::varDecayActivity() {
	if ((var_inc *= 1.05) > 1e100) {
		for (int i = 0; i < nVars(); i++) activity[i] *= 1e-100;
		for (int i = 0; i < engine.vars.size(); i++) engine.vars[i]->activity *= 1e-100;
		var_inc *= 1e-100;
                maxActivity *= 1e-100;
	}
}

inline void SAT::varBumpActivity(Lit p) {
	int v = var(p);
	if (so.vsids) {
		activity[v] += var_inc;
                if(activity[v] > maxActivity)
                  maxActivity = activity[v];
		if (order_heap.inHeap(v)) order_heap.decrease(v);
		if (so.sat_polarity == 1) polarity[v] = sign(p)^1;
		if (so.sat_polarity == 2) polarity[v] = sign(p);
	}
	if (c_info[v].cons_type == 1) {
		int var_id = c_info[v].cons_id;
		if (!ivseen[var_id]) {
			engine.vars[var_id]->activity += var_inc;
                        if(engine.vars[var_id]->activity > maxActivity)
                          maxActivity = activity[v];
			ivseen[var_id] = true;
			ivseen_toclear.push(var_id);
		}
	}
}

inline void SAT::claDecayActivity() {
	if ((cla_inc *= 1.001) > 1e20) {
		for (int i = 0; i < learnts.size(); i++) learnts[i]->activity() *= 1e-20;
                for (int i = 0; i < receiveds.size(); i++) receiveds[i]->activity() *= 1e-20;
		cla_inc *= 1e-20;
	}
}

//---------
// main methods

Clause* SAT::_getExpl(Lit p) {
//	fprintf(stderr, "L%d - %d\n", decisionLevel(), trailpos[var(p)]);
	Reason& r = reason[var(p)];
	return engine.propagators[r.d.d2]->explain(p, r.d.d1);
}

Clause* SAT::getConfl(Reason& r, Lit p) {
	switch(r.d.type) {
		case 0:
			return r.pt;
		case 1:
			return engine.propagators[r.d.d2]->explain(p, r.d.d1);
		default:
			Clause& c = *short_expl;
			c.sz = r.d.type; c[1] = toLit(r.d.d1); c[2] = toLit(r.d.d2);
			return short_expl;
	}
}

void SAT::analyze() {
	avg_depth += 0.01*(decisionLevel()-avg_depth);

	checkConflict();
	varDecayActivity();
	claDecayActivity();
	getLearntClause();
	explainUnlearnable();
	clearSeen();

	int btlevel = findBackTrackLevel();
	back_jumps += decisionLevel()-1-btlevel;
//	fprintf(stderr, "btlevel = %d\n", btlevel);
	btToLevel(btlevel);
	confl = NULL;

	if (so.sort_learnt_level && out_learnt.size() >= 4) {
		std::sort((Lit*) out_learnt + 2, (Lit*) out_learnt + out_learnt.size(), lit_sort);
	}

	Clause *c = Clause_new(out_learnt, true);
	c->activity() = cla_inc;

	learntLenBumpActivity(c->size());

#ifdef PARALLEL
        if (so.parallel){
          if( so.learn &&
                  c->size() <= so.maxClSz)                
          {
                slave.shareClause(*c);
                if(c->size() == 1)
                  slave.unitFound = true;
          }
        }
#endif

	if (so.learn && c->size() >= 2) addClause(*c, so.one_watch);
	if (!so.learn || c->size() <= 2) rtrail.last().push(c);

	enqueue(out_learnt[0], c->size() == 2 ? Reason(out_learnt[1]) : c);

	if (PRINT_ANALYSIS) printClause(*c);

	if (so.ldsbad) {
		assert(!so.parallel);
		vec<Lit> out_learnt2;
		out_learnt2.push(out_learnt[0]);
		for (int i = 0; i < decisionLevel(); i++) out_learnt2.push(~decLit(decisionLevel()-i));
		c = Clause_new(out_learnt2, true);
		rtrail.last().push(c);
	}

	if (so.ldsb && !ldsb.processImpl(c)) engine.async_fail = true;

	if (learnts.size() >= so.nof_learnts ||
		learnts_literals >= so.learnts_mlimit/4) reduceDB();

}


void SAT::getLearntClause() {
	Lit p = lit_Undef;
	int pathC = 0;
	int clevel = findConflictLevel();
	vec<Lit>& ctrail = trail[clevel];
	Clause* expl = confl;
	Reason last_reason = NULL;

	index = ctrail.size();
	out_learnt.clear();
	out_learnt.push();      // (leave room for the asserting literal)

	while (true) {

		assert(expl != NULL);          // (otherwise should be UIP)
		Clause& c = *expl;

		if (PRINT_ANALYSIS) {
			if (p != lit_Undef) c[0] = p;
			printClause(c);
		}

		if (c.learnt) c.activity() += cla_inc;

		for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++) {
			Lit q = c[j];
			int x = var(q);
			if (!seen[x]) {
				varBumpActivity(q);
				seen[x] = 1;
				if (isCurLevel(x)) pathC++;
				else out_learnt.push(q);
			}
		}


FindNextExpl:

		assert(pathC > 0);

		// Select next clause to look at:
		while (!seen[var(ctrail[--index])]);
		assert(index >= 0);
		p = ctrail[index];
		seen[var(p)] = 0;
		pathC--;

		if (pathC == 0 && flags[var(p)].uipable) break;

		if (last_reason == reason[var(p)]) goto FindNextExpl;
		last_reason = reason[var(p)];
		expl = getExpl(p);

	}

	out_learnt[0] = ~p;

}

int SAT::findConflictLevel() {
	int tp = -1;
	for (int i = 0; i < confl->size(); i++) {
		int l = trailpos[var((*confl)[i])];
		if (l > tp) tp = l;
	}
	int clevel = engine.tpToLevel(tp);

	if (so.sym_static && clevel == 0) {
		btToLevel(0);
		engine.async_fail = true;
		NOT_SUPPORTED;
		// need to abort analyze as well
		return 0;
	}
	assert(clevel > 0);

	// duplicate conflict clause in case it gets freed
	if (clevel < decisionLevel() && confl->temp_expl) {
		confl = Clause_new(*confl);
		confl->temp_expl = 1;
		rtrail[clevel].push(confl);
	}
	btToLevel(clevel);

//	fprintf(stderr, "clevel = %d\n", clevel);

	if (PRINT_ANALYSIS) printf("Conflict: level = %d\n", clevel);

	return clevel;
}

void SAT::explainUnlearnable() {
	pushback_time -= wallClockTime();

	vec<Lit> removed;
	for (int i = 1; i < out_learnt.size(); i++) {
		Lit p = out_learnt[i];
		if (flags[var(p)].learnable) continue;
		assert(!reason[var(p)].isLazy());
		Clause& c = *getExpl(~p);
		removed.push(p);
		out_learnt[i] = out_learnt.last(); out_learnt.pop(); i--;
		for (int j = 1; j < c.size(); j++) {
			Lit q = c[j];
			if (!seen[var(q)]) {
				seen[var(q)] = 1;
				out_learnt.push(q);
			}
		}
	}

	for (int i = 0; i < removed.size(); i++) seen[var(removed[i])] = 0;

	pushback_time += wallClockTime();
}

void SAT::clearSeen() {
	for (int i = 0; i < ivseen_toclear.size(); i++) ivseen[ivseen_toclear[i]] = false;
	ivseen_toclear.clear();

	for (int i = 0; i < out_learnt.size(); i++) seen[var(out_learnt[i])] = 0;    // ('seen[]' is now cleared)
}

int SAT::findBackTrackLevel() {
	if (out_learnt.size() < 2) {
		nrestarts++;
		return 0;
	}

	int max_i = 1;
	for (int i = 2; i < out_learnt.size(); i++) {
		if (trailpos[var(out_learnt[i])] > trailpos[var(out_learnt[max_i])]) max_i = i;
	}
	Lit p = out_learnt[max_i];
	out_learnt[max_i] = out_learnt[1];
	out_learnt[1] = p;

	return engine.tpToLevel(trailpos[var(p)]);
}


//---------
// debug

// Lit:meaning:value:level

void SAT::printLit(Lit p) {
	if (isRootLevel(var(p))) return;
	printf("%d:", toInt(p));
	ChannelInfo& ci = c_info[var(p)];
	if (ci.cons_type == 1) {
		engine.vars[ci.cons_id]->printLit(ci.val, ci.val_type * 3 ^ sign(p));
	} else if (ci.cons_type == 2) {
		engine.propagators[ci.cons_id]->printLit(ci.val, sign(p));
	} else {
		printf(":%d:%d, ", sign(p), trailpos[var(p)]);
	}
}

template <class T>
void SAT::printClause(T& c) {
	printf("Size:%d - ", c.size());
	printLit(c[0]);
	printf(" <- ");
	for (int i = 1; i < c.size(); i++) printLit(~c[i]);
	printf("\n");
}

void SAT::checkConflict() {
	assert(confl != NULL);
	for (int i = 0; i < confl->size(); i++) {
		if (value((*confl)[i]) != l_False) printf("Analyze: %dth lit is not false\n", i);
		assert(value((*confl)[i]) == l_False);
	}

/*
	printf("Clause %d\n", learnts.size());
	printf("Conflict clause: ");
	printClause(*confl);
*/

}

void SAT::checkExplanation(Clause& c, int clevel, int index) {
	NOT_SUPPORTED;
	for (int i = 1; i < c.size(); i++) {
		assert(value(c[i]) == l_False);
		assert(trailpos[var(c[i])] < engine.trail_lim[clevel]);
		vec<Lit>& ctrail = trail[trailpos[var(c[i])]];
		int pos = -1;
		for (int j = 0; j < ctrail.size(); j++) {
			if (var(ctrail[j]) == var(c[i])) {
				pos = j;
				if (trailpos[var(c[i])] == clevel) assert(j <= index);
				break;
			}
		}
		assert(pos != -1);
	}

}

