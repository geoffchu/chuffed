
#include <chuffed/core/options.h>
#include <chuffed/core/engine.h>
#include <chuffed/core/sat.h>

Options so;

Options::Options() :
		nof_solutions(1)
	, time_out(1800)
	, rnd_seed(0)
	, verbosity(0)
	, print_sol(true)
	, restart_base(1000000000)

	, toggle_vsids(false)
	, branch_random(false)
	, switch_to_vsids_after(1000000000)
	, sat_polarity(0)

	, prop_fifo(false)

	, disj_edge_find(true)
	, disj_set_bp(true)

	, cumu_global(true)

	, sat_simplify(true)
	, fd_simplify(true)

	, lazy(true)
	, finesse(true)
	, learn(true)
	, vsids(false)
        , rnd_init(false)
#if PHASE_SAVING
	, phase_saving(0)
#endif
	, sort_learnt_level(false)
	, one_watch(true)

	, eager_limit(1000)
	, sat_var_limit(2000000)
	, nof_learnts(100000)
	, learnts_mlimit(500000000)

	, lang_ext_linear(false)
    
	, mdd(false)
	, mip(false)
	, mip_branch(false)

	, sym_static(false)
	, ldsb(false)
	, ldsbta(false)
	, ldsbad(false)
      // Parallel options
        , parallel(false)
        , num_threads(-1)
        , thread_no(-1)
        , share_param(10)
        , bandwidth(3000000)
        , trial_size(50000)
        , share_act(0)
        , num_cores(1)
        , purePortfolio(false)
        , mixWS_Portfolio(false)
        , stealFromLastSuc(true)
        , maxClSz(2) 
        , shareBounds(true)
	, saved_clauses(0)
	, use_uiv(false)

	, alldiff_cheat(true)
	, alldiff_stage(true)
{}

char* hasPrefix(char* str, const char* prefix) {
	int len = strlen(prefix);
	if (strncmp(str, prefix, len) == 0) return str + len;
	else return NULL;
}

void parseOptions(int& argc, char**& argv) {
	int i, j;
	const char* value;

	#define parseIntArg(name)                                 \
	if ((value = hasPrefix(argv[i], "-" #name "="))) {        \
		so.name = atoi(value);                                  \
	} else 

	#define parseBoolArg(name)                                \
	if ((value = hasPrefix(argv[i], "-" #name "="))) {        \
		so.name = (strcmp(value, "true") == 0);                 \
	} else 

	for (i = j = 1; i < argc; i++) {

		// PLEASE KEEP THE HELP TEXT IN flatzinc/fzn_chuffed.c UPDATED
		parseIntArg(nof_solutions)
		parseIntArg(time_out)
		parseIntArg(rnd_seed)
		parseIntArg(verbosity)
		parseBoolArg(print_sol)
		parseIntArg(restart_base)

		parseBoolArg(toggle_vsids)
		parseBoolArg(branch_random)
		parseIntArg(switch_to_vsids_after)
		parseIntArg(sat_polarity)

		parseBoolArg(prop_fifo)

		parseBoolArg(disj_edge_find)
		parseBoolArg(disj_set_bp)
		
		parseBoolArg(cumu_global)

		parseBoolArg(sat_simplify)
		parseBoolArg(fd_simplify)

		parseBoolArg(lazy)
		parseBoolArg(finesse)
		parseBoolArg(learn)
		parseBoolArg(vsids)
                parseBoolArg(rnd_init)
		parseBoolArg(sort_learnt_level)
		parseBoolArg(one_watch)

		parseIntArg(eager_limit)
		parseIntArg(sat_var_limit)
		parseIntArg(nof_learnts)
		parseIntArg(learnts_mlimit)

		parseBoolArg(lang_ext_linear)

		parseBoolArg(mdd)
		parseBoolArg(mip)
		parseBoolArg(mip_branch)

		parseBoolArg(sym_static)
		parseBoolArg(ldsb)
		parseBoolArg(ldsbta)
		parseBoolArg(ldsbad)

		parseBoolArg(well_founded)

		parseBoolArg(parallel)
		parseIntArg(share_param)
		parseIntArg(bandwidth)
		parseIntArg(trial_size)
		parseIntArg(share_act)
                parseBoolArg(purePortfolio)
                parseBoolArg(mixWS_Portfolio)
                parseBoolArg(greedyInit)
                parseBoolArg(stealFromLastSuc)
                parseIntArg(maxClSz)
                parseBoolArg(shareBounds)
		parseIntArg(saved_clauses)
		parseBoolArg(use_uiv)

		parseBoolArg(alldiff_cheat)
		parseBoolArg(alldiff_stage)
		
		if (strcmp(argv[i], "-a") == 0) {
			so.nof_solutions = 0;
		} else if (strcmp(argv[i], "-f") == 0) {
			so.toggle_vsids = true;
			so.restart_base = 100;
		} else if (strcmp(argv[i], "-p") == 0) {
//			so.parallel = true;
			so.num_cores = atoi(argv[++i]);
		} else 

		if ((value = hasPrefix(argv[i], "-S"))) {
			so.verbosity = 1;
		} else if (argv[i][0] == '-') {
			ERROR("Unknown flag %s\n", argv[i]);
		} else argv[j++] = argv[i];
	}
	argc = j;

	rassert(so.sym_static + so.ldsb + so.ldsbta + so.ldsbad <= 1);

	if (so.ldsbta || so.ldsbad) so.ldsb = true;
	if (so.ldsb) rassert(so.lazy);
	if (so.mip_branch) rassert(so.mip);
	if (so.vsids) engine.branching->add(&sat);
        if (so.purePortfolio){
          so.toggle_vsids = true;
          so.restart_base = 100;
        }

#ifndef PARALLEL
	if (so.parallel) {
		fprintf(stderr, "Parallel solving not supported! Please recompile with PARALLEL=true.\n");
		rassert(false);
	}
#endif

}
