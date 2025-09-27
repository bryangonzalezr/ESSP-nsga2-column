/* Routine for mergeing two populations */

# include <stdio.h>
# include <stdlib.h>
# include <math.h>

# include "global.h"
# include "rand.h"

extern problem_instance *pi; // Global problem instance

/* Routine to merge two populations into one */
void merge(population *pop1, population *pop2, population *pop3)

{
    int i, k;
    for (i=0; i<popsize; i++)
    {
        copy_ind (&(pop1->ind[i]), &(pop3->ind[i]));
    }
    for (i=0, k=popsize; i<popsize; i++, k++)
    {
        copy_ind (&(pop2->ind[i]), &(pop3->ind[k]));
    }
    return;
}


void copy_ind(individual *ind1, individual *ind2)
{
    int i, j;

    // Copiar atributos simples
    ind2->rank = ind1->rank;
    ind2->constr_violation = ind1->constr_violation;

    // Copiar xreal
    if (nreal != 0) {
        if (!ind2->xreal) ind2->xreal = (double *)malloc(nreal * sizeof(double));
        for (i = 0; i < nreal; i++) ind2->xreal[i] = ind1->xreal[i];
    }

    // Copiar xbin y gene
    if (nbin != 0) {
        if (!ind2->xbin) ind2->xbin = (double *)malloc(nbin * sizeof(double));
        if (!ind2->gene) ind2->gene = (int **)malloc(nbin * sizeof(int *));
        for (i = 0; i < nbin; i++) {
            if (!ind2->gene[i]) ind2->gene[i] = (int *)malloc(nbits[i] * sizeof(int));
            ind2->xbin[i] = ind1->xbin[i];
            for (j = 0; j < nbits[i]; j++)
                ind2->gene[i][j] = ind1->gene[i][j];
        }
    }

    // Copiar objetivos
    if (!ind2->obj) ind2->obj = (double *)malloc(nobj * sizeof(double));
    for (i = 0; i < nobj; i++) ind2->obj[i] = ind1->obj[i];

    // Copiar restricciones
    if (ncon != 0) {
        if (!ind2->constr) ind2->constr = (double *)malloc(ncon * sizeof(double));
        for (i = 0; i < ncon; i++) ind2->constr[i] = ind1->constr[i];
    }

    // ========== PARTE CRÍTICA: Copiar secuencias dinámicas ==========
    
    // Liberar memoria existente del destino
    if (ind2->seqs) {
        for (i = 0; i < pi->num_employees; i++) {
            if (ind2->seqs[i]) {
                // IMPORTANTE: No liberar las secuencias individuales aquí 
                // porque son shallow copies que pueden estar siendo usadas
                free(ind2->seqs[i]);
                ind2->seqs[i] = NULL;
            }
        }
        free(ind2->seqs);
    }
    
    if (ind2->seq_start_days) {
        for (i = 0; i < pi->num_employees; i++) {
            if (ind2->seq_start_days[i]) {
                free(ind2->seq_start_days[i]);
                ind2->seq_start_days[i] = NULL;
            }
        }
        free(ind2->seq_start_days);
    }
    
    if (ind2->num_seqs) {
        free(ind2->num_seqs);
    }

    // Asignar memoria nueva para los arrays principales
    ind2->num_seqs = (int *)malloc(pi->num_employees * sizeof(int));
    ind2->seq_start_days = (int **)malloc(pi->num_employees * sizeof(int *));
    ind2->seqs = (ssequence ***)malloc(pi->num_employees * sizeof(ssequence **));
    
    // Inicializar punteros a NULL
    for (i = 0; i < pi->num_employees; i++) {
        ind2->seq_start_days[i] = NULL;
        ind2->seqs[i] = NULL;
        ind2->num_seqs[i] = 0;
    }

    // Copiar empleado por empleado
    for (i = 0; i < pi->num_employees; i++) {
        int num_sequences = ind1->num_seqs[i];
        ind2->num_seqs[i] = num_sequences;
        
        if (num_sequences > 0) {
            // CRÍTICO: Asignar memoria basada en el número REAL de secuencias
            ind2->seqs[i] = (ssequence **)malloc(num_sequences * sizeof(ssequence *));
            ind2->seq_start_days[i] = (int *)malloc(num_sequences * sizeof(int));
            
            if (!ind2->seqs[i] || !ind2->seq_start_days[i]) {
                fprintf(stderr, "ERROR: No se pudo asignar memoria para empleado %d (%d secuencias)\n", 
                        i, num_sequences);
                exit(1);
            }
            
            // Copiar secuencia por secuencia
            for (j = 0; j < num_sequences; j++) {
                // SHALLOW COPY: Mantener el comportamiento original
                // Las secuencias son compartidas entre individuos
                ind2->seqs[i][j] = ind1->seqs[i][j];
                ind2->seq_start_days[i][j] = ind1->seq_start_days[i][j];
            }
        }
    }
}