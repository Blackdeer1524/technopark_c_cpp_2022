#include <stdio.h>

#include "matrix.h"


int main(void) {
    Matrix *test = create_matrix(1, 1);
    test->items[0][0] = 1;
//    test->items[0][1] = 1;
//    test->items[0][2] = 1;
//
//    test->items[1][0] = 1;
//    test->items[1][1] = 1;
//    test->items[1][2] = 1;

//    test->items[2][0] = 1;
//    test->items[2][1] = 1;
//    test->items[2][2] = 1;

    double val;
    Matrix *minor_test = get_minor(test, 0, 0);
    free_matrix(minor_test);

    det(test, &val);

    Matrix *adj_test = adj(test);
    free_matrix(adj_test);

    Matrix *inv_test = inv(test);
    free_matrix(inv_test);

    free_matrix(test);

    char path[] = "./project/tests/data/test_matrices_mul/case_1/left.txt";
    Matrix *creation_test = create_matrix_from_file(path);
    free_matrix(creation_test);

    printf("done\n");
    return 0;
}
