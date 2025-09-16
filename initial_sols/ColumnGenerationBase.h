#ifndef COLUMN_GENERATION_BASE_H
#define COLUMN_GENERATION_BASE_H

#include "ProblemInstance.h"
#include <vector>
#include <random>
#include <memory>

// A column represents a complete schedule for one employee
struct Column {
    int employee_id;
    std::vector<int> shifts;  // shifts[day] = shift_id (0 = no shift)
    double cost;              // reduced cost or objective value
    bool feasible;
    double constraint_violation;  // Total constraint violations
    std::vector<double> constraint_details;  // Individual constraint violations
    double preference_cost;   // Employee preference violations
    
    Column(int emp_id, int horizon) : employee_id(emp_id), shifts(horizon, 0), 
                                     cost(0.0), feasible(true), constraint_violation(0.0),
                                     constraint_details(8, 0.0), preference_cost(0.0) {}
};

// Pool of columns for each employee
struct ColumnPool {
    std::vector<std::vector<Column>> employee_pools;  // employee_pools[emp_id] = vector of columns
    ProblemInstance* instance;  // Reference to instance for printing names
    
    ColumnPool(int num_employees, ProblemInstance* inst = nullptr) : employee_pools(num_employees), instance(inst) {}
    
    void addColumn(const Column& column);
    int getTotalColumns() const;
    void printPool() const;
};

// Base class for column generation methods
class ColumnGenerationMethod {
protected:
    ProblemInstance* instance;
    std::mt19937 rng;
    
public:
    ColumnGenerationMethod(ProblemInstance* inst) : instance(inst), rng(std::random_device{}()) {}
    virtual ~ColumnGenerationMethod() = default;
    
    // Pure virtual method to be implemented by each strategy
    virtual Column generateColumn(int employee_id) = 0;
    
    // Virtual method that can be overridden if needed
    virtual std::string getMethodName() const = 0;
    
    // Common utility methods
    bool isColumnFeasible(const Column& column);
    double calculateColumnCost(const Column& column);
    void evaluateColumn(Column& column);
};

// Main column generator class
class ColumnGenerator {
private:
    ProblemInstance* instance;
    ColumnPool pool;
    std::unique_ptr<ColumnGenerationMethod> method;
    
public:
    ColumnGenerator(ProblemInstance* inst) : instance(inst), pool(inst->employees.size(), inst) {}
    
    // Set the generation method
    void setMethod(std::unique_ptr<ColumnGenerationMethod> new_method) {
        method = std::move(new_method);
    }
    
    // Generate initial columns using the current method
    void generateInitialPool(int columns_per_employee = 10);
    
    // Get the column pool
    const ColumnPool& getPool() const { return pool; }
    ColumnPool& getPool() { return pool; }
    
    // Get current method name
    std::string getCurrentMethodName() const {
        return method ? method->getMethodName() : "No method set";
    }
};

#endif