/* ===== Modificaciones: modularización de generación de secuencias + ILS inicial ===== */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
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
ssequence ***ssequences_pool_emp; /* por empleado: ssequences_pool_emp[emp][idx] */
int *num_sequences_pool_emp;
seq_length_index *seq_index;
int count = 0;

/* employees_pool: por empleado, un array dinámico de emp_assign (copias) */
emp_assign **employees_pool;               /* employees_pool[emp] -> emp_assign[] */
int *employees_pool_capacity;             /* capacidad por empleado */
int *count_employees_pool;                /* contadores por empleado */

/* Forward de tus funciones existentes */
void backtrackWorkingSequence_C(problem_instance *pi, employee *emp, int *current, int current_len,
                                int *shift_count, ssequence ***ssequences_pool, int *num_results, int *pool_size);
void evaluate_ind(individual *ind, problem_instance *pi); /* ya definida en tu código */
void init_seq_index_for_employee(int emp, int max_length);
void add_seq_to_index(int emp, int length, int pool_idx);
void backtracking_employee_seq(emp_assign *current_emp, int day, int consecutive_shifts, int consecutive_off);

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

    if (!local_pools || !num_sequences_emp || !pool_size_emp) {
        fprintf(stderr, "malloc failed in generate_sequences_for_all\n");
        exit(1);
    }

    /* inicializar estructuras de índices */
    seq_index = malloc(num_emps * sizeof(seq_length_index));
    if (!seq_index) { fprintf(stderr, "malloc seq_index failed\n"); exit(1); }
    for (int e = 0; e < num_emps; e++) {
        init_seq_index_for_employee(e, pi->employees[e].max_consecutive_shifts);
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
            if (!current || !shift_count) { fprintf(stderr,"calloc failed\n"); exit(1); }
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
    if (!num_sequences_pool_emp || !ssequences_pool_emp) { fprintf(stderr,"malloc failed\n"); exit(1); }

    for (int e = 0; e < num_emps; e++) {
        num_sequences_pool_emp[e] = num_sequences_emp[e];
        if (num_sequences_emp[e] > 0) {
            ssequences_pool_emp[e] = malloc(num_sequences_emp[e] * sizeof(ssequence*));
            if (!ssequences_pool_emp[e]) { fprintf(stderr,"malloc failed\n"); exit(1); }
            for (int s = 0; s < num_sequences_emp[e]; s++) {
                ssequences_pool_emp[e][s] = local_pools[e][s];
                /* NOTE: add_sequence_to_pool ya actualizó seq_index durante generación,
                   por eso NO volvemos a llamar add_seq_to_index aquí (evitamos duplicados). */

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

/* ===================== initialize_pop integrado (usa generate_sequences_for_all + ILS) ===================== */

void initialize_pop(population *pop, problem_instance *pi) {
    printf("Initializing population and generating sequences per employee...\n");

    generate_sequences_for_all(pi, 0); // genera pool de secuencias base (por tipo)

    int num_emps = pi->num_employees;

    /* ====== Crear pools de asignaciones factibles por empleado ====== */
    employees_pool = malloc(num_emps * sizeof(emp_assign*));
    employees_pool_capacity = malloc(num_emps * sizeof(int));
    count_employees_pool = calloc(num_emps, sizeof(int));

    if (!employees_pool || !employees_pool_capacity || !count_employees_pool) {
        fprintf(stderr, "malloc failed for employees_pool structures\n");
        exit(1);
    }

    for (int e = 0; e < num_emps; e++) {
        employees_pool_capacity[e] = 50;
        employees_pool[e] = malloc(employees_pool_capacity[e] * sizeof(emp_assign));
        if (!employees_pool[e]) {
            fprintf(stderr, "malloc failed employees_pool[%d]\n", e);
            exit(1);
        }
        count_employees_pool[e] = 0;
    }
    printf("Empezando a crear\n");
    for (int e = 0; e < num_emps; e++) {
        printf("Empleado %d...\n", e);

        emp_assign current_emp;
        current_emp.emp_id = e;
        current_emp.num_seqs = 0;
        current_emp.seq_start_day = malloc(pi->horizon_length * sizeof(int));
        current_emp.seqs = malloc(pi->horizon_length * sizeof(ssequence*));

        if (!current_emp.seq_start_day || !current_emp.seqs) {
            fprintf(stderr, "malloc failed for current_emp arrays (emp %d)\n", e);
            exit(1);
        }

        for (int i = 0; i < pi->horizon_length; i++) {
            current_emp.seq_start_day[i] = -1;
            current_emp.seqs[i] = NULL;
        }

        printf("Empezando el backtracking\n");
        backtracking_employee_seq(&current_emp, 0, 0, 0);
        printf("Terminando el backtracking\n");

        free(current_emp.seq_start_day);
        free(current_emp.seqs);

        printf("Listo\n");
    }

    printf("Ya se crearon\n");
    /* Mostrar resumen */
    int total_pool_size = 0;
    for (int e = 0; e < num_emps; e++) total_pool_size += count_employees_pool[e];
    printf("Total feasible emp_assign found: %d (avg %.1f per employee)\n",
           total_pool_size, (double)total_pool_size / num_emps);

    /* ====== Inicializar población ====== */
    for (int i = 0; i < popsize; i++) {
        individual *ind = &(pop->ind[i]);

        int total_size = pi->horizon_length * pi->num_employees;
        for (int j = 0; j < total_size; j++) ind->xreal[j] = 0;

        ind->seqs = malloc(num_emps * sizeof(ssequence**));
        ind->seq_start_days = malloc(num_emps * sizeof(int*));
        ind->num_seqs = malloc(num_emps * sizeof(int));

        if (!ind->seqs || !ind->seq_start_days || !ind->num_seqs) {
            fprintf(stderr, "malloc failed for ind arrays\n");
            exit(1);
        }

        /* ====== Selección aleatoria de asignaciones factibles ====== */
        for (int e = 0; e < num_emps; e++) {
            int pool_size = count_employees_pool[e];
            if (pool_size == 0) {
                fprintf(stderr, "Warning: no feasible sequences found for employee %d\n", e);
                ind->seqs[e] = NULL;
                ind->seq_start_days[e] = NULL;
                ind->num_seqs[e] = 0;
                continue;
            }

            int r = rand() % pool_size;  // seleccion aleatoria
            emp_assign *chosen = &employees_pool[e][r];

            ind->num_seqs[e] = chosen->num_seqs;
            ind->seqs[e] = malloc(chosen->num_seqs * sizeof(ssequence*));
            ind->seq_start_days[e] = malloc(chosen->num_seqs * sizeof(int));

            for (int s = 0; s < chosen->num_seqs; s++) {
                ind->seqs[e][s] = chosen->seqs[s];
                ind->seq_start_days[e][s] = chosen->seq_start_day[s];
            }

            /* Marca los turnos en xreal */
            for (int s = 0; s < chosen->num_seqs; s++) {
                ssequence *seq = chosen->seqs[s];
                int start = chosen->seq_start_day[s];
                for (int d = 0; d < seq->length; d++) {
                    int day = start + d;
                    if (day >= pi->horizon_length) break;
                    ind->xreal[day * num_emps + e] = seq->shifts[d];
                }
            }
        }

        evaluate_ind(ind, pi);

        if (i % 10 == 0) printf("Initialization progress: built individual %d\n", i);
    }

    printf("Initialization finished (popsize=%d)\n", popsize);
}

/* ===================== Resto de funciones existentes (backtrackWorkingSequence_C, add_sequence_to_pool, etc)
   deben seguir en el fichero tal cual las tenías. No las reescribo aquí para no duplicar.
   Ya dejé generate_sequences_for_all llamando a backtrackWorkingSequence_C cuando mode==0.
*/

/* add_sequence_to_pool: asegura pool resizing y actualiza índice por longitud */
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

/* backtrackWorkingSequence_C: sin cambios lógicos, solo chequeo de memory */
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
        if (!seq) { fprintf(stderr,"malloc seq failed\n"); exit(1); }
        seq->length = current_len;
        seq->total_minutes = total_minutes;
        if (current_len > 0) {
            seq->shifts = malloc(sizeof(int) * current_len);
            if (!seq->shifts) { fprintf(stderr,"malloc seq->shifts failed\n"); exit(1); }
            memcpy(seq->shifts, current, sizeof(int) * current_len);
        } else {
            seq->shifts = NULL;
        }

        add_sequence_to_pool(ssequences_pool, seq, num_results, pool_size, emp->id);
    }

    // Try all shifts
    for (int shift_id = 1; shift_id < pi->num_shifts; shift_id++) {
        if (emp->max_shifts[shift_id] == 0) {
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

    if (!seq_index[emp].by_length || !seq_index[emp].count_by_length || !seq_index[emp].capacity_by_length) {
        fprintf(stderr, "malloc failed in init_seq_index_for_employee\n");
        exit(1);
    }

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

/* helper para obtener turno en día dado a partir de current_emp (busca en secuencias añadidas) */
static inline int get_shift_for_day(emp_assign *current_emp, int day) {
    for (int i = 0; i < current_emp->num_seqs; i++) {
        ssequence *seq = current_emp->seqs[i];
        int start_day = current_emp->seq_start_day[i];
        if (day >= start_day && day < start_day + seq->length) {
            return seq->shifts[day - start_day];
        }
    }
    return 0; // descanso por defecto
}

/* (Se mantienen las funciones can_add_sequence, violates_R2, etc. tal cual las tenías). */
/* ... (aquí van can_add_sequence y violates_R2 tal como las definiste en tu código original) ... */




bool can_add_sequence(emp_assign *current_emp){
    int emp_id = current_emp->emp_id;
    employee *emp = &pi->employees[emp_id];

    int horizon_length = pi->horizon_length;
    int num_shifts = pi->num_shifts;

    // Estado acumulado
    int *shift_count = (int *)malloc(num_shifts * sizeof(int));
    if (!shift_count) return false;
    for (int i = 0; i < num_shifts; i++) shift_count[i] = 0;

    int total_minutes = 0;
    int consecutive_shifts = 0;
    int consecutive_off = 0;
    int weekcount = 0;

    // Recorrer el horizonte
    for (int day = 0; day < horizon_length; day++) {
        int shift_id = get_shift_for_day(current_emp, day);

        // R1: días libres específicos
        for (int j = 0; j < emp->num_days_off; j++) {
            if (emp->days_off[j] == day && shift_id != 0) {
                free(shift_count);
                // printf("Restr 1\n");
                return false; // CORREGIDO: debe retornar false
            }
        }

        // R2: máximo por tipo de turno
        if (shift_id > 0 && shift_id < num_shifts) {
            shift_count[shift_id]++;
            if (shift_count[shift_id] > emp->max_shifts[shift_id]) {
                free(shift_count);
                // printf("Restr 2\n");
                return false;
            }
        }

        // R3: incompatibilidades
        // if (day > 0) {
        //     int prev_shift = get_shift_for_day(current_emp, day - 1);
        //     if (prev_shift > 0 && shift_id > 0) { // CORREGIDO: verificar que shift_id también sea > 0
        //         for (int k = 0; k < pi->shifts[prev_shift].num_incompatible_shifts; k++) {
        //             if (pi->shifts[prev_shift].incompatible_shifts[k] == shift_id) {
        //                 free(shift_count);
        //                 printf("Restr 1\n");
        //                 return false;
        //             }
        //         }
        //     }
        // }

        // R4: min/max consecutivos
        // if (shift_id != 0) {
        //     if (consecutive_off > 0) {
        //         if (consecutive_off < emp->min_consecutive_days_off) {
        //             free(shift_count);
        //             printf("Restr 3\n");
        //             return false;
        //         }
        //         consecutive_off = 0;
        //     }
        //     consecutive_shifts++;
        //     if (consecutive_shifts > emp->max_consecutive_shifts) {
        //         free(shift_count);
        //         printf("Restr 3\n");
        //         return false;
        //     }
        // } else {
        //     if (consecutive_shifts > 0) {
        //         if (consecutive_shifts < emp->min_consecutive_shifts) {
        //             free(shift_count);
        //             printf("Restr 3\n");
        //             return false;
        //         }
        //         consecutive_shifts = 0;
        //     }
        //     consecutive_off++;
        // }

        // R5: fines de semana
        if (day % 7 == 5) { // sábado
            if (shift_id != 0) {
                weekcount++;
            } else if (day + 1 < horizon_length && get_shift_for_day(current_emp, day + 1) != 0) {
                weekcount++;
            }
        }

        total_minutes += pi->shifts[shift_id].length;
    }

    // R6: chequeo de fines de semana
    if (weekcount > emp->max_weekends) {
        // printf("R6\n");
        free(shift_count);
        return false;
    }

    // // R7: validación final de consecutivos
    // // CORREGIDO: validar si terminamos en medio de una secuencia
    // if (consecutive_shifts > 0 && consecutive_shifts < emp->min_consecutive_shifts) {
    //     free(shift_count);
    //     return false;
    // }
    // if (consecutive_off > 0 && consecutive_off < emp->min_consecutive_days_off) {
    //     free(shift_count);
    //     return false;
    // }

    free(shift_count);
    return true;
}

bool violates_R2(emp_assign *current_emp, ssequence *new_seq, employee *emp, problem_instance *pi) {
    int num_shifts = pi->num_shifts;
    int *shift_count = (int *)calloc(num_shifts, sizeof(int));
    if (!shift_count) return true;

    // Contar turnos ya asignados
    for (int i = 0; i < current_emp->num_seqs; i++) {
        ssequence *seq = current_emp->seqs[i];
        for (int d = 0; d < seq->length; d++) {
            int sid = seq->shifts[d];
            if (sid > 0 && sid < num_shifts)
                shift_count[sid]++;
        }
    }

    // Contar turnos del candidato
    for (int d = 0; d < new_seq->length; d++) {
        int sid = new_seq->shifts[d];
        if (sid > 0 && sid < num_shifts)
            shift_count[sid]++;
    }

    // Validar límites
    bool violates = false;
    for (int s = 1; s < num_shifts; s++) {
        if (shift_count[s] > emp->max_shifts[s]) {
            violates = true;
            break;
        }
    }

    free(shift_count);
    return violates;
}

/* ----------------- Inserción segura de emp_assign factible en employees_pool ----------------- */
/* Esta función se usa dentro del backtracking cuando se encuentra un emp_assign factible. */
void store_feasible_emp_assign(emp_assign *emp_ptr) {
    int e = emp_ptr->emp_id;

    if (count_employees_pool[e] >= employees_pool_capacity[e]) {
        employees_pool_capacity[e] *= 2;
        employees_pool[e] = realloc(employees_pool[e],
                                    employees_pool_capacity[e] * sizeof(emp_assign));
        if (!employees_pool[e]) {
            fprintf(stderr, "realloc failed for employees_pool[%d]\n", e);
            exit(1);
        }
    }

    // Copia por valor (estructuras pequeñas)
    employees_pool[e][count_employees_pool[e]] = *emp_ptr;
    count_employees_pool[e]++;

    // No liberar emp_ptr, porque apunta a arrays útiles
}

/* ===================== Backtracking por empleado para construir emp_assign factibles ===================== */

void backtracking_employee_seq(emp_assign *current_emp, int day,
                               int consecutive_shifts, int consecutive_off) {
    int emp_id = current_emp->emp_id;

    /* ====== Caso base ====== */
    if (eval_employee_feasible(current_emp, pi)) {
            // printf("Alo...");
            int emp_id = current_emp->emp_id;

            /* Crear copia dinámica completa (deep copy) */
            emp_assign *aux_emp = malloc(sizeof(emp_assign));
            // if (!aux_emp) {
            //     fprintf(stderr, "malloc failed for aux_emp struct\n");
            //     exit(1);
            // }

            aux_emp->emp_id = emp_id;
            aux_emp->num_seqs = current_emp->num_seqs;

            if (aux_emp->num_seqs > 0) {
                aux_emp->seq_start_day = malloc(aux_emp->num_seqs * sizeof(int));
                aux_emp->seqs = malloc(aux_emp->num_seqs * sizeof(ssequence*));
                // if (!aux_emp->seq_start_day || !aux_emp->seqs) {
                //     fprintf(stderr, "malloc failed copying aux_emp arrays\n");
                //     free(aux_emp);
                //     exit(1);
                // }

                for (int i = 0; i < aux_emp->num_seqs; i++) {
                    aux_emp->seq_start_day[i] = current_emp->seq_start_day[i];
                    aux_emp->seqs[i] = current_emp->seqs[i]; // punteros compartidos
                }
            } else {
                aux_emp->seq_start_day = NULL;
                aux_emp->seqs = NULL;
            }

            store_feasible_emp_assign(aux_emp);
            // printf("Adios\n");
            return;
        }

    /* ====== Determinar rango de largos posibles ====== */
    int max_length = seq_index[emp_id].max_length;
    if (day + max_length > pi->horizon_length)
        max_length = pi->horizon_length - day;

    int min_length = pi->employees[emp_id].min_consecutive_shifts;
    if (day == 0 || day >= pi->horizon_length - min_length)
        min_length = 1;

    /* ====== OPCIÓN 1: Asignar secuencia de trabajo ====== */
    for (int i = min_length; i <= max_length; i++) {
        int available = seq_index[emp_id].count_by_length[i];
        if (available == 0) continue;

        employee *emp = &pi->employees[emp_id];

        /* Probar varias secuencias aleatorias de este largo (hasta available intentos) */
        for (int attempt = 0; attempt < available; attempt++) {
            int seq_num = rnd(0, available-1); /* asumo rnd devuelve [0, available-1] */
            int seq_idx = seq_index[emp_id].by_length[i][seq_num];
            ssequence *ch_seq = ssequences_pool_emp[emp_id][seq_idx];

            /* Chequear restricción R2 */
            if (violates_R2(current_emp, ch_seq, emp, pi)) {
                continue;
            }

            /* Agregar secuencia (apuntar al pool) */
            current_emp->seq_start_day[current_emp->num_seqs] = day;
            current_emp->seqs[current_emp->num_seqs] = ch_seq;
            current_emp->num_seqs++;

            if (can_add_sequence(current_emp)) {
                int next_day = day + ch_seq->length;

                /* CASO A: Después de la secuencia, poner días OFF */
                int min_off = emp->min_consecutive_days_off;
                int max_off = pi->horizon_length - next_day;

                for (int days_off = min_off; days_off <= max_off; days_off++) {
                    if (next_day + days_off <= pi->horizon_length) {
                        backtracking_employee_seq(current_emp,
                                                  next_day + days_off,
                                                  0, days_off);
                    }
                }

                /* CASO B: Terminar justo al final */
                if (next_day == pi->horizon_length) {
                    backtracking_employee_seq(current_emp, next_day, 0, 0);
                }
            }

            /* Deshacer */
            current_emp->num_seqs--;
            break; /* ya intentamos una secuencia válida para este largo; pasar al siguiente largo */
        }
    }

    /* ====== OPCIÓN 2: Empezar con días OFF (solo al inicio) ====== */
    if (day == 0) {
        int min_off = pi->employees[emp_id].min_consecutive_days_off;
        int max_off = 5 - day;
        if (max_off < min_off) max_off = min_off;
        for (int days_off = min_off; days_off <= max_off; days_off++) {
            if (day + days_off <= pi->horizon_length) {
                backtracking_employee_seq(current_emp,
                                          day + days_off,
                                          0, days_off);
            }
        }
    }
}
