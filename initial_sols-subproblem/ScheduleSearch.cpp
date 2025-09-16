#include "ScheduleSearch.h"

void backtrackWorkingSequence(
    const ProblemInstance& instance,
    const Employee& emp,
    vector<int>& current,
    vector<int>& shift_count,
    vector<Sequence>& results
) {
    // --- Stop if sequence is too long ---
    if (emp.max_consecutive_shifts > 0 && (int)current.size() > emp.max_consecutive_shifts)
        return;

    // --- If within bounds, store it ---
    if ((int)current.size() >= emp.min_consecutive_shifts &&
        (emp.max_consecutive_shifts == 0 || (int)current.size() <= emp.max_consecutive_shifts)) {

        int total_minutes = 0;
        for (int s : current) {
            if (s > 0 && s < (int)instance.shifts.size())
                total_minutes += instance.shifts[s].length;
        }
        //add shift_count vector<pair<int,int>>

        vector<pair<int,int>> sc;
        for(int i=0;i<shift_count.size();i++){
            if(shift_count[i]>0){
                sc.push_back({i,shift_count[i]});
            }
        }

        results.push_back({current, sc, (int)current.size(), total_minutes});
    }

    int num_shifts = instance.shifts.size();
    for (int shift_id = 1; shift_id < num_shifts; ++shift_id) { // exclude "off" (0)
        // --- R2: Max shifts per type (local check within sequence) ---
        if (shift_id < (int)emp.max_shifts.size()) {
        int max_allowed = emp.max_shifts[shift_id];

        // Si no se permite este turno o se supera el límite
        if (max_allowed == 0 || shift_count[shift_id] >= max_allowed) {
            if(emp.name=="E"){
                cout<<"Skipping shift "<<shift_id<<" for employee "<<emp.name
                    << " Current Count: "<<shift_count[shift_id]
                    <<" Max Allowed: "<<max_allowed<<endl;
            }
            continue; // saltar este turno
        }
    }

        // --- R3: Incompatible consecutive shifts ---
        if (!current.empty()) {
            int prev_shift = current.back();
            const auto& incompatible = instance.shifts[prev_shift].incompatible_shifts;
            
            if (find(incompatible.begin(), incompatible.end(), shift_id) != incompatible.end())
                continue;
        }

        // --- Choose ---
        current.push_back(shift_id);
        shift_count[shift_id]++;

        backtrackWorkingSequence(instance, emp, current, shift_count, results);

        // --- Undo ---
        shift_count[shift_id]--;
        current.pop_back();
    }
}


vector<Sequence> generateWorkingSequences(
    const ProblemInstance& instance,
    int employee_id
) {
    const Employee& emp = instance.employees[employee_id];
    vector<Sequence> results;
    vector<int> current;
    vector<int> shift_count(instance.shifts.size(), 0);

    backtrackWorkingSequence(instance, emp, current, shift_count, results);

    return results;
}

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

bool isEmployeeFeasible(const EmployeeSchedule& schedule, const ProblemInstance& instance) {
    const Employee& emp = instance.employees[schedule.employee_id];

    vector<int> full_schedule(instance.horizon_length, 0);
    for (const auto& ps : schedule.placed_sequences) {
        for (int i = 0; i < ps.second.length; i++) {
            int day = ps.first + i;
            if (day < instance.horizon_length) {
                if (full_schedule[day] != 0) {
                    // ya había algo en este día => solapamiento
                    return false;
                }
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
    //sequences have shift_count vector<pair<int,int> now
    for (const auto& sc : schedule.shift_count) {
        if (sc > 0 && sc < (int)emp.max_shifts.size()) {
            if (emp.max_shifts[&sc - &schedule.shift_count[0]] > 0 &&
                sc > emp.max_shifts[&sc - &schedule.shift_count[0]]) {
                return false;
            }
        }
    }



    // Max consecutive shifts

    if (emp.max_consecutive_shifts > 0) {
        // Ordenar por día inicial
        auto seqs = schedule.placed_sequences;
        sort(seqs.begin(), seqs.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        int consecutive_length = 0;
        int prev_end = -1; // día final de la secuencia anterior

        for (const auto& seq : seqs) {
            int start = seq.first;
            int len   = seq.second.length;
            int end   = start + len;

            if (prev_end == start) { 
                // secuencia pegada a la anterior
                consecutive_length += len;
            } else {
                // hay un hueco, reiniciamos contador
                consecutive_length = len;
            }

            if (consecutive_length > emp.max_consecutive_shifts) {
                return false; // excede el máximo permitido
            }

            prev_end = end;
        }
    }
    //incompatible shift checks (end shift of a sequence with start of next sequence on consecutive sequences)
    for (size_t s = 1; s < schedule.placed_sequences.size(); s++) {
        const auto& prev_seq = schedule.placed_sequences[s - 1];
        const auto& curr_seq = schedule.placed_sequences[s];

        int prev_end_day = prev_seq.first + prev_seq.second.length - 1; // último día de la secuencia anterior
        int curr_start_day = curr_seq.first;                               // primer día de la secuencia actual

        // Solo revisar si son secuencias consecutivas
        if (curr_start_day == prev_end_day + 1) {
            int prev_shift = prev_seq.second.shifts.back();
            int curr_shift = curr_seq.second.shifts.front();

            const auto& incompatible = instance.shifts[prev_shift].incompatible_shifts;
            if (std::find(incompatible.begin(), incompatible.end(), curr_shift) != incompatible.end()) {
                return false;
            }
        }
    }

    std::vector<int> shift_count_total(instance.shifts.size(), 0);

    for (const auto& ps : schedule.placed_sequences) {
        const Sequence& seq = ps.second;

        // Verificar cada tipo de turno dentro de la secuencia
        for (const auto& sc : seq.shift_count) {
            int shift_id = sc.first;
            int count = sc.second;

            if (shift_id < (int)shift_count_total.size() && shift_id < (int)emp.max_shifts.size()) {
                int new_total = shift_count_total[shift_id] + count;

                // Revisar si excede el máximo permitido
                if (emp.max_shifts[shift_id] > 0 && new_total > emp.max_shifts[shift_id]) {
                    // Aquí podrías rechazar esta secuencia o penalizar
                    std::cout << "Employee " << emp.name 
                            << " exceeds max shifts for shift " << shift_id 
                            << ": current " << shift_count_total[shift_id] 
                            << " + seq " << count 
                            << " > max " << emp.max_shifts[shift_id] << std::endl;
                    return false; // o continue, dependiendo de dónde lo uses
                }

                // Actualizar conteo total
                shift_count_total[shift_id] = new_total;
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


Population ScheduleSearch::initializePopulation() {
    Population pop;
    pop.pop_size = pop_size;
    pop.individuals.resize(pop_size);

    for (int i = 0; i < pop_size; i++) {
        Individual ind;

        for (int emp_id = 0; emp_id < (int)instance.employees.size(); emp_id++) {
            EmployeeSchedule sched;
            sched.employee_id = emp_id;
            sched.shift_count.assign(instance.shifts.size(), 0);

            const auto& sequences = employee_sequences[emp_id]; // 👈 reutilizamos el pool
            if (sequences.empty()) {
                ind.schedules.push_back(sched);
                continue;
            }

            int day = 0;
            for (int trial = 0; trial < 3 && day < instance.horizon_length; trial++) {
                int idx = rand() % sequences.size();
                const auto& seq = sequences[idx];

                if (day + seq.length <= instance.horizon_length) {
                    sched.placed_sequences.push_back(std::make_pair(day, seq));
                    sched.total_minutes += seq.total_minutes;
                    for (int s : seq.shifts) sched.shift_count[s]++;
                    day += seq.length + 1; // dejar 1 día libre
                }
            }

            ind.schedules.push_back(sched);
        }

        evaluate(ind);
        pop.individuals[i] = ind;
    }

    return pop;
}

double ScheduleSearch::evaluate(Individual& ind) {
    ind.cost = 0.0;
    ind.total_preference = 0.0;
    
    // Matriz para rastrear cobertura actual: [día][turno] = cantidad de empleados
    std::vector<std::vector<int>> current_coverage(instance.horizon_length, 
                                                  std::vector<int>(instance.shifts.size(), 0));

    for (auto& sched : ind.schedules) {
        const Employee& emp = instance.employees[sched.employee_id];

        // Recalcular total_minutes y shift_count
        sched.total_minutes = 0;
        sched.shift_count.assign(instance.shifts.size(), 0);

        // Construir horario completo (día -> turno trabajado o 0 si libre)
        std::vector<int> full_schedule(instance.horizon_length, 0);

        for (const auto& ps : sched.placed_sequences) {
            const Sequence& seq = ps.second;
            sched.total_minutes += seq.total_minutes;

            for (int i = 0; i < seq.length; i++) {
                int day = ps.first + i;
                if (day < instance.horizon_length) {
                    int shift_id = seq.shifts[i];
                    full_schedule[day] = shift_id;
                    if (shift_id >= 0 && shift_id < (int)sched.shift_count.size()) {
                        sched.shift_count[shift_id]++;
                        // Actualizar cobertura actual
                        current_coverage[day][shift_id]++;
                    }
                }
            }
        }

        // Penalizar si no es factible
        if (!isEmployeeFeasible(sched, instance)) {
            ind.cost += 1000000; // penalización fuerte
            continue;
        }

        // Evaluar preferencias de turnos usando el horario completo
        sched.total_preference = 0;
        for (int day = 0; day < instance.horizon_length; day++) {
            int shift_id = full_schedule[day];

            // Si no trabaja este día (shift_id == 0)
            if (shift_id == 0) {
                // Penalización si había un on-request para este día/turno
                for (int s = 0; s < (int)instance.shifts.size(); s++) {
                    int on_weight = emp.shift_on_requests[day][s];
                    if (on_weight > 0) {
                        sched.total_preference += on_weight; // penalizar por no cumplir
                    }
                }
                continue;
            }

            // Si trabaja este día, convertir shift_id a índice si es necesario
            int shift_index = shift_id; // Ajustar según tu indexación
            if (shift_index >= 0 && shift_index < (int)instance.shifts.size()) {
                
                // Penalización si había un off-request y lo trabaja igual
                int off_weight = emp.shift_off_requests[day][shift_index];
                if (off_weight > 0) {
                    sched.total_preference += off_weight; // Penalizar
                }
            }
        }

        // Costos acumulados de preferencias
        ind.total_preference += sched.total_preference;
        ind.cost += sched.total_preference;
    }

    // ============= CÁLCULO DE PENALIZACIÓN POR COBERTURA =============
    double coverage_penalty = 0.0;
    
    for (int day = 0; day < instance.horizon_length; day++) {
        for (int shift = 0; shift < (int)instance.shifts.size(); shift++) {
            int required = instance.cover_requirements[day][shift];
            int actual = current_coverage[day][shift];
            
            if (actual < required) {
                // UNDER COVERAGE: faltan empleados
                int under_count = required - actual;
                int under_weight = instance.under_cover_weights[day][shift];
                coverage_penalty += under_count * under_weight;
                
                // Debug opcional
                /*
                std::cout << "Under coverage - Day: " << (day + 1) 
                         << ", Shift: " << shift 
                         << ", Required: " << required 
                         << ", Actual: " << actual 
                         << ", Penalty: " << (under_count * under_weight) << std::endl;
                */
            }
            else if (actual > required) {
                // OVER COVERAGE: sobran empleados
                int over_count = actual - required;
                int over_weight = instance.over_cover_weights[day][shift];
                coverage_penalty += over_count * over_weight;
                
                // Debug opcional
                /*
                std::cout << "Over coverage - Day: " << (day + 1) 
                         << ", Shift: " << shift 
                         << ", Required: " << required 
                         << ", Actual: " << actual 
                         << ", Penalty: " << (over_count * over_weight) << std::endl;
                */
            }
            // Si actual == required, no hay penalización
        }
    }
    
    // Agregar penalización por cobertura al costo total
    ind.cost += coverage_penalty;
    
    // Opcional: almacenar la penalización por cobertura por separado
    // ind.coverage_penalty = coverage_penalty;
    
    return ind.cost;
}


Individual ScheduleSearch::tournamentSelection(const Population& pop, int k) {
    vector<Individual> candidates;
    for (int i = 0; i < k; i++) {
        int idx = rand() % pop.pop_size;
        candidates.push_back(pop.individuals[idx]);
    }
    return *min_element(candidates.begin(), candidates.end());
}

Individual ScheduleSearch::crossover(const Individual& p1, const Individual& p2) {
    Individual child;
    child.schedules.resize(p1.schedules.size());

    for (size_t i = 0; i < p1.schedules.size(); i++) {
        if (rand() % 2) child.schedules[i] = p1.schedules[i];
        else child.schedules[i] = p2.schedules[i];
    }

    evaluate(child);
    return child;
}

void ScheduleSearch::mutate(Individual& ind, double mutation_rate) {
    int horizon = instance.horizon_length -1;
    int num_emps = ind.schedules.size();

    auto hasOverlap = [](const vector<pair<int, Sequence>>& placed_sequences, 
                         int new_start, int new_length, int exclude_idx = -1) {
        for (int i = 0; i < (int)placed_sequences.size(); ++i) {
            if (i == exclude_idx) continue;
            int start = placed_sequences[i].first;
            int len = placed_sequences[i].second.length;
            if (max(start, new_start) < min(start + len, new_start + new_length)) {
                return true;
            }
        }
        return false;
    };

    // Función para eliminar solapamientos existentes
    auto removeOverlaps = [&hasOverlap](vector<pair<int, Sequence>>& sequences) {
        if (sequences.empty()) return;
        
        // Crear vector de índices y ordenarlos por día de inicio
        vector<int> indices(sequences.size());
        iota(indices.begin(), indices.end(), 0);
        
        sort(indices.begin(), indices.end(), 
             [&sequences](int a, int b) {
                 return sequences[a].first < sequences[b].first;
             });
        
        vector<pair<int, Sequence>> cleaned;
        for (int idx : indices) {
            const auto& seq = sequences[idx];
            if (!hasOverlap(cleaned, seq.first, seq.second.length)) {
                cleaned.push_back(seq);
            }
        }
        sequences = cleaned;
    };

    // Primero, eliminar todos los solapamientos existentes
    for (auto& sched : ind.schedules) {
        removeOverlaps(sched.placed_sequences);
    }

    for (auto& sched : ind.schedules) {
        if (((double)rand() / RAND_MAX) < mutation_rate) {
            int mutation_type = rand() % 5; // 5 operadores

            switch (mutation_type) {
            case 0: { // SequenceShift
                if (!sched.placed_sequences.empty()) {
                    int seq_idx = rand() % (int)sched.placed_sequences.size();
                    int seq_len = sched.placed_sequences[seq_idx].second.length;
                    int max_start = horizon - seq_len;
                    if (max_start >= 0) {
                        int tries = 10;
                        while (tries--) {
                            int new_day = rand() % (max_start + 1);
                            if (!hasOverlap(sched.placed_sequences, new_day, seq_len, seq_idx)) {
                                sched.placed_sequences[seq_idx].first = new_day;
                                break;
                            }
                        }
                    }
                }
                break;
            }
            case 1: { // SwapSequencesWithinEmployee
                if (sched.placed_sequences.size() > 1) {
                    int idx1 = rand() % (int)sched.placed_sequences.size();
                    int idx2 = rand() % (int)sched.placed_sequences.size();
                    if (idx1 != idx2) {
                        int start1 = sched.placed_sequences[idx1].first;
                        int start2 = sched.placed_sequences[idx2].first;
                        int len1 = sched.placed_sequences[idx1].second.length;
                        int len2 = sched.placed_sequences[idx2].second.length;

                        // Verificar que el intercambio no cause solapamientos
                        bool canSwap = true;
                        
                        // Verificar si seq1 en pos2 se solapa con otras (excluyendo seq2)
                        for (int k = 0; k < (int)sched.placed_sequences.size(); ++k) {
                            if (k == idx1 || k == idx2) continue;
                            int start_k = sched.placed_sequences[k].first;
                            int len_k = sched.placed_sequences[k].second.length;
                            if (max(start_k, start2) < min(start_k + len_k, start2 + len1)) {
                                canSwap = false;
                                break;
                            }
                        }
                        
                        // Verificar si seq2 en pos1 se solapa con otras (excluyendo seq1)
                        if (canSwap) {
                            for (int k = 0; k < (int)sched.placed_sequences.size(); ++k) {
                                if (k == idx1 || k == idx2) continue;
                                int start_k = sched.placed_sequences[k].first;
                                int len_k = sched.placed_sequences[k].second.length;
                                if (max(start_k, start1) < min(start_k + len_k, start1 + len2)) {
                                    canSwap = false;
                                    break;
                                }
                            }
                        }

                        if (canSwap) {
                            sched.placed_sequences[idx1].first = start2;
                            sched.placed_sequences[idx2].first = start1;
                        }
                    }
                }
                break;
            }
            case 2: { // SwapSequencesBetweenEmployees
                int other_emp = rand() % num_emps;
                if (other_emp != sched.employee_id &&
                    !sched.placed_sequences.empty() &&
                    !ind.schedules[other_emp].placed_sequences.empty()) {

                    int idx1 = rand() % (int)sched.placed_sequences.size();
                    int len1 = sched.placed_sequences[idx1].second.length;
                    int max_start1 = horizon - len1;

                    vector<int> candidates;
                    for (int j = 0; j < (int)ind.schedules[other_emp].placed_sequences.size(); j++) {
                        int len2 = ind.schedules[other_emp].placed_sequences[j].second.length;
                        int max_start2 = horizon - len2;
                        if (len2 == len1 && 
                            sched.placed_sequences[idx1].first <= max_start2 &&
                            ind.schedules[other_emp].placed_sequences[j].first <= max_start1) {
                            candidates.push_back(j);
                        }
                    }

                    if (!candidates.empty()) {
                        int idx2 = candidates[rand() % (int)candidates.size()];
                        
                        // Verificar que el intercambio no cause solapamientos
                        int start1 = sched.placed_sequences[idx1].first;
                        int start2 = ind.schedules[other_emp].placed_sequences[idx2].first;
                        int len2 = ind.schedules[other_emp].placed_sequences[idx2].second.length;
                        
                        bool canSwap = !hasOverlap(sched.placed_sequences, start2, len2, idx1) &&
                                      !hasOverlap(ind.schedules[other_emp].placed_sequences, start1, len1, idx2);
                        
                        if (canSwap) {
                            swap(sched.placed_sequences[idx1], 
                                 ind.schedules[other_emp].placed_sequences[idx2]);
                        }
                    }
                }
                break;
            }
            case 3: { // ReplaceSequenceFromPool
                if (!sched.placed_sequences.empty()) {
                    int seq_idx = rand() % (int)sched.placed_sequences.size();
                    const auto& pool = employee_sequences[sched.employee_id];
                    if (!pool.empty()) {
                        int tries = 10;
                        while (tries--) {
                            const Sequence& candidate = pool[rand() % (int)pool.size()];
                            int new_len = candidate.length;
                            int max_start = horizon - new_len;
                            if (max_start >= 0) {
                                int new_start = rand() % (max_start + 1);
                                if (!hasOverlap(sched.placed_sequences, new_start, new_len, seq_idx)) {
                                    sched.placed_sequences[seq_idx].first = new_start;
                                    sched.placed_sequences[seq_idx].second = candidate;
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
            }
            case 4: { // RelocateSequenceToOtherEmployee
                int other_emp = rand() % num_emps;
                if (other_emp != sched.employee_id && !sched.placed_sequences.empty()) {
                    int seq_idx = rand() % (int)sched.placed_sequences.size();
                    auto seq_pair = sched.placed_sequences[seq_idx];

                    int seq_len = seq_pair.second.length;
                    int max_start = horizon - seq_len;

                    if (max_start >= 0) {
                        int tries = 10;
                        while (tries--) {
                            int new_start = rand() % (max_start + 1);
                            if (new_start >= 0 && new_start + seq_len <= horizon &&
                                !hasOverlap(ind.schedules[other_emp].placed_sequences, new_start, seq_len)) {
                                // Insert en otro empleado
                                ind.schedules[other_emp].placed_sequences.push_back({new_start, seq_pair.second});
                                // Borrar del actual
                                sched.placed_sequences.erase(sched.placed_sequences.begin() + seq_idx);
                                break;
                            }
                        }
                    }
                }
                break;
            }
            }
            
            // Después de cada mutación, eliminar cualquier solapamiento que pueda haber quedado
            removeOverlaps(sched.placed_sequences);
        }
    }
    
    // Limpieza final para asegurar que no hay solapamientos
    for (auto& sched : ind.schedules) {
        removeOverlaps(sched.placed_sequences);
    }
    
    evaluate(ind);
}

#include <chrono>

Individual ScheduleSearch::bestIndividual(const Population& pop) {
    return *min_element(pop.individuals.begin(), pop.individuals.end());
}

Individual ScheduleSearch::runGA() {
    using namespace std::chrono;
    auto start_time = high_resolution_clock::now();

    Population pop = initializePopulation();
    Individual best = bestIndividual(pop);

    for (int gen = 0; gen < generations; gen++) {
        Population new_pop;
        new_pop.pop_size = pop_size;

        for (int i = 0; i < pop_size; i++) {
            Individual p1 = tournamentSelection(pop);
            Individual p2 = tournamentSelection(pop);
            Individual child = crossover(p1, p2);
            mutate(child, 0.1);
            new_pop.individuals.push_back(child);
        }

        best = min(best, bestIndividual(new_pop));
        pop = new_pop;

        // cout << "Generation " << gen << " best cost: " << best.cost << endl;
    }

    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end_time - start_time).count();
    cout << "GA execution time: " << duration << " ms" << endl;

    return best;
}
void ScheduleSearch::printIndividual(const Individual& ind, const ProblemInstance& instance) {
    string filename = "auxprint.out";
    ofstream outFile(filename);
    if (!outFile.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }
    cout << "Individual cost: " << ind.cost << endl;
    for (size_t emp = 0; emp < ind.schedules.size(); ++emp) {
        cout << "  Employee " << emp << ": ";
        printEmployeeSchedule(ind.schedules[emp], instance);
        //print each sequence
        for (const auto& ps : ind.schedules[emp].placed_sequences) {
            cout << "    Sequence starting day " << ps.first << " (length " << ps.second.length << "): ";
            for (int s : ps.second.shifts) {
                if (s == 0) cout << "-";
                else cout << instance.shifts[s].name[0];
            }
            cout << " | Minutes: " << ps.second.total_minutes << endl;
        }
        cout << "Total minutes: " << ind.schedules[emp].total_minutes << endl;
    }
    

    //sacar de nuevo pero con \t entre cada empleado
    outFile << "Compact Schedule:\n";
    for (size_t emp = 0; emp < ind.schedules.size(); ++emp) {

        // Copiar y ordenar secuencias por día de inicio
        auto sequences = ind.schedules[emp].placed_sequences;
        sort(sequences.begin(), sequences.end(), [](const auto& a, const auto& b){
            return a.first < b.first;
        });

        int day = 1; // asumir que los días empiezan en 1
        for (const auto& ps : sequences) {
            int start_day = ps.first + 1; // ajustar si tus días empiezan en 0 en la estructura
            // imprimir días vacíos antes de la secuencia
            for (; day < start_day; ++day) {
                outFile << "-\t";
            }
            // imprimir los turnos de la secuencia
            for (int s : ps.second.shifts) {
                if (s == 0) outFile << "-\t";
                else outFile << instance.shifts[s].name[0] << "\t";
                day++;
            }
        }
        // rellenar con "-" hasta el final del horizonte
        for (; day <= instance.horizon_length; ++day) {
            outFile << "-\t";
        }
        outFile << "\n";
    }
    outFile.close();
    
    
}

void ScheduleSearch::printEmployeeSchedule(const EmployeeSchedule& schedule, const ProblemInstance& instance) {
    int horizon = instance.horizon_length;
    vector<int> full(horizon, 0);
    for (const auto& ps : schedule.placed_sequences) {
        int start = ps.first;
        const Sequence& seq = ps.second;
        for (int i = 0; i < seq.length; ++i) {
            int day = start + i;
            if (day < horizon) full[day] = seq.shifts[i];
        }
    }
    for (int day = 0; day < horizon; ++day) {
        int shift_id = full[day];
        if (shift_id == 0) {
            cout << "-";
        } else {
            cout << instance.shifts[shift_id].name[0];
        }
    }
    cout << "Total Pref: " << schedule.total_preference;
    cout << endl;
}
