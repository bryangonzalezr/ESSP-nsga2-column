#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "rand.h"
#include "time.h"

extern ssequence ***ssequences_pool_emp; // pools per employee
extern int *num_sequences_pool_emp;      // number of sequences per employee
extern seq_length_index *seq_index;

extern emp_assign **employees_pool;
extern int *count_employees_pool;

extern double mut1_p;
extern double mut2_p;
extern double mut3_p;
extern double mut4_p;
extern double mut5_p;
extern double mutammount;
extern double pmut_real;

// Forward declarations
void mutation_ind_sequence(individual *ind, problem_instance *pi);
void mutation_remove(individual *ind, problem_instance *pi, int emp);
void mutation_add(individual *ind, problem_instance *pi, int emp);
void mutation_shift(individual *ind, problem_instance *pi, int emp);
void mutation_change(individual *ind, problem_instance *pi, int emp);
void mutation_replace_from_pool(individual *ind, problem_instance *pi, int emp); // MUT5
int check_overlap(int start1, int len1, int start2, int len2);
void remove_overlapping_sequences(individual *ind, int emp, int new_start, int new_len);


void mutation_pop(population *pop, problem_instance *pi) {
    for (int i = 0; i < popsize; i++) {
        if (randomperc() <= pmut_real) {
            int max_num_mutations = (pi->num_employees * pi->horizon_length) * mutammount;
            int num_mutations = rnd(0, max_num_mutations);

            for (int m = 0; m < num_mutations; m++) {
                mutation_ind_sequence(&(pop->ind[i]), pi);
            }
        }
    }
}

void mutation_ind_sequence(individual *ind, problem_instance *pi) {
    int emp = rnd(0, pi->num_employees - 1);

    if (num_sequences_pool_emp[emp] == 0) return;

    // Probabilidades ponderadas
    double total_p = mut1_p + mut2_p + mut3_p + mut4_p + mut5_p;
    double r = randomperc();
    double p1 = mut1_p / total_p;
    double p2 = p1 + (mut2_p / total_p);
    double p3 = p2 + (mut3_p / total_p);
    double p4 = p3 + (mut4_p / total_p);
    double p5 = p4 + (mut5_p / total_p);

    int mutation_type = 0;
    if (r < p1) mutation_type = 0;
    else if (r < p2) mutation_type = 1;
    else if (r < p3) mutation_type = 2;
    else if (r < p4) mutation_type = 3;
    else mutation_type = 4; // MUT5

    switch (mutation_type) {
        case 0: mutation_remove(ind, pi, emp); break;
        case 1: mutation_add(ind, pi, emp); break;
        case 2: mutation_shift(ind, pi, emp); break;
        case 3: mutation_change(ind, pi, emp); break;
        case 4: mutation_replace_from_pool(ind, pi, emp); break;
    }
}

/* ===================== MUTACIONES CLÁSICAS ===================== */

void mutation_remove(individual *ind, problem_instance *pi, int emp) {
    if (ind->num_seqs[emp] <= 0) return;
    int seq_idx = rnd(0, ind->num_seqs[emp] - 1);
    for (int i = seq_idx; i < ind->num_seqs[emp] - 1; i++) {
        ind->seqs[emp][i] = ind->seqs[emp][i + 1];
        ind->seq_start_days[emp][i] = ind->seq_start_days[emp][i + 1];
    }
    ind->num_seqs[emp]--;
}

void mutation_add(individual *ind, problem_instance *pi, int emp) {
    if (num_sequences_pool_emp[emp] == 0) return;

    seq_length_index *idx = &seq_index[emp];
    int length;
    do { length = rnd(1, idx->max_length); } while (idx->count_by_length[length] == 0);

    int pos = rnd(0, idx->count_by_length[length] - 1);
    int pool_idx = idx->by_length[length][pos];
    ssequence *seq = ssequences_pool_emp[emp][pool_idx];

    int horizon = pi->horizon_length;
    if (seq->length > horizon) return;

    int start_day = rnd(0, horizon - seq->length);
    remove_overlapping_sequences(ind, emp, start_day, seq->length);

    ind->seqs[emp][ind->num_seqs[emp]] = seq;
    ind->seq_start_days[emp][ind->num_seqs[emp]] = start_day;
    ind->num_seqs[emp]++;
}

void mutation_shift(individual *ind, problem_instance *pi, int emp) {
    if (ind->num_seqs[emp] <= 0) return;

    int seq_idx = rnd(0, ind->num_seqs[emp] - 1);
    ssequence *seq = ind->seqs[emp][seq_idx];
    int current_start = ind->seq_start_days[emp][seq_idx];
    int horizon = pi->horizon_length;

    int new_start = rnd(0, horizon - seq->length);
    remove_overlapping_sequences(ind, emp, new_start, seq->length);

    ind->seq_start_days[emp][seq_idx] = new_start;
}

void mutation_change(individual *ind, problem_instance *pi, int emp) {
    if (ind->num_seqs[emp] <= 0) return;
    int seq_idx = rnd(0, ind->num_seqs[emp] - 1);
    ssequence *current_seq = ind->seqs[emp][seq_idx];
    int current_start = ind->seq_start_days[emp][seq_idx];
    int length = current_seq->length;

    seq_length_index *idx = &seq_index[emp];
    if (idx->count_by_length[length] <= 1) return;

    int pos = rnd(0, idx->count_by_length[length] - 1);
    int pool_idx = idx->by_length[length][pos];
    ssequence *new_seq = ssequences_pool_emp[emp][pool_idx];

    remove_overlapping_sequences(ind, emp, current_start, new_seq->length);
    ind->seqs[emp][seq_idx] = new_seq;
}

/* ===================== MUT5: INTERCAMBIO CON EMPLOYEE POOL ===================== */

void mutation_replace_from_pool(individual *ind, problem_instance *pi, int emp) {
    if (count_employees_pool[emp] == 0) return;

    int choice = rnd(0, count_employees_pool[emp] - 1);
    emp_assign *replacement = &employees_pool[emp][choice];

    // Limpiar las secuencias actuales del empleado
    ind->num_seqs[emp] = 0;

    // Asignar las secuencias desde la pool
    for (int i = 0; i < replacement->num_seqs; i++) {
        ssequence *seq = replacement->seqs[i];
        int start_day = replacement->seq_start_day[i];

        remove_overlapping_sequences(ind, emp, start_day, seq->length);

        ind->seqs[emp][ind->num_seqs[emp]] = seq;
        ind->seq_start_days[emp][ind->num_seqs[emp]] = start_day;
        ind->num_seqs[emp]++;
    }

  
}

/* ===================== UTILIDADES ===================== */

int check_overlap(int start1, int len1, int start2, int len2) {
    int end1 = start1 + len1 - 1;
    int end2 = start2 + len2 - 1;
    return !(end1 < start2 || start1 > end2);
}

void remove_overlapping_sequences(individual *ind, int emp, int new_start, int new_len) {
    for (int i = ind->num_seqs[emp] - 1; i >= 0; i--) {
        int existing_start = ind->seq_start_days[emp][i];
        int existing_len = ind->seqs[emp][i]->length;
        if (check_overlap(new_start, new_len, existing_start, existing_len)) {
            for (int j = i; j < ind->num_seqs[emp] - 1; j++) {
                ind->seqs[emp][j] = ind->seqs[emp][j + 1];
                ind->seq_start_days[emp][j] = ind->seq_start_days[emp][j + 1];
            }
            ind->num_seqs[emp]--;
        }
    }
}
