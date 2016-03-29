
#pragma once

#include <fs0_types.hxx>
#include <constraints/gecode/extensions.hxx>
#include <constraints/gecode/utils/indexed_tupleset.hxx>
#include <atom.hxx>
#include <utils/index.hxx>
#include <unordered_set>

namespace fs0 { class Problem; class State; class RPGData; }

namespace fs0 { namespace gecode {

class EffectSchemaCSPHandler;
class GecodeRPGBuilder;
class GecodeRPGLayer;

class LiftedCRPG {
protected:
// 	typedef std::shared_ptr<BaseActionCSPHandler> ActionHandlerPtr;
	typedef std::shared_ptr<EffectSchemaCSPHandler> EffectHandlerPtr;
	
public:
	LiftedCRPG(const Problem& problem, std::vector<EffectHandlerPtr>&& managers, std::vector<IndexedTupleset>&& symbol_tuplesets, std::shared_ptr<GecodeRPGBuilder> builder);

	
	virtual ~LiftedCRPG() {}
	
	//! The actual evaluation of the heuristic value for any given non-relaxed state s.
	long evaluate(const State& seed);
	
	//! The computation of the heuristic value. Returns -1 if the RPG layer encoded in the relaxed state is not a goal,
	//! otherwise returns h_{FF}.
	//! To be subclassed in other RPG-based heuristics such as h_max
	virtual long computeHeuristic(const State& seed, const GecodeRPGLayer& layer, const RPGData& rpg);
	
	
	//!
	static std::vector<IndexedTupleset> index_tuplesets(const ProblemInfo& info);

protected:
	typedef std::vector<std::vector<unsigned>> AchieverIndex;
	
	
	//! The actual planning problem
	const Problem& _problem;
	const ProblemInfo& _info;
	
	//! The set of action managers, one per every action
	const std::vector<EffectHandlerPtr> _managers;
	
	//! The RPG building helper
	const std::shared_ptr<GecodeRPGBuilder> _builder;
	
	//!
	ExtensionHandler _extension_handler;
	
	//! For each logical symbol with index 'i', '_symbol_tuple_indexes[i]' contains a gecode extension (table) with all tuples
	//! <t1, ..., tn, j>, where j is the unique index (unique across all tuples of that symbol) of the tuple <t1, ..., tn>
	//! that might belong to symbol i's underlying relation.
	// TODO - IS THIS NEEDED?	
	std::vector<IndexedTupleset> _symbol_tuplesets;
	
	//! An index of all the problem atoms.
// 	Index<Atom> _atom_table;
	
	//! a map from atom index to the set of action / effect managers that can (potentially) achieve that atom.
	//! let L = _atom_achievers[i] be the vector of all potential achievers of atom with index 'i'.
	//! Then each element j in L is the index of an effect manager in '_managers'.
// 	const AchieverIndex _atom_achievers;
	
	//! If atom with index 'i' has state variable f(t_1, ..., t_n), then '_atom_variable_tuples[i]' is the vector {t_1, ..., t_n}
// 	const std::vector<std::vector<ObjectIdx>> _atom_variable_tuples;
	

	
// 	static std::vector<std::vector<ObjectIdx>> index_variable_tuples(const ProblemInfo& info, const Index<Atom>& index);
	
	//! A helper to index all of the problem's atoms.
// 	static Index<Atom> index_atoms(const ProblemInfo& info);
	
	//! A helper to build the index of atom achievers.
// 	static AchieverIndex build_achievers_index(const std::vector<EffectHandlerPtr>& managers, const Index<Atom>& atom_idx);
	
	//! Check that all the given managers are indeed effect managers, and downcast them
// 	static std::vector<EffectHandlerPtr> prepare_managers(const std::vector<ActionHandlerPtr>& managers);
	
	
	//! Remove from the set of given tuplesets all those tuples implicitly contained in the given seed state
// 	static void prune_tuplesets(const State& seed, std::vector<Tupleset>& tuplesets);
	
	
	
	//!
	static std::vector<IndexedTupleset::TupleVector> compute_all_reachable_tuples(const ProblemInfo& info);
	
	//!
	std::vector<std::unordered_set<unsigned>> compute_reached_tuples(const State& seed) const;
	
	void reach_atom(VariableIdx variable, ObjectIdx value, std::vector<std::unordered_set<unsigned>>& reached) const;

};

} } // namespaces
