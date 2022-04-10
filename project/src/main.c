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

    matrix->items[2][0] = 2;
    matrix->items[2][1] = 123;
    matrix->items[2][2] = 1244;

    double val;
    get_elem(matrix, 3, 3, &val);
    printf("%lf\n", val);

    //double test_det = 0;
    Matrix *test_adj = create_matrix_from_file("./mat_creation.txt");
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 3; j++) {
            printf("%lf ", test_adj->items[i][j]);
        }
        printf("\n");
    }

    return 0;
}

