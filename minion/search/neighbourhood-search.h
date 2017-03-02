#include "neighbourhood-def.h"
#include <ctime>
#include <cstdlib>
#include <memory>
#include "NeighbourhoodChoosingStrategies.h"


#ifndef MINION_NEIGHBOURHOOD_SEARCH
#define MINION_NEIGHBOURHOOD_SEARCH

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

template<typename NeighbourhoodSelectionStrategy>
struct NeighbourhoodSearchManager : public Controller::SearchManager {
    shared_ptr<Propagate> prop;
    vector<SearchOrder> base_order;
    NeighbourhoodContainer nhc;
    const NeighbourhoodSelectionStrategy &selectionStrategy;
    NeighbourhoodSearchManager(shared_ptr<Propagate> _prop,
                               vector<SearchOrder> _base_order,
                               NeighbourhoodContainer _nhc,
                                const NeighbourhoodSelectionStrategy &selectionStrategy)
    : prop(_prop), base_order(_base_order), nhc(_nhc), selectionStrategy(selectionStrategy)
    { }

    void searchNeighbourhoods(vector<DomainInt> &solution, DomainInt &minValue, vector<int> &activatedNeighbourhoods)
    {
        // Save state of the world
        int depth = Controller::get_world_depth();
        Controller::world_push();

        auto vo = Controller::make_search_order_multiple(base_order);

        if (activatedNeighbourhoods.empty()){
            nhc.shadow_disable.assign(1);
        } else{
            switchOnNeighbourhoods(activatedNeighbourhoods);
        }

        prop->prop(vo->getVars());
        if(getState().isFailed())
        {
            D_FATAL_ERROR("Neighbourhoods Assignment unsatisfiable");
        }

        auto sm = make_shared<Controller::StandardSearchManager>
                    (vo, prop, Controller::standard_time_ctrlc_checks,
                    [&solution, &minValue, this](){
                        for(const auto& var : this->nhc.shadow_mapping[0])
                            solution.push_back(var.getAssignedValue());
                        minValue = getState().getOptimiseVar()->getMin();
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
        vector<DomainInt> solution;
        DomainInt  minValue;
        searchNeighbourhoods(solution, minValue, {});

        if (solution.empty()){
            return;
        }


        copyOverIncumbent(solution);
        vector<int> activatedNeighbourhoods;
        while (!(activatedNeighbourhoods = selectionStrategy.getNeighbourHoodsToActivate(nhc)).empty()){
            //Set the lower bound of the optimized variable
            getState().getOptimiseVar()->setMin(minValue);
            auto startTime = std::chrono::high_resolution_clock::now();
            searchNeighbourhoods(solution, minValue, activatedNeighbourhoods);
            auto endTime = std::chrono::high_resolution_clock::now();

            if (!solution.empty()){
                copyOverIncumbent(solution);

                /*
                 * 1. Need to update the neighbourhoods with the stats
                 * 2.
                 */



            }

        }

        exit(1);
    }

    typedef std::chrono::high_resolution_clock::time_point timePoint;


    u_int64_t getTimeTaken(timePoint startTime, timePoint endTime){
      return std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    }







    void updateNeighbourhoodStats(vector<int> &activatedNeighbourhoods, struct NeighbourhoodStats &neighbourhoodStats ){
        selectionStrategy.updateNeighbourhoodStats(activatedNeighbourhoods, neighbourhoodStats);
    }




    /**
     * Switch on the neighbourhood activation vars
     * Find the set of primary variables not contained in any neighbourhoods and assign them to the incumbent solution
     * @param neighbourHoodIndexes
     */
    void switchOnNeighbourhoods(const vector<int> &neighbourHoodIndexes){
       std::unordered_set<AnyVarRef> shadowVariables();
       for (int i: neighbourHoodIndexes){
           Neighbourhood &neighbourhood = nhc.neighbourhoods[i];
           neighbourhood.activation.assign(1);

           for (auto & n:neighbourhood.vars){
                shadowVariables().insert(n);
           }
       }

        /*
         * If the primary variable is not contained in any shadow neighbourhoods, assign its incumbent solution
         */
        for (auto &var: nhc.shadow_mapping[0]){
           if (shadowVariables().count(var) == 0){
              var.assign(var.getAssignedValue());
           }
        }
    }

    void copyOverIncumbent(const vector<DomainInt> &solution){
        for (int i = 0; i < nhc.shadow_mapping[0].size(); i++){
           nhc.shadow_mapping[1][i].assign(nhc.shadow_mapping[0][i].getAssignedValue());
        }
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

#endif

