#ifndef HEURISTIC_METHOD_H
#define HEURISTIC_METHOD_H

#include "ColumnGenerationBase.h"

class HeuristicMethod : public ColumnGenerationMethod {
public:
    HeuristicMethod(ProblemInstance* inst) : ColumnGenerationMethod(inst) {}
    
    Column generateColumn(int employee_id) override;
    std::string getMethodName() const override { return "Heuristic Method"; }
};

#endif