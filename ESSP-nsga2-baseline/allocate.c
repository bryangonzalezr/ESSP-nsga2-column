/* Memory allocation and deallocation routines */

# include <stdio.h>
# include <stdlib.h>
# include <math.h>

# include "global.h"
# include "rand.h"

extern problem_instance *pi; // Global problem instance
/* Function to allocate memory to a population */
void allocate_memory_pop (population *pop, int size)
{
    int i;
    pop->ind = (individual *)malloc(size*sizeof(individual));
    for (i=0; i<size; i++)
    {
        allocate_memory_ind (&(pop->ind[i]));
    }
    return;
}

/* Function to allocate memory to an individual */
void allocate_memory_ind (individual *ind)
{
    int j;
    if (nreal != 0)
    {
        ind->xreal = (int *)malloc(nreal*sizeof(int));
    }
    if (nbin != 0)
    {
        ind->xbin = (double *)malloc(nbin*sizeof(double));
        ind->gene = (int **)malloc(nbin*sizeof(int *));
        for (j=0; j<nbin; j++)
        {
            ind->gene[j] = (int *)malloc(nbits[j]*sizeof(int));
        }
    }
    ind->obj = (double *)malloc(nobj*sizeof(double));
    if (ncon != 0)
    {
        ind->constr = (double *)malloc(ncon*sizeof(double));
    }

    // Additional allocations for sequences
    if (pi == NULL) {
        fprintf(stderr, "Error: global problem_instance 'pi' is NULL in allocate_memory_ind.\n");
        exit(EXIT_FAILURE);
    }
    ind->num_seqs = (int *)malloc(pi->num_employees * sizeof(int));
    ind->seq_start_days = (int **)malloc(pi->num_employees * sizeof(int *));
    for (int e = 0; e < pi->num_employees; e++) {
        ind->seq_start_days[e] = (int *)malloc(pi->horizon_length * sizeof(int)); // max possible sequences
    }
    ind->seqs = (ssequence ***)malloc(pi->num_employees * sizeof(ssequence **));
    for (int e = 0; e < pi->num_employees; e++) {
        ind->seqs[e] = (ssequence **)malloc(pi->horizon_length * sizeof(ssequence *)); // max possible sequences
    }
    return;
}

/* Function to deallocate memory to a population */
void deallocate_memory_pop (population *pop, int size)
{
    int i;
    for (i=0; i<size; i++)
    {
        deallocate_memory_ind (&(pop->ind[i]));
    }
    free (pop->ind);
    return;
}

/* Function to deallocate memory to an individual */
void deallocate_memory_ind (individual *ind)
{
    int j;
    if (nreal != 0)
    {
        free(ind->xreal);
    }
    if (nbin != 0)
    {
        for (j=0; j<nbin; j++)
        {
            free(ind->gene[j]);
        }
        free(ind->xbin);
        free(ind->gene);
    }
    free(ind->obj);
    if (ncon != 0)
    {
        free(ind->constr);
    }
    return;
}
