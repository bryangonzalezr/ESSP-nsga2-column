
#include "ProblemInstance.h"   
#include <bits/stdc++.h>
using namespace std;

// ========== STRUCTURES ==========
struct Sequence {
    vector<int> shifts;
    vector<pair<int, int>> shift_count; // (shift_id, count)
    int length = 0;
    int total_minutes = 0;
};

struct EmployeeSchedule {
    int employee_id;
    vector<pair<int, Sequence>> placed_sequences; // (start_day, sequence)
    int total_preference=0;
    int total_minutes = 0;

    // Cached constraint tracking for efficiency
    vector<int> shift_count; // Count of each shift type used
    int weekend_count = 0;   // Number of weekends worked
};

struct Individual {
    vector<EmployeeSchedule> schedules;
    double cost;
    double total_preference =0.0;
    
    // NUEVAS VARIABLES
    bool is_feasible;           
    double feasibility_penalty; 
    double objective_cost=0.0;
    double coverage_cost=0.0;
    
    bool operator<(const Individual& other) const {
        if (is_feasible != other.is_feasible) {
            return is_feasible > other.is_feasible;
        }
        return cost < other.cost;
    }
};

struct Population {
    vector<Individual> individuals;
    int pop_size;
};

// ========== GENETIC ALGORITHM BASE ==========
bool isEmployeeFeasible(const EmployeeSchedule& sched, const ProblemInstance& instance);
vector<Sequence> generateWorkingSequences(const ProblemInstance& instance,int employee_id);


class ScheduleSearch {
public:
    ScheduleSearch(const ProblemInstance& inst)
    : instance(inst), pop_size(30), generations(5000) { // defaults
    employee_sequences.resize(instance.employees.size());
    for (int emp_id = 0; emp_id < (int)instance.employees.size(); emp_id++) {
        employee_sequences[emp_id] = generateWorkingSequences(instance, emp_id);
        }
    }

    Individual runGA();
    void printIndividual(const Individual& ind, const ProblemInstance& instance);

private:
    ProblemInstance instance;
    int pop_size;
    int generations;
    vector<vector<Sequence>> employee_sequences;

    // Core GA functions
    Population initializePopulation();
    double evaluate(Individual& ind);
    Individual tournamentSelection(const Population& pop, int k = 2);
    Individual crossover(const Individual& p1, const Individual& p2);
    void mutate(Individual& ind, double mutation_rate);
    void printEmployeeSchedule(const EmployeeSchedule& schedule, const ProblemInstance& instance);

    // Utility
    Individual bestIndividual(const Population& pop);
};
