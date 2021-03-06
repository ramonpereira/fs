
#pragma once

#include <fs_types.hxx>
#include <constraints/direct/direct_rpg_builder.hxx>
#include <constraints/direct/action_manager.hxx>

namespace fs0 {

class GroundAction;
class Problem;
class State;
class RelaxedState;
class RPGData;

class DirectCRPG {
public:
	typedef GroundAction Action;

	DirectCRPG(const Problem& problem, std::vector<std::unique_ptr<DirectActionManager>>&& managers, std::shared_ptr<DirectRPGBuilder> builder);
	virtual ~DirectCRPG() = default;
	
	DirectCRPG(const DirectCRPG&) = delete;
	DirectCRPG(DirectCRPG&&) = default;
	DirectCRPG& operator=(const DirectCRPG& other) = delete;
	DirectCRPG& operator=(DirectCRPG&& other) = default;
	
	//! The actual evaluation of the heuristic value for any given non-relaxed state s.
	long evaluate(const State& seed);
	
	//! A version where only certain actions are allowed
	long evaluate(const State& seed, const std::vector<ActionIdx>& whitelist);
	
	//! The computation of the heuristic value. Returns -1 if the RPG layer encoded in the relaxed state is not a goal,
	//! otherwise returns h_{FF}.
	//! To be subclassed in other RPG-based heuristics such as h_max
	virtual long computeHeuristic(const State& seed, const RelaxedState& state, const RPGData& bookkeeping);
	
protected:
	//! The actual planning problem
	const Problem& _problem;
	
	//! The set of action managers, one per every action
	std::vector<std::unique_ptr<DirectActionManager>> _managers;
	
	//! A whitelist including all possible actions
	std::vector<unsigned> all_whitelist;
	
	//! The RPG building helper
	const std::shared_ptr<DirectRPGBuilder> _builder;
};

//! The h_max version
class DirectCHMax : public DirectCRPG {
public:
 	DirectCHMax(const Problem& problem, std::vector<std::unique_ptr<DirectActionManager>>&& managers, std::shared_ptr<DirectRPGBuilder> builder);
	~DirectCHMax() = default;
	
	//! The hmax heuristic only cares about the size of the RP graph.
	long computeHeuristic(const State& seed, const RelaxedState& state, const RPGData& bookkeeping) override;
};

} // namespaces
