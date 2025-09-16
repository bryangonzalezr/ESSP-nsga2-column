#include "ColumnGenerationBase.h"
#include <iostream>
#include <algorithm>

// ColumnPool implementations
void ColumnPool::addColumn(const Column& column) {
    if (column.employee_id >= 0 && column.employee_id < employee_pools.size()) {
        employee_pools[column.employee_id].push_back(column);
    }
}

int ColumnPool::getTotalColumns() const {
    int total = 0;
    for (const auto& pool : employee_pools) {
        total += pool.size();
    }
    return total;
}

void ColumnPool::printPool() const {
    std::cout << "\n=== Column Pool Summary ===" << std::endl;
    for (int emp = 0; emp < employee_pools.size(); ++emp) {
        std::cout << "Employee " << emp;
        if (instance && emp < instance->employees.size()) {
            std::cout << " (" << instance->employees[emp].name << ")";
        }
        std::cout << ": " << employee_pools[emp].size() << " columns" << std::endl;
        
        for (int col = 0; col < std::min(3, (int)employee_pools[emp].size()); ++col) {
            const auto& column = employee_pools[emp][col];
            std::cout << "  Column " << col << " (cost=" << column.cost 
                      << ", violations=" << column.constraint_violation << "): ";
            for (int day = 0; day < (int)column.shifts.size(); ++day) {
                if (instance && column.shifts[day] < instance->shifts.size()) {
                    std::cout << instance->shifts[column.shifts[day]].name << " ";
                } else {
                    std::cout << column.shifts[day] << " ";
                }
            }
            std::cout << (column.feasible ? " [feasible]" : " [infeasible]") << std::endl;
            
            // Show constraint details if there are violations
            if (column.constraint_violation > 0) {
                std::cout << "    Constraint violations: ";
                const char* constraint_names[] = {"DaysOff", "MaxShifts", "Incompatible", 
                                                "MinConsecutive", "MaxConsecutive", "Weekends", 
                                                "MaxMinutes", "MinMinutes"};
                for (int c = 0; c < column.constraint_details.size(); ++c) {
                    if (column.constraint_details[c] > 0) {
                        std::cout << constraint_names[c] << "(" << column.constraint_details[c] << ") ";
                    }
                }
                std::cout << std::endl;
            }
        }
    }
    std::cout << "Total columns: " << getTotalColumns() << std::endl;
}

// ColumnGenerator implementations
void ColumnGenerator::generateInitialPool(int columns_per_employee) {
    if (!method) {
        std::cerr << "Error: No column generation method set!" << std::endl;
        return;
    }
    
    std::cout << "Generating initial column pool using " << method->getMethodName() << "..." << std::endl;

    for (int emp = 0; emp < instance->employees.size(); ++emp) {
        std::cout << "Generating columns for employee " << emp << std::endl;
        int added = 0;
        int attempts = 0;
        const int max_attempts = columns_per_employee * 10; // Avoid infinite loops
        
        while (added < columns_per_employee && attempts < max_attempts) {
            attempts++;
            Column column = method->generateColumn(emp);
            
            if (column.feasible) {
                pool.addColumn(column);
                added++;
                // std::cout << "  Generated feasible column " << added << "/" << columns_per_employee << std::endl;
            }
        }
        
        if (added < columns_per_employee) {
            std::cout << "  Warning: Only generated " << added << " feasible columns for employee " << emp << std::endl;
        }
    }

    std::cout << "Generated " << pool.getTotalColumns() << " initial columns using " << method->getMethodName() << std::endl;
}

bool ColumnGenerationMethod::isColumnFeasible(const Column& column) {
    const Employee& emp = instance->employees[column.employee_id];
    int horizon_length = instance->horizon_length;
    int num_shifts = instance->shifts.size();

    std::vector<int> shift_count(num_shifts, 0);
    int consecutive_shifts = 0;
    int consecutive_off = 0;
    int consecutive_shifts_for_r4 = 0;
    int total_minutes = 0;
    int weekcount = 0;

    int filled_days = 0;
    for (int day = 0; day < horizon_length; ++day) {
        int shift_id = column.shifts[day];
        if (shift_id < 0) break; // columna incompleta
        filled_days++;

        // --- R1: Mandatory days off ---
        for (int day_off : emp.days_off) {
          
        
            if (day == day_off && shift_id != 0) {
                return false;
            }
        }

        // --- R2: Max shifts per type ---
        if (shift_id >= 0 && shift_id < num_shifts) {
            shift_count[shift_id]++;
            if (!emp.max_shifts.empty() && shift_id < emp.max_shifts.size()) {
                if (emp.max_shifts[shift_id] > 0 && shift_count[shift_id] > emp.max_shifts[shift_id]){
                    return false;
                }
            }
        }

        // --- R3: Incompatible consecutive shifts ---
        if (day > 0) {
            int prev_shift = column.shifts[day - 1];
            if (prev_shift > 0 && shift_id > 0 && prev_shift < num_shifts) {
                const auto& incompatible = instance->shifts[prev_shift].incompatible_shifts;
                if (std::find(incompatible.begin(), incompatible.end(), shift_id) != incompatible.end()){
                    return false;
                }
            }
        }

        // --- R4: Min consecutive shifts / days off ---
        if (shift_id == 0 && consecutive_shifts > 0) {
            if (emp.min_consecutive_shifts > 0 && consecutive_shifts < emp.min_consecutive_shifts)
                return false;
            consecutive_shifts = 0;
        }
        if (shift_id != 0 && consecutive_off > 0) {
            if (emp.min_consecutive_days_off > 0 && consecutive_off < emp.min_consecutive_days_off){
                return false;
            }
            consecutive_off = 0;
        }

        // Update consecutive counters
        if (shift_id != 0) {
            consecutive_shifts++;
            consecutive_shifts_for_r4++;
            consecutive_off = 0;
            if (emp.max_consecutive_shifts > 0 && consecutive_shifts_for_r4 > emp.max_consecutive_shifts){
                return false; // R5
            }
        } else {
            consecutive_off++;
            consecutive_shifts_for_r4 = 0;
            consecutive_shifts = 0;
        }

        // --- R6: Max weekends ---
        if (day % 7 == 5) { // Saturday
            if (shift_id != 0) weekcount++;
            else if (day + 1 < horizon_length && column.shifts[day + 1] != 0) weekcount++;
        }

        // --- R7 & R8: Total minutes ---
        if (shift_id > 0 && shift_id < num_shifts) {
            total_minutes += instance->shifts[shift_id].length;
            if (emp.max_total_minutes > 0 && total_minutes > emp.max_total_minutes){
                return false; // ya excedió
            }
        }
    }

    int days_left = horizon_length - filled_days;

    // --- Chequeo de posibilidad futura ---
    // ¿Podría alcanzar el mínimo de minutos con lo que falta?
    if (emp.min_total_minutes > 0) {
        int longest = 0;
        for (const auto& s : instance->shifts) longest = std::max(longest, s.length);
        int max_possible = total_minutes + days_left * longest;
        if (max_possible < emp.min_total_minutes){
            return false; // imposible llegar al mínimo
        }
    }

    // --- Si ya completó toda la columna ---
    if (filled_days == horizon_length) {
        if (emp.max_weekends > 0 && weekcount > emp.max_weekends) 
            {
                return false;}

        if (emp.min_total_minutes > 0 && total_minutes < emp.min_total_minutes) {
            return false;
        }
    }

    return true; // factible completa o aún parcialmente factible
}


double ColumnGenerationMethod::calculateColumnCost(const Column& column) {
    // Simple cost based on constraint violations and preferences
    return column.preference_cost;
}

void ColumnGenerationMethod::evaluateColumn(Column& column) {
    const Employee& emp = instance->employees[column.employee_id];
    int horizon_length = instance->horizon_length;
    int num_shifts = instance->shifts.size();
    
    // Reset evaluation metrics
    column.constraint_violation = 0.0;
    column.preference_cost = 0.0;
    column.constraint_details.assign(8, 0.0);
    
    // Employee-specific tracking
    std::vector<int> shift_count(num_shifts, 0);
    int consecutive_shifts = 0;
    int consecutive_off = 0;
    int total_minutes = 0;
    int weekcount = 0;
    int consecutive_shifts_for_r4 = 0;
    
    for (int day = 0; day < horizon_length; day++) {
        int shift_id = column.shifts[day];
        
        // Initialize consecutive counters for day 0
        if (day == 0) {
            if (shift_id == 0) {
                consecutive_shifts = 0;
                consecutive_off = horizon_length;
            } else {
                consecutive_shifts = horizon_length;
                consecutive_off = 0;
            }
        }
        
        // R1: Days off constraints (mandatory days off)
        bool is_mandatory_day_off = false;
        for (int day_off : emp.days_off) {
            if (day == day_off && shift_id != 0) {
                column.constraint_violation += 1.0;
                column.constraint_details[0] += 1.0;
                is_mandatory_day_off = true;
                break;
            }
        }
        
        // R2: Max shifts per type
        if (shift_id >= 0 && shift_id < num_shifts) {
            shift_count[shift_id]++;
            if (!emp.max_shifts.empty() && shift_id < emp.max_shifts.size()) {
                if (emp.max_shifts[shift_id] > 0 && shift_count[shift_id] > emp.max_shifts[shift_id]) {
                    column.constraint_violation += 1.0;
                    column.constraint_details[1] += 1.0;
                    break;
                }
            }
        }
        
        // R3: Incompatible shifts
        if (day > 0) {
            int prev_shift = column.shifts[day - 1];
            if (prev_shift > 0 && shift_id > 0 && prev_shift < instance->shifts.size()) {
                const auto& incompatible = instance->shifts[prev_shift].incompatible_shifts;
                for (int incomp_shift : incompatible) {
                    if (incomp_shift == shift_id) {
                        column.constraint_violation += 1.0;
                        column.constraint_details[2] += 1.0;
                        break;
                    }
                }
            }
        }
        
        // R4: Min consecutive shifts and min consecutive days off
        if (shift_id == 0 && consecutive_shifts > 0) {
            if (emp.min_consecutive_shifts > 0 && consecutive_shifts < emp.min_consecutive_shifts) {
                column.constraint_violation += 1.0;
                column.constraint_details[3] += 1.0;
            }
        }
        
        if (shift_id != 0 && consecutive_off > 0) {
            if (emp.min_consecutive_days_off > 0 && consecutive_off < emp.min_consecutive_days_off) {
                column.constraint_violation += 1.0;
                column.constraint_details[3] += 1.0;
            }
        }
        
        // Update consecutive counters
        if (shift_id != 0) {
            consecutive_shifts++;
            consecutive_shifts_for_r4++;
            consecutive_off = 0;
            
            // R5: Max consecutive shifts
            if (emp.max_consecutive_shifts > 0 && consecutive_shifts_for_r4 > emp.max_consecutive_shifts) {
                column.constraint_violation += 1.0;
                column.constraint_details[4] += 1.0;
            }
        } else {
            consecutive_off++;
            consecutive_shifts_for_r4 = 0;
            consecutive_shifts = 0;
        }
        
        // R6: Weekend counting
        if (day % 7 == 5) { // Saturday
            if (shift_id != 0) {
                weekcount++;
            } else if (day + 1 < horizon_length && column.shifts[day + 1] != 0) {
                weekcount++; // Sunday shift counts as weekend work
            }
        }
        
        // R7: Total minutes tracking
        if (shift_id > 0 && shift_id < instance->shifts.size()) {
            total_minutes += instance->shifts[shift_id].length;
        }
        
        // Preference costs (if shift request data exists)
        if (!instance->shift_on_requests.empty() && !instance->shift_off_requests.empty() &&
            column.employee_id < instance->shift_on_requests.size() &&
            day < instance->shift_on_requests[column.employee_id].size()) {
            
            for (int s = 0; s < num_shifts; s++) {
                if (s < instance->shift_on_requests[column.employee_id][day].size() &&
                    s < instance->shift_off_requests[column.employee_id][day].size()) {
                    
                    if (s == shift_id) {
                        // Penalty for working when requested off
                        column.preference_cost += instance->shift_off_requests[column.employee_id][day][s];
                    } else {
                        // Penalty for not working when requested on
                        column.preference_cost += instance->shift_on_requests[column.employee_id][day][s];
                    }
                }
            }
        }
    }
    
    // R6: Max weekends constraint
    if (emp.max_weekends > 0 && weekcount > emp.max_weekends) {
        column.constraint_violation += 1.0;
        column.constraint_details[5] += 1.0;
    }
    
    // R7: Max total minutes constraint
    if (emp.max_total_minutes > 0 && total_minutes > emp.max_total_minutes) {
        column.constraint_violation += 1.0;
        column.constraint_details[6] += 1.0;
    }
    
    // R8: Min total minutes constraint
    if (emp.min_total_minutes > 0 && total_minutes < emp.min_total_minutes) {
        column.constraint_violation += 1.0;
        column.constraint_details[7] += 1.0;
    }
    
    // Determine feasibility
    column.feasible = (column.constraint_violation == 0.0);
    
    // Calculate total cost
    column.cost = calculateColumnCost(column);
}