#include <cstdio>
#include <cassert>
#include <chuffed/core/options.h>
#include <chuffed/core/engine.h>
#include <chuffed/core/sat.h>
#include <chuffed/core/propagator.h>
#include <chuffed/branching/branching.h>
#include <chuffed/mip/mip.h>
#include <chuffed/parallel/parallel.h>
#include <chuffed/ldsb/ldsb.h>

Engine engine;

uint64_t bit[65];

Tint trail_inc;

Engine::Engine() :
		finished_init(false)
	, problem(NULL)
	, opt_var(NULL)
	, best_sol(-1)
	, last_prop(NULL)
        , stop_init_phase(false)
	, start_time(wallClockTime())
	, opt_time(0)
	, conflicts(0)
	, nodes(1)
	, propagations(0)
	, solutions(0)
	, next_simp_db(0)
{
	p_queue.growTo(num_queues);
	for (int i = 0; i < 64; i++) bit[i] = ((long long) 1 << i);
	branching = new BranchGroup();
	mip = new MIP();
}

inline void Engine::newDecisionLevel() {
	trail_inc++;
	trail_lim.push(trail.size());
	sat.newDecisionLevel();
	if (so.mip) mip->newDecisionLevel();
	assert(dec_info.size() == decisionLevel());
}

inline void Engine::doFixPointStuff() {
	// ask other objects to do fix point things
	for (int i = 0; i < pseudo_props.size(); i++) {
		pseudo_props[i]->doFixPointStuff();
	}
}

inline void Engine::makeDecision(DecInfo& di, int alt) {
	++nodes;
	if (di.var) ((IntVar*) di.var)->set(di.val, di.type ^ alt);
	else sat.enqueue(toLit(di.val ^ alt));
	if (so.ldsb && di.var && di.type == 1) ldsb.processDec(sat.trail.last()[0]);
//	if (opt_var && di.var == opt_var && ((IntVar*) di.var)->isFixed()) printf("objective = %d\n", ((IntVar*) di.var)->getVal());
}

void optimize(IntVar* v, int t) {
	engine.opt_var = v;
	engine.opt_type = t;
	engine.branching->add(v);
	v->setPreferredVal(t == OPT_MIN ? PV_MIN : PV_MAX);

}

inline bool Engine::constrain() {
	best_sol = opt_var->getVal();
	opt_time = wallClockTime() - start_time - init_time;

	sat.btToLevel(0);

	if (so.parallel && so.shareBounds) {
		Lit p = opt_type ? opt_var->getLit(best_sol+1, 2) : opt_var->getLit(best_sol-1, 3);
		vec<Lit> ps;
		ps.push(p);
		Clause *c = Clause_new(ps, true);
		slave.shareClause(*c);
		free(c);
	}

//	printf("opt_var = %d, opt_type = %d, best_sol = %d\n", opt_var->var_id, opt_type, best_sol);
//	printf("opt_var min = %d, opt_var max = %d\n", opt_var->min, opt_var->max);

	if (so.mip) mip->setObjective(best_sol);
        bool rVal = (opt_type ? opt_var->setMin(best_sol+1) : opt_var->setMax(best_sol-1));
        PAR_EXEC(slave.sendObjective(best_sol);)
        
        return rVal;
}

bool Engine::propagate() {

	if (async_fail) {
		async_fail = false;
		assert(!so.lazy || sat.confl);
		return false;
	}

	last_prop = NULL;

	WakeUp:

	if (!sat.consistent() && !sat.propagate()) return false;

	for (int i = 0; i < v_queue.size(); i++) {
		v_queue[i]->wakePropagators();
	}
	v_queue.clear();

	if (sat.confl) return false;

	last_prop = NULL;

	for (int i = 0; i < num_queues; i++) {
		if (p_queue[i].size()) {
			Propagator *p = p_queue[i].last(); p_queue[i].pop();
			propagations++;
			bool ok = p->propagate();
			p->clearPropState();
			if (!ok) return false;
			goto WakeUp;
		}
	}

	return true;
}

// Clear all uncleared intermediate propagation states
void Engine::clearPropState() {
	for (int i = 0; i < v_queue.size(); i++) v_queue[i]->clearPropState();
	v_queue.clear();

	for (int i = 0; i < num_queues; i++) {
		for (int j = 0; j < p_queue[i].size(); j++) p_queue[i][j]->clearPropState();
		p_queue[i].clear();
	}
}

void Engine::btToPos(int pos) {
	for (int i = trail.size(); i-- > pos; ) {
		trail[i].undo();
	}
  trail.resize(pos);
}

void Engine::btToLevel(int level) {
	if (decisionLevel() == 0 && level == 0) return;
	assert(decisionLevel() > level);

	btToPos(trail_lim[level]);
  trail_lim.resize(level);
	dec_info.resize(level);
}



void Engine::topLevelCleanUp() {
	trail.clear();

	if (so.fd_simplify && propagations >= next_simp_db) simplifyDB();

	sat.topLevelCleanUp();
}

void Engine::simplifyDB() {
	int cost = 0;
	for (int i = 0; i < propagators.size(); i++) {
		cost += propagators[i]->checkSatisfied();
	}
	cost += propagators.size();
	for (int i = 0; i < vars.size(); i++) {
		cost += vars[i]->simplifyWatches();
	}
	cost += vars.size();
	cost *= 10;
//	printf("simp db cost: %d\n", cost);
	next_simp_db = propagations + cost;
}

void Engine::blockCurrentSol() {
	if (outputs.size() == 0) NOT_SUPPORTED;
	Clause& c = *Reason_new(outputs.size());
	bool root_failure = true;
	for (int i = 0; i < outputs.size(); i++) {
		Var *v = (Var*) outputs[i];
		if (v->getType() == BOOL_VAR) {
			c[i] = ((BoolView*) outputs[i])->getValLit();
		} else {
			c[i] = ((IntVar*) outputs[i])->getValLit();
		}
		if (!sat.isRootLevel(var(c[i]))) root_failure = false;
		assert(sat.value(c[i]) == l_False);
	}
	if (root_failure) sat.btToLevel(0);
	sat.confl = &c;
}


int Engine::getRestartLimit(int starts) {
//	return so.restart_base * ((int) pow(1.5, starts));
//	return so.restart_base;
	return (((starts-1) & ~starts) + 1) * so.restart_base;
}

void Engine::toggleVSIDS() {
	if (!so.vsids) {
		vec<Branching*> old_x;
		branching->x.moveTo(old_x);
		branching->add(&sat);
		for (int i = 0; i < old_x.size(); i++) branching->add(old_x[i]);
		branching->fin = 0;
		branching->cur = -1;
		so.vsids = true;
	} else {
		vec<Branching*> old_x;
		branching->x.moveTo(old_x);
		for (int i = 1; i < old_x.size(); i++) branching->add(old_x[i]);
		branching->fin = 0;
		branching->cur = -1;
		so.vsids = false;
	}
}

RESULT Engine::search() {
	int starts = 0;
	int nof_conflicts = so.restart_base;
	int conflictC = 0;
        int conflicts_last_receive = 0; // When did I check messages for the last time?
        int minConflictsBetweenChecks = 100;
        bool restartSinceLastCheck = false;

	while (true) {
		if (so.parallel && slave.checkMessages()) return RES_UNK;
                
                if(so.parallel && stop_init_phase){
                          sat.btToLevel(0);
                          assumptions.clear();
                          clearPropState();
                          sat.confl = NULL;
                          // Reset flag
                          stop_init_phase = false;
                          
                          return RES_GREEDY_DONE;
                        }

		if (!propagate()) {

			clearPropState();

			Conflict:
			conflicts++; conflictC++;

			if (time(NULL) > so.time_out) {
				printf("Time limit exceeded!\n");
				return RES_UNK;
			}

			if (decisionLevel() == 0) { return RES_GUN; }

			// Derive learnt clause and perform backjump
			if (so.lazy) {
				sat.analyze();
			}	else {
				sat.confl = NULL;
				DecInfo& di = dec_info.last();
				sat.btToLevel(decisionLevel()-1);
				makeDecision(di, 1);
			}

            if (!so.vsids && !so.toggle_vsids &&  conflictC >= so.switch_to_vsids_after) {
	    	if (so.restart_base >= 1000000000) so.restart_base = 100;
                sat.btToLevel(0);
                toggleVSIDS();
                restartSinceLastCheck=true;
            }

		} else {

			if (conflictC >= nof_conflicts) {
				starts++;
				nof_conflicts += getRestartLimit((starts+1)/2);
				sat.btToLevel(0);
				sat.confl = NULL;
                                restartSinceLastCheck=true;
				if (so.lazy && so.toggle_vsids && (starts % 2 == 0)) toggleVSIDS();
				continue;
			}
                        else if( (so.parallel && !so.toggle_vsids && slave.newClausesReceived())){
                          if(conflictC > conflicts_last_receive + minConflictsBetweenChecks){
                            sat.btToLevel(0);
                            restartSinceLastCheck=true;
                            sat.confl = NULL;
                            continue;
                          }
                        }
			
			if (decisionLevel() == 0) topLevelCleanUp();

			DecInfo *di = NULL;
			if(so.parallel && decisionLevel() == 0 && slave.state()==RUNNING_GREEDY){
                          assert(assumptions.size() == 1);
                          if(sat.value(toLit(assumptions[0])) == l_True){
                            return RES_GREEDY_DONE;
                          }
                        }
			// Propagate assumptions
			while (decisionLevel() < assumptions.size()) {
				int p = assumptions[decisionLevel()];
				if (sat.value(toLit(p)) == l_True) {
					// Dummy decision level:
					assert(sat.trail.last().size() == sat.qhead.last());
					engine.dec_info.push(DecInfo(NULL, p));
					newDecisionLevel();
				} else if (sat.value(toLit(p)) == l_False) {
                                       if(!so.parallel)
                                         return RES_LUN;

                                        if(slave.state() == NORMAL_SEARCH){
                                          return RES_LUN;
                                        }
                                        else{
                                          return RES_GREEDY_DONE;
                                        }
				} else {
					di = new DecInfo(NULL, p);
					break;
				}
			}

			if (!di) di = branching->branch();

			if (!di) {
				solutions++;
				if (so.print_sol) {
					problem->print();
					printf("----------\n");
          fflush(stdout);
				}
				if (!opt_var) {
					if (solutions == so.nof_solutions) return RES_SAT;
					if (so.lazy) blockCurrentSol();
					goto Conflict;
				}
				if (!constrain()) {
					return RES_GUN;
				}
				continue;
			}


			engine.dec_info.push(*di);
      newDecisionLevel();

			doFixPointStuff();

			makeDecision(*di, 0);
			delete di;

		}
	}
}

void Engine::solve(Problem *p) {
	problem = p;

	init();
        half_time = time(NULL)+so.time_out/2;

	so.time_out += time(NULL);

	init_time = wallClockTime() - start_time;
	base_memory = memUsed();

	if (!so.parallel) {
		// sequential
		status = search();
		if (status == RES_GUN) {
			if (solutions > 0)
				printf("==========\n");
			else
				printf("=====UNSATISFIABLE=====\n");
		}
	} else {
		// parallel
		if (so.thread_no == -1) master.solve();
		else slave.solve();
		if (so.thread_no == -1 && master.status == RES_GUN) printf("==========\n");
	}

	if (so.verbosity >= 1) printStats();

  if (so.parallel) master.finalizeMPI();
}
