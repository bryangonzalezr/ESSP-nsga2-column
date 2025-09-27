/* ===== Modificaciones: modularización de generación de secuencias + ILS inicial ===== */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include "global.h"
#include "rand.h"

extern int popsize;
extern int nreal;
extern int nbin;
extern int init_type;
extern int ls_iters;
extern int ils_reset; /* lo usaremos como default iterations de ILS si >0 */
extern int *nbits;
extern double *min_realvar;
extern double *max_realvar;

/* Pools globales (se mantienen como antes) */
ssequence **ssequences_pool;
ssequence ***ssequences_pool_emp;
int *num_sequences_pool_emp;
seq_length_index *seq_index;


/* Forward de tus funciones existentes */
void backtrackWorkingSequence_C(problem_instance *pi, employee *emp, int *current, int current_len,
                                int *shift_count, ssequence ***ssequences_pool, int *num_results, int *pool_size);
void evaluate_ind(individual *ind, problem_instance *pi); /* ya definida en tu código */

/* ----------------- Helpers seguros para manipular secuencias en individuo ----------------- */

/* Comprueba si seq puede colocarse en ind para emp comenzando en start_day (sin overlap). */
int can_place_sequence(individual *ind, problem_instance *pi, int emp, ssequence *seq, int start_day) {
    if (!ind || !pi || !seq) return 0;
    int horizon = pi->horizon_length;
    int num_emps = pi->num_employees;
    if (start_day < 0 || start_day + seq->length > horizon) return 0;
    for (int i = 0; i < seq->length; i++) {
        if ((int)round(ind->xreal[(start_day + i) * num_emps + emp]) != 0) return 0; /* overlap */
    }
    return 1;
}

/* Coloca seq en ind para emp en start_day y actualiza estructuras (asume espacio en allocated_size). */
void place_sequence(individual *ind, problem_instance *pi, int emp, ssequence *seq, int start_day, int *allocated_size) {
    if (!ind || !pi || !seq) return;
    int num_emps = pi->num_employees;
    /* Asegura que arrays de secuencias existen */
    if (ind->seqs[emp] == NULL) {
        int init = (allocated_size[emp] > 0 ? allocated_size[emp] : 4);
        ind->seqs[emp] = malloc(init * sizeof(ssequence *));
        ind->seq_start_days[emp] = malloc(init * sizeof(int));
        allocated_size[emp] = init;
    }
    if (ind->num_seqs[emp] >= allocated_size[emp]) {
        allocated_size[emp] = allocated_size[emp] > 0 ? allocated_size[emp] * 2 : 4;
        ind->seqs[emp] = realloc(ind->seqs[emp], allocated_size[emp] * sizeof(ssequence *));
        ind->seq_start_days[emp] = realloc(ind->seq_start_days[emp], allocated_size[emp] * sizeof(int));
        if (!ind->seqs[emp] || !ind->seq_start_days[emp]) {
            fprintf(stderr, "Malloc/realloc falló en place_sequence\n");
            exit(1);
        }
    }

    for (int i = 0; i < seq->length; i++) {
        ind->xreal[(start_day + i) * num_emps + emp] = seq->shifts[i];
    }
    ind->seqs[emp][ind->num_seqs[emp]] = seq;
    ind->seq_start_days[emp][ind->num_seqs[emp]] = start_day;
    ind->num_seqs[emp]++;
}

/* Elimina la última secuencia añadida al empleado emp (asume que es la que queremos deshacer) */
void remove_last_sequence(individual *ind, problem_instance *pi, int emp) {
    if (!ind || !pi) return;
    if (ind->num_seqs[emp] <= 0) return;
    int last = ind->num_seqs[emp] - 1;
    ssequence *seq = ind->seqs[emp][last];
    int start = ind->seq_start_days[emp][last];
    int num_emps = pi->num_employees;
    for (int i = 0; i < seq->length; i++) {
        ind->xreal[(start + i) * num_emps + emp] = 0;
    }
    ind->num_seqs[emp]--;
    /* No liberamos seq porque pertenece al pool global */
}

/* ===================== Modularización de generación de secuencias ===================== */

/* Genera (o no) secuencias por empleado.
   mode == 0 -> usa backtrackWorkingSequence_C (comportamiento previo)
   mode == 1 -> placeholder: crea pools vacíos (posibilidad de implementar otra fuente)
*/
void generate_sequences_for_all(problem_instance *pi, int mode) {
    int num_emps = pi->num_employees;

    /* allocate local pools */
    ssequence ***local_pools = malloc(num_emps * sizeof(ssequence**));
    int *num_sequences_emp = malloc(num_emps * sizeof(int));
    int *pool_size_emp = malloc(num_emps * sizeof(int));

    /* inicializar estructuras de índices */
    seq_index = malloc(num_emps * sizeof(seq_length_index));
    for (int e = 0; e < num_emps; e++) {
        init_seq_index_for_employee(e, pi->horizon_length);
    }

    for (int e = 0; e < num_emps; e++) {
        pool_size_emp[e] = 16;
        local_pools[e] = malloc(pool_size_emp[e] * sizeof(ssequence*));
        if (!local_pools[e]) { fprintf(stderr,"malloc pool failed\n"); exit(1); }
        num_sequences_emp[e] = 0;
    }

    if (mode == 0) {
        /* comportamiento actual: backtrack para cada empleado */
        for (int e = 0; e < num_emps; e++) {
            employee *emp = &pi->employees[e];
            int *current = calloc(pi->horizon_length, sizeof(int));
            int *shift_count = calloc(pi->num_shifts, sizeof(int));
            num_sequences_emp[e] = 0;

            backtrackWorkingSequence_C(pi, emp, current, 0, shift_count,
                                    &local_pools[e], &num_sequences_emp[e], &pool_size_emp[e]);

            printf("Generated %d sequences for employee %s\n", num_sequences_emp[e], emp->name);

            free(current);
            free(shift_count);
        }
    } else {
        /* placeholder: no generar secuencias (vacío) */
        for (int e = 0; e < num_emps; e++) {
            num_sequences_emp[e] = 0;
            printf("Placeholder: no sequences generated for employee %s\n", pi->employees[e].name);
        }
    }

    /* Copiar a globals (ssequences_pool_emp, num_sequences_pool_emp) */
    num_sequences_pool_emp = malloc(num_emps * sizeof(int));
    ssequences_pool_emp = malloc(num_emps * sizeof(ssequence**));
    for (int e = 0; e < num_emps; e++) {
        num_sequences_pool_emp[e] = num_sequences_emp[e];
        if (num_sequences_emp[e] > 0) {
            ssequences_pool_emp[e] = malloc(num_sequences_emp[e] * sizeof(ssequence*));
            for (int s = 0; s < num_sequences_emp[e]; s++) {
                ssequences_pool_emp[e][s] = local_pools[e][s];

                /* indexar por largo */
                int len = local_pools[e][s]->length;
                add_seq_to_index(e, len, s);

                /* Print sequence for employee */
                printf("Emp %s: Seq %d (len=%d, min=%d): ", pi->employees[e].name, s, local_pools[e][s]->length, local_pools[e][s]->total_minutes);
                for (int k = 0; k < local_pools[e][s]->length; k++) {
                    printf("%d ", local_pools[e][s]->shifts[k]);
                }
                printf("\n");
            }
        } else {
            ssequences_pool_emp[e] = NULL;
        }
    }

    /* liberar arrays locales (no las secuencias) */
    for (int e = 0; e < num_emps; e++) free(local_pools[e]);
    free(local_pools);
    free(pool_size_emp);
    free(num_sequences_emp);
}

/* ===================== ILS inicial (perturbación: add) ===================== */

/* Inicializa un individuo con ILS corta usando add como perturbación.
   Parámetros internos ajustables: ils_iters_default, attempts por iteración, tries por intento de colocación.
*/
void initialize_individual_ils(individual *ind, problem_instance *pi, ssequence ***ssequences_pool_emp_local, int *num_sequences_emp) {
    if (!ind || !pi) return;
    int horizon = pi->horizon_length;
    int num_emps = pi->num_employees;

    /* Asegurar xreal inicializado a 0 */
    for (int d = 0; d < horizon; d++)
        for (int e = 0; e < num_emps; e++)
            ind->xreal[d * num_emps + e] = 0;

    /* inicializar estructuras de secuencias */
    ind->seqs = (ssequence***)malloc(num_emps * sizeof(ssequence**));
    ind->seq_start_days = (int**)malloc(num_emps * sizeof(int*));
    ind->num_seqs = (int*)malloc(num_emps * sizeof(int));
    int *allocated_size = (int*)malloc(num_emps * sizeof(int));
    for (int e = 0; e < num_emps; e++) {
        ind->seqs[e] = NULL;
        ind->seq_start_days[e] = NULL;
        ind->num_seqs[e] = 0;
        allocated_size[e] = 4;
    }

    /* Evaluación inicial (vacío) */
    evaluate_ind(ind, pi);
    double best_constr = ind->constr_violation;
    double best_obj_sum = ind->obj[0] + ind->obj[1];

    /* parámetros ILS */
    int ils_iters_default = (ils_reset > 0 ? ils_reset : 12); /* iteraciones ILS */
    int ils_attempts_per_iter = 1000; /* cuántas perturbaciones probar por iteración */
    int try_starts_per_attempt = 12; /* intentos de colocación por perturbación */

    for (int iter = 0; iter < ils_iters_default; iter++) {
        int accepted_any = 0;
        for (int attempt = 0; attempt < ils_attempts_per_iter; attempt++) {
            /* Perturbación: intentar 'add' una secuencia aleatoria con placement aleatorio */
            int placed = 0;
            int chosen_emp = -1;
            ssequence *chosen_seq = NULL;
            int chosen_start = -1;

            /* Try several times to find a valid placement */
            for (int t = 0; t < try_starts_per_attempt && !placed; t++) {
                int emp = rnd(0, num_emps - 1);
                int pool_size = num_sequences_emp[emp];
                if (pool_size <= 0) continue;
                int seq_idx = rnd(0, pool_size - 1);
                ssequence *seq = ssequences_pool_emp_local[emp][seq_idx];
                if (!seq) continue;
                /* pick random start */
                int start = rnd(0, (horizon - seq->length >= 0) ? (horizon - seq->length) : 0);
                if (start + seq->length > horizon) continue;
                if (!can_place_sequence(ind, pi, emp, seq, start)) continue;
                /* place temporarily */
                place_sequence(ind, pi, emp, seq, start, allocated_size);
                chosen_emp = emp;
                chosen_seq = seq;
                chosen_start = start;
                placed = 1;
            }

            if (!placed) continue; /* no se encontró ubicación en este attempt */

            /* evaluar tras colocar */
            evaluate_ind(ind, pi);
            double new_constr = ind->constr_violation;
            double new_obj_sum = ind->obj[0] + ind->obj[1];

            /* criterio de aceptación: primero mejorar factibilidad (constr closer to 0), luego mejoras en el sum objetivo */
            int accept = 0;
            if (new_constr > best_constr) accept = 1;
            else if (fabs(new_constr - best_constr) < 1e-9 && new_obj_sum < best_obj_sum) accept = 1;

            if (accept) {
                best_constr = new_constr;
                best_obj_sum = new_obj_sum;
                accepted_any = 1;
                /* keep placement (ya está en ind) */
            } else {
                /* revertir => quitar la última secuencia añadida del empleado elegido */
                if (chosen_emp != -1) remove_last_sequence(ind, pi, chosen_emp);
                /* re-evaluar para restaurar valores previos */
                evaluate_ind(ind, pi);
            }
        } /* attempts por iter */

        /* Diversificación ligera: si ninguna perturbación aceptada, permitir una aceptación azarosa (simulated annealing estilo) */
        if (!accepted_any) {
            /* small chance to add a random valid sequence to escape */
            if (rnd(0, 9) == 0) { /* 10% */
                int tries = 0;
                while (tries < 50) {
                    int emp = rnd(0, num_emps - 1);
                    int pool_size = num_sequences_emp[emp];
                    if (pool_size <= 0) { tries++; continue; }
                    int seq_idx = rnd(0, pool_size - 1);
                    ssequence *seq = ssequences_pool_emp_local[emp][seq_idx];
                    if (!seq) { tries++; continue; }
                    int start = rnd(0, (horizon - seq->length >= 0) ? (horizon - seq->length) : 0);
                    if (start + seq->length > horizon) { tries++; continue; }
                    if (!can_place_sequence(ind, pi, emp, seq, start)) { tries++; continue; }
                    place_sequence(ind, pi, emp, seq, start, allocated_size);
                    evaluate_ind(ind, pi);
                    best_constr = ind->constr_violation;
                    best_obj_sum = ind->obj[0] + ind->obj[1];
                    break;
                }
            }
        }
    } /* iteraciones ILS */

    /* final: ind ya está listo */
    free(allocated_size);
}

/* ===================== initialize_individual_random (tu versión, sin srand interno) ===================== */

void initialize_individual_random(individual *ind, problem_instance *pi,
                                  ssequence ***ssequences_pool_emp_local, int *num_sequences_emp) {
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
            int idx = rnd(0, pool_size - 1);
            ssequence *seq = ssequences_pool_emp_local[e][idx];

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

            if (day < horizon && rnd(0,1) == 0) day++;
        }
    }

    free(allocated_size);
}

/* ===================== initialize_pop integrado (usa generate_sequences_for_all + ILS) ===================== */

void initialize_pop(population *pop, problem_instance *pi) {
    printf("Initializing population and generating sequences per employee...\n");

    /* generar secuencias: mode 0 -> backtrack (comportamiento por defecto) */
    generate_sequences_for_all(pi, 0);

    

    /* proporciones: configurable aquí (ajusta como quieras) */
    int ils_count = popsize / 2; /* cuántos individuos con ILS inicial */
    int random_count = popsize - ils_count;

    for (int i = 0; i < popsize; i++) {
        individual *ind = &(pop->ind[i]);
        /* asegurar xreal inicializado */
        int total_size = pi->horizon_length * pi->num_employees;
        /* Si xreal no apunta a memoria válida, asume que ya fue alocado en otro lugar de tu código.
           Si no, sería necesario malloc aquí. Asumo que ind->xreal ya está reservado en la estructura. */
        for (int j = 0; j < total_size; j++) ind->xreal[j] = 0;

        ind->seqs = (ssequence***)malloc(pi->num_employees * sizeof(ssequence**));
        ind->seq_start_days = (int**)malloc(pi->num_employees * sizeof(int*));
        ind->num_seqs = (int*)malloc(pi->num_employees * sizeof(int));
        for (int e = 0; e < pi->num_employees; e++) {
            ind->seqs[e] = NULL;
            ind->seq_start_days[e] = NULL;
            ind->num_seqs[e] = 0;
        }

        if (i < ils_count) {
            initialize_individual_ils(ind, pi, ssequences_pool_emp, num_sequences_pool_emp);
        } else {
            initialize_individual_random(ind, pi, ssequences_pool_emp, num_sequences_pool_emp);
        }

        evaluate_ind(ind, pi);
        if ((i % 10) == 0) printf("Initialization progress: built individual %d\n", i);
    }

    printf("Initialization finished (popsize=%d)\n", popsize);
}

/* ===================== Resto de funciones existentes (backtrackWorkingSequence_C, add_sequence_to_pool, etc)
   deben seguir en el fichero tal cual las tenías. No las reescribo aquí para no duplicar.
   Ya dejé generate_sequences_for_all llamando a backtrackWorkingSequence_C cuando mode==0.
*/


void add_sequence_to_pool(
    ssequence ***pool,
    ssequence *seq,
    int *num_results,
    int *pool_size,
    int emp
) {
    if (*num_results >= *pool_size) {
        *pool_size *= 2;
        *pool = realloc(*pool, (*pool_size) * sizeof(ssequence*));
        if (!*pool) {
            fprintf(stderr, "Memory allocation failed for sequence pool!\n");
            exit(1);
        }
    }

    (*pool)[*num_results] = seq;

    // --- Actualizar índice por longitud ---
    int len = seq->length;
    seq_length_index *idx = &seq_index[emp];

    if (idx->capacity_by_length[len] == 0) {
        idx->capacity_by_length[len] = 16;
        idx->by_length[len] = malloc(idx->capacity_by_length[len] * sizeof(int));
    } else if (idx->count_by_length[len] >= idx->capacity_by_length[len]) {
        idx->capacity_by_length[len] *= 2;
        idx->by_length[len] = realloc(idx->by_length[len],
                                      idx->capacity_by_length[len] * sizeof(int));
    }

    idx->by_length[len][idx->count_by_length[len]++] = *num_results;

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

    if (current_len>0 &&(emp->max_consecutive_shifts == 0 || current_len <= emp->max_consecutive_shifts)) {

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


        add_sequence_to_pool(ssequences_pool, seq, num_results, pool_size, emp->id);
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

void init_seq_index_for_employee(int emp, int max_length) {
    seq_index[emp].max_length = max_length;
    seq_index[emp].by_length = malloc((max_length + 1) * sizeof(int*));
    seq_index[emp].count_by_length = calloc(max_length + 1, sizeof(int));
    seq_index[emp].capacity_by_length = calloc(max_length + 1, sizeof(int));

    for (int l = 0; l <= max_length; l++) {
        seq_index[emp].by_length[l] = NULL;
        seq_index[emp].count_by_length[l] = 0;
        seq_index[emp].capacity_by_length[l] = 0;
    }
}


void add_seq_to_index(int emp, int length, int pool_idx) {
    if (length > seq_index[emp].max_length) return;

    if (seq_index[emp].count_by_length[length] >= seq_index[emp].capacity_by_length[length]) {
        int new_cap = (seq_index[emp].capacity_by_length[length] == 0) ? 4 : seq_index[emp].capacity_by_length[length] * 2;
        seq_index[emp].by_length[length] = realloc(seq_index[emp].by_length[length], new_cap * sizeof(int));
        seq_index[emp].capacity_by_length[length] = new_cap;
    }

    seq_index[emp].by_length[length][seq_index[emp].count_by_length[length]] = pool_idx;
    seq_index[emp].count_by_length[length]++;
}