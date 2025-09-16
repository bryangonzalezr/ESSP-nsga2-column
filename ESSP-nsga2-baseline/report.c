/* Routines for storing population data into files */

# include <stdio.h>
# include <stdlib.h>
# include <math.h>

# include "global.h"
# include "rand.h"

/* Function to print the information of a population in a file */
void report_pop (population *pop, FILE *fpt)
{
    int i, j, k;
    for (i=0; i<popsize; i++)
    {
        for (j=0; j<nobj; j++)
        {
            fprintf(fpt, "%d\t", (int)pop->ind[i].obj[j]);
        }
        if (ncon!=0)
        {
            // for (j=0; j<ncon; j++)
            // {
            //     fprintf(fpt,"%e\t",pop->ind[i].constr[j]);
            // }
        }
        if (nreal!=0)
        {
            for (j=0; j<nreal; j++)
            {
                fprintf(fpt,"%d\t",pop->ind[i].xreal[j]);
            }
        }
        if (nbin!=0)
        {
            for (j=0; j<nbin; j++)
            {
                for (k=0; k<nbits[j]; k++)
                {
                    fprintf(fpt,"%d\t",pop->ind[i].gene[j][k]);
                }
            }
        }
        fprintf(fpt,"%d\t",(int)pop->ind[i].constr_violation);
        fprintf(fpt,"%d\t",pop->ind[i].rank);
        fprintf(fpt,"%e\n",pop->ind[i].crowd_dist);
    }
    return;
}

/* Function to print the information of feasible and non-dominated population in a file */
void report_feasible (population *pop, FILE *fpt)
{
    int i, j, k;
    for (i=0; i<popsize; i++)
    {
        if (pop->ind[i].constr_violation == 0.0 && pop->ind[i].rank==1)
        {
            for (j=0; j<nobj; j++)
            {
                //report as int
            
                fprintf(fpt, "%d\t", (int)pop->ind[i].obj[j]);

            }
            if (ncon!=0)
            {
                for (j=0; j<ncon; j++)
                {
                    fprintf(fpt,"%e\t",pop->ind[i].constr[j]);
                }
            }
            if (nreal!=0)
            {
                for (j=0; j<nreal; j++)
                {
                    fprintf(fpt,"%d\t",pop->ind[i].xreal[j]);
                }
            }
            if (nbin!=0)
            {
                for (j=0; j<nbin; j++)
                {
                    for (k=0; k<nbits[j]; k++)
                    {
                        fprintf(fpt,"%d\t",pop->ind[i].gene[j][k]);
                    }
                }
            }
            fprintf(fpt,"%e\t",pop->ind[i].constr_violation);
            fprintf(fpt,"%d\t",pop->ind[i].rank);
            fprintf(fpt,"%e\n",pop->ind[i].crowd_dist);
        }
    }
    return;
}


void export_of (population *pop, FILE *fpt){

    int i, j, k;
    for (i=0; i<popsize; i++)
    {
        {
        if (pop->ind[i].constr_violation == 0.0 && pop->ind[i].rank==1)
        {
            for (j=0; j<nobj; j++)
            
            {

                fprintf(fpt, "%d ", (int)pop->ind[i].obj[j]);
            }
            fprintf(fpt, "\n");
        }
        }
    }
    return;

}

void export_pop_full(population *pop, FILE *fpt, problem_instance *pi){
    // printf("E \\ D");
    // for (int i = 0; i < pi->horizon_length; i++) {
    //     printf("\tDay %d", i + 1);
    // }
    // printf("\n");
    
    // // Print schedule for each employee
    // for (int j = 0; j < pi->num_employees; j++) {
    //     printf("%s", pi->employees[j].name);
    //     for (int i = 0; i < pi->horizon_length; i++) {
    //         printf("\t%s", pi->shifts[ind->xreal[i * pi->num_employees + j]].name);
    //     }
    //     printf("\n");
    // }
    int i, j, k;
    for (i=0; i<popsize; i++)
    {
        
        fprintf(fpt, "Individual %d\n", i);
        

        // if(i==0){
        //     printf("Individual %d\n", i);
        //     printIndividual(&(pop->ind[i]), pi);
        //     printf("Individual %d\n", i+1);
        //     printIndividual(&(pop->ind[i+1]), pi);
        //     population *pop2 = (population *)malloc(sizeof(population));
        //     allocate_memory_pop(pop2, 2);
            
        //     realcross(&(pop->ind[i]), &(pop->ind[i+1]), &(pop2->ind[0]), &(pop2->ind[1]));
        //     printf("Child 1\n");
        //     printIndividual(&(pop2->ind[0]), pi);

        //     printf("Child 2\n");
        //     printIndividual(&(pop2->ind[1]), pi);
        // }

        for (j=0; j<pi->horizon_length; j++)
        {
            if (j != 0)
            {
                fprintf(fpt, "\t");
            }
            fprintf(fpt, "%d", j + 1);
        }
        fprintf(fpt, "\n");
        for (j=0; j<pi->num_employees; j++)
        {
            for (k=0; k<pi->horizon_length; k++)
            {
            if (k == 0)
            {
                fprintf(fpt, "%s", pi->shifts[pop->ind[i].xreal[k * pi->num_employees + j]].name);
            }
            else
            {
                fprintf(fpt, "\t%s", pi->shifts[pop->ind[i].xreal[k * pi->num_employees + j]].name);
            }
            }
            fprintf(fpt, "\n");
        }
        //report objectives
        for (j=0; j<nobj; j++)
        {
            fprintf(fpt, "Objective %d: %d\n", j, (int)pop->ind[i].obj[j]);
        }
        //constr violation
        fprintf(fpt, "Constr violation: %d\n", (int)pop->ind[i].constr_violation);
        fprintf(fpt, "Restrictions violated:\n");
        
        for (j=0; j<ncon; j++)
        {
            
            fprintf(fpt, "%d ", (int)pop->ind[i].constr[j]);
            
        }
        fprintf(fpt, "\n\n");
        
    }

}