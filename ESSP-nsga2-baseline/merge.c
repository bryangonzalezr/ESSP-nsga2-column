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

/* Routine to copy an individual 'ind1' into another individual 'ind2' */
void copy_ind(individual *ind1, individual *ind2)
{
    int i, j;

    ind2->rank = ind1->rank;
    ind2->constr_violation = ind1->constr_violation;
    // ind2->crowd_dist = ind1->crowd_dist;

    // xreal
    if (nreal > 0) {
        if (!ind2->xreal) ind2->xreal = malloc(nreal * sizeof(double));
        for (i = 0; i < nreal; i++) ind2->xreal[i] = ind1->xreal[i];
    }

    // xbin + gene
    if (nbin > 0) {
        if (!ind2->xbin) ind2->xbin = malloc(nbin * sizeof(double));
        if (!ind2->gene) {
            ind2->gene = calloc(nbin, sizeof(int *));  // zero-init
        }
        for (i = 0; i < nbin; i++) {
            if (!ind2->gene[i]) {
                ind2->gene[i] = malloc(nbits[i] * sizeof(int));
            }
            ind2->xbin[i] = ind1->xbin[i];
            for (j = 0; j < nbits[i]; j++)
                ind2->gene[i][j] = ind1->gene[i][j];
        }
    }

    // objectives
    if (!ind2->obj) ind2->obj = malloc(nobj * sizeof(double));
    for (i = 0; i < nobj; i++) ind2->obj[i] = ind1->obj[i];

    // constraints
    if (ncon > 0) {
        if (!ind2->constr) ind2->constr = malloc(ncon * sizeof(double));
        for (i = 0; i < ncon; i++) ind2->constr[i] = ind1->constr[i];
    }

    // sequences
    if (!ind2->num_seqs) ind2->num_seqs = malloc(pi->num_employees * sizeof(int));
    if (!ind2->seq_start_days) ind2->seq_start_days = calloc(pi->num_employees, sizeof(int *));
    if (!ind2->seqs) ind2->seqs = calloc(pi->num_employees, sizeof(ssequence **));

    for (i = 0; i < pi->num_employees; i++) {
        ind2->num_seqs[i] = ind1->num_seqs[i];

        if (!ind2->seq_start_days[i]) {
            ind2->seq_start_days[i] = malloc(ind1->num_seqs[i] * sizeof(int));
        }
        if (!ind2->seqs[i]) {
            ind2->seqs[i] = malloc(ind1->num_seqs[i] * sizeof(ssequence *));
        }

        for (j = 0; j < ind1->num_seqs[i]; j++) {
            // ⚠️ Shallow copy (share same seqs)
            ind2->seqs[i][j] = ind1->seqs[i][j];
            ind2->seq_start_days[i][j] = ind1->seq_start_days[i][j];
        }
    }
}

