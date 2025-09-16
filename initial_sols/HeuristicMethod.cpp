#include "HeuristicMethod.h"
#include <algorithm>
#include <random>

Column HeuristicMethod::generateColumn(int employee_id) {
    Column column(employee_id, instance->horizon_length);
    const Employee& emp = instance->employees[employee_id];

    std::uniform_int_distribution<> shift_dist(0, instance->shifts.size() - 1);

    // Step 1: Start with random shifts within min/max total minutes
    int total_minutes = 0;
    for (int day = 0; day < instance->horizon_length; ++day) {
        // Check if this is a mandatory day off
        if (std::find(emp.days_off.begin(), emp.days_off.end(), day) != emp.days_off.end()) {
            column.shifts[day] = 0; // Must be day off
            continue;
        }

        int s = 0;
        for (int attempts = 0; attempts < 10; ++attempts) {  // try a few times to find valid shift
            s = shift_dist(rng);
            if (s == 0) break;  // day off is always allowed
            
            int new_total = total_minutes + instance->shifts[s].length;
            if (emp.max_total_minutes == 0 || new_total <= emp.max_total_minutes) {
                break; // This shift is within total minutes limit
            }
        }

        column.shifts[day] = s;
        if (s > 0) {
            total_minutes += instance->shifts[s].length;
        }
    }

    // Step 2: Iteratively improve the solution by random toggles
    const int max_iterations = 1000;
    int iterations = 0;
    
    while (iterations < max_iterations) {
        if (isColumnFeasible(column)) break;  // Solution is feasible
        
        iterations++;
        
        // Pick a random day to modify
        std::uniform_int_distribution<> day_dist(0, instance->horizon_length - 1);
        int day = day_dist(rng);

        // Skip mandatory days off
        if (std::find(emp.days_off.begin(), emp.days_off.end(), day) != emp.days_off.end()) {
            continue;
        }

        // Try to find a better shift for this day
        int old_shift = column.shifts[day];
        std::vector<int> candidate_shifts;
        
        // Add all shifts (including day off) as candidates
        for (int s = 0; s < instance->shifts.size(); s++) {
            candidate_shifts.push_back(s);
        }
        
        // Shuffle candidates to try them in random order
        std::shuffle(candidate_shifts.begin(), candidate_shifts.end(), rng);
        
        // Try each candidate shift
        for (int s : candidate_shifts) {
            if (s == old_shift) continue; // Skip current shift
            
            column.shifts[day] = s;
            
            // Quick check: does this improve feasibility?
            if (isColumnFeasible(column)) {
                break; // Found a good shift
            }
        }
        
        // If no improvement found, revert to original
        if (!isColumnFeasible(column)) {
            column.shifts[day] = old_shift;
        }
    }

    // Evaluate the final column
    evaluateColumn(column);
    return column;
}