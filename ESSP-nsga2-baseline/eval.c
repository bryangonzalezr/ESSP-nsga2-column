/* Routine for evaluating population members  */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "global.h"
#include "rand.h"
#include <unistd.h>



int maxprint = 1;

extern double *min_realvar;
extern double *max_realvar;
/* Routine to evaluate objective function values and constraints for a population */
void evaluate_pop(population *pop, problem_instance *pi)
{
    int i;
    
    for (i = 0; i < popsize; i++)
    {
        evaluate_ind(&(pop->ind[i]), pi);
    }
    return;
}

/* Routine to evaluate objective function values and constraints for an individual */
/*void evaluate_ind (individual *ind)
{
    int j;
    test_problem (ind->xreal, ind->xbin, ind->gene, ind->obj, ind->constr);
    if (ncon==0)
    {
        ind->constr_violation = 0.0;
    }
    else
    {
        ind->constr_violation = 0.0;
        for (j=0; j<ncon; j++)
        {
            if (ind->constr[j]<0.0)
            {
                ind->constr_violation += ind->constr[j];
            }
        }
    }
    return;
}*/

/* Routine to evaluate objective function values and constraints for an individual */

void evaluate_ind(individual *ind, problem_instance *pi)
{
    int num_employees = pi->num_employees;
    int horizon_length = pi->horizon_length;
    int num_shifts = pi->num_shifts;

    ind->constr_violation = 0.0;

    // Create array to store amount of times each restriction is violated
    ind->constr = (double *)malloc(9*sizeof(double));
    if (ind->constr == NULL)
    {
        fprintf(stderr, "Memory allocation failed for constraints.\n");
        exit(EXIT_FAILURE);
    }
    // Initialize constraints to 0
    for (int i = 0; i < 9; i++)
    {
        ind->constr[i] = 0.0;
    }

    // Arrays to store the number of employees assigned to each shift on each day
    int **shift_coverage = (int **)malloc(horizon_length * sizeof(int *));
    for (int i = 0; i < horizon_length; i++) {
        shift_coverage[i] = (int *)calloc(num_shifts, sizeof(int));
    }

    // Initialize objective values
    double obj2 = 0.0; // Employee satisfaction (preferences)
    double obj1 = 0.0; // Shift coverage
    // sleep(1);

    for (int employee = 0; employee < num_employees; employee++) {
        int *shift_count = (int *)calloc(num_shifts, sizeof(int));
        int consecutive_shifts = 0;
        int consecutive_off = 0;
        int total_minutes = 0;
        int weekcount = 0;
        int consecutive_shifts_for_r4 = 0;
        

        for (int day = 0; day < horizon_length; day++) {
            int shift_id = (int)round(ind->xreal[day * num_employees + employee]);

            if (day == 0 && shift_id == 0) {
                consecutive_shifts = 0;
                consecutive_off = pi->horizon_length;
            }else if (day == 0 && shift_id !=0) {
                consecutive_shifts = pi->horizon_length;
                consecutive_off=0;
            }

            // R1: Days off
            if (shift_id > max_realvar[day * num_employees + employee] ||
                shift_id < min_realvar[day * num_employees + employee]) {
                shift_id =0;
            }

            // R2: max per shift type
            if (shift_id >= 0 ) {
                shift_count[shift_id]++;
                if (shift_count[shift_id] > pi->employees[employee].max_shifts[shift_id]) {
                    ind->constr_violation -= 1.0;
                    ind->constr[1] += 1.0;
                }
            }

            // R3: incompatible shifts
            if (day > 0) {
                int prev_shift = (int)round(ind->xreal[(day - 1) * num_employees + employee]);
                if (prev_shift >= 0 && shift_id >= 0 && pi->shifts[prev_shift].num_incompatible_shifts > 0) {
                    for (int j = 0; j < pi->shifts[prev_shift].num_incompatible_shifts; j++) {
                        if (pi->shifts[prev_shift].incompatible_shifts[j] == shift_id) {
                            
                            ind->constr_violation -= 1.0;
                            ind->constr[2] += 1.0;
                        }
                    }
                }
            }

            // R4: min consecutive shifts
            if (shift_id == 0 && consecutive_shifts > 0) {
            if (consecutive_shifts < pi->employees[employee].min_consecutive_shifts) {
               
                ind->constr_violation -= 1.0;
                ind->constr[3] += 1.0;
                }
                
            }

            // Si hoy es turno y el bloque anterior fue de descanso
            if (shift_id != 0 && consecutive_off > 0) {
                if (consecutive_off < pi->employees[employee].min_consecutive_days_off) {
                    
                    ind->constr_violation -= 1.0;
                    ind->constr[3] += 1.0;
                }
            }

            // Actualiza contadores y chequea R5 (máximo de turnos)
            if (shift_id != 0) {
                consecutive_shifts++;
                consecutive_shifts_for_r4++;
                consecutive_off = 0;

                if (consecutive_shifts_for_r4 > pi->employees[employee].max_consecutive_shifts) {
                    
                    ind->constr_violation -= 1.0;
                    ind->constr[4] += 1.0;
                }
            } else {
                consecutive_off++;
                consecutive_shifts_for_r4 = 0;
                consecutive_shifts = 0;
            }

            // R6: weekends
            if (day % 7 == 5) {
                if (shift_id != 0) {
                    weekcount++;
                } else if (day + 1 < horizon_length) {
                    int next_shift = (int)round(ind->xreal[(day + 1) * num_employees + employee]);
                    if (next_shift != 0) {
                        weekcount++;
                    }
                }
            }

            // R7: total minutes
            if (shift_id >= 0) {
                total_minutes += pi->shifts[shift_id].length;
                if (total_minutes > pi->employees[employee].max_total_minutes) {
                    
                    ind->constr_violation -= 1.0;
                    ind->constr[5] += 1.0;
                }
            }

            // Objective 1: Preferences
            for (int s = 0; s < num_shifts; s++) {
                if (s == shift_id) {
                    obj2 += pi->shift_off_requests[employee][day][s];
                } else {
                    obj2 += pi->shift_on_requests[employee][day][s];
                }
            }

            // Coverage tracking
            if (shift_id > 0 && shift_id < num_shifts) {
                if (shift_id > max_realvar[day * num_employees + employee] ||
                    shift_id < min_realvar[day * num_employees + employee]) {
                    continue;
                }else{
                    shift_coverage[day][shift_id]++;

                }   
            }
        }

        // R6: max weekends
        if (weekcount > pi->employees[employee].max_weekends) {
            
            ind->constr_violation -= 1.0;
            ind->constr[6] += 1.0;
        }

        // R8: min total minutes
        if (total_minutes < pi->employees[employee].min_total_minutes) {
            
            ind->constr_violation -= 1.0;
            ind->constr[7] += 1.0;
        }

        free(shift_count);
    }

    // Objective 2: Shift coverage penalties
    for (int day = 0; day < horizon_length; day++) {
        for (int s = 1; s < num_shifts; s++) { // Start from 1 to skip the empty shift
            int required = pi->cover_requirements[day][s];
            int actual = shift_coverage[day][s];
            
            
            
            // Calculate under-cover penalty
            if (actual < required) {
                double penalty = (required - actual) * pi->under_cover_weights[day][s];
                
                obj1 += penalty;
            }
            // Calculate over-cover penalty
            else if (actual > required) {
                double penalty = (actual - required) * pi->over_cover_weights[day][s];
                
                obj1 += penalty;
            }
        }
    }

    // Assign objectives to individual
    ind->obj[0] = obj1; // Employee satisfaction
    ind->obj[1] = obj2; // Shift coverage

    // Free allocated memory
    for (int i = 0; i < horizon_length; i++) {
        free(shift_coverage[i]);
    }
    free(shift_coverage);
}