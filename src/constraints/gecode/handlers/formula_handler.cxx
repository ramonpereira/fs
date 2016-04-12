
#include <languages/fstrips/language.hxx>
#include <languages/fstrips/scopes.hxx>
#include <constraints/gecode/handlers/formula_handler.hxx>
#include <constraints/gecode/helper.hxx>
#include <heuristics/relaxed_plan/rpg_data.hxx>
#include <utils/logging.hxx>
#include <utils/binding.hxx>
#include <constraints/gecode/utils/novelty_constraints.hxx>
#include <state.hxx>

#include <gecode/driver.hh>

namespace fs0 { namespace gecode {
	
FormulaCSPHandler::FormulaCSPHandler(const fs::Formula* formula, const TupleIndex& tuple_index, bool approximate, bool use_novelty_constraint)
	:  BaseCSPHandler(tuple_index, approximate),
	  _formula(formula)
{
	setup();
	
	createCSPVariables(use_novelty_constraint);
	register_csp_constraints();
	
	Helper::postBranchingStrategy(_base_csp);
	
	// MRJ: in order to be able to clone a CSP, we need to ensure that it is "stable" i.e. propagate all constraints until fixed point
	Gecode::SpaceStatus st = _base_csp.status();
	
	if (st == Gecode::SpaceStatus::SS_SOLVED) {
		FINFO("main", "Formula CSP was statically solved:" << std::endl <<  *this);
	} else if (st == Gecode::SpaceStatus::SS_FAILED) {
		FINFO("main", "Formula CSP statically failed:" << *this);
		_failed = true;
	} else {
		FINFO("main", "Formula CSP after the initial, static propagation: " << *this);
	}
	
	index_scopes(); // This needs to be _after_ the CSP variable registration
}

FormulaCSPHandler::~FormulaCSPHandler() { delete _formula; }

bool FormulaCSPHandler::compute_support(SimpleCSP* csp, Atom::vctr& support, const State& seed) const {
	
	Gecode::DFS<SimpleCSP> engine(csp);
	SimpleCSP* solution = engine.next();
	if (!solution) return false;
	
	FFDEBUG("heuristic", "Formula CSP solution found: " << *solution);
	
	// First process the direct state variables
	for (const auto& element:_translator.getAllInputVariables()) {
		VariableIdx variable = element.first;
		ObjectIdx value = _translator.resolveVariableFromIndex(element.second, *solution).val();
		support.push_back(Atom(variable, value));
	}
	
	// Now the support of atoms such as 'clear(b)' that might appear in formulas in non-negated form.
	support.insert(support.end(), _atom_state_variables.begin(), _atom_state_variables.end());
	
	// And finally the support derived from nested terms
	extract_nested_term_support(solution, _nested_fluents, _translator.buildAssignment(*solution), Binding(), support);
	
	delete solution;
	return true;
}


void FormulaCSPHandler::index_scopes() {
	// We index all the nested fluents appearing in the formula
	fs::ScopeUtils::TermSet nested;
	fs::ScopeUtils::computeIndirectScope(_formula, nested);
	_nested_fluents = std::vector<fs::FluentHeadedNestedTerm::cptr>(nested.cbegin(), nested.cend());
}

bool FormulaCSPHandler::check_solution_exists(SimpleCSP* csp) const {
	Gecode::DFS<SimpleCSP> engine(csp);
	SimpleCSP* solution = engine.next();
	if (!solution) return false;
// 	std::cout << "Formula solution: " << std::endl; print(std::cout, *solution); std::cout << std::endl;
	delete solution;
	return true;
}

void FormulaCSPHandler::recoverApproximateSupport(gecode::SimpleCSP* csp, Atom::vctr& support, const State& seed) const {
	throw UnimplementedFeatureException("Approximate formula processing needs to be rethought after the redesign to move to extensional constraints");
	// We have already propagated constraints with the call to status(), so we simply arbitrarily pick one consistent value per variable.
	
	/*
	// First process the direct state variables
	for (const VariableIdx variable:_translator.getDirectInputVariables()) {
		const Gecode::IntVar& csp_var = _translator.resolveInputStateVariable(*csp, variable);
		Gecode::IntVarValues values(csp_var);  // This returns a set with all consistent values for the given variable
		assert(values()); // Otherwise the CSP would be inconsistent!
		
		// If the original value makes the situation a goal, then we don't need to add anything for this variable.
		int seed_value = seed.getValue(variable);
		int selected = Helper::selectValueIfExists(values, seed_value);
		if (selected == seed_value) continue;
		support.push_back(Atom(variable, selected)); // It not, we simply pick the first consistent value
	}
	*/
	// Now process the indirect state variables
	// TODO - This part on approximate recovery of values for nested term fluents is a bit iffy.
	// For state variables acting as nested fluent indexes, we are not ensuring that the same value is selected for them
	// when acting as support and when acting as index - but this is subject to change when we move into arity > 1 nested fluents,
	// so I don't think it is worth the extra effort refactoring this now.
	std::set<VariableIdx> inserted;
	
	
	
	/*
	for (const NestedFluentElementTranslator& fluent_translator:_nested_fluent_translators) {
		auto nested_translator = fluent_translator.getNestedFluentData();
		auto idx_variable = nested_translator.getIndex(*csp);
		Gecode::IntVarValues values(idx_variable);
		assert(values()); // Otherwise the CSP would be inconsistent!
		int arbitrary_element = values.val();
		nested_translator.getTableVariables()[arbitrary_element];
		VariableIdx state_variable = nested_translator.getTableVariables()[arbitrary_element];
		
		if (inserted.find(state_variable) == inserted.end()) { // Don't push twice to the support the same atom
			auto fluent = fluent_translator.getTerm();
			auto csp_var = _translator.resolveVariable(fluent, CSPVariableType::Input, *csp);
			Gecode::IntVarValues values(csp_var);
			assert(values()); // Otherwise the CSP would be inconsistent!
			support.push_back(Atom(state_variable, values.val())); // Simply push an arbitrary value
			inserted.insert(state_variable);
		}
	}
	*/
}

void FormulaCSPHandler::create_novelty_constraint() {
	// We register the adequate variables through the NoveltyConstraint object
// 	_novelty = WeakNoveltyConstraint::create(_translator, _formula, {});
}

// In the case of a single formula, we just retrieve and index all terms and atoms
void FormulaCSPHandler::index() {
	const auto conditions =  _formula->all_atoms();
	const auto terms = _formula->all_terms();
	
	// Index formula elements
	index_formula_elements(conditions, terms);	
}

} } // namespaces
