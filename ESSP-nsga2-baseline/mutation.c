#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "rand.h"
#include "time.h"

extern ssequence ***ssequences_pool_emp; // pools per employee
extern int *num_sequences_pool_emp;      // number of sequences per employee
extern seq_length_index *seq_index;
 

extern double mut1_p;
extern double mut2_p;
extern double mut3_p;
extern double mut4_p;
extern double mut5_p;
extern double mutammount;      // máximo de mutaciones por individuo
extern double pmut_real;    // probabilidad de mutar individuo

// Forward declarations
void mutation_ind_sequence(individual *ind, problem_instance *pi);
void mutation_remove(individual *ind, problem_instance *pi, int emp);
void mutation_shift(individual *ind, problem_instance *pi, int emp);
void mutation_change(individual *ind, problem_instance *pi, int emp);
int check_overlap(int start1, int len1, int start2, int len2);
void remove_overlapping_sequences(individual *ind, int emp, int new_start, int new_len);


void mutation_pop(population *pop, problem_instance *pi) {

    




    for (int i = 0; i < popsize; i++) {
        if (randomperc() <= pmut_real) {
            // Sorteamos cuántas mutaciones aplicar a este individuo
            int max_num_mutations = (pi->num_employees*pi->horizon_length)*mutammount;
            int num_mutations = rnd(0, max_num_mutations); // entre 0 y mutammount

            for (int m = 0; m < num_mutations; m++) {
                mutation_ind_sequence(&(pop->ind[i]), pi);
            }
        }
    }
}

void mutation_ind_sequence(individual *ind, problem_instance *pi) {
    int emp = rnd(0, pi->num_employees - 1);

    if (num_sequences_pool_emp[emp] == 0) return; // No sequences available



    // Select mutation type randomly
    double r = randomperc();
    // Weight the probabilities of each mutation type
    double total_p = mut1_p + mut2_p + mut3_p + mut4_p;
    double p1 = mut1_p / total_p;
    double p2 = p1 + (mut2_p / total_p);
    double p3 = p2 + (mut3_p / total_p);
    double p4 = p3 + (mut4_p / total_p);
    // mut5_p es el resto (reset mutation)

    int mutation_type;
    if (r < p1) {
        mutation_type = 0; // Remove
    } else if (r < p2) {
        mutation_type = 1; // Add
    } else if (r < p3) {
        mutation_type = 2; // Shift
    } else if (r < p4) {
        mutation_type = 3; // Change
    }

 

    switch (mutation_type) {
        case 0:
            mutation_remove(ind, pi, emp);
            break;
        case 1:
            mutation_add(ind, pi, emp);
            
            break;
        case 2:
            mutation_shift(ind, pi, emp);
            
            break;
        case 3:
            mutation_change(ind, pi, emp);
            
            break;
    }
}


void mutation_remove(individual *ind, problem_instance *pi, int emp) {
    if (ind->num_seqs[emp] <= 0) return; // No sequences to remove

    int seq_idx = rnd(0, ind->num_seqs[emp] - 1);
    
    // Shift sequences to remove the selected one
    for (int i = seq_idx; i < ind->num_seqs[emp] - 1; i++) {
        ind->seqs[emp][i] = ind->seqs[emp][i + 1];
        ind->seq_start_days[emp][i] = ind->seq_start_days[emp][i + 1];
    }
    ind->num_seqs[emp]--;
}

void mutation_add(individual *ind, problem_instance *pi, int emp) {
    if (num_sequences_pool_emp[emp] == 0) return;

    seq_length_index *idx = &seq_index[emp];

    // elegir un largo con secuencias disponibles
    int length;
    do {
        length = rnd(1, idx->max_length);
    } while (idx->count_by_length[length] == 0);

    // elegir una secuencia de ese largo
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
    if (ind->num_seqs[emp] <= 0) return; // No sequences to shift

    int seq_idx = rnd(0, ind->num_seqs[emp] - 1);
    ssequence *seq = ind->seqs[emp][seq_idx];
    int current_start = ind->seq_start_days[emp][seq_idx];
    int horizon = pi->horizon_length;

    // Calculate possible directions
    int can_move_left = (current_start > 0);
    int can_move_right = (current_start + seq->length < horizon);

    int direction;
    int new_start;

    if (can_move_left && can_move_right) {
        // Can move in both directions, choose randomly
        direction = (rand() % 2 == 0) ? -1 : 1;
    } else if (can_move_left) {
        direction = -1; // Move left
    } else if (can_move_right) {
        direction = 1;  // Move right
    } else {
        // No valid direction within horizon, choose random position
        new_start = rnd(0, horizon - seq->length);
        
        // Remove the current sequence temporarily
        for (int i = seq_idx; i < ind->num_seqs[emp] - 1; i++) {
            ind->seqs[emp][i] = ind->seqs[emp][i + 1];
            ind->seq_start_days[emp][i] = ind->seq_start_days[emp][i + 1];
        }
        ind->num_seqs[emp]--;

        // Remove overlapping sequences and add at new position
        remove_overlapping_sequences(ind, emp, new_start, seq->length);
        
        ind->seqs[emp][ind->num_seqs[emp]] = seq;
        ind->seq_start_days[emp][ind->num_seqs[emp]] = new_start;
        ind->num_seqs[emp]++;
        return;
    }

    // Move in the chosen direction
    int shift_amount = rnd(1, 3); // Move 1-3 positions
    new_start = current_start + (direction * shift_amount);

    // Ensure we don't go out of bounds
    if (new_start < 0) new_start = 0;
    if (new_start + seq->length > horizon) new_start = horizon - seq->length;

    // If new position is the same as current, do nothing
    if (new_start == current_start) return;

    // Remove the current sequence temporarily
    for (int i = seq_idx; i < ind->num_seqs[emp] - 1; i++) {
        ind->seqs[emp][i] = ind->seqs[emp][i + 1];
        ind->seq_start_days[emp][i] = ind->seq_start_days[emp][i + 1];
    }
    ind->num_seqs[emp]--;

    // Remove overlapping sequences at new position
    remove_overlapping_sequences(ind, emp, new_start, seq->length);

    // Add sequence at new position
    ind->seqs[emp][ind->num_seqs[emp]] = seq;
    ind->seq_start_days[emp][ind->num_seqs[emp]] = new_start;
    ind->num_seqs[emp]++;
}

void mutation_change(individual *ind, problem_instance *pi, int emp) {
    if (ind->num_seqs[emp] <= 0) return;

    int seq_idx = rnd(0, ind->num_seqs[emp] - 1);
    ssequence *current_seq = ind->seqs[emp][seq_idx];
    int current_start = ind->seq_start_days[emp][seq_idx];
    int length = current_seq->length;

    seq_length_index *idx = &seq_index[emp];
    if (idx->count_by_length[length] <= 1) return; // no hay alternativa

    int pool_idx;
    ssequence *new_seq;
    int count=0;
    
    int pos = rnd(0, idx->count_by_length[length] - 1);
    pool_idx = idx->by_length[length][pos];
    new_seq = ssequences_pool_emp[emp][pool_idx];
        

    int horizon = pi->horizon_length;
    int new_start = current_start;
    if (new_start + new_seq->length > horizon) {
        new_start = horizon - new_seq->length;
    }
    if (new_start < 0) new_start = 0;

    // quitar la secuencia actual
    for (int i = seq_idx; i < ind->num_seqs[emp] - 1; i++) {
        ind->seqs[emp][i] = ind->seqs[emp][i + 1];
        ind->seq_start_days[emp][i] = ind->seq_start_days[emp][i + 1];
    }
    ind->num_seqs[emp]--;

    remove_overlapping_sequences(ind, emp, new_start, new_seq->length);

    ind->seqs[emp][ind->num_seqs[emp]] = new_seq;
    ind->seq_start_days[emp][ind->num_seqs[emp]] = new_start;
    ind->num_seqs[emp]++;
}

int check_overlap(int start1, int len1, int start2, int len2) {
    int end1 = start1 + len1 - 1;
    int end2 = start2 + len2 - 1;
    return !(end1 < start2 || start1 > end2);
}

void remove_overlapping_sequences(individual *ind, int emp, int new_start, int new_len) {
    // Remove sequences that overlap with the new sequence position
    for (int i = ind->num_seqs[emp] - 1; i >= 0; i--) {
        int existing_start = ind->seq_start_days[emp][i];
        int existing_len = ind->seqs[emp][i]->length;
        
        if (check_overlap(new_start, new_len, existing_start, existing_len)) {
            // Remove overlapping sequence
            for (int j = i; j < ind->num_seqs[emp] - 1; j++) {
                ind->seqs[emp][j] = ind->seqs[emp][j + 1];
                ind->seq_start_days[emp][j] = ind->seq_start_days[emp][j + 1];
            }
            ind->num_seqs[emp]--;
        }
    }
}

// Función auxiliar para liberar la memoria estática al final del programa
void cleanup_mutation_memory() {
    // Esta función debería ser llamada al final del programa
    static int *alloc_sizes = NULL; // Acceder a la variable estática
    if (alloc_sizes) {
        free(alloc_sizes);
        alloc_sizes = NULL;
    }
}

