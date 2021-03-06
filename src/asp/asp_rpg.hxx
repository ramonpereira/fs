
#pragma once

#include <memory>
#include <asp/model_handler.hxx>

namespace fs0 { class State; class Problem; }

namespace fs0 { namespace asp {

template <typename RPGBaseHeuristic>
class ASPRPG {
protected:
	//! The LP handler
	ModelHandler _handler;
	
	//! The RPG-based heuristic that we will use to (re-)construct the RPG based on the allowed
	//! actions given by our LP solution
	RPGBaseHeuristic _heuristic;
	
public:
	ASPRPG(const Problem& problem, RPGBaseHeuristic&& heuristic);

	//! The actual evaluation of the heuristic value for any given non-relaxed state s.
	long evaluate(const State& seed);
};

} } // namespaces
