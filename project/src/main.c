#include "matrix.h"
#include <stdio.h>


int main(void) {
    Matrix *matrix = create_matrix(3, 3);
    matrix->items[0][0] = 1;
    matrix->items[0][1] = 32;
    matrix->items[0][2] = 1;

    matrix->items[1][0] = 5324.2;
    matrix->items[1][1] = 1;
    matrix->items[1][2] = 1;

    matrix->items[2][0] = 0;
    matrix->items[2][1] = 0;
    matrix->items[2][2] = 0;

    double test_det = 0;

    det(matrix, &test_det);
    printf("%lf\n", test_det);
    return 0;
}

