#include <stdio.h>

#include "matrix.h"

void print_matrix(Matrix *);

void print_matrix(Matrix *mat) {
    if (mat == NULL) {
        printf("NULL\n\n");
        return;
    }

    for (size_t i = 0; i < mat->n_rows; ++i) {
        for (size_t j = 0; j < mat->n_cols; ++j) {
            printf("%lf ", mat->items[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int main(void) {
    Matrix *test = create_matrix(1, 1);

    test->items[0][0] = 100;
//    test->items[0][1] = 1;
//    test->items[0][2] = 1;
//
//    test->items[1][0] = 0;
//    test->items[1][1] = 1;
//    test->items[1][2] = 0;
//
//    test->items[2][0] = 0;
//    test->items[2][1] = 0;
//    test->items[2][2] = 1;
//

//    Matrix *minor_test = get_minor(test, 0, 0);
//    free_matrix(minor_test);
//
//    double val;
//    int status = det(test, &val);
//    printf("%d %lf\n\n", status, val);
//
//    Matrix *adj_test = adj(test);
//    print_matrix(adj_test);

//    free_matrix(adj_test);
//
    Matrix *inv_test = inv(test);
    Matrix *test_mul = mul_scalar(test, 1/test->items[0][0]);

    print_matrix(test_mul);
    print_matrix(inv_test);
    print_matrix(test);
//
    free_matrix(test_mul);
    free_matrix(inv_test);
    free_matrix(test);
//
//    char path[] = "./project/tests/data/test_matrices_mul/case_1/left.txt";
//    Matrix *creation_test = create_matrix_from_file(path);
//    free_matrix(creation_test);
//
//    printf("done\n");

    return 0;
}
