/* Routine for evaluating population members  */

# include <stdio.h>
# include <stdlib.h>
# include <math.h>
# include <string.h>
# include "global.h"
# include "rand.h"




void preferenceIndividual(individual *ind, problem_instance *pi, int print) {
    int total_preference = 0;
    for (int i = 0; i < pi->horizon_length; i++) {
        for (int j = 0; j < pi->num_employees; j++) {
            int assigned_shift = ind->xreal[i * pi->num_employees + j];
            
            // Check shift on requests
            for (int s = 0; s < pi->num_shifts; s++) {
                if (pi->shift_on_requests[j][i][s] > 0) {
                    if (s == assigned_shift) {
                        // Request fulfilled, no penalty
                        // printf("Employee %s, Day %d, Shift %s: On request fulfilled (weight %d)\n", 
                        //     pi->employees[j].name, i+1, pi->shifts[s].name, pi->shift_on_requests[j][i][s]);
                        
                        
                    } else {
                        // Request not fulfilled, add to total_preference
                        total_preference += pi->shift_on_requests[j][i][s];
                        if(print){
                            printf("Employee %s, Day %d, Shift %s: On request not fulfilled (weight %d)\n", 
                                pi->employees[j].name, i+1, pi->shifts[s].name, pi->shift_on_requests[j][i][s]);
                        }
                        
                        
                    }
                }
            }
            
            // Check shift off requests
            if (assigned_shift != -1 && pi->shift_off_requests[j][i][assigned_shift] > 0) {
                total_preference += pi->shift_off_requests[j][i][assigned_shift];
                if(print){
                    printf("Employee %s, Day %d, Shift %s: Off request violated (weight %d)\n", 
                    pi->employees[j].name, i+1, pi->shifts[assigned_shift].name, pi->shift_off_requests[j][i][assigned_shift]);
                }
                
                
            }
        }
    }
    ind->obj[1] = total_preference;
}

void coveringIndividual(individual *ind, problem_instance *pi, int print){
    int total_cover = 0;
    for (int i = 0; i < pi->horizon_length; i++)
    {
        int under_coverage = 0;
        int over_coverage = 0;
        int *shift_ammount = (int *)malloc(pi->num_shifts * sizeof(int));
        for(int j = 0; j < pi->num_shifts; j++){
            shift_ammount[j] = 0;
        }
        for(int j = 0; j < pi->num_employees; j++){
            int shift = ind->xreal[i * pi->num_employees + j];
            if (shift != 0)
            {
                shift_ammount[shift]++;
                under_coverage += pi->cover_requirements[i][shift];
                over_coverage += pi->cover_requirements[i][shift];
            }
        }
        for (int j = 1; j < pi->num_shifts; j++)
        {
            if (shift_ammount[j] < pi->cover_requirements[i][j])
            {
                total_cover += pi->under_cover_weights[i][j] * (pi->cover_requirements[i][j] - shift_ammount[j]);
                if(print){
                    printf("Day %d, Shift %s: Under cover (weight %d)\n", i+1, pi->shifts[j].name, pi->under_cover_weights[i][j] * (pi->cover_requirements[i][j] - shift_ammount[j]));
                }
            }
            if (shift_ammount[j] > pi->cover_requirements[i][j])
            {
                total_cover += pi->over_cover_weights[i][j] * (shift_ammount[j] - pi->cover_requirements[i][j]);
                if(print){
                    printf("Day %d, Shift %s: Over cover (weight %d)\n", i+1, pi->shifts[j].name, pi->over_cover_weights[i][j] * (shift_ammount[j] - pi->cover_requirements[i][j]));
                }
            }
        }
        free(shift_ammount);

    }
    ind->obj[0] = total_cover;
    


}

void printIndividual(individual *ind, problem_instance *pi) {
    // Print headers for days
    printf("E \\ D");
    for (int i = 0; i < pi->horizon_length; i++) {
        printf("\tDay %d", i + 1);
    }
    printf("\n");
    
    // Print schedule for each employee
    for (int j = 0; j < pi->num_employees; j++) {
        printf("%s", pi->employees[j].name);
        for (int i = 0; i < pi->horizon_length; i++) {
            printf("\t%s", pi->shifts[ind->xreal[i * pi->num_employees + j]].name);
        }
        printf("\n");
    }
}

void printProblemInstance(problem_instance *pi) {
    printf("\n====== Instance Read ======\n");

    printf("Horizon length: %d\n", pi->horizon_length);

    printf("\n--- Shifts ---\n");
    for (int i = 0; i < pi->num_shifts; i++) {
        printf("Shift %d: Name = %s, Length = %d\n", 
               pi->shifts[i].id, pi->shifts[i].name, pi->shifts[i].length);
        printf("    Incompatible shifts: ");
        for (int j = 0; j < pi->shifts[i].num_incompatible_shifts; j++) {
            int shift_id = pi->shifts[i].incompatible_shifts[j];
            char *shift_name = pi->shifts[shift_id].name;
            printf("%s ", shift_name);
        }
        printf("\n");
    }

    printf("\n--- Employees ---\n");
    for (int i = 0; i < pi->num_employees; i++) {
        printf("Employee %d: Name = %s\n", pi->employees[i].id, pi->employees[i].name);
        printf("    Days off: ");
        for (int j = 0; j < pi->employees[i].num_days_off; j++) {
            printf("%d ", pi->employees[i].days_off[j]);
        }
        printf("\n");
        printf("    Max shifts:\n");
        if(pi->employees[i].max_shifts != NULL){
            for (int j = 0; j < pi->num_shifts; j++) {
                printf("    Shift %s: %d\n",pi->shifts[j].name, pi->employees[i].max_shifts[j]);
            }
        }
            
        printf("    Min total minutes: %d\n", pi->employees[i].min_total_minutes);
        printf("    Max total minutes: %d\n", pi->employees[i].max_total_minutes);
        printf("    Min consecutive shifts: %d\n", pi->employees[i].min_consecutive_shifts);
        printf("    Max consecutive shifts: %d\n", pi->employees[i].max_consecutive_shifts);
        printf("    Min consecutive days off: %d\n", pi->employees[i].min_consecutive_days_off);
        printf("    Max weekends: %d\n", pi->employees[i].max_weekends);
    }

    printf("\n--- Cover Requirements ---\n");
    for (int d = 0; d < pi->horizon_length; d++) {
        for (int s = 0; s < pi->num_shifts; s++) {
            printf("Day %d, Shift %s: %d\n", d+1, pi->shifts[s].name, pi->cover_requirements[d][s]);
        }
    }

    printf("\n--- Shift On/Off Requests ---\n");
    for (int i = 0; i < pi->num_employees; i++) {
        for (int d = 0; d < pi->horizon_length; d++) {
            for (int s = 0; s < pi->num_shifts; s++) {
                if (pi->shift_on_requests[i][d][s] > 0) {
                    printf("Employee %s, Day %d, Shift %s: On request (weight %d)\n", 
                           pi->employees[i].name, d+1, pi->shifts[s].name, pi->shift_on_requests[i][d][s]);
                }
                if (pi->shift_off_requests[i][d][s] > 0) {
                    printf("Employee %s, Day %d, Shift %s: Off request (weight %d)\n", 
                           pi->employees[i].name, d+1, pi->shifts[s].name, pi->shift_off_requests[i][d][s]);
                }
            }
        }
    }

    printf("\n--- Cover Weights ---\n");
    for (int d = 0; d < pi->horizon_length; d++) {
        for (int s = 0; s < pi->num_shifts; s++) {
            printf("Day %d, Shift %s: Under cover weight = %d, Over cover weight = %d\n", 
                   d+1, pi->shifts[s].name, pi->under_cover_weights[d][s], pi->over_cover_weights[d][s]);
        }
    }

    printf("\n====== End of Instance ======\n");
}