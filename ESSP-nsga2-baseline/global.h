/* This file contains the variable and function declarations */

# ifndef _GLOBAL_H_
# define _GLOBAL_H_

# define INF 1.0e14
# define EPS 1.0e-14
# define E  2.71828182845905
# define PI 3.14159265358979
# define GNUPLOT_COMMAND "gnuplot -persist"


typedef struct{
    int *shifts;
    int length;
    int total_minutes;
}
ssequence;

typedef struct {
    int rank;
    double constr_violation;
    int *xreal;
    int **gene;
    double *xbin;
    double *obj;
    double *constr;
    double *constr_type;
    double crowd_dist;

    // New: sequences per employee
    int **seq_start_days;   // For each employee, array of start days for their sequences
    ssequence ***seqs;      // For each employee, array of pointers to sequences
    int *num_seqs;          // Number of sequences assigned per employee
} individual;

typedef struct
{
    individual *ind;
}
population;

typedef struct lists
{
    int index;
    struct lists *parent;
    struct lists *child;
}
list;



typedef struct {
    int id;
    int length;  
    char *name;
    int *incompatible_shifts;
    int num_incompatible_shifts;
} shift;

typedef struct {
    int id;
    char *name;
    int *max_shifts;
    int *days_off;
    int num_days_off;
    int max_total_minutes;
    int min_total_minutes;
    int max_consecutive_shifts;
    int min_consecutive_shifts;
    int min_consecutive_days_off;
    int max_weekends;
} employee;

typedef struct {
    int horizon_length;
    shift *shifts;
    int num_shifts;
    employee *employees;
    int num_employees;
    int **cover_requirements;  
    int **under_cover_weights;  
    int **over_cover_weights;   
    int ***shift_on_requests;   
    int ***shift_off_requests;   
} problem_instance;


extern int nreal;
extern int nbin;
extern int nobj;
extern int ncon;
extern int popsize;
extern int init_type;
extern int ls_iters;
extern int ils_reset;
extern int mibe_block_size;
extern int mibs_block_size;
extern double pcross_real;
extern double pcross_bin;
extern double pmut_real;
extern double pmut_bin;
extern double eta_c;
extern double eta_m;
extern int ngen;
extern int nbinmut;
extern int nrealmut;
extern int nbincross;
extern int nrealcross;
extern int *nbits;
extern double *min_realvar;
extern double *max_realvar;
extern double *min_binvar;
extern double *max_binvar;
extern int bitlength;
extern int choice;
extern int obj1;
extern int obj2;
extern int obj3;
extern int angle1;
extern int angle2;

extern problem_instance *pi;

extern double mut1_p;
extern double mut2_p;
extern double mut3_p;
extern double mut4_p;
extern double mut5_p;
extern double pmo;
extern double mutammount;

extern double cross1_p;
extern double cross2_p;

extern ssequence ***ssequences_pool_emp;
extern ssequence **ssequences_pool;
extern int *num_sequences_pool_emp;



void allocate_memory_pop (population *pop, int size);
void allocate_memory_ind (individual *ind);
void deallocate_memory_pop (population *pop, int size);
void deallocate_memory_ind (individual *ind);

double maximum (double a, double b);
double minimum (double a, double b);

void crossover (individual *parent1, individual *parent2, individual *child1, individual *child2, problem_instance *pi);
void realcross (individual *parent1, individual *parent2, individual *child1, individual *child2);
void bincross (individual *parent1, individual *parent2, individual *child1, individual *child2);

void assign_crowding_distance_list (population *pop, list *lst, int front_size);
void assign_crowding_distance_indices (population *pop, int c1, int c2);
void assign_crowding_distance (population *pop, int *dist, int **obj_array, int front_size);

void decode_pop (population *pop);
void decode_ind (individual *ind);
void decode_pop_sequences(population *pop, problem_instance *pi);

void onthefly_display (population *pop, FILE *gp, int ii);

int check_dominance (individual *a, individual *b);

void evaluate_pop (population *pop, problem_instance *pi);
void evaluate_ind (individual *ind, problem_instance *pi);

void fill_nondominated_sort (population *mixed_pop, population *new_pop);
void crowding_fill (population *mixed_pop, population *new_pop, int count, int front_size, list *cur);

void initialize_pop (population *pop, problem_instance *pi);
void initialize_ind (individual *ind);

void insert (list *node, int x);
list* del (list *node);

void merge(population *pop1, population *pop2, population *pop3);
void copy_ind (individual *ind1, individual *ind2);

void mutation_pop (population *pop, problem_instance *pi);
void mutation_ind (individual *ind, problem_instance *pi);
void bin_mutate_ind (individual *ind);
void real_mutate_ind (individual *ind, problem_instance *pi);

void test_problem (int *xreal, double *xbin, int **gene, double *obj, double *constr);

void assign_rank_and_crowding_distance (population *new_pop);

void report_pop (population *pop, FILE *fpt);
void report_feasible (population *pop, FILE *fpt);
void report_ind (individual *ind, FILE *fpt);

void quicksort_front_obj(population *pop, int objcount, int obj_array[], int obj_array_size);
void q_sort_front_obj(population *pop, int objcount, int obj_array[], int left, int right);
void quicksort_dist(population *pop, int *dist, int front_size);
void q_sort_dist(population *pop, int *dist, int left, int right);

void selection (population *old_pop, population *new_pop, problem_instance *pi);
individual* tournament (individual *ind1, individual *ind2);

# endif
