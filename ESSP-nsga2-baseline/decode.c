/* Routines to decode the population */

# include <stdio.h>
# include <stdlib.h>
# include <math.h>

# include "global.h"
# include "rand.h"

extern ssequence ***ssequences_pool_emp; // pools per employee
extern int *num_sequences_pool_emp;      // number of sequences per employee

/* Function to decode a population to find out the binary variable values based on its bit pattern */
void decode_pop (population *pop)
{
    int i;
    if (nbin!=0)
    {
        for (i=0; i<popsize; i++)
        {
            decode_ind (&(pop->ind[i]));
        }
    }
    return;
}

void decode_pop_sequences(population *pop, problem_instance *pi) {
    for (int i = 0; i < popsize; i++) {
        decode_individual_sequences(&(pop->ind[i]), pi);
    }
}

/* Function to decode an individual to find out the binary variable values based on its bit pattern */
void decode_ind (individual *ind)
{
    int j, k;
    int sum;
    if (nbin!=0)
    {
        for (j=0; j<nbin; j++)
        {
            sum=0;
            for (k=0; k<nbits[j]; k++)
            {
                if (ind->gene[j][k]==1)
                {
                    sum += pow(2,nbits[j]-1-k);
                }
            }
            ind->xbin[j] = min_binvar[j] + (double)sum*(max_binvar[j] - min_binvar[j])/(double)(pow(2,nbits[j])-1);
        }
    }
    return;
}

void decode_individual_sequences(individual *ind, problem_instance *pi) {
    if (!ind || !ind->seqs) return;

    int num_emps = pi->num_employees;
    int horizon = pi->horizon_length;

    for (int d = 0; d < horizon; d++)
        for (int e = 0; e < num_emps; e++)
            ind->xreal[d * num_emps + e] = 0;

    for (int e = 0; e < num_emps; e++) {
        if (!ind->seqs[e] || ind->num_seqs[e] <= 0) continue;

        for (int s = 0; s < ind->num_seqs[e]; s++) {
            int start_day = ind->seq_start_days[e][s];
            ssequence *seq = ind->seqs[e][s];

            if (!seq || !seq->shifts) continue;
            if (start_day < 0 || start_day + seq->length > horizon) continue;

            for (int i = 0; i < seq->length; i++)
                ind->xreal[(start_day + i) * num_emps + e] = seq->shifts[i];
        }
    }
}
