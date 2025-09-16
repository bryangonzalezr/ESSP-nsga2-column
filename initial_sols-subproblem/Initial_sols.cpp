#include "ProblemInstance.h"
#include "ScheduleSearch.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <climits>
#include <numeric>

using namespace std;




// void printEmployeeSchedule(const EmployeeSchedule& schedule, const ProblemInstance& instance) {
//     int horizon = instance.horizon_length;
//     // Build full schedule initialized as OFF (0)
//     vector<int> full(horizon, 0);
//     for (const auto& ps : schedule.placed_sequences) {
//         int start = ps.first;
//         const Sequence& seq = ps.second;
//         for (int i = 0; i < seq.length; ++i) {
//             int day = start + i;
//             if (day < horizon) full[day] = seq.shifts[i];
//         }
//     }
//     // Print as ---DD--DD format
//     for (int day = 0; day < horizon; ++day) {
//         int shift_id = full[day];
//         if (shift_id == 0) {
//             cout << "-";
//         } else {
//             cout << instance.shifts[shift_id].name[0]; // First letter of shift name
//         }
//     }
//     cout << endl;
// }

// void backtrackWorkingSequence(
//     const ProblemInstance& instance,
//     const Employee& emp,
//     vector<int>& current,
//     vector<int>& shift_count,
//     vector<Sequence>& results
// ) {
//     // --- Stop if sequence is too long ---
//     if (emp.max_consecutive_shifts > 0 && (int)current.size() > emp.max_consecutive_shifts)
//         return;

//     // --- If within bounds, store it ---
//     if ((int)current.size() >= emp.min_consecutive_shifts &&
//         (emp.max_consecutive_shifts == 0 || (int)current.size() <= emp.max_consecutive_shifts)) {

//         int total_minutes = 0;
//         for (int s : current) {
//             if (s > 0 && s < (int)instance.shifts.size())
//                 total_minutes += instance.shifts[s].length;
//         }
//         results.push_back({current, (int)current.size(), total_minutes});
//     }

//     int num_shifts = instance.shifts.size();
//     for (int shift_id = 1; shift_id < num_shifts; ++shift_id) { // exclude "off" (0)
//         // --- R2: Max shifts per type (local check within sequence) ---
//         if (!emp.max_shifts.empty() && shift_id < (int)emp.max_shifts.size()) {
//             if (emp.max_shifts[shift_id] > 0 && shift_count[shift_id] + 1 > emp.max_shifts[shift_id])
//                 continue;
//         }

//         // --- R3: Incompatible consecutive shifts ---
//         if (!current.empty()) {
//             int prev_shift = current.back();
//             const auto& incompatible = instance.shifts[prev_shift].incompatible_shifts;
            
//             if (find(incompatible.begin(), incompatible.end(), shift_id) != incompatible.end())
//                 continue;
//         }

//         // --- Choose ---
//         current.push_back(shift_id);
//         shift_count[shift_id]++;

//         backtrackWorkingSequence(instance, emp, current, shift_count, results);

//         // --- Undo ---
//         shift_count[shift_id]--;
//         current.pop_back();
//     }
// }





int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <instance_file_path>" << endl;
        return 1;
    }

    string instancePath = argv[1];
    ProblemInstance problem;

    if (!problem.readInputFile(instancePath)) {
        cerr << "Failed to read instance file: " << instancePath << endl;
        return 1;
    }

    problem.printSummary();

    // --- 1. Generar secuencias factibles por empleado ---
    for (int emp_id = 0; emp_id < (int)problem.employees.size(); ++emp_id) {
        auto sequences = generateWorkingSequences(problem, emp_id);

        cout << "Generated " << sequences.size()
             << " feasible working sequences for employee " << emp_id << endl;

        for (int i = 0; i < min(10, (int)sequences.size()); i++) { // Mostrar solo 10
            cout << "Seq " << i + 1 << ": ";
            for (int s : sequences[i].shifts)
                cout << problem.shifts[s].name << " ";
            cout << " (Length: " << sequences[i].length << ")";
            cout << " (Total Minutes: " << sequences[i].total_minutes << ")";
            cout << " (Shift Counts: ";
            for (const auto& sc : sequences[i].shift_count) {
                cout << "[Shift " << sc.first << ": " << sc.second << "] ";
            }
            cout << endl;
        }
        cout << endl;
    }

    // --- 2. Ejecutar GA ---
    cout << "\n=== Running Genetic Algorithm ===\n" << endl;
    ScheduleSearch ga(problem);

    Individual best = ga.runGA();

    cout << "\n=== FINAL BEST SOLUTION ===\n";
    cout << "Is Feasible: " << (best.is_feasible ? "YES" : "NO") << endl;
    cout << "Total Cost: " << best.cost << endl;
    cout << "  - Objective Cost: " << best.objective_cost << endl;
    cout << "  - Feasibility Penalty: " << best.feasibility_penalty << endl;
    cout << "Total Preference: " << best.total_preference << endl;
    cout << "\n";

    // Mostrar estadísticas de factibilidad por empleado
    int feasible_employees = 0;
    for (const auto& sched : best.schedules) {
        if (isEmployeeFeasible(sched, problem)) {
            feasible_employees++;
        }
    }
    cout << "Feasible Employees: " << feasible_employees << "/" << best.schedules.size() << endl;

    // Mostrar horarios detallados
    ga.printIndividual(best, problem);

    // Resumen rápido de horarios (opcional, descomentado)
    cout << "\n=== SCHEDULE SUMMARY ===\n";
    for (const auto& sched : best.schedules) {
        bool emp_feasible = isEmployeeFeasible(sched, problem);
        cout << "Employee " << sched.employee_id 
             << " [" << (emp_feasible ? "FEASIBLE" : "INFEASIBLE") << "]: ";
        
        for (const auto& ps : sched.placed_sequences) {
            cout << "[Day " << ps.first << ": ";
            for (int s : ps.second.shifts) {
                if (s == 0) cout << "OFF ";
                else cout << problem.shifts[s].name << " ";
            }
            cout << "] ";
        }
        cout << "(Minutes: " << sched.total_minutes << ")";
        cout << endl;
    }

    return 0;
}

