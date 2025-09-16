#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "global.h"
#include "rand.h"

extern int popsize;
extern int nreal;
extern int nbin;
extern int init_type;
extern int ls_iters;
extern int ils_reset;
extern int *nbits;
extern double *min_realvar;
extern double *max_realvar;

ssequence **ssequences_pool;
ssequence ***ssequences_pool_emp;
int *num_sequences_pool_emp;
 
/* Add initialization type parameter */

void backtrackWorkingSequence_C(problem_instance *pi,employee *emp,int *current,int current_len,int *shift_count,ssequence ***ssequences_pool,int *num_results,int *pool_size);
void initial_solution(individual *ind, problem_instance *pi);

void initialize_individual_random(individual *ind, problem_instance *pi,
                                  ssequence ***ssequences_pool_emp, int *num_sequences_emp) {
    srand(time(NULL));

    int horizon = pi->horizon_length;
    int num_emps = pi->num_employees;

    // Inicializamos todo a 0 (off)
    for (int d = 0; d < horizon; d++)
        for (int e = 0; e < num_emps; e++)
            ind->xreal[d * num_emps + e] = 0;

    // Inicializar arrays de secuencias
    ind->seqs = (ssequence***)malloc(num_emps * sizeof(ssequence**));
    ind->seq_start_days = (int**)malloc(num_emps * sizeof(int*));
    ind->num_seqs = (int*)malloc(num_emps * sizeof(int));

    // Mantener un arreglo con el tamaño actual asignado para cada empleado
    int *allocated_size = (int*)malloc(num_emps * sizeof(int));

    for (int e = 0; e < num_emps; e++) {
        int pool_size = num_sequences_emp[e] > 0 ? num_sequences_emp[e] : 1;
        ind->seqs[e] = (ssequence**)malloc(pool_size * sizeof(ssequence*));
        ind->seq_start_days[e] = (int*)malloc(pool_size * sizeof(int));
        ind->num_seqs[e] = 0;
        allocated_size[e] = pool_size;
    }

    // Para cada empleado
    for (int e = 0; e < num_emps; e++) {
        int day = 0;
        int pool_size = num_sequences_emp[e];
        if (pool_size == 0) continue; // no sequences available

        while (day < horizon) {
            int idx = rand() % pool_size;
            ssequence *seq = ssequences_pool_emp[e][idx];

            if (day + seq->length > horizon) {
                day++;
                continue;
            }

            int overlap = 0;
            for (int i = 0; i < seq->length; i++) {
                if (ind->xreal[(day + i) * num_emps + e] != 0) {
                    overlap = 1;
                    break;
                }
            }
            if (overlap) {
                day++;
                continue;
            }

            // Expand arrays if needed
            if (ind->num_seqs[e] >= allocated_size[e]) {
                allocated_size[e] *= 2;
                ind->seqs[e] = realloc(ind->seqs[e], allocated_size[e] * sizeof(ssequence*));
                ind->seq_start_days[e] = realloc(ind->seq_start_days[e], allocated_size[e] * sizeof(int));
            }

            // Asignar la secuencia en xreal
            for (int i = 0; i < seq->length; i++) {
                ind->xreal[(day + i) * num_emps + e] = seq->shifts[i];
            }

            // Guardar la secuencia en el nuevo arreglo de secuencias
            ind->seqs[e][ind->num_seqs[e]] = seq;
            ind->seq_start_days[e][ind->num_seqs[e]] = day;
            ind->num_seqs[e]++;

            day += seq->length;

            if (day < horizon && rand() % 2 == 0) day++;
        }
       


    }


    free(allocated_size);
}


void initialize_pop(population *pop, problem_instance *pi) {

    printf("Initializing population and generating sequences per employee...\n");

    // Crear pool de secuencias por empleado
    ssequence ***ssequences_pool_emp_local = malloc(pi->num_employees * sizeof(ssequence**));
    int *num_sequences_emp = malloc(pi->num_employees * sizeof(int));
    int *pool_size_emp = malloc(pi->num_employees * sizeof(int));

    // Inicializamos tamaño inicial del pool
    for (int e = 0; e < pi->num_employees; e++) {
        pool_size_emp[e] = 16; // tamaño inicial
        ssequences_pool_emp_local[e] = malloc(pool_size_emp[e] * sizeof(ssequence*));
    }

    // Para cada empleado, generar secuencias
    for (int e = 0; e < pi->num_employees; e++) {
        employee *emp = &pi->employees[e];
        int *current = calloc(pi->horizon_length, sizeof(int));
        int *shift_count = calloc(pi->num_shifts, sizeof(int));
        num_sequences_emp[e] = 0;
        

        backtrackWorkingSequence_C(pi, emp, current, 0, shift_count,
                                &ssequences_pool_emp_local[e], &num_sequences_emp[e], &pool_size_emp[e]);


        printf("Generated %d sequences for employee %s\n", num_sequences_emp[e], emp->name);

        free(current);
        free(shift_count);
    }

    // Initialize global number of sequences per employee
    num_sequences_pool_emp = malloc(pi->num_employees * sizeof(int));
    for (int e = 0; e < pi->num_employees; e++) {
        num_sequences_pool_emp[e] = num_sequences_emp[e];
    }

    ssequences_pool_emp = malloc(pi->num_employees * sizeof(ssequence**));

    for (int e = 0; e < pi->num_employees; e++) {
        ssequences_pool_emp[e] = malloc(num_sequences_emp[e] * sizeof(ssequence*));
        num_sequences_pool_emp[e] = num_sequences_emp[e];

        // Copy sequences from local pool to global pool
        for (int s = 0; s < num_sequences_emp[e]; s++) {
            ssequences_pool_emp[e][s] = ssequences_pool_emp_local[e][s]; // if you used a local array first
        }
    }



    // free(pool_size_emp);

    extern int init_type;  // Asegura que se declaran como externas si están definidas en otro archivo
    extern int popsize;

    for (int i = 0; i < popsize; i++) {
        individual *ind = &(pop->ind[i]);
        // Initialize all values to zero (empty)
        for (int j = 0; j < pi->horizon_length * pi->num_employees; j++) {
            ind->xreal[j] = 0;
        }
        ind->seqs = (ssequence***)malloc(pi->num_employees * sizeof(ssequence**));
        ind->seq_start_days = (int**)malloc(pi->num_employees * sizeof(int*));
        ind->num_seqs = (int*)malloc(pi->num_employees * sizeof(int));
        for (int e = 0; e < pi->num_employees; e++) {
            ind->seqs[e] = NULL;
            ind->seq_start_days[e] = NULL;
            ind->num_seqs[e] = 0;
        }
        printf("Individual %d initialized empty\n", i);
    }
    //debug firts individual
    printf("First individual after random initialization:\n");
    for (int e=0; e < pi->num_employees; e++) {
        printf("Employee %s: ", pi->employees[e].name);
        //print sequences
        printf("Num sequences: %d\n", pop->ind[3].num_seqs[e]);
        for (int s = 0; s < pop->ind[3].num_seqs[e]; s++) {
            printf("[Seq start day: %d, shifts: ", pop->ind[3].seq_start_days[e][s]);
            for (int j = 0; j < pop->ind[3].seqs[e][s]->length; j++) {
                printf("%d ", pop->ind[3].seqs[e][s]->shifts[j]);
            }
            printf("] ");
        }
        printf("\n");
    }

}


void add_sequence_to_pool(ssequence ***pool, ssequence *seq, int *num_results, int *pool_size) {
    if (*num_results >= *pool_size) {
        *pool_size *= 2;
        *pool = realloc(*pool, (*pool_size) * sizeof(ssequence*));
        if (!*pool) {
            fprintf(stderr, "Memory allocation failed for sequence pool!\n");
            exit(1);
        }
    }
    (*pool)[*num_results] = seq;
    (*num_results)++;
}


void backtrackWorkingSequence_C(
    problem_instance *pi,
    employee *emp,
    int *current,
    int current_len,
    int *shift_count,
    ssequence ***ssequences_pool,
    int *num_results,
    int *pool_size
) {
    if (emp->max_consecutive_shifts > 0 && current_len > emp->max_consecutive_shifts)
        return;

    if (current_len >= emp->min_consecutive_shifts &&
        (emp->max_consecutive_shifts == 0 || current_len <= emp->max_consecutive_shifts)) {

        int total_minutes = 0;
        for (int i = 0; i < current_len; i++) {
            int s = current[i];
            if (s > 0 && s < pi->num_shifts)
                total_minutes += pi->shifts[s].length;
        }

        ssequence *seq = malloc(sizeof(ssequence));
        seq->length = current_len;
        seq->total_minutes = total_minutes;
        if (current_len > 0) {
            seq->shifts = malloc(sizeof(int) * current_len);
            memcpy(seq->shifts, current, sizeof(int) * current_len);
        } else {
            seq->shifts = NULL;
        }


        add_sequence_to_pool(ssequences_pool, seq, num_results, pool_size);
    }

    // Try all shifts
    for (int shift_id = 1; shift_id < pi->num_shifts; shift_id++) {
        if (emp->max_shifts[shift_id] > 0 &&
            shift_count[shift_id] >= emp->max_shifts[shift_id]) {
            continue;
        }


        if (current_len > 0) {
            int prev_shift = current[current_len-1];
            bool incompatible = false;
            for (int k = 0; k < pi->shifts[prev_shift].num_incompatible_shifts; k++) {
                if (pi->shifts[prev_shift].incompatible_shifts[k] == shift_id) {
                    incompatible = true;
                    break;
                }
            }
            if (incompatible) continue;
        }

        current[current_len] = shift_id;
        shift_count[shift_id]++;
        backtrackWorkingSequence_C(pi, emp, current, current_len+1, shift_count, ssequences_pool, num_results, pool_size);
        shift_count[shift_id]--;
    }
}