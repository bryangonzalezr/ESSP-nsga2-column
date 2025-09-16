#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"

#define MAX_LINE_LENGTH 1024



void readEmployees(FILE *f, problem_instance *pi);
void readHorizon(FILE *f, problem_instance *pi);
void readShifts(FILE *f, problem_instance *pi);
void readShiftLengths(FILE *f, problem_instance *pi);
void readMaxShifts(FILE *f, problem_instance *pi);
void readMinMaxMinutes(FILE *f, problem_instance *pi);
void readConsecutiveShifts(FILE *f, problem_instance *pi);
void readConsecutiveDaysOff(FILE *f, problem_instance *pi);
void readMaxWeekends(FILE *f, problem_instance *pi);
void readShiftOnOffRequests(FILE *f, problem_instance *pi);
void readCoverRequirements(FILE *f, problem_instance *pi);
void readCoverWeights(FILE *f, problem_instance *pi);
void readEmployeeDaysOff(FILE *f, problem_instance *pi);
void readIncompatibleShifts(FILE *f, problem_instance *pi);

void findDef(FILE *f, const char *def) {
    char word[MAX_LINE_LENGTH];
    while (fscanf(f, " %1023s", word) == 1) {
        if (strcmp(word, def) == 0) break;
    }
}

void removeSemicolon(char *line) {
    char *semicolon = strchr(line, ';');
    if (semicolon) *semicolon = '\0';
}
int countWords(const char *line) {
    if (line == NULL) {
        fprintf(stderr, "Input line is NULL.\n");
        return -1;
    }

    int words = 0;
    char *token, *copy;
    size_t len = strlen(line) + 1;  
    copy = (char *)malloc(len);
    if (copy == NULL) {
        perror("malloc");
        return -1;
    }
    strncpy(copy, line, len);

    token = strtok(copy, " \t\n");
    while (token != NULL) {
        words++;
        token = strtok(NULL, " \t\n");
    }

    free(copy);
    return words;
}



int readInputFile(const char* filePath, problem_instance *pi) {
    FILE* fh = fopen(filePath, "r");
    if (fh == NULL) {
        printf("File does not exist: %s\n", filePath);
        return 0;
    }

    readEmployees(fh, pi);
    

    readEmployeeDaysOff(fh, pi);


    readHorizon(fh, pi);

    readShifts(fh, pi);

    readShiftLengths(fh, pi);

    readMaxShifts(fh, pi);

    readIncompatibleShifts(fh, pi);

    readMinMaxMinutes(fh, pi);

    readConsecutiveShifts(fh, pi);

    readConsecutiveDaysOff(fh, pi);

    readMaxWeekends(fh, pi);

    readShiftOnOffRequests(fh, pi);

    readCoverRequirements(fh, pi);

    readCoverWeights(fh, pi);


    nreal = pi->num_employees * pi->horizon_length;
    nbin = 0;
    nobj = 2;
    ncon = 8;
    max_realvar = malloc(nreal * sizeof(double));
    min_realvar = malloc(nreal * sizeof(double));
    
    // for (int i = 0; i < nreal; i++) {
    //     max_realvar[i] = pi->num_shifts-1;
    //     min_realvar[i] = 0;
    // }

    //days off 
    for (int i = 0; i < pi->horizon_length; i++)
    {
        for (int j = 0; j < pi->num_employees; j++)
        {
            //days off
            if (pi->employees[j].num_days_off > 0)
            {
                for (int k = 0; k < pi->employees[j].num_days_off; k++)
                {
                    if (i == pi->employees[j].days_off[k])
                    {
                        max_realvar[i * pi->num_employees + j] = 0;
                        min_realvar[i * pi->num_employees + j] = 0;
                        break;
                    }
                    else
                    {
                        max_realvar[i * pi->num_employees + j] = pi->num_shifts-1;
                        min_realvar[i * pi->num_employees + j] = 0;
                    }
                }
            }
            else
            {
                max_realvar[i * pi->num_employees + j] = pi->num_shifts-1;
                min_realvar[i * pi->num_employees + j] = 0;
            }
        }
    }
    
    
    
    
    fclose(fh);
    return 1;
}

void readEmployees(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];
    findDef(f, "I:=");
    fgets(line, sizeof(line), f);
    removeSemicolon(line);
    pi->num_employees = countWords(line);
    pi->employees = malloc(pi->num_employees * sizeof(employee));
    
    char *token = strtok(line, " \t\n");
    for (int i = 0; i < pi->num_employees; i++) {
        pi->employees[i].id = i;
        pi->employees[i].name = strcpy(malloc(strlen(token) + 1), token);
        token = strtok(NULL, " \t\n");
    }
    printf("Employees: %d\n", pi->num_employees);
}

void readEmployeeDaysOff(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];
    char set_name[MAX_LINE_LENGTH];
    
    for (int i = 0; i < pi->num_employees; i++) {
        sprintf(set_name, "N[%s]:=", pi->employees[i].name);
        rewind(f);
        findDef(f, set_name);
        if (fgets(line, sizeof(line), f) == NULL) {
            // Handle error if `fgets` fails
            continue;
        }

        if (fgets(line, sizeof(line), f) != NULL && line[0] != ';') {
            // Allocate initial memory
            pi->employees[i].num_days_off = 0;
            pi->employees[i].days_off = malloc(pi->horizon_length * sizeof(int));
            if (pi->employees[i].days_off == NULL) {
                // Handle allocation failure
                continue;
            }

            do {
                int day;
                if (sscanf(line, "%d", &day) == 1) {
                    // Reallocate only if necessary
                    if (pi->employees[i].num_days_off >= pi->horizon_length) {
                        int new_size = (pi->employees[i].num_days_off + 1) * sizeof(int);
                        int* temp = realloc(pi->employees[i].days_off, new_size);
                        if (temp == NULL) {
                            // Handle reallocation failure and exit loop
                            free(pi->employees[i].days_off);
                            pi->employees[i].days_off = NULL;
                            break;
                        }
                        pi->employees[i].days_off = temp;
                    }
                    pi->employees[i].days_off[pi->employees[i].num_days_off++] = day - 1;  // Convert to 0-based index
                }
            } while (fgets(line, sizeof(line), f) && line[0] != ';');

            // Shrink the allocated memory to fit the actual size
            int* temp = realloc(pi->employees[i].days_off, pi->employees[i].num_days_off * sizeof(int));
            if (temp == NULL && pi->employees[i].num_days_off > 0) {
                // Handle reallocation failure
                free(pi->employees[i].days_off);
                pi->employees[i].days_off = NULL;
            } else {
                pi->employees[i].days_off = temp;
            }
        } else {
            pi->employees[i].num_days_off = 0;
            pi->employees[i].days_off = NULL;
        }
    }
}

void readHorizon(FILE *f, problem_instance *pi) {
    rewind(f);
    findDef(f, "h:=");
    fscanf(f, "%d", &pi->horizon_length);
}

void readShifts(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];
    findDef(f, "T:=");
    fgets(line, sizeof(line), f);
    removeSemicolon(line);
    pi->num_shifts = countWords(line)+1;
    pi->shifts = malloc(((pi->num_shifts)+1) * sizeof(shift));

    //add empty shift
    pi->shifts[0].id = 0;
    pi->shifts[0].name = strcpy(malloc(strlen("-") + 1), "-");
    pi->shifts[0].length = 0;
    pi->shifts[0].num_incompatible_shifts = NULL;


    
    char *token = strtok(line, " \t\n");
    for (int i = 1; i < (pi->num_shifts); i++) {
        pi->shifts[i].id = i;
        pi->shifts[i].name = strcpy(malloc(strlen(token) + 1), token);
        token = strtok(NULL, " \t\n");
    }
}
void readIncompatibleShifts(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];
    char set_name[MAX_LINE_LENGTH];
    
    for (int i = 0; i < pi->num_shifts; i++) {
        sprintf(set_name, "R[%s]:=", pi->shifts[i].name);
        rewind(f);
        findDef(f, set_name);
        if (fgets(line, sizeof(line), f) != NULL && line[0] != ';') {
            removeSemicolon(line);
            pi->shifts[i].num_incompatible_shifts = countWords(line);
            pi->shifts[i].incompatible_shifts = malloc(pi->shifts[i].num_incompatible_shifts * sizeof(int));
            
            int index = 0;
            char *token = strtok(line, " \t\n");
            while (token != NULL) {
                for (int j = 0; j < pi->num_shifts; j++) {
                    if (strcmp(token, pi->shifts[j].name) == 0) {
                        pi->shifts[i].incompatible_shifts[index++] = j;
                        break;
                    }
                }
                token = strtok(NULL, " \t\n");
            }
        } else {
            pi->shifts[i].num_incompatible_shifts = 0;
            pi->shifts[i].incompatible_shifts = NULL;
        }
    }
}


void readShiftLengths(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];
    findDef(f, "l:=");
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        char shift_name[MAX_LINE_LENGTH];
        int length;
        sscanf(line, "%s %d", shift_name, &length);
        for (int i = 0; i < pi->num_shifts; i++) {
            if (strcmp(pi->shifts[i].name, shift_name) == 0) {
                pi->shifts[i].length = length;
                break;
            }
        }
    }
}

void readMaxShifts(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];
    findDef(f, "m:=");
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        char emp_name[MAX_LINE_LENGTH], shift_name[MAX_LINE_LENGTH];
        int max_shifts;
        sscanf(line, "[%[^,],%[^]]] %d", emp_name, shift_name, &max_shifts);
        for (int i = 0; i < pi->num_employees; i++) {
            if (strcmp(pi->employees[i].name, emp_name) == 0) {
                if (pi->employees[i].max_shifts == NULL) {
                    pi->employees[i].max_shifts = malloc(pi->num_shifts * sizeof(int));
                    for (int j = 0; j < pi->num_shifts; j++) {
                        if(j==0){
                            pi->employees[i].max_shifts[j] = pi->horizon_length;
                        }else{
                            pi->employees[i].max_shifts[j] = 0;
                        }   
                    }
                }
                int shift_id;
                for (shift_id = 0; shift_id < pi->num_shifts; shift_id++) {
                    if (strcmp(pi->shifts[shift_id].name, shift_name) == 0) {
                        break;
                    }
                }
                pi->employees[i].max_shifts[shift_id] = max_shifts;
                break;
            }
        }
    }
}

void readMinMaxMinutes(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];
    findDef(f, "b:=");
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        char emp_name[MAX_LINE_LENGTH];
        int min_minutes;
        sscanf(line, "%s %d", emp_name, &min_minutes);
        for (int i = 0; i < pi->num_employees; i++) {
            if (strcmp(pi->employees[i].name, emp_name) == 0) {
                pi->employees[i].min_total_minutes = min_minutes;
                break;
            }
        }
    }

    rewind(f);
    findDef(f, "c:=");
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        char emp_name[MAX_LINE_LENGTH];
        int max_minutes;
        sscanf(line, "%s %d", emp_name, &max_minutes);
        for (int i = 0; i < pi->num_employees; i++) {
            if (strcmp(pi->employees[i].name, emp_name) == 0) {
                pi->employees[i].max_total_minutes = max_minutes;
                break;
            }
        }
    }
}

void readConsecutiveShifts(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];
    findDef(f, "f:=");
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        char emp_name[MAX_LINE_LENGTH];
        int min_consecutive;
        sscanf(line, "%s %d", emp_name, &min_consecutive);
        for (int i = 0; i < pi->num_employees; i++) {
            if (strcmp(pi->employees[i].name, emp_name) == 0) {
                pi->employees[i].min_consecutive_shifts = min_consecutive;
                break;
            }
        }
    }

    rewind(f);
    findDef(f, "g:=");
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        char emp_name[MAX_LINE_LENGTH];
        int max_consecutive;
        sscanf(line, "%s %d", emp_name, &max_consecutive);
        for (int i = 0; i < pi->num_employees; i++) {
            if (strcmp(pi->employees[i].name, emp_name) == 0) {
                pi->employees[i].max_consecutive_shifts = max_consecutive;
                break;
            }
        }
    }
}

void readConsecutiveDaysOff(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];
    findDef(f, "o:=");
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        char emp_name[MAX_LINE_LENGTH];
        int min_days_off;
        sscanf(line, "%s %d", emp_name, &min_days_off);
        for (int i = 0; i < pi->num_employees; i++) {
            if (strcmp(pi->employees[i].name, emp_name) == 0) {
                pi->employees[i].min_consecutive_days_off = min_days_off;
                break;
            }
        }
    }
}

void readMaxWeekends(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];
    findDef(f, "a:=");
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        char emp_name[MAX_LINE_LENGTH];
        int max_weekends;
        sscanf(line, "%s %d", emp_name, &max_weekends);
        for (int i = 0; i < pi->num_employees; i++) {
            if (strcmp(pi->employees[i].name, emp_name) == 0) {
                pi->employees[i].max_weekends = max_weekends;
                break;
            }
        }
    }
}

void readShiftOnOffRequests(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];


    // Allocate memory for shift on/off requests
    pi->shift_on_requests = malloc(pi->num_employees * sizeof(int**));
    pi->shift_off_requests = malloc(pi->num_employees * sizeof(int**));
    if (!pi->shift_on_requests || !pi->shift_off_requests) {
        printf("Error: Memory allocation failed for employee request arrays.\n");
        return;
    }

    for (int i = 0; i < pi->num_employees; i++) {
        pi->shift_on_requests[i] = malloc(pi->horizon_length * sizeof(int*));
        pi->shift_off_requests[i] = malloc(pi->horizon_length * sizeof(int*));
        if (!pi->shift_on_requests[i] || !pi->shift_off_requests[i]) {
            printf("Error: Memory allocation failed for employee %d.\n", i);
            return;
        }

        for (int j = 0; j < pi->horizon_length; j++) {
            pi->shift_on_requests[i][j] = calloc(pi->num_shifts, sizeof(int));
            pi->shift_off_requests[i][j] = calloc(pi->num_shifts, sizeof(int));
            if (!pi->shift_on_requests[i][j] || !pi->shift_off_requests[i][j]) {
                printf("Error: Memory allocation failed for employee %d, day %d.\n", i, j);
                return;
            }
        }
    }


    // Read shift on requests (q parameter)
    findDef(f, "q:=");
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        char emp_name[MAX_LINE_LENGTH], shift_name[MAX_LINE_LENGTH];
        int day, weight;
        if (sscanf(line, "[%[^,],%d,%[^]]] %d", emp_name, &day, shift_name, &weight) == 4) {
            day--; // Adjust for 0-based indexing

            int emp_index = -1, shift_index = -1;
            for (int i = 0; i < pi->num_employees; i++) {
                if (strcmp(pi->employees[i].name, emp_name) == 0) {
                    emp_index = i;
                    break;
                }
            }
            for (int i = 0; i < pi->num_shifts; i++) {
                if (strcmp(pi->shifts[i].name, shift_name) == 0) {
                    shift_index = i;
                    break;
                }
            }

            if (emp_index != -1 && shift_index != -1) {
                pi->shift_on_requests[emp_index][day][shift_index] = weight;
                
            } 
        } else {
            printf("Warning: Malformed line in shift on requests: %s\n", line);
        }
    }

    // Read shift off requests (p parameter)
    findDef(f, "p:=");
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        char emp_name[MAX_LINE_LENGTH], shift_name[MAX_LINE_LENGTH];
        int day, weight;
        if (sscanf(line, "[%[^,],%d,%[^]]] %d", emp_name, &day, shift_name, &weight) == 4) {
            day--; // Adjust for 0-based indexing

            int emp_index = -1, shift_index = -1;
            for (int i = 0; i < pi->num_employees; i++) {
                if (strcmp(pi->employees[i].name, emp_name) == 0) {
                    emp_index = i;
                    break;
                }
            }
            for (int i = 0; i < pi->num_shifts; i++) {
                if (strcmp(pi->shifts[i].name, shift_name) == 0) {
                    shift_index = i;
                    break;
                }
            }

            if (emp_index != -1 && shift_index != -1) {
                pi->shift_off_requests[emp_index][day][shift_index] = weight;
                
            } else {
                printf("Warning: Employee %s or shift %s not found in definitions.\n", emp_name, shift_name);
            }
        } else {
            printf("Warning: Malformed line in shift off requests: %s\n", line);
        }
    }

    printf("Finished reading shift on/off requests.\n");
}
void readCoverRequirements(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];
    findDef(f, "s:=");
    fgets(line, sizeof(line), f);
    
    pi->cover_requirements = malloc(pi->horizon_length * sizeof(int *));
    for (int i = 0; i < pi->horizon_length; i++) {
        pi->cover_requirements[i] = malloc(pi->num_shifts * sizeof(int));
    }
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        int day, requirement;
        char shift_name[MAX_LINE_LENGTH];
        sscanf(line, "[%d,%[^]]] %d", &day, shift_name, &requirement);
        day--; // Adjust for 0-based indexing
        for (int i = 0; i < pi->num_shifts; i++) {
            if (strcmp(pi->shifts[i].name, shift_name) == 0) {
                pi->cover_requirements[day][i] = requirement;
                break;
            }
        }
    }
}

void readCoverWeights(FILE *f, problem_instance *pi) {
    rewind(f);
    char line[MAX_LINE_LENGTH];

    // Allocate memory for cover weights
    pi->under_cover_weights = malloc(pi->horizon_length * sizeof(int*));
    pi->over_cover_weights = malloc(pi->horizon_length * sizeof(int*));
    for (int i = 0; i < pi->horizon_length; i++) {
        pi->under_cover_weights[i] = calloc(pi->num_shifts, sizeof(int));
        pi->over_cover_weights[i] = calloc(pi->num_shifts, sizeof(int));
    }

    // Read under cover weights (u parameter)
    findDef(f, "u:=");
    fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        int day;
        char shift_name[MAX_LINE_LENGTH];
        int weight;
        sscanf(line, "[%d,%[^]]] %d", &day, shift_name, &weight);
        day--; // Adjust for 0-based indexing
        int shift_index = -1;
        for (int i = 0; i < pi->num_shifts; i++) {
            if (strcmp(pi->shifts[i].name, shift_name) == 0) {
                shift_index = i;
                break;
            }
        }
        if (shift_index != -1) {
            pi->under_cover_weights[day][shift_index] = weight;
        }
    }

    // Read over cover weights (v parameter)
    findDef(f, "v:=");
    fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f) && line[0] != ';') {
        int day;
        char shift_name[MAX_LINE_LENGTH];
        int weight;
        sscanf(line, "[%d,%[^]]] %d", &day, shift_name, &weight);
        day--; // Adjust for 0-based indexing
        int shift_index = -1;
        for (int i = 0; i < pi->num_shifts; i++) {
            if (strcmp(pi->shifts[i].name, shift_name) == 0) {
                shift_index = i;
                break;
            }
        }
        if (shift_index != -1) {
            pi->over_cover_weights[day][shift_index] = weight;
        }
    }
}