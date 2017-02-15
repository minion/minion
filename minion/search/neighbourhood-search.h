#include "neighbourhood-def.h"

void inline get_single_solution() {
  if(getState().isOptimisationProblem()) {
    if(!getState().getOptimiseVar()->isAssigned()) {
      cerr << "The optimisation variable isn't assigned at a solution node!" << endl;
      cerr << "Put it in the variable ordering?" << endl;
      cerr << "Aborting Search" << endl;
      exit(1);
    }

    if(getOptions().printonlyoptimal) {
      getState().storedSolution += "Solution found with Value: " +
                                   tostring(getState().getRawOptimiseVar()->getAssignedValue()) +
                                   "\n";
    } else {
      cout << "Solution found with Value: " << getState().getRawOptimiseVar()->getAssignedValue()
           << endl;
    }

    getState().setOptimiseValue(getState().getOptimiseVar()->getAssignedValue() + 1);
  }
  // Note that sollimit = -1 if all solutions should be found.
  if(getState().getSolutionCount() == getOptions().sollimit)
    throw EndOfSearch();
}


struct NeighbourhoodSearchManager : public Controller::SearchManager {
    shared_ptr<Propagate> prop;
    vector<SearchOrder> base_order;
    NeighbourhoodContainer nhc;

    NeighbourhoodSearchManager(shared_ptr<Propagate> _prop,
                               vector<SearchOrder> _base_order,
                               NeighbourhoodContainer _nhc)
    : prop(_prop), base_order(_base_order), nhc(_nhc)
    { }

    void get_initial_solution()
    {
        // Save state of the world
        int depth = Controller::get_world_depth();
        Controller::world_push();
        vector<DomainInt> solution;

        auto vo = Controller::make_search_order_multiple(base_order);

        // Prepare to find a shadow assignment.
        nhc.shadow_disable.assign(1);
        prop->prop(vo->getVars());
        if(getState().isFailed())
        {
            D_FATAL_ERROR("Shadow disable unsatisfiable");
        }
        Controller::world_push();

        auto sm = make_shared<Controller::StandardSearchManager>
                    (vo, prop, Controller::standard_time_ctrlc_checks,
                    [&solution, this](){
                        for(const auto& var : this->nhc.shadow_mapping[0])
                            solution.push_back(var.getAssignedValue());
                        throw EndOfSearch();
                    }, [](){});
        try {
            sm->search();
        }
        catch(EndOfSearch&)
        { }
        Controller::world_pop_to_depth(depth);
    }

    virtual void search() 
    {
        get_initial_solution();
        exit(1);
    }

};

shared_ptr<Controller::SearchManager>
MakeNeighbourhoodSearch(PropagationLevel prop_method,
                        vector<SearchOrder> base_order,
                        NeighbourhoodContainer nhc)
{
    shared_ptr<Propagate> prop = Controller::make_propagator(prop_method);
    return std::make_shared<NeighbourhoodSearchManager>(prop, base_order, nhc);
}