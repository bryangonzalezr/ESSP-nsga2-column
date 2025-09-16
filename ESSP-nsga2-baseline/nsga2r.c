
# include <stdio.h>
# include <stdlib.h>
# include <math.h>
# include <unistd.h>

# include "global.h"
# include "rand.h"
# include "time.h"

int nreal;
int nbin;
int nobj;
int ncon;
int popsize;
int init_type;
int ls_iters;
int ils_reset;
int mibe_block_size;
int mibs_block_size;
double pcross_real;
double pcross_bin;
double pmut_real;
double pmut_bin;

double mut1_p;
double mut2_p;
double mut3_p;
double mut4_p;
double mut5_p;
double pmo;
double mutammount;

double cross1_p;
double cross2_p;

double eta_c;
double eta_m;
int ngen;
int nbinmut;
int nrealmut;
int nbincross;
int nrealcross;
int *nbits;
double *min_realvar;
double *max_realvar;
double *min_binvar;
double *max_binvar;
int bitlength;
int choice;
int obj1;
int obj2;
int obj3;
int angle1;
int angle2;
int run_number;

problem_instance *pi;

int main (int argc, char **argv)
{
    int i;
    FILE *fpt1;
    FILE *fpt2;
    FILE *fpt3;
    FILE *fpt4;
    FILE *fpt5;
    FILE *fpt6;
    FILE *fpt7;

    pi = malloc(sizeof(problem_instance));
    population *parent_pop;
    population *child_pop;
    population *mixed_pop;
    if (argc<2)
    {
        printf("\n Usage ./nsga2r instance_route random_seed popsize ngen nobj pcross_bin pmut_bin\n./nsga2r 0.123 b-Instancia14_cap2_relacion7UnoUnoUnoTodosDistintos.dat 100 100 2 0.6 0.01 \n");
        exit(1);
    }
    seed = (double)atof(argv[1]);
    if (seed<=0.0 || seed>=1.0){
        printf("\n Entered seed value is wrong, seed value must be in (0,1) \n");
        exit(1);
    }
    fpt1 = fopen("initial_pop.out","w");
    fpt2 = fopen("final_pop.out","w");
    fpt3 = fopen("best_pop.out","w");
    fpt4 = fopen("all_pop.out","w");
    fpt5 = fopen("params.out","w");
    fprintf(fpt1,"# This file contains the data of initial population\n");
    fprintf(fpt2,"# This file contains the data of final population\n");
    fprintf(fpt3,"# This file contains the data of final feasible population (if found)\n");
    fprintf(fpt4,"# This file contains the data of all generations\n");
    fprintf(fpt5,"# This file contains information about inputs as read by the program\n");

    char * instance_route = argv[2];
    readInputFile(instance_route, pi);

    popsize = atoi(argv[3]);
    if (popsize<4 || (popsize%4)!= 0){
        printf("\n population size read is : %d",popsize);
        printf("\n Wrong population size entered, hence exiting \n");
        exit (1);
    }
    ngen = atoi(argv[4]);
    if (ngen<1){
        printf("\n number of generations read is : %d",ngen);
        printf("\n Wrong nuber of generations entered, hence exiting \n");
        exit (1);
    }
    nobj = atoi(argv[5]);
    if (nobj<1){
        printf("\n number of objectives entered is : %d",nobj);
        printf("\n Wrong number of objectives entered, hence exiting \n");
        exit (1);
    }
    init_type = atoi(argv[6]);
    if (init_type<0 || init_type>2){
        printf("\n Initialization type entered is : %d",init_type);
        printf("\n Wrong initialization type entered, hence exiting \n");
        exit (1);
    }
    ls_iters = atoi(argv[7]);
    if (ls_iters<1){
        printf("\n Number of local search iterations entered is : %d",ls_iters);
        printf("\n Wrong number of local search iterations entered, hence exiting \n");
        exit (1);
    }
    ils_reset = atoi(argv[8]);
    if (ils_reset<1){
        printf("\n Number of ILS reset iterations entered is : %d",ils_reset);
        printf("\n Wrong number of ILS reset iterations entered, hence exiting \n");
        exit (1);
    }

    mibe_block_size = atoi(argv[9]);
    if (mibe_block_size<1){
        printf("\n Block size for mutation in block exchange entered is : %d",mibe_block_size);
        printf("\n Wrong block size for mutation in block exchange entered, hence exiting \n");
        exit (1);
    }

    mibs_block_size = atoi(argv[10]);
    if (mibs_block_size<1){
        printf("\n Block size for mutation in block swap entered is : %d",mibs_block_size);
        printf("\n Wrong block size for mutation in block swap entered, hence exiting \n");
        exit (1);
    }

    /* Setear en lectura de instancia --
    printf("\n Enter the number of binary variables : ");
    scanf("%d",&nbin);
    if (nbin<0)
    {
        printf ("\n number of binary variables entered is : %d",nbin);
        printf ("\n Wrong number of binary variables entered, hence exiting \n");
        exit(1);
    }
    if (nbin != 0)
    {
        nbits = (int *)malloc(nbin*sizeof(int));
        min_binvar = (double *)malloc(nbin*sizeof(double));
        max_binvar = (double *)malloc(nbin*sizeof(double));
        for (i=0; i<nbin; i++)
        {
            printf ("\n Enter the number of bits for binary variable %d : ",i+1);
            scanf ("%d",&nbits[i]);
            if (nbits[i] < 1)
            {
                printf("\n Wrong number of bits for binary variable entered, hence exiting");
                exit(1);
            }
            printf ("\n Enter the lower limit of binary variable %d : ",i+1);
            scanf ("%lf",&min_binvar[i]);
            printf ("\n Enter the upper limit of binary variable %d : ",i+1);
            scanf ("%lf",&max_binvar[i]);
            if (max_binvar[i] <= min_binvar[i])
            {
                printf("\n Wrong limits entered for the min and max bounds of binary variable entered, hence exiting \n");
                exit(1);
            }
        }
    */
    pcross_real = atof (argv[11]);
    if (pcross_real<0.0 || pcross_real>1.0){
        printf("\n Probability of crossover entered is : %e",pcross_real);
        printf("\n Entered value of probability of crossover of binary variables is out of bounds, hence exiting \n");
        exit (1);
    }
    pmut_real = atof (argv[12]);
    if (pmut_real<0.0 || pmut_real>1.0){
        printf("\n Probability of mutation entered is : %e",pmut_real);
        printf("\n Entered value of probability  of mutation of binary variables is out of bounds, hence exiting \n");
        exit (1);
    }

    mut1_p = atof(argv[13]);
    if (mut1_p<0.0 || mut1_p>1.0){
        printf("\n Probability of mutation 1 entered is : %e",mut1_p);
        printf("\n Entered value of probability  of mutation of binary variables is out of bounds, hence exiting \n");
        exit (1);
    }
    mut2_p = atof(argv[14]);
    if (mut2_p<0.0 || mut2_p>1.0){
        printf("\n Probability of mutation 2 entered is : %e",mut2_p);
        printf("\n Entered value of probability  of mutation of binary variables is out of bounds, hence exiting \n");
        exit (1);
    }
    mut3_p = atof(argv[15]);
    if (mut3_p<0.0 || mut3_p>1.0){
        printf("\n Probability of mutation 3 entered is : %e",mut3_p);
        printf("\n Entered value of probability  of mutation of binary variables is out of bounds, hence exiting \n");
        exit (1);
    }
    mut4_p = atof(argv[16]);
    if (mut4_p<0.0 || mut4_p>1.0){
        printf("\n Probability of mutation 4 entered is : %e",mut4_p);
        printf("\n Entered value of probability  of mutation of binary variables is out of bounds, hence exiting \n");
        exit (1);
    }
    mut5_p = atof(argv[17]);
    if (mut5_p<0.0 || mut5_p>1.0){
        printf("\n Probability of mutation 5 entered is : %e",mut5_p);
        printf("\n Entered value of probability  of mutation of binary variables is out of bounds, hence exiting \n");
        exit (1);
    }
    pmo = atof(argv[18]);
    if (pmo<0.0 || pmo>1.0){
        printf("\n Probability of real mutation to activate or deactivate entered is : %e",pmo);
        printf("\n Entered value of probability  of mutation of binary variables is out of bounds, hence exiting \n");
        exit (1);
    }

    mutammount = atof(argv[19]);
    if (mutammount<=0.0 || mutammount>1.0){
        printf("\n Amount of mutation : %e",mutammount);
        printf("\n Entered value of probability  of mutation of binary variables is out of bounds, hence exiting \n");
        exit (1);
    }

    cross1_p = atof(argv[20]);
    if (cross1_p<0.0 || cross1_p>1.0){
        printf("\n Probability of crossover 1 entered is : %e",cross1_p);
        printf("\n Entered value of probability  of mutation of binary variables is out of bounds, hence exiting \n");
        exit (1);
    }
    cross2_p = atof(argv[21]);
    if (cross2_p<0.0 || cross2_p>1.0){
        printf("\n Probability of crossover 2 entered is : %e",cross2_p);
        printf("\n Entered value of probability  of mutation of binary variables is out of bounds, hence exiting \n");
        exit (1);
    }

    run_number = atoi(argv[22]);
    if (run_number<1){
        printf("\n Run number entered is : %d",run_number);
        printf("\n Wrong run number entered, hence exiting \n");
        exit (1);
    }

    int run_mode = atoi(argv[23]);
    if (run_mode<0 || run_mode>1){
        printf("\n Run mode entered is : %d",run_mode);
        printf("\n Wrong run mode entered, hence exiting \n");
        exit (1);
    }

    //imprimir todos los parametros
    printf("\n Instance route = %s",instance_route);
    printf("\n Population size = %d",popsize);
    printf("\n Number of generations = %d",ngen);
    printf("\n Number of objective functions = %d",nobj);
    printf("\n Number of constraints = %d",ncon);
    printf("\n Number of binary variables = %d",nbin);
    if (nbin!=0)
    {
        for (i=0; i<nbin; i++)
        {
            printf("\n Number of bits for binary variable %d = %d",i+1,nbits[i]);
            printf("\n Lower limit of binary variable %d = %e",i+1,min_binvar[i]);
            printf("\n Upper limit of binary variable %d = %e",i+1,max_binvar[i]);
        }
        printf("\n Probability of crossover of binary variable = %e",pcross_bin);
        printf("\n Probability of mutation of binary variable = %e",pmut_bin);
    }
    printf("\n Probability of crossover of real variable = %e",pcross_real);
    printf("\n Probability of mutation of real variable = %e",pmut_real);
    printf("\n Probability of mutation 1 = %e",mut1_p);
    printf("\n Probability of mutation 2 = %e",mut2_p);
    printf("\n Probability of mutation 3 = %e",mut3_p);
    printf("\n Probability of mutation 4 = %e",mut4_p);
    printf("\n Probability of mutation 5 = %e",mut5_p);
    printf("\n Probability of real mutation to activate or deactivate = %e",pmo);
    printf("\n Amount of mutation = %e",mutammount);
    printf("\n Probability of crossover 1 = %e",cross1_p);
    printf("\n Probability of crossover 2 = %e",cross2_p);
    printf("\n run number = %d",run_number);
    printf("\n run mode = %d",run_mode);

    

    printf("\n Input data successfully entered, now performing initialization \n");
    fprintf(fpt5,"\n Population size = %d",popsize);
    fprintf(fpt5,"\n Number of generations = %d",ngen);
    fprintf(fpt5,"\n Number of objective functions = %d",nobj);
    /*fprintf(fpt5,"\n Number of constraints = %d",ncon);
    fprintf(fpt5,"\n Number of real variables = %d",nreal);
    if (nreal!=0)
    {
        for (i=0; i<nreal; i++)
        {
            fprintf(fpt5,"\n Lower limit of real variable %d = %e",i+1,min_realvar[i]);
            fprintf(fpt5,"\n Upper limit of real variable %d = %e",i+1,max_realvar[i]);
        }
        fprintf(fpt5,"\n Probability of crossover of real variable = %e",pcross_real);
        fprintf(fpt5,"\n Probability of mutation of real variable = %e",pmut_real);
        fprintf(fpt5,"\n Distribution index for crossover = %e",eta_c);
        fprintf(fpt5,"\n Distribution index for mutation = %e",eta_m);
    }*/
    fprintf(fpt5,"\n Number of binary variables = %d",nbin);
    if (nbin!=0)
    {
        for (i=0; i<nbin; i++)
        {
            fprintf(fpt5,"\n Number of bits for binary variable %d = %d",i+1,nbits[i]);
            fprintf(fpt5,"\n Lower limit of binary variable %d = %e",i+1,min_binvar[i]);
            fprintf(fpt5,"\n Upper limit of binary variable %d = %e",i+1,max_binvar[i]);
        }
        fprintf(fpt5,"\n Probability of crossover of binary variable = %e",pcross_bin);
        fprintf(fpt5,"\n Probability of mutation of binary variable = %e",pmut_bin);
    }
    fprintf(fpt5,"\n Seed for random number generator = %e",seed);
    bitlength = 0;
    if (nbin!=0)
    {
        for (i=0; i<nbin; i++)
        {
            bitlength += nbits[i];
        }
    }
    fprintf(fpt1,"# of objectives = %d, # of constraints = %d, # of real_var = %d, # of bits of bin_var = %d, constr_violation, rank, crowding_distance\n",nobj,ncon,nreal,bitlength);
    fprintf(fpt2,"# of objectives = %d, # of constraints = %d, # of real_var = %d, # of bits of bin_var = %d, constr_violation, rank, crowding_distance\n",nobj,ncon,nreal,bitlength);
    fprintf(fpt3,"# of objectives = %d, # of constraints = %d, # of real_var = %d, # of bits of bin_var = %d, constr_violation, rank, crowding_distance\n",nobj,ncon,nreal,bitlength);
    fprintf(fpt4,"# of objectives = %d, # of constraints = %d, # of real_var = %d, # of bits of bin_var = %d, constr_violation, rank, crowding_distance\n",nobj,ncon,nreal,bitlength);
    nbinmut = 0;
    nrealmut = 0;
    nbincross = 0;
    nrealcross = 0;
    printf("\n Allocating memory for populations \n");
    parent_pop = (population *)malloc(sizeof(population));
    child_pop = (population *)malloc(sizeof(population));
    mixed_pop = (population *)malloc(sizeof(population));
    printf("\n Memory allocation for populations done, now initializing \n");
    allocate_memory_pop (parent_pop, popsize);
    printf("\n Memory allocation for parent population done \n");
    allocate_memory_pop (child_pop, popsize);
    printf("\n Memory allocation for child population done \n");
    allocate_memory_pop (mixed_pop, 2*popsize);
    printf("\n Memory allocation for mixed population done \n");
    randomize();
    double time_counter = 0;
    time_counter = clock();
    printf("\n Performing initialization \n");
    initialize_pop (parent_pop, pi);
    printf("\n Initialization done \n");
    //print population
    

     

    printIndividual(parent_pop[0], pi);

    double time_init = clock() - time_counter;
    
    printf("\n Initialization done, now performing first generation\n");

    decode_pop_sequences(parent_pop, pi);
    printf("\n Decoding done, now performing evaluation of initial population\n");
  
    evaluate_pop (parent_pop, pi);
    printf("\n constr indiv 0 %f", parent_pop->ind[0].constr_violation);
    assign_rank_and_crowding_distance (parent_pop);
    report_pop (parent_pop, fpt1);
    fprintf(fpt4,"# gen = 1\n");
    report_pop(parent_pop,fpt4);
    printf("\n gen = 1");
    fflush(stdout);
    /*if (choice!=0)
        onthefly_display (parent_pop,gp,1);*/
    fflush(fpt1);
    fflush(fpt2);
    fflush(fpt3);
    fflush(fpt4);
    fflush(fpt5);
    sleep(1);
    double best_constraint = -INFINITY;
    for (int j = 0; j < popsize; j++) {
        if (parent_pop->ind[j].constr_violation > best_constraint) {
            best_constraint = parent_pop->ind[j].constr_violation;
        }
    }
    double *acceleration = (double *)malloc(ngen*sizeof(double));
    int current_gen = 1;
    for (i=2; i<=ngen; i++)
    {
        if (i%10000==0)
        {
            printf("\n gen = %d",i);
            fflush(stdout);
        }
        double prev_pop_best_constraint = best_constraint;
        best_constraint = -INFINITY;

        
    
        selection (parent_pop, child_pop, pi);
        mutation_pop (child_pop, pi);
        decode_pop_sequences(child_pop, pi);
        evaluate_pop(child_pop, pi);

     
        merge (parent_pop, child_pop, mixed_pop);
        fill_nondominated_sort (mixed_pop, parent_pop);
        current_gen = i;

        
        for (int j = 0; j < popsize; j++) {
            if (parent_pop->ind[j].constr_violation > best_constraint) {
                best_constraint = parent_pop->ind[j].constr_violation;
            }

        }
        // printf("\n Best constraint violation in generation %d = %f", i, best_constraint);
        double current_acceleration = (best_constraint == 0) ? 1.0 : (best_constraint - prev_pop_best_constraint) / best_constraint;
        //printf("\n Current acceleration = %f", current_acceleration);
        acceleration[i-1] = current_acceleration;

        if (best_constraint >= 0.0 && run_mode == 1)
        {
            printf("\n Feasible solution found in generation %d",i);
            break;
        }
        
       


        

        /* Comment following four lines if information for all
        generations is not desired, it will speed up the execution */
        // fprintf(fpt4,"# gen = %d\n",i);
        // report_pop(parent_pop,fpt4);
        // fflush(fpt4);
        // printf("\n gen = %d",i);
    }

    printf("\n Generations finished, now reporting solutions\n");
    double average_acceleration = 0.0;
    for (i=1; i<ngen; i++)
    {
        average_acceleration += acceleration[i];
    }
    average_acceleration /= (current_gen-1);
    printf("\n Average acceleration = %f", average_acceleration);
    printf("\n Best constraint violation in generation %d = %f", current_gen, best_constraint);




    report_pop(parent_pop,fpt2);
    report_feasible(parent_pop,fpt3);
    if (nreal!=0)
    {
        fprintf(fpt5,"\n Number of crossover of real variable = %d",nrealcross);
        fprintf(fpt5,"\n Number of mutation of real variable = %d",nrealmut);
    }
    if (nbin!=0)
    {
        fprintf(fpt5,"\n Number of crossover of binary variable = %d",nbincross);
        fprintf(fpt5,"\n Number of mutation of binary variable = %d",nbinmut);
    }
    //report solution as data 
    //get instance name
    char * instance_name = strrchr(instance_route, '/');

    char dir_path[256];
    snprintf(dir_path, sizeof(dir_path), "sols/%s", instance_name);
    mkdir(dir_path, 0777);
    snprintf(dir_path, sizeof(dir_path), "sols/%s/allout", instance_name);
    mkdir(dir_path, 0777);
    snprintf(dir_path, sizeof(dir_path), "sols/%s/allout/of_%d.out", instance_name, run_number);
    fpt6 = fopen(dir_path, "w");
    export_of(parent_pop, fpt6);

    snprintf(dir_path, sizeof(dir_path), "sols/%s/allout/full_data_%d.out", instance_name, run_number);
    fpt7 = fopen(dir_path, "w");
    export_pop_full(parent_pop, fpt7, pi);
    fflush(stdout);
    fflush(fpt1);
    fflush(fpt2);
    fflush(fpt3);
    fflush(fpt4);
    fflush(fpt5);
    fflush(fpt6);
    fflush(fpt7);
    fclose(fpt1);
    fclose(fpt2);
    fclose(fpt3);
    fclose(fpt4);
    fclose(fpt5);
    fclose(fpt6);
    fclose(fpt7);
    individual *best_individual = &parent_pop->ind[0];
    for (int j = 1; j < popsize; j++) {
        if (parent_pop->ind[j].rank < best_individual->rank || 
            (parent_pop->ind[j].rank == best_individual->rank && 
             parent_pop->ind[j].crowd_dist > best_individual->crowd_dist)) {
            best_individual = &parent_pop->ind[j];
        }
    }

    // Store the values we need before freeing memory
    double best_obj[3] = {0.0, 0.0, 0.0}; // Initialize to handle cases where nobj < 3
    double best_constraint_violation = best_individual->constr_violation;
    
    // Copy objective values safely
    for (int j = 0; j < nobj && j < 3; j++) {
        best_obj[j] = best_individual->obj[j];
    }

    // Now free the memory
    if (nbin!=0)
    {
        free (min_binvar);
        free (max_binvar);
        free (nbits);
    }
    deallocate_memory_pop (parent_pop, popsize);
    deallocate_memory_pop (child_pop, popsize);
    deallocate_memory_pop (mixed_pop, 2*popsize);
    free (parent_pop);
    free (child_pop);
    free (mixed_pop);
    printf("\n Routine successfully exited \n");
    
    time_counter = clock() - time_counter;
    double execution_time = time_counter/CLOCKS_PER_SEC;
    printf("\n Time taken = %f seconds\n", execution_time);
    printf("\n Time taken for initialization = %f\n", time_init/CLOCKS_PER_SEC);


    // Calculate objective combination (obj1 + obj2)
    double obj_combination = 0.0;
    if (nobj >= 2) {
        obj_combination = best_obj[0] + best_obj[1];  // obj1 + obj2
    } else if (nobj >= 1) {
        obj_combination = best_obj[0];
    }

    printf("\n Best individual objectives: obj[0] = %f", best_obj[0]);
    if (nobj >= 2) printf(", obj[1] = %f", best_obj[1]);
    if (nobj >= 3) printf(", obj[2] = %f", best_obj[2]);
    printf("\n Best individual constraint violation = %f", best_constraint_violation);
    printf("\n Number of objectives: %d", nobj);

    // Define weights for the ponderation
    double w_obj = 1.0;        // weight for objectives
    double w_constraint = 1.0; // weight for constraint violation
    double w_time = 1.0;       // weight for execution time

    // Ensure positive values by taking absolute values and adding small epsilon to avoid zeros
    double positive_obj = fabs(obj_combination) + 1.0;
    double positive_constraint = fabs(best_constraint_violation);
    double positive_time = fabs(execution_time) + 1.0;

    // Calculate the final weighted value
    double weighted_value = w_obj * positive_obj + 
                           w_constraint * positive_constraint * positive_obj + 
                           w_time * positive_time;

    printf("\n Objective combination (obj[0]+obj[1]): %f", obj_combination);
    printf("\n Constraint violation weight: %f", best_constraint_violation);
    printf("\n Execution time weight: %f", execution_time);
    printf("\n Positive obj: %f, Positive constraint: %f, Positive time: %f", positive_obj, positive_constraint, positive_time);
    printf("\n Final weighted value: %f", weighted_value);
    printf("\n%f\n", weighted_value);

    return (0);

}