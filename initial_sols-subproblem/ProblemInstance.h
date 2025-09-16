#ifndef SCHEDULING_READER_H
#define SCHEDULING_READER_H

#include <vector>
#include <string>
#include <memory>

class Shift {
public:
    int id;
    int length;
    std::string name;
    std::vector<int> incompatible_shifts;

    Shift() : id(0), length(0) {}
    Shift(int id, const std::string& name) : id(id), length(0), name(name) {}
};

class Employee {
public:
    int id;
    std::string name;
    std::vector<int> max_shifts;
    std::vector<int> days_off;
    int max_total_minutes;
    int min_total_minutes;
    int max_consecutive_shifts;
    int min_consecutive_shifts;
    int min_consecutive_days_off;
    int max_weekends;

    // NEW
    std::vector<std::vector<int>> shift_on_requests;   // [day][shift] = weight
    std::vector<std::vector<int>> shift_off_requests;  // [day][shift] = weight

    Employee() : id(0), max_total_minutes(0), min_total_minutes(0),
                 max_consecutive_shifts(0), min_consecutive_shifts(0),
                 min_consecutive_days_off(0), max_weekends(0) {}

    Employee(int id, const std::string& name) : id(id), name(name),
                 max_total_minutes(0), min_total_minutes(0),
                 max_consecutive_shifts(0), min_consecutive_shifts(0),
                 min_consecutive_days_off(0), max_weekends(0) {}
};


class ProblemInstance {
private:
    static const int MAX_LINE_LENGTH = 1024;

    void findDefinition(std::ifstream& file, const std::string& def);
    void removeSemicolon(std::string& line);
    int countWords(const std::string& line);
    
    void readEmployees(std::ifstream& file);
    void readEmployeeDaysOff(std::ifstream& file);
    void readHorizon(std::ifstream& file);
    void readShifts(std::ifstream& file);
    void readShiftLengths(std::ifstream& file);
    void readMaxShifts(std::ifstream& file);
    void readMinMaxMinutes(std::ifstream& file);
    void readConsecutiveShifts(std::ifstream& file);
    void readConsecutiveDaysOff(std::ifstream& file);
    void readMaxWeekends(std::ifstream& file);
    void readShiftOnOffRequests(std::ifstream& file);
    void readCoverRequirements(std::ifstream& file);
    void readCoverWeights(std::ifstream& file);
    void readIncompatibleShifts(std::ifstream& file);

public:
    int horizon_length;
    std::vector<Shift> shifts;
    std::vector<Employee> employees;
    std::vector<std::vector<int>> cover_requirements;
    std::vector<std::vector<int>> under_cover_weights;
    std::vector<std::vector<int>> over_cover_weights;
    std::vector<std::vector<std::vector<int>>> shift_on_requests;
    std::vector<std::vector<std::vector<int>>> shift_off_requests;

    ProblemInstance() : horizon_length(0) {}
    
    bool readInputFile(const std::string& filePath);
    void printSummary() const;
    
    // Getters
    int getNumEmployees() const { return employees.size(); }
    int getNumShifts() const { return shifts.size(); }
    int getHorizonLength() const { return horizon_length; }
};

#endif // SCHEDULING_READER_H