	
#include <languages/fstrips/language.hxx>
#include <problem.hxx>
#include <actions/actions.hxx>
#include <actions/grounding.hxx>
#include <constraints/gecode/handlers/lifted_effect_csp.hxx>
#include <constraints/gecode/utils/novelty_constraints.hxx>
#include <constraints/gecode/supports.hxx>
#include <utils/printers/actions.hxx>
#include <aptk2/tools/logging.hxx>
#include <heuristics/relaxed_plan/rpg_index.hxx>
#include <gecode/search.hh>

namespace fs0 { namespace gecode {


std::vector<std::unique_ptr<LiftedEffectCSP>> LiftedEffectCSP::create_smart(const std::vector<const PartiallyGroundedAction*>& schemata, const TupleIndex& tuple_index, bool approximate, bool novelty) {
	const ProblemInfo& info = ProblemInfo::getInstance();
	std::vector<std::unique_ptr<LiftedEffectCSP>> handlers;
	
	for (const PartiallyGroundedAction* schema:schemata) {
		LPT_DEBUG("main", "Smart grounding of action " << *schema << "...");
		for (unsigned eff_idx = 0; eff_idx < schema->getEffects().size(); ++eff_idx) {
			for (const PartiallyGroundedAction* smart_action:ActionGrounder::compile_action_parameters_away(schema, eff_idx, info)) {
				const fs::ActionEffect* effect = smart_action->getEffects().at(eff_idx);
				
				LPT_DEBUG("main", "\tPartially grounded action: " << *smart_action);
				
				if (effect->is_del()) { // Ignore delete effects
					LPT_DEBUG("main", "\tIgnoring delete effect \"" << *effect);
					delete smart_action;
					continue;
				}
				
				for (const fs::ActionEffect* flat_effect:ActionGrounder::compile_nested_fluents_away(effect, info)) {
					
					auto handler = std::unique_ptr<LiftedEffectCSP>(new LiftedEffectCSP(*smart_action, flat_effect, tuple_index, approximate));
					if (handler->init(novelty)) {
						LPT_DEBUG("main", "\tSmart grounding of effect \"" << *schema->getEffects().at(eff_idx) << " results in (possibly partially) grounded action " << *smart_action);
						handlers.push_back(std::move(handler));
					} else {
						LPT_DEBUG("main", "\tSmart grounding of effect \"" << *schema->getEffects().at(eff_idx) << " results in non-applicable CSP");
					}
				}
				
				delete smart_action;
			}
		}
	}
	return handlers;
}


LiftedEffectCSP::LiftedEffectCSP(const PartiallyGroundedAction& action, const fs::ActionEffect* effect, const TupleIndex& tuple_index, bool approximate) :
	LiftedActionCSP(action, tuple_index, approximate, true), _effects({effect}),  _achievable_tuple_idx(INVALID_TUPLE)
{}

LiftedEffectCSP::~LiftedEffectCSP() {
	delete _effects[0];
}

bool LiftedEffectCSP::init(bool use_novelty_constraint) {
	if (!LiftedActionCSP::init(use_novelty_constraint)) return false;
	
	_lhs_symbol = index_lhs_symbol(get_effect());
	_rhs_variable = _translator.resolveVariableIndex(get_effect()->rhs());
	_effect_tuple = index_tuple_indexes(get_effect());
	_achievable_tuple_idx = detect_achievable_tuple();

	// Register all fluent symbols involved
	_tuple_indexes = _translator.index_fluents(_all_terms);
	
	return true;
}

TupleIdx LiftedEffectCSP::detect_achievable_tuple() const {
	const ProblemInfo& info = ProblemInfo::getInstance();
	
	TupleIdx achievable_tuple_idx = INVALID_TUPLE;
	
	// We necessarily assume that the head of the effect is fluent-less
	if (info.isPredicate(_lhs_symbol)) { // If the effect is predicative, it must be an add-effect
		achievable_tuple_idx = _tuple_index.to_index(_lhs_symbol, _effect_tuple);
	} else {
		auto constant = dynamic_cast<const fs::Constant*>(get_effect()->rhs());
		if (constant) {
			ValueTuple tuple(_effect_tuple);
			tuple.push_back(constant->getValue());
			achievable_tuple_idx = _tuple_index.to_index(_lhs_symbol, tuple);
		}
	}
	
	return achievable_tuple_idx;
}

ValueTuple LiftedEffectCSP::index_tuple_indexes(const fs::ActionEffect* effect) {
	auto lhs_statevar = check_valid_effect(effect);
	
	ValueTuple variables;
	for (auto subterm:lhs_statevar->getSubterms()) {
		auto constant = dynamic_cast<const fs::Constant*>(subterm);
		assert(constant);  // Otherwise this could not be a state variable
		variables.push_back(constant->getValue());
	}
	return variables;
}

void LiftedEffectCSP::log() const {
	assert(_effects.size() == 1);
	LPT_EDEBUG("heuristic", "Processing effect schema \"" << *get_effect() << " of action " << _action);
}

const fs::ActionEffect* LiftedEffectCSP::get_effect() const { 
	assert(_effects.size() == 1);
	return _effects[0];
}

unsigned LiftedEffectCSP::index_lhs_symbol(const fs::ActionEffect* effect) {
	return check_valid_effect(effect)->getSymbolId();
}

const fs::StateVariable* LiftedEffectCSP::check_valid_effect(const fs::ActionEffect* effect) {
	auto lhs_statevar = dynamic_cast<const fs::StateVariable*>(effect->lhs());
	if (!lhs_statevar) throw std::runtime_error("LiftedEffectCSP accepts only effects with state-variable (fluent-less) heads");
	return lhs_statevar;
}


void LiftedEffectCSP::seek_novel_tuples(RPGIndex& rpg) const {
	if (GecodeCSP* csp = instantiate(rpg)) {
		if (!csp->checkConsistency()) {
			LPT_EDEBUG("heuristic", "The effect CSP cannot produce any new tuple");
		}
		else {
			Gecode::DFS<GecodeCSP> engine(csp);
			unsigned num_solutions = 0;
			while (GecodeCSP* solution = engine.next()) {
		// 		LPT_EDEBUG("heuristic", std::endl << "Processing action CSP solution #"<< num_solutions + 1 << ": " << print::csp(_translator, *solution))
				process_effect_solution(solution, rpg);
				++num_solutions;
				delete solution;
			}
			LPT_EDEBUG("heuristic", "The Effect CSP produced " << num_solutions << " novel tuples");
		}
		delete csp;
	}
}

TupleIdx LiftedEffectCSP::compute_reached_tuple(const GecodeCSP* solution) const {
	TupleIdx tuple_idx = _achievable_tuple_idx;
	if (tuple_idx == INVALID_TUPLE) { // i.e. we have a functional effect, and thus need to factor the function result into the tuple.
		ValueTuple tuple(_effect_tuple); // Copy the tuple
		tuple.push_back(_translator.resolveValueFromIndex(_rhs_variable, *solution));
		tuple_idx = _tuple_index.to_index(_lhs_symbol, tuple);
	}
	return tuple_idx;
}

void LiftedEffectCSP::process_effect_solution(const GecodeCSP* solution, RPGIndex& rpg) const {
	TupleIdx tuple_idx = compute_reached_tuple(solution);
	
	bool reached = rpg.reached(tuple_idx);
	LPT_EDEBUG("heuristic", "Processing effect \"" << *get_effect() << "\" produces " << (reached ? "repeated" : "new") << " tuple " << tuple_idx);
	
	if (reached) return; // The value has already been reached before
	
	// Otherwise, the value is actually new - we extract the actual support from the solution
	std::vector<TupleIdx> support = Supports::extract_support(solution, _translator, _tuple_indexes, _necessary_tuples);
	rpg.add(tuple_idx, get_action_id(solution), std::move(support));
}


void LiftedEffectCSP::create_novelty_constraint() {
	_novelty = new EffectNoveltyConstraint(_translator, get_effect());
}

void LiftedEffectCSP::post_novelty_constraint(GecodeCSP& csp, const RPGIndex& rpg) const {
	if (_novelty) _novelty->post_constraint(csp, rpg);
}


const std::vector<const fs::ActionEffect*>& LiftedEffectCSP::get_effects() const {
	return _effects;
}

const fs::Formula* LiftedEffectCSP::get_precondition() const {
	return _action.getPrecondition();
}

} } // namespaces
