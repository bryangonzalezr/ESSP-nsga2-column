#ifndef RANDOM_METHOD_H
#define RANDOM_METHOD_H

#include "ColumnGenerationBase.h"

class RandomMethod : public ColumnGenerationMethod {
public:
    RandomMethod(ProblemInstance* inst) : ColumnGenerationMethod(inst) {}
    
    Column generateColumn(int employee_id) override;
    std::string getMethodName() const override { return "Random Method"; }

private:
    int getMaxShiftLength() const;  // <-- no RandomMethod:: here
};

#endif
