#include "RandomMethod.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <thread>
#include <chrono>

int RandomMethod::getMaxShiftLength() const {
    int longest = 0;
    for (const auto& s : instance->shifts)
        longest = std::max(longest, s.length);
    return longest;
}

Column RandomMethod::generateColumn(int employee_id) {
    const Employee& emp = instance->employees[employee_id];

    // retry until feasible
    while (true) {
        Column column(employee_id, instance->horizon_length);
        int total_minutes = 0;
        std::vector<int> shift_count(instance->shifts.size(), 0);

        // Track consecutive periods
        int consecutive_shifts = 0;
        int consecutive_off = 0;
        bool in_shift_period = false;
        bool in_off_period = true; // Start assuming we're in an off period

        for (int day = 0; day < instance->horizon_length; ++day) {
            // Mandatory day off
            if (std::find(emp.days_off.begin(), emp.days_off.end(), day) != emp.days_off.end()) {
                column.shifts[day] = 0;
                
                // Update consecutive tracking
                if (in_shift_period && consecutive_shifts > 0) {
                    // Check if we're breaking a shift period too early
                    if (emp.min_consecutive_shifts > 0 && consecutive_shifts < emp.min_consecutive_shifts) {
                        // This would violate minimum consecutive shifts - this column will be infeasible
                        // But we continue to generate it and let the retry handle it
                    }
                    in_shift_period = false;
                    consecutive_shifts = 0;
                }
                
                if (!in_off_period) {
                    in_off_period = true;
                    consecutive_off = 1;
                } else {
                    consecutive_off++;
                }
                continue;
            }

            bool need_more_minutes = (emp.min_total_minutes > 0 &&
                                      total_minutes + (instance->horizon_length - day) * getMaxShiftLength() < emp.min_total_minutes);

            bool exceeded_minutes = (emp.max_total_minutes > 0 && total_minutes >= emp.max_total_minutes);

            // Check if we're in the middle of a consecutive period that shouldn't be broken
            bool must_continue_shifts = (in_shift_period && consecutive_shifts > 0 && 
                                       emp.min_consecutive_shifts > 0 && 
                                       consecutive_shifts < emp.min_consecutive_shifts);
            
            bool must_continue_off = (in_off_period && consecutive_off > 0 && 
                                    emp.min_consecutive_days_off > 0 && 
                                    consecutive_off < emp.min_consecutive_days_off);

            if (exceeded_minutes || must_continue_off) {
                // Must assign OFF
                column.shifts[day] = 0;
                
                // Update consecutive tracking
                if (in_shift_period && consecutive_shifts > 0) {
                    // We're ending a shift period
                    if (emp.min_consecutive_shifts > 0 && consecutive_shifts < emp.min_consecutive_shifts) {
                        // This violates minimum consecutive shifts - column will be infeasible
                    }
                    in_shift_period = false;
                    consecutive_shifts = 0;
                }
                
                if (!in_off_period) {
                    in_off_period = true;
                    consecutive_off = 1;
                } else {
                    consecutive_off++;
                }
                
            } else if (need_more_minutes || must_continue_shifts) {
                // Must assign a shift
                std::vector<int> candidates;
                for (int s = 1; s < instance->shifts.size(); ++s) {
                    if (s < emp.max_shifts.size() && emp.max_shifts[s] == 0) continue; // not allowed
                    if (s < emp.max_shifts.size() && emp.max_shifts[s] > 0 && shift_count[s] >= emp.max_shifts[s]) continue; // already reached cap
                    candidates.push_back(s);
                }
                
                if (!candidates.empty()) {
                    int shift_id = candidates[std::uniform_int_distribution<>(0, candidates.size() - 1)(rng)];
                    column.shifts[day] = shift_id;
                    shift_count[shift_id]++;
                    total_minutes += instance->shifts[shift_id].length;
                    
                    // Update consecutive tracking
                    if (in_off_period && consecutive_off > 0) {
                        // We're ending an off period
                        if (emp.min_consecutive_days_off > 0 && consecutive_off < emp.min_consecutive_days_off) {
                            // This violates minimum consecutive days off - column will be infeasible
                        }
                        in_off_period = false;
                        consecutive_off = 0;
                    }
                    
                    if (!in_shift_period) {
                        in_shift_period = true;
                        consecutive_shifts = 1;
                    } else {
                        consecutive_shifts++;
                    }
                    
                } else {
                    // No valid shifts available - fallback to off
                    column.shifts[day] = 0;
                    
                    // Update consecutive tracking
                    if (in_shift_period && consecutive_shifts > 0) {
                        if (emp.min_consecutive_shifts > 0 && consecutive_shifts < emp.min_consecutive_shifts) {
                            // This violates minimum consecutive shifts
                        }
                        in_shift_period = false;
                        consecutive_shifts = 0;
                    }
                    
                    if (!in_off_period) {
                        in_off_period = true;
                        consecutive_off = 1;
                    } else {
                        consecutive_off++;
                    }
                }
                
            } else {
                // Free choice - but consider consecutive constraints
                bool choose_shift = (std::uniform_int_distribution<>(0, 1)(rng) == 1);
                
                // Bias the choice to avoid breaking consecutive periods
                if (in_shift_period && consecutive_shifts > 0 && 
                    emp.min_consecutive_shifts > 0 && consecutive_shifts < emp.min_consecutive_shifts) {
                    // Strongly prefer to continue the shift period
                    choose_shift = (std::uniform_int_distribution<>(0, 9)(rng) < 8); // 80% chance to continue
                }
                
                if (in_off_period && consecutive_off > 0 && 
                    emp.min_consecutive_days_off > 0 && consecutive_off < emp.min_consecutive_days_off) {
                    // Strongly prefer to continue the off period
                    choose_shift = (std::uniform_int_distribution<>(0, 9)(rng) < 2); // 20% chance to break off period
                }
                
                if (choose_shift) {
                    // Try to assign a shift
                    std::vector<int> candidates;
                    for (int s = 1; s < instance->shifts.size(); ++s) {
                        if (s < emp.max_shifts.size() && emp.max_shifts[s] == 0) continue;
                        if (s < emp.max_shifts.size() && emp.max_shifts[s] > 0 && shift_count[s] >= emp.max_shifts[s]) continue;
                        
                        // Check incompatible consecutive shifts (R3)
                        if (day > 0) {
                            int prev_shift = column.shifts[day - 1];
                            if (prev_shift > 0 && prev_shift < instance->shifts.size()) {
                                const auto& incompatible = instance->shifts[prev_shift].incompatible_shifts;
                                if (std::find(incompatible.begin(), incompatible.end(), s) != incompatible.end()) {
                                    continue; // Skip this shift as it's incompatible with previous day's shift
                                }
                            }
                        }
                        
                        candidates.push_back(s);
                    }
                    
                    if (!candidates.empty()) {
                        int shift_id = candidates[std::uniform_int_distribution<>(0, candidates.size() - 1)(rng)];
                        column.shifts[day] = shift_id;
                        shift_count[shift_id]++;
                        total_minutes += instance->shifts[shift_id].length;
                        
                        // Update consecutive tracking
                        if (in_off_period && consecutive_off > 0) {
                            if (emp.min_consecutive_days_off > 0 && consecutive_off < emp.min_consecutive_days_off) {
                                // This violates minimum consecutive days off
                            }
                            in_off_period = false;
                            consecutive_off = 0;
                        }
                        
                        if (!in_shift_period) {
                            in_shift_period = true;
                            consecutive_shifts = 1;
                        } else {
                            consecutive_shifts++;
                        }
                        
                    } else {
                        // No valid shifts - assign off
                        column.shifts[day] = 0;
                        
                        // Update consecutive tracking
                        if (in_shift_period && consecutive_shifts > 0) {
                            if (emp.min_consecutive_shifts > 0 && consecutive_shifts < emp.min_consecutive_shifts) {
                                // This violates minimum consecutive shifts
                            }
                            in_shift_period = false;
                            consecutive_shifts = 0;
                        }
                        
                        if (!in_off_period) {
                            in_off_period = true;
                            consecutive_off = 1;
                        } else {
                            consecutive_off++;
                        }
                    }
                } else {
                    // Choose off
                    column.shifts[day] = 0;
                    
                    // Update consecutive tracking
                    if (in_shift_period && consecutive_shifts > 0) {
                        if (emp.min_consecutive_shifts > 0 && consecutive_shifts < emp.min_consecutive_shifts) {
                            // This violates minimum consecutive shifts
                        }
                        in_shift_period = false;
                        consecutive_shifts = 0;
                    }
                    
                    if (!in_off_period) {
                        in_off_period = true;
                        consecutive_off = 1;
                    } else {
                        consecutive_off++;
                    }
                }
            }
        }

        // Final check for any ongoing consecutive periods at the end of the horizon
        if (in_shift_period && consecutive_shifts > 0 && 
            emp.min_consecutive_shifts > 0 && consecutive_shifts < emp.min_consecutive_shifts) {
            // The schedule ends with an incomplete shift period - will be infeasible
        }
        
        if (in_off_period && consecutive_off > 0 && 
            emp.min_consecutive_days_off > 0 && consecutive_off < emp.min_consecutive_days_off) {
            // The schedule ends with an incomplete off period - will be infeasible
        }

        evaluateColumn(column);
        if (column.feasible) {
            return column; // ✅ done
        }
        else{
            // why 
            std::cout << "Generated infeasible column for employee " << emp.name << ": ";
            for (int d = 0; d < column.shifts.size(); d++)
            {
                std::cout << column.shifts[d] << " ";
            }
            std::cout << "\nViolations: " << column.constraint_violation << "(details: ";
            for (const auto& detail : column.constraint_details)
            {
                std::cout << detail << " ";
            }
            std::cout << ")\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        }
        
        // Optional debugging (uncommented for troubleshooting)
        // std::cout << "Generated infeasible column for employee " << emp.name << ": ";
        // for (int d = 0; d < column.shifts.size(); d++) {
        //     std::cout << column.shifts[d] << " ";
        // }
        // std::cout << "\nViolations: " << column.constraint_violation << " (details: ";
        // for (const auto& detail : column.constraint_details) {
        //     std::cout << detail << " ";
        // }
        // std::cout << ")\n";
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}