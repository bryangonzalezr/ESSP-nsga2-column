/* Some utility functions (not part of the algorithm) */

# include <stdio.h>
# include <stdlib.h>
# include <math.h>
# include <stdbool.h>

# include "global.h"
# include "rand.h"

/* Function to return the maximum of two variables */
double maximum (double a, double b)
{
    if (a>b)
    {
        return(a);
    }
    return (b);
}

/* Function to return the minimum of two variables */
double minimum (double a, double b)
{
    if (a<b)
    {
        return (a);
    }
    return (b);
}

bool is_incompatible(int value, int *set, int size) {
    for (int i = 0; i < size; i++) {
        if (set[i] == value) {
            return true;
        }
    }
    return false;
}

void shuffle(int *array, int n) {
    if (n > 1) {
        for (int i = 0; i < n - 1; i++) {
            int j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}


