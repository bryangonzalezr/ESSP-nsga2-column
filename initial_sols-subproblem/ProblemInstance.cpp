#include "ProblemInstance.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

void ProblemInstance::findDefinition(std::ifstream& file, const std::string& def) {
    std::string word;
    file.clear();
    file.seekg(0, std::ios::beg);
    while (file >> word) {
        if (word == def) break;
    }
}

void ProblemInstance::removeSemicolon(std::string& line) {
    auto pos = line.find(';');
    if (pos != std::string::npos) {
        line = line.substr(0, pos);
    }
}

int ProblemInstance::countWords(const std::string& line) {
    std::istringstream iss(line);
    std::string word;
    int count = 0;
    while (iss >> word) {
        count++;
    }
    return count;
}

bool ProblemInstance::readInputFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cout << "File does not exist: " << filePath << std::endl;
        return false;
    }

    try {
        readEmployees(file);
        readEmployeeDaysOff(file);
        readHorizon(file);
        readShifts(file);
        readShiftLengths(file);
        readMaxShifts(file);
        readIncompatibleShifts(file);
        readMinMaxMinutes(file);
        readConsecutiveShifts(file);
        readConsecutiveDaysOff(file);
        readMaxWeekends(file);
        readShiftOnOffRequests(file);
        readCoverRequirements(file);
        readCoverWeights(file);

        std::cout << "Successfully read input file" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error reading input file: " << e.what() << std::endl;
        return false;
    }
}

void ProblemInstance::readEmployees(std::ifstream& file) {
    findDefinition(file, "I:=");
    std::string line;
    std::getline(file, line); // consume rest of current line
    std::cout<<line<<std::endl;

    removeSemicolon(line);
    
    std::istringstream iss(line);
    std::string name;
    int id = 0;
    
    employees.clear();
    while (iss >> name) {
        employees.emplace_back(id++, name);
    }
    
    std::cout << "Employees: " << employees.size() << std::endl;
}

void ProblemInstance::readEmployeeDaysOff(std::ifstream& file) {
    for (auto& emp : employees) {
        std::string set_name = "N[" + emp.name + "]:=";
        findDefinition(file, set_name);
        
        std::string line;
        std::getline(file, line); // consume rest of current line
        
        emp.days_off.clear();
        while (std::getline(file, line) && !line.empty() && line[0] != ';') {
            std::istringstream iss(line);
            int day;
            if (iss >> day) {
                emp.days_off.push_back(day - 1); // Convert to 0-based index
            }
        }
    }
}

void ProblemInstance::readHorizon(std::ifstream& file) {
    findDefinition(file, "h:=");
    file >> horizon_length;
}

void ProblemInstance::readShifts(std::ifstream& file) {
    findDefinition(file, "T:=");
    std::string line;
    std::getline(file, line); 
    removeSemicolon(line);
    
    shifts.clear();
    // Add empty shift
    shifts.emplace_back(0, "-");
    shifts[0].length = 0;
    
    std::istringstream iss(line);
    std::string name;
    int id = 1;
    
    while (iss >> name) {
        shifts.emplace_back(id++, name);
    }
}

void ProblemInstance::readIncompatibleShifts(std::ifstream& file) {
    for (auto& shift : shifts) {
        std::string set_name = "R[" + shift.name + "]:=";
        findDefinition(file, set_name);

        std::string line;
        shift.incompatible_shifts.clear();
        std::string all_names;

        // Read lines until we find a line containing ';'
        while (std::getline(file, line)) {
            auto semicolon_pos = line.find(';');
            if (semicolon_pos != std::string::npos) {
                // Only take up to the semicolon
                all_names += " " + line.substr(0, semicolon_pos);
                break;
            } else {
                all_names += " " + line;
            }
        }

        // Remove leading/trailing whitespace
        all_names.erase(0, all_names.find_first_not_of(" \t\r\n"));
        all_names.erase(all_names.find_last_not_of(" \t\r\n") + 1);

        std::istringstream iss(all_names);
        std::string incompatible_name;
        while (iss >> incompatible_name) {
            for (size_t i = 0; i < shifts.size(); ++i) {
                if (shifts[i].name == incompatible_name) {
                    shift.incompatible_shifts.push_back(i);
                    break;
                }
            }
        }
    }
    // Debug print
    for (const auto& shift : shifts) {
        std::cout << "Shift " << shift.name << " incompatible with: ";
        for (int incomp : shift.incompatible_shifts) {
            std::cout << shifts[incomp].name << " ";
        }
        std::cout << std::endl;
    }
}

void ProblemInstance::readShiftLengths(std::ifstream& file) {
    findDefinition(file, "l:=");
    std::string line;
    std::getline(file, line); // consume rest of current line
    
    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        std::istringstream iss(line);
        std::string shift_name;
        int length;
        
        if (iss >> shift_name >> length) {
            for (auto& shift : shifts) {
                if (shift.name == shift_name) {
                    shift.length = length;
                    break;
                }
            }
        }
    }
}

void ProblemInstance::readMaxShifts(std::ifstream& file) {
    findDefinition(file, "m:=");
    std::string line;
    std::getline(file, line); // consume rest of current line
    
    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        // Parse format: [emp_name,shift_name] max_shifts
        auto bracket_start = line.find('[');
        auto bracket_end = line.find(']');
        auto comma_pos = line.find(',', bracket_start);
        
        if (bracket_start != std::string::npos && bracket_end != std::string::npos && 
            comma_pos != std::string::npos) {
            
            std::string emp_name = line.substr(bracket_start + 1, comma_pos - bracket_start - 1);
            std::string shift_name = line.substr(comma_pos + 1, bracket_end - comma_pos - 1);
            
            std::string remaining = line.substr(bracket_end + 1);
            std::istringstream iss(remaining);
            int max_shifts;
            
            if (iss >> max_shifts) {
                for (auto& emp : employees) {
                    if (emp.name == emp_name) {
                        if (emp.max_shifts.empty()) {
                            emp.max_shifts.resize(shifts.size());
                            emp.max_shifts[0] = horizon_length; // empty shift
                        }
                        
                        for (size_t i = 0; i < shifts.size(); ++i) {
                            if (shifts[i].name == shift_name) {
                                emp.max_shifts[i] = max_shifts;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    //debug print   
    for (const auto& emp : employees) {
        std::cout << "Max shifts for Employee " << emp.name << ": ";
        for (size_t i = 0; i < emp.max_shifts.size(); ++i) {
            if (emp.max_shifts[i] > 0) {
                std::cout << "[" << shifts[i].name << ": " << emp.max_shifts[i] << "] ";
            }
        }
        std::cout << std::endl;
    }
}

void ProblemInstance::readMinMaxMinutes(std::ifstream& file) {
    // Read min minutes
    findDefinition(file, "b:=");
    std::string line;
    std::getline(file, line); // consume rest of current line
    
    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        std::istringstream iss(line);
        std::string emp_name;
        int min_minutes;
        
        if (iss >> emp_name >> min_minutes) {
            for (auto& emp : employees) {
                if (emp.name == emp_name) {
                    emp.min_total_minutes = min_minutes;
                    break;
                }
            }
        }
    }
    
    // Read max minutes
    findDefinition(file, "c:=");
    std::getline(file, line); // consume rest of current line
    
    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        std::istringstream iss(line);
        std::string emp_name;
        int max_minutes;
        
        if (iss >> emp_name >> max_minutes) {
            for (auto& emp : employees) {
                if (emp.name == emp_name) {
                    emp.max_total_minutes = max_minutes;
                    break;
                }
            }
        }
    }
}

void ProblemInstance::readConsecutiveShifts(std::ifstream& file) {
    // Read min consecutive shifts
    findDefinition(file, "f:=");
    std::string line;
    std::getline(file, line); // consume rest of current line
    
    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        std::istringstream iss(line);
        std::string emp_name;
        int min_consecutive;
        
        if (iss >> emp_name >> min_consecutive) {
            for (auto& emp : employees) {
                if (emp.name == emp_name) {
                    emp.min_consecutive_shifts = min_consecutive;
                    break;
                }
            }
        }
    }
    
    // Read max consecutive shifts
    findDefinition(file, "g:=");
    std::getline(file, line); // consume rest of current line
    
    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        std::istringstream iss(line);
        std::string emp_name;
        int max_consecutive;
        
        if (iss >> emp_name >> max_consecutive) {
            for (auto& emp : employees) {
                if (emp.name == emp_name) {
                    emp.max_consecutive_shifts = max_consecutive;
                    break;
                }
            }
        }
    }
}

void ProblemInstance::readConsecutiveDaysOff(std::ifstream& file) {
    findDefinition(file, "o:=");
    std::string line;
    std::getline(file, line); // consume rest of current line
    
    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        std::istringstream iss(line);
        std::string emp_name;
        int min_days_off;
        
        if (iss >> emp_name >> min_days_off) {
            for (auto& emp : employees) {
                if (emp.name == emp_name) {
                    emp.min_consecutive_days_off = min_days_off;
                    break;
                }
            }
        }
    }
}

void ProblemInstance::readMaxWeekends(std::ifstream& file) {
    findDefinition(file, "a:=");
    std::string line;
    std::getline(file, line); // consume rest of current line
    
    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        std::istringstream iss(line);
        std::string emp_name;
        int max_weekends;
        
        if (iss >> emp_name >> max_weekends) {
            for (auto& emp : employees) {
                if (emp.name == emp_name) {
                    emp.max_weekends = max_weekends;
                    break;
                }
            }
        }
    }
}

void ProblemInstance::readShiftOnOffRequests(std::ifstream& file) {
    // Initialize per-employee request matrices
    for (auto& emp : employees) {
        emp.shift_on_requests.assign(horizon_length, std::vector<int>(shifts.size(), 0));
        emp.shift_off_requests.assign(horizon_length, std::vector<int>(shifts.size(), 0));
    }

    // Read shift on requests
    findDefinition(file, "q:=");
    std::string line;
    std::getline(file, line); // consume rest of current line

    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        auto bracket_start = line.find('[');
        auto bracket_end = line.find(']');

        if (bracket_start != std::string::npos && bracket_end != std::string::npos) {
            std::string content = line.substr(bracket_start + 1, bracket_end - bracket_start - 1);

            auto first_comma = content.find(',');
            auto second_comma = content.find(',', first_comma + 1);

            if (first_comma != std::string::npos && second_comma != std::string::npos) {
                std::string emp_name = content.substr(0, first_comma);
                std::string day_str = content.substr(first_comma + 1, second_comma - first_comma - 1);
                std::string shift_name = content.substr(second_comma + 1);

                int day = std::stoi(day_str) - 1;

                std::string remaining = line.substr(bracket_end + 1);
                std::istringstream iss(remaining);
                int weight;

                if (iss >> weight) {
                    int emp_idx = -1, shift_idx = -1;

                    for (size_t i = 0; i < employees.size(); ++i) {
                        if (employees[i].name == emp_name) {
                            emp_idx = i;
                            break;
                        }
                    }

                    for (size_t i = 0; i < shifts.size(); ++i) {
                        if (shifts[i].name == shift_name) {
                            shift_idx = i;
                            break;
                        }
                    }

                    if (emp_idx != -1 && shift_idx != -1) {
                        employees[emp_idx].shift_on_requests[day][shift_idx] = weight;
                    }
                }
            }
        }
    }

    // Read shift off requests
    findDefinition(file, "p:=");
    std::getline(file, line); // consume rest of current line

    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        auto bracket_start = line.find('[');
        auto bracket_end = line.find(']');

        if (bracket_start != std::string::npos && bracket_end != std::string::npos) {
            std::string content = line.substr(bracket_start + 1, bracket_end - bracket_start - 1);

            auto first_comma = content.find(',');
            auto second_comma = content.find(',', first_comma + 1);

            if (first_comma != std::string::npos && second_comma != std::string::npos) {
                std::string emp_name = content.substr(0, first_comma);
                std::string day_str = content.substr(first_comma + 1, second_comma - first_comma - 1);
                std::string shift_name = content.substr(second_comma + 1);

                int day = std::stoi(day_str) - 1;

                std::string remaining = line.substr(bracket_end + 1);
                std::istringstream iss(remaining);
                int weight;

                if (iss >> weight) {
                    int emp_idx = -1, shift_idx = -1;

                    for (size_t i = 0; i < employees.size(); ++i) {
                        if (employees[i].name == emp_name) {
                            emp_idx = i;
                            break;
                        }
                    }

                    for (size_t i = 0; i < shifts.size(); ++i) {
                        if (shifts[i].name == shift_name) {
                            shift_idx = i;
                            break;
                        }
                    }

                    if (emp_idx != -1 && shift_idx != -1) {
                        employees[emp_idx].shift_off_requests[day][shift_idx] = weight;
                    }
                }
            }
        }
    }

    //debug print
    // for (const auto& emp : employees) {
    //     std::cout << "Shift ON requests for Employee " << emp.name << ":\n";
    //     for (int d = 0; d < horizon_length; ++d) {
    //         for (size_t s = 0; s < shifts.size(); ++s) {
    //             if (emp.shift_on_requests[d][s] > 0) {
    //                 std::cout << "  Day " << (d) << ", Shift " << shifts[s].name 
    //                           << ", Weight " << emp.shift_on_requests[d][s] << std::endl;
    //             }
    //         }
    //     }
    //     std::cout << "Shift OFF requests for Employee " << emp.name << ":\n";
    //     for (int d = 0; d < horizon_length; ++d) {
    //         for (size_t s = 0; s < shifts.size(); ++s) {
    //             if (emp.shift_off_requests[d][s] > 0) {
    //                 std::cout << "  Day " << (d) << ", Shift " << shifts[s].name 
    //                           << ", Weight " << emp.shift_off_requests[d][s] << std::endl;
    //             }
    //         }
    //     }
    // }
                                
    std::cout << "Finished reading shift on/off requests." << std::endl;
}

void ProblemInstance::readCoverRequirements(std::ifstream& file) {
    findDefinition(file, "s:=");
    std::string line;
    std::getline(file, line); // consume rest of current line
    
    cover_requirements.resize(horizon_length);
    for (int i = 0; i < horizon_length; ++i) {
        cover_requirements[i].resize(shifts.size(), 0);
    }
    
    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        // Parse format: [day,shift_name] requirement
        auto bracket_start = line.find('[');
        auto bracket_end = line.find(']');
        auto comma_pos = line.find(',', bracket_start);
        
        if (bracket_start != std::string::npos && bracket_end != std::string::npos && 
            comma_pos != std::string::npos) {
            
            std::string day_str = line.substr(bracket_start + 1, comma_pos - bracket_start - 1);
            std::string shift_name = line.substr(comma_pos + 1, bracket_end - comma_pos - 1);
            
            int day = std::stoi(day_str) - 1; // Convert to 0-based
            
            std::string remaining = line.substr(bracket_end + 1);
            std::istringstream iss(remaining);
            int requirement;
            
            if (iss >> requirement) {
                for (size_t i = 0; i < shifts.size(); ++i) {
                    if (shifts[i].name == shift_name) {
                        cover_requirements[day][i] = requirement;
                        break;
                    }
                }
            }
        }
    }
}

void ProblemInstance::readCoverWeights(std::ifstream& file) {
    // Initialize weight matrices
    under_cover_weights.resize(horizon_length);
    over_cover_weights.resize(horizon_length);
    
    for (int i = 0; i < horizon_length; ++i) {
        under_cover_weights[i].resize(shifts.size(), 0);
        over_cover_weights[i].resize(shifts.size(), 0);
    }
    
    // Read under cover weights
    findDefinition(file, "u:=");
    std::string line;
    std::getline(file, line); // consume rest of current line
    
    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        // Parse format: [day,shift_name] weight
        auto bracket_start = line.find('[');
        auto bracket_end = line.find(']');
        auto comma_pos = line.find(',', bracket_start);
        
        if (bracket_start != std::string::npos && bracket_end != std::string::npos && 
            comma_pos != std::string::npos) {
            
            std::string day_str = line.substr(bracket_start + 1, comma_pos - bracket_start - 1);
            std::string shift_name = line.substr(comma_pos + 1, bracket_end - comma_pos - 1);
            
            int day = std::stoi(day_str) - 1; // Convert to 0-based
            
            std::string remaining = line.substr(bracket_end + 1);
            std::istringstream iss(remaining);
            int weight;
            
            if (iss >> weight) {
                for (size_t i = 0; i < shifts.size(); ++i) {
                    if (shifts[i].name == shift_name) {
                        under_cover_weights[day][i] = weight;
                        break;
                    }
                }
            }
        }
    }
    
    // Read over cover weights
    findDefinition(file, "v:=");
    std::getline(file, line); // consume rest of current line
    
    while (std::getline(file, line) && !line.empty() && line[0] != ';') {
        // Same parsing logic as under cover weights
        auto bracket_start = line.find('[');
        auto bracket_end = line.find(']');
        auto comma_pos = line.find(',', bracket_start);
        
        if (bracket_start != std::string::npos && bracket_end != std::string::npos && 
            comma_pos != std::string::npos) {
            
            std::string day_str = line.substr(bracket_start + 1, comma_pos - bracket_start - 1);
            std::string shift_name = line.substr(comma_pos + 1, bracket_end - comma_pos - 1);
            
            int day = std::stoi(day_str) - 1; // Convert to 0-based
            
            std::string remaining = line.substr(bracket_end + 1);
            std::istringstream iss(remaining);
            int weight;
            
            if (iss >> weight) {
                for (size_t i = 0; i < shifts.size(); ++i) {
                    if (shifts[i].name == shift_name) {
                        over_cover_weights[day][i] = weight;
                        break;
                    }
                }
            }
        }
    }
}

void ProblemInstance::printSummary() const {
    std::cout << "\n=== Problem Instance Summary ===" << std::endl;
    std::cout << "Horizon Length: " << horizon_length << std::endl;
    std::cout << "Number of Employees: " << employees.size() << std::endl;
    std::cout << "Number of Shifts: " << shifts.size() << std::endl;
    
    std::cout << "\nEmployees:" << std::endl;
    for (const auto& emp : employees) {
        std::cout << "  ID: " << emp.id << ", Name: " << emp.name;
        if (!emp.days_off.empty()) {
            std::cout << ", Days off: ";
            for (int day : emp.days_off) {
                std::cout << day + 1 << " "; // Convert back to 1-based for display
            }
        }
        std::cout << std::endl;
    }
    
    std::cout << "\nShifts:" << std::endl;
    for (const auto& shift : shifts) {
        std::cout << "  ID: " << shift.id << ", Name: " << shift.name 
                  << ", Length: " << shift.length << " minutes";
        if (!shift.incompatible_shifts.empty()) {
            std::cout << ", Incompatible with: ";
            for (int incomp : shift.incompatible_shifts) {
                std::cout << shifts[incomp].name << " ";
            }
        }
        std::cout << std::endl;
    }
}