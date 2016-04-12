
#include <search/engines/gbfs_crpg_lifted.hxx>
#include <problem.hxx>
#include <aptk2/search/algorithms/best_first_search.hxx>
#include <heuristics/relaxed_plan/gecode_crpg.hxx>
#include <heuristics/relaxed_plan/lifted_crpg.hxx>
#include <constraints/gecode/gecode_rpg_builder.hxx>
#include <constraints/gecode/handlers/action_schema_handler.hxx>

#include <state.hxx>
#include <actions/lifted_action_iterator.hxx>

using namespace fs0::gecode;

namespace fs0 { namespace engines {
std::unique_ptr<aptk::SearchAlgorithm<LiftedStateModel>> GBFSLiftedPlannerCreator::create(const Config& config, LiftedStateModel& model) const {
	
	const Problem& problem = model.getTask();
	
	// The CSP handlers for applicable action iteration: we do not need novelty constraints (because they are instantiated on a standard state, not on a RPG layer),
	// and we need full CSP resolution
	const std::vector<const PartiallyGroundedAction*>& base_actions = problem.getPartiallyGroundedActions();

	model.set_handlers(ActionSchemaCSPHandler::create_derived(base_actions, problem.get_tuple_index(), false, false));
	
	if (Config::instance().getCSPModel() != Config::CSPModel::ActionSchemaCSP && Config::instance().getCSPModel() != Config::CSPModel::EffectSchemaCSP) {
		throw std::runtime_error("WARNING: Lifted planning needs a lifted CSP model.");
	}
	
	bool novelty = Config::instance().useNoveltyConstraint();
	bool approximate = Config::instance().useApproximateActionResolution();
	
	auto gecode_builder = GecodeRPGBuilder::create(problem.getGoalConditions(), problem.getStateConstraints(), problem.get_tuple_index());
	
	std::vector<std::shared_ptr<BaseActionCSPHandler>> csp_handlers;
	if (Config::instance().getCSPModel() == Config::CSPModel::ActionSchemaCSP) {
		csp_handlers = ActionSchemaCSPHandler::create(base_actions, problem.get_tuple_index(), approximate, novelty);
	} else { // EffectSchemaCSP
		assert(false); // Currently disabled
// 		std::vector<IndexedTupleset> symbol_tuplesets = LiftedCRPG::index_tuplesets(problem.getProblemInfo());
// 		csp_handlers = EffectSchemaCSPHandler::create(base_actions, symbol_tuplesets, approximate, novelty);
	}
	
	GecodeCRPG gecode_builder_heuristic(problem, std::move(csp_handlers), std::move(gecode_builder));
	return std::unique_ptr<LiftedEngine>(new aptk::StlBestFirstSearch<SearchNode, GecodeCRPG, LiftedStateModel>(model, std::move(gecode_builder_heuristic)));
}






} } // namespaces
