
#pragma once

#include <unordered_set>

#include <fs_types.hxx>
#include <atom.hxx>
#include <constraints/gecode/gecode_csp.hxx>
#include <constraints/gecode/utils/extensional_constraint.hxx>
#include <constraints/gecode/csp_translator.hxx>


namespace fs0 { namespace language { namespace fstrips { class Term; class AtomicFormula; class FluentHeadedNestedTerm; }}}
namespace fs = fs0::language::fstrips;

namespace fs0 { class RPGData; class Binding; }

namespace fs0 { namespace gecode {

//! The base interface class for all gecode CSP handlers
class BaseCSP {
public:
	BaseCSP(const TupleIndex& tuple_index, bool approximate);
	virtual ~BaseCSP() = default;
	
	//! Create a new action CSP constraint by the given RPG layer domains
	//! Ownership of the generated pointer belongs to the caller
	GecodeCSP* instantiate(const RPGIndex& graph) const;
	GecodeCSP* instantiate(const State& state) const;
	
	const CSPTranslator& getTranslator() const { return _translator; }

	static void registerTermVariables(const fs::Term* term, CSPTranslator& translator);
	static void registerTermConstraints(const fs::Term* term, CSPTranslator& translator);
	static void registerFormulaVariables(const fs::AtomicFormula* condition, CSPTranslator& translator);
	static void registerFormulaConstraints(const fs::AtomicFormula* condition, CSPTranslator& translator);

	//! Prints a representation of the object to the given stream.
	friend std::ostream& operator<<(std::ostream &os, const BaseCSP& o) { return o.print(os); }
	std::ostream& print(std::ostream& os) const { return print(os, _base_csp); }
	std::ostream& print(std::ostream& os, const GecodeCSP& csp) const;
	
protected:
	//! The base Gecode CSP
	GecodeCSP _base_csp;
	
	//! Whether the underlying CSP gecode space has already been detected as failed.
	bool _failed;
	
	//! Whether to solve the CSPs completely or approximately
	bool _approximate;

	//! A translator to map planning variables with gecode variables
	CSPTranslator _translator;
	
	// All (distinct) FSTRIPS terms that participate in the CSP
	std::unordered_set<const fs::Term*> _all_terms;
	
	// All (distinct) FSTRIPS atomic formulas that participate in the CSP
	std::unordered_set<const fs::AtomicFormula*> _all_formulas;
	
	//! The Ids of the state variables that are relevant to some formula via a (predicative) atom.
	std::vector<TupleIdx> _necessary_tuples;
	
	const TupleIndex& _tuple_index;
	
	//!
	std::vector<ExtensionalConstraint> _extensional_constraints;
	
	//! Index all terms and formulas appearing in the formula / actions which will be relevant to the CSP
	virtual void index() = 0;

	void register_csp_variables();

	void register_csp_constraints();
	
	//! Registers the variables of the CSP into the CSP translator
	void createCSPVariables(bool use_novelty_constraint);
	
	//! Common indexing routine - places all conditions and terms appearing in formulas into their right place
	void index_formula_elements(const std::vector<const fs::AtomicFormula*>& conditions, const std::vector<const fs::Term*>& terms);
	
	//! By default, we create no novelty constraint
	virtual void create_novelty_constraint() {}
	
	//! By default, we post no novelty constraint whatsoever
	virtual void post_novelty_constraint(GecodeCSP& csp, const RPGIndex& rpg) const {}
};

} } // namespaces
