// Helper function to check if taking a day off violates min_consecutive_days_off
bool canTakeDayOff(const EmployeeSchedule& current, const Employee& emp, int day, const ProblemInstance& instance) {
    // If no min_consecutive_days_off constraint, always can take day off
    if (emp.min_consecutive_days_off <= 1) return true;
    
    // If no sequences placed yet, can take day off
    if (current.placed_sequences.empty()) return true;
    
    // Find the last working day
    int last_work_day = -1;
    for (const auto& ps : current.placed_sequences) {
        int seq_end = ps.first + ps.second.length - 1;
        last_work_day = max(last_work_day, seq_end);
    }
    
    // If we're at day 0 or right after last work, we can start a consecutive off period
    if (day == 0 || day == last_work_day + 1) return true;
    
    // Check if we're in the middle of a consecutive off period
    // Count consecutive days off before this day
    int consecutive_off = 0;
    for (int d = day - 1; d >= 0; d--) {
        bool is_work_day = false;
        for (const auto& ps : current.placed_sequences) {
            if (d >= ps.first && d < ps.first + ps.second.length) {
                is_work_day = true;
                break;
            }
        }
        if (is_work_day) break;
        consecutive_off++;
    }
    
    // If we already have enough consecutive days off, we can continue
    return consecutive_off + 1 >= emp.min_consecutive_days_off;
}

// Helper function to count weekends worked
int countWeekendsInSchedule(const EmployeeSchedule& schedule, const ProblemInstance& instance) {
    vector<int> full_schedule(instance.horizon_length, 0);
    
    // Build full schedule
    for (const auto& ps : schedule.placed_sequences) {
        for (int i = 0; i < ps.second.length; i++) {
            int day = ps.first + i;
            if (day < instance.horizon_length) {
                full_schedule[day] = ps.second.shifts[i];
            }
        }
    }
    
    int weekend_count = 0;
    for (int day = 0; day < instance.horizon_length; day += 7) {
        // Check Saturday and Sunday (assuming week starts on Monday, day 0)
        int saturday = day + 5;  // Day 5, 12, 19, etc.
        int sunday = day + 6;    // Day 6, 13, 20, etc.
        
        bool weekend_worked = false;
        if (saturday < instance.horizon_length && full_schedule[saturday] != 0) {
            weekend_worked = true;
        }
        if (sunday < instance.horizon_length && full_schedule[sunday] != 0) {
            weekend_worked = true;
        }
        
        if (weekend_worked) weekend_count++;
    }
    
    return weekend_count;
}

// Enhanced pruning function for minimum minutes constraint
bool canReachMinimumMinutes(const EmployeeSchedule& current, const Employee& emp, 
                           const vector<Sequence>& sequences, int current_day, 
                           const ProblemInstance& instance) {
    if (emp.min_total_minutes <= 0) return true;
    
    int days_remaining = instance.horizon_length - current_day;
    int current_minutes = current.total_minutes;
    
    // Quick check: if we already meet the minimum, we're good
    if (current_minutes >= emp.min_total_minutes) return true;
    
    // Calculate maximum possible minutes from the best sequences
    // Sort sequences by minutes per day ratio
    vector<int> sequence_minutes_per_day;
    for (const auto& seq : sequences) {
        if (seq.length > 0) {
            sequence_minutes_per_day.push_back(seq.total_minutes / seq.length);
        }
    }
    
    if (sequence_minutes_per_day.empty()) return false;
    
    // Use the best minutes per day ratio
    sort(sequence_minutes_per_day.rbegin(), sequence_minutes_per_day.rend());
    int best_minutes_per_day = sequence_minutes_per_day[0];
    
    // Conservative estimate: assume we can work all remaining days at best rate
    int max_possible_minutes = current_minutes + (days_remaining * best_minutes_per_day);
    
    return max_possible_minutes >= emp.min_total_minutes;
}

// Enhanced function to estimate minimum days needed to reach target minutes
int estimateMinDaysForMinutes(const Employee& emp, const vector<Sequence>& sequences, int current_minutes) {
    if (emp.min_total_minutes <= 0 || current_minutes >= emp.min_total_minutes) return 0;
    
    int needed_minutes = emp.min_total_minutes - current_minutes;
    
    // Find the most efficient sequence (highest minutes per day)
    int best_minutes_per_day = 0;
    for (const auto& seq : sequences) {
        if (seq.length > 0) {
            int minutes_per_day = seq.total_minutes / seq.length;
            best_minutes_per_day = max(best_minutes_per_day, minutes_per_day);
        }
    }
    
    if (best_minutes_per_day == 0) return INT_MAX; // Can't reach target
    
    return (needed_minutes + best_minutes_per_day - 1) / best_minutes_per_day; // Ceiling division
}

void backtrackEmployeeSchedule(
    const ProblemInstance& instance,
    const Employee& emp,
    const vector<Sequence>& all_sequences,
    int day,
    EmployeeSchedule& current,
    vector<EmployeeSchedule>& results,
    int max_solutions = 10000 // Limit to prevent explosion
) {
    // Limit number of solutions to prevent explosion
    // if (results.size() >= max_solutions) {
    //     return;
    // }
    
    // Base case: reached end of horizon
    if (day >= instance.horizon_length) {
        if (isEmployeeFeasible(current, instance)) {
            results.push_back(current);
        }
        return;
    }
    
    // Skip mandatory days off
    if (find(emp.days_off.begin(), emp.days_off.end(), day) != emp.days_off.end()) {
        backtrackEmployeeSchedule(instance, emp, all_sequences, day + 1, current, results, max_solutions);
        return;
    }

    // Count consecutive days off and skip if needed
    if (!current.placed_sequences.empty()) {
        const auto& last_seq = current.placed_sequences.back();
        int consecutive_current_off = day - (last_seq.first + last_seq.second.length);
        if (consecutive_current_off < emp.min_consecutive_days_off) {
            backtrackEmployeeSchedule(instance, emp, all_sequences, day + 1, current, results, max_solutions);
            return;
        }
    }
    
    // Enhanced pruning for minimum minutes
    if (!canReachMinimumMinutes(current, emp, all_sequences, day, instance)) {
        return; // Cannot reach minimum minutes with remaining days
    }
    
    // Early pruning: check if current partial schedule can still be feasible
    // R7: Max total minutes - early termination if already exceeded
    if (emp.max_total_minutes > 0 && current.total_minutes > emp.max_total_minutes) {
        return;
    }
    
    // R6: Weekend constraint - early pruning
    if (emp.max_weekends > 0) {
        int current_weekends = countWeekendsInSchedule(current, instance);
        if (current_weekends > emp.max_weekends) {
            return; // Already exceeded weekend limit
        }
    }
    
    // Enhanced minimum minutes check with tighter bounds
    if (emp.min_total_minutes > 0) {
        int days_remaining = instance.horizon_length - day;
        int min_days_needed = estimateMinDaysForMinutes(emp, all_sequences, current.total_minutes);
        
        // If we need more working days than we have remaining days, prune
        if (min_days_needed > days_remaining) {
            return;
        }
        
        // If we're in the last few days and still need significant minutes, be more aggressive
        if (days_remaining <= 5) {
            int remaining_minutes_needed = emp.min_total_minutes - current.total_minutes;
            if (remaining_minutes_needed > 0) {
                // Calculate maximum possible minutes from remaining days using best sequences
                int max_possible_from_remaining = 0;
                for (const auto& seq : all_sequences) {
                    if (seq.length <= days_remaining) {
                        max_possible_from_remaining = max(max_possible_from_remaining, seq.total_minutes);
                    }
                }
                
                // If even the best sequence can't help us reach the minimum, prune
                if (max_possible_from_remaining < remaining_minutes_needed && days_remaining < 3) {
                    return;
                }
            }
        }
    }
    
    bool tried_work_sequence = false;
    
    // Try placing work sequences (prioritize longer/more efficient sequences when close to deadline)
    vector<int> sequence_indices(all_sequences.size());
    iota(sequence_indices.begin(), sequence_indices.end(), 0);
    
    // If we're running low on time for minimum minutes, prioritize high-minute sequences
    if (emp.min_total_minutes > 0 && current.total_minutes < emp.min_total_minutes) {
        int days_remaining = instance.horizon_length - day;
        int remaining_minutes_needed = emp.min_total_minutes - current.total_minutes;
        
        if (days_remaining <= remaining_minutes_needed / 100) { // Arbitrary threshold
            sort(sequence_indices.begin(), sequence_indices.end(), [&](int a, int b) {
                return all_sequences[a].total_minutes > all_sequences[b].total_minutes;
            });
        }
    }
    
    for (int seq_idx : sequence_indices) {
        const auto& seq = all_sequences[seq_idx];
        
        // Check if sequence fits in remaining horizon
        if (day + seq.length > instance.horizon_length) continue;
        
        // Check overlap with mandatory days off
        bool overlaps_day_off = false;
        for (int i = 0; i < seq.length; i++) {
            if (find(emp.days_off.begin(), emp.days_off.end(), day + i) != emp.days_off.end()) {
                overlaps_day_off = true;
                break;
            }
        }
        if (overlaps_day_off) continue;
        
        // Check R2: Max shifts per type constraint at schedule level
        bool violates_max_shifts = false;
        vector<int> temp_shift_count = current.shift_count;
        for (int shift_id : seq.shifts) {
            if (shift_id > 0 && shift_id < temp_shift_count.size()) {
                temp_shift_count[shift_id]++;
                if (!emp.max_shifts.empty() && shift_id < emp.max_shifts.size()) {
                    if (emp.max_shifts[shift_id] > 0 && temp_shift_count[shift_id] > emp.max_shifts[shift_id]) {
                        violates_max_shifts = true;
                        break;
                    }
                }
            }
        }
        if (violates_max_shifts) continue;
        
        // Check R7: Max total minutes constraint
        if (emp.max_total_minutes > 0 && current.total_minutes + seq.total_minutes > emp.max_total_minutes) {
            continue;
        }
        
        // Check R5: Max consecutive shifts when combining sequences
        if (emp.max_consecutive_shifts > 0 && !current.placed_sequences.empty()) {
            const auto& last_seq = current.placed_sequences.back();
            int last_seq_end = last_seq.first + last_seq.second.length;
            
            if (day == last_seq_end) {
                int consecutive_work_days = last_seq.second.length + seq.length;
                
                for (int i = current.placed_sequences.size() - 2; i >= 0; i--) {
                    const auto& prev_seq = current.placed_sequences[i];
                    int prev_seq_end = prev_seq.first + prev_seq.second.length;
                    
                    if (prev_seq_end == current.placed_sequences[i + 1].first) {
                        consecutive_work_days += prev_seq.second.length;
                    } else {
                        break;
                    }
                }
                
                if (consecutive_work_days > emp.max_consecutive_shifts) {
                    continue;
                }
            }
        }
        
        // Check R6: Max weekends constraint - pruning during placement
        if (emp.max_weekends > 0) {
            int additional_weekends = 0;
            for (int i = 0; i < seq.length; i++) {
                int seq_day = day + i;
                int day_of_week = seq_day % 7;
                
                if (day_of_week == 5 || day_of_week == 6) {
                    int weekend_start = seq_day - (day_of_week == 6 ? 1 : 0);
                    
                    bool weekend_already_counted = false;
                    for (const auto& ps : current.placed_sequences) {
                        for (int j = 0; j < ps.second.length; j++) {
                            int work_day = ps.first + j;
                            int work_day_of_week = work_day % 7;
                            int work_weekend_start = work_day - (work_day_of_week == 6 ? 1 : 0);
                            
                            if (work_weekend_start == weekend_start && (work_day_of_week == 5 || work_day_of_week == 6)) {
                                weekend_already_counted = true;
                                break;
                            }
                        }
                        if (weekend_already_counted) break;
                    }
                    
                    if (!weekend_already_counted) {
                        additional_weekends++;
                        break;
                    }
                }
            }
            
            int current_weekends = countWeekendsInSchedule(current, instance);
            if (current_weekends + additional_weekends > emp.max_weekends) {
                continue;
            }
        }
        
        tried_work_sequence = true;
        
        // Place sequence
        current.placed_sequences.push_back({day, seq});
        current.total_minutes += seq.total_minutes;
        
        // Update shift counts
        for (int shift_id : seq.shifts) {
            if (shift_id > 0 && shift_id < current.shift_count.size()) {
                current.shift_count[shift_id]++;
            }
        }
        
        // Recurse to next available day after this sequence
        backtrackEmployeeSchedule(instance, emp, all_sequences, day + seq.length, current, results, max_solutions);
        
        // Undo placement
        for (int shift_id : seq.shifts) {
            if (shift_id > 0 && shift_id < current.shift_count.size()) {
                current.shift_count[shift_id]--;
            }
        }
        current.placed_sequences.pop_back();
        current.total_minutes -= seq.total_minutes;
    }
    
    // Try taking day off only if we tried at least one work sequence or minimum minutes constraint allows it
    if (canTakeDayOff(current, emp, day, instance)) {
        // Be more restrictive about taking days off when we need to meet minimum minutes
        bool can_take_day_off = true;
        
        if (emp.min_total_minutes > 0 && current.total_minutes < emp.min_total_minutes) {
            int days_remaining_after_off = instance.horizon_length - day - 1;
            int min_days_needed = estimateMinDaysForMinutes(emp, all_sequences, current.total_minutes);
            
            // Don't take day off if we're cutting it too close to minimum minutes requirement
            if (min_days_needed >= days_remaining_after_off) {
                can_take_day_off = false;
            }
        }
        
        if (can_take_day_off) {
            backtrackEmployeeSchedule(instance, emp, all_sequences, day + 1, current, results, max_solutions);
        }
    }
}


bool isEmployeeFeasible(const EmployeeSchedule& schedule, const ProblemInstance& instance) {
    const Employee& emp = instance.employees[schedule.employee_id];
    
    // Build complete schedule for final validation
    vector<int> full_schedule(instance.horizon_length, 0);
    for (const auto& ps : schedule.placed_sequences) {
        for (int i = 0; i < ps.second.length; i++) {
            int day = ps.first + i;
            if (day < instance.horizon_length) {
                full_schedule[day] = ps.second.shifts[i];
            }
        }
    }
    
    // R1: Mandatory days off
    for (int day_off : emp.days_off) {
        if (day_off < instance.horizon_length && full_schedule[day_off] != 0) {
            return false;
        }
    }
    
    // R2: Max shifts per type
    for (int shift_id = 0; shift_id < schedule.shift_count.size(); shift_id++) {
        if (!emp.max_shifts.empty() && shift_id < emp.max_shifts.size()) {
            if (emp.max_shifts[shift_id] > 0 && schedule.shift_count[shift_id] > emp.max_shifts[shift_id]) {
                return false;
            }
        }
    }
    
    // R4: Min consecutive days off
    if (emp.min_consecutive_days_off > 1) {
        int consecutive_off = 0;
        bool in_off_period = false;
        
        for (int day = 0; day < instance.horizon_length; day++) {
            if (full_schedule[day] == 0) {
                if (day == 0) {
                    consecutive_off = instance.horizon_length;
                }
                consecutive_off++;
                in_off_period = true;
            } else {
                if (in_off_period && consecutive_off < emp.min_consecutive_days_off) {
                    return false;
                }
                consecutive_off = 0;
                in_off_period = false;
            }
        }
    }
    
    // R6: Max weekends
    if (emp.max_weekends > 0) {
        int weekend_count = countWeekendsInSchedule(schedule, instance);
        if (weekend_count > emp.max_weekends) {
            return false;
        }
    }
    
    // R7: Max total minutes
    if (emp.max_total_minutes > 0 && schedule.total_minutes > emp.max_total_minutes) {
        return false;
    }
    
    // R8: Min total minutes
    if (emp.min_total_minutes > 0 && schedule.total_minutes < emp.min_total_minutes) {
        return false;
    }
    
    return true;
}