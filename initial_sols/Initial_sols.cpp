#include "ProblemInstance.h"
#include "ColumnGenerationBase.h"
#include "HeuristicMethod.h"
#include "RandomMethod.h"
#include <iostream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <algorithm>


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <instance_file_path>" << std::endl;
        std::cout << "Example: " << argv[0] << " b-Instancia14_cap2_relacion7UnoUnoUnoTodosDistintos.dat" << std::endl;
        return 1;
    }

    std::string instancePath = argv[1];
    
    std::cout << "Reading instance file: " << instancePath << std::endl;
    
    ProblemInstance problem;
    
    if (problem.readInputFile(instancePath)) {
        std::cout << "File read successfully!" << std::endl;
        problem.printSummary();

        ColumnGenerator generator(&problem);
    
        std::vector<std::pair<std::string, std::unique_ptr<ColumnGenerationMethod>>> methods;
        // methods.push_back({"Heuristic", std::make_unique<HeuristicMethod>(&problem)});
        methods.push_back({"Random", std::make_unique<RandomMethod>(&problem)});
   
        
        int columns_per_employee = 5;

        // Para guardar resultados de cada método
        struct MethodStats {
            std::string name;
            long long time_ms;
            int total_columns;
            int feasible_columns;
            double feasibility_rate;
            double avg_cost;
            double avg_violations;
        };
        std::vector<MethodStats> summary;

        for (auto& [method_name, method] : methods) {
            std::cout << "\n" << std::string(50, '=') << std::endl;
            std::cout << "Testing " << method_name << " Method" << std::endl;
            std::cout << std::string(50, '=') << std::endl;
            
            // Set the method
            generator.setMethod(std::move(method));
            
            // Measure time
            auto start = std::chrono::high_resolution_clock::now();
            
            // Generate columns
            generator.generateInitialPool(columns_per_employee);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            // Calculate results
            const auto& pool = generator.getPool();
            int total_columns = pool.getTotalColumns();
            int feasible_columns = 0;
            double total_cost = 0.0;
            double total_violations = 0.0;
            
            for (const auto& emp_pool : pool.employee_pools) {
                for (const auto& col : emp_pool) {
                    if (col.feasible) feasible_columns++;
                    total_cost += col.cost;
                    total_violations += col.constraint_violation;
                }
            }
            
            double feasibility_rate = (double)feasible_columns / total_columns * 100.0;
            double avg_cost = total_columns > 0 ? total_cost / total_columns : 0.0;
            double avg_violations = total_columns > 0 ? total_violations / total_columns : 0.0;

            std::cout << "\nTime taken: " << duration.count() << " ms" << std::endl;
            std::cout << "Total columns generated: " << total_columns << std::endl;
            std::cout << "Feasible columns: " << feasible_columns << std::endl;
            std::cout << "Feasibility rate: " << std::fixed << std::setprecision(2) << feasibility_rate << "%" << std::endl;
            std::cout << "Average cost: " << std::fixed << std::setprecision(3) << avg_cost << std::endl;
            std::cout << "Average violations: " << std::fixed << std::setprecision(3) << avg_violations << std::endl;
            
            // Print detailed pool information (only if not too many columns)
            if (total_columns <= 50000) {
                pool.printPool();
            } else {
                std::cout << "\n(Detailed pool info omitted - too many columns)" << std::endl;
            }
            
            // Guardar stats
            summary.push_back({method_name, duration.count(), total_columns, feasible_columns, 
                              feasibility_rate, avg_cost, avg_violations});
            
            // Clear pool for next method
            generator.getPool().employee_pools.clear();
            for (int i = 0; i < problem.employees.size(); i++) {
                generator.getPool().employee_pools.push_back(std::vector<Column>());
            }
        }

        // === Summary comparativo ===
        std::cout << "\n\n" << std::string(80, '=') << std::endl;
        std::cout << "SUMMARY COMPARISON BETWEEN METHODS" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        std::cout << std::left 
                  << std::setw(15) << "Method" 
                  << std::setw(12) << "Time(ms)" 
                  << std::setw(12) << "TotalCols"
                  << std::setw(14) << "FeasibleCols"
                  << std::setw(15) << "Feasibility(%)"
                  << std::setw(12) << "Avg Cost"
                  << "Avg Viol." 
                  << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        for (const auto& s : summary) {
            std::cout << std::left 
                      << std::setw(15) << s.name 
                      << std::setw(12) << s.time_ms 
                      << std::setw(12) << s.total_columns
                      << std::setw(14) << s.feasible_columns
                      << std::setw(15) << std::fixed << std::setprecision(1) << s.feasibility_rate
                      << std::setw(12) << std::fixed << std::setprecision(3) << s.avg_cost
                      << std::fixed << std::setprecision(3) << s.avg_violations
                      << std::endl;
        }

       
        
    } else {
        std::cerr << "Failed to read instance file: " << instancePath << std::endl;
        return 1;
    }
    
    return 0;
}