#include <stdlib.h>
#include <stdio.h>
#include "matrix.h"


Matrix *create_matrix(size_t rows, size_t cols)
{
    Matrix *new_matrix = malloc(sizeof(Matrix));
    if (new_matrix == NULL)
        return NULL;

    new_matrix->n_rows = rows;
    new_matrix->n_cols = cols;

    // allocation of an array of pointers to double
    new_matrix->items = (double **) calloc(rows, sizeof (double *));
    if (new_matrix->items == NULL)
        return NULL;

    for (size_t i=0; i<rows; ++i)
    {
        new_matrix->items[i] = (double *) calloc(cols, sizeof (double));
        if (new_matrix->items[i] == NULL)
            return NULL;
    }
    return new_matrix;
}

Matrix* create_matrix_from_file(const char* path_file)
{
    size_t i, j, n_rows, n_cols;
    FILE *matrix_data;
    matrix_data = fopen(path_file, "r");
    if (matrix_data == NULL)
        return NULL;

    int scanning_results;
    scanning_results = fscanf(matrix_data, "%zu %zu", &n_rows, &n_cols);
    if (scanning_results != 2)
        return NULL;

    Matrix *read_matrix = create_matrix(n_rows, n_cols);

    for (i=0; i<n_rows; ++i)
        for (j=0; j<n_cols; ++j)
        {
            scanning_results = fscanf(matrix_data, "%lf", &(read_matrix->items[i][j]));
            if (!scanning_results)
                return NULL;
        }
    return read_matrix;
}

void free_matrix(Matrix* matrix)
{
    for (size_t i = 0; i < matrix->n_rows; ++i)
    {
        free(matrix->items[i]);
    }
    free(matrix->items);
    free(matrix);
}

int get_rows(const Matrix* matrix, size_t* rows)
{
    *rows = matrix->n_rows;
    return 0;
}

int get_cols(const Matrix* matrix, size_t* cols)
{
    *cols = matrix->n_cols;
    return 0;
}

int get_elem(const Matrix* matrix, size_t row, size_t col, double* val)
{
    if (row >= matrix->n_rows || col >= matrix->n_cols)
        return 1;
    *val = matrix->items[row][col];
    return 0;
}

int set_elem(Matrix* matrix, size_t row, size_t col, double val)
{
    if (row >= matrix->n_rows || col >= matrix->n_cols)
        return 1;
    matrix->items[row][col] = val;
    return 0;
}

Matrix* mul_scalar(const Matrix* matrix, double val)
{
    Matrix *scaled_matrix = create_matrix(matrix->n_rows, matrix->n_cols);
    if (scaled_matrix == NULL)
        return NULL;
    for (size_t i=0; i<matrix->n_rows; ++i)
        for (size_t j=0; j<matrix->n_cols; ++j)
            scaled_matrix->items[i][j] = val * matrix->items[i][j];
    return scaled_matrix;
}

Matrix* transp(const Matrix* matrix)
{
    Matrix *transposed_matrix = create_matrix(matrix->n_cols, matrix->n_rows);
    if (transposed_matrix == NULL)
        return NULL;
    for (size_t i=0; i<matrix->n_rows; ++i)
        for (size_t j=0; j<matrix->n_cols; ++j)
            transposed_matrix->items[j][i] = matrix->items[i][j];
    return transposed_matrix;
}

Matrix* sum(const Matrix* l, const Matrix* r)
{
    Matrix *sum_matrix = create_matrix(l->n_rows, l->n_cols);
    if (sum_matrix == NULL)
        return NULL;
    for (size_t i=0; i<sum_matrix->n_rows; ++i)
        for (size_t j=0; j<sum_matrix->n_cols; ++j)
            sum_matrix->items[i][j] = l->items[i][j] + r->items[i][j];
    return sum_matrix;
}

Matrix* sub(const Matrix* l, const Matrix* r)
{
    Matrix *sub_matrix = create_matrix(l->n_rows, l->n_cols);
    if (sub_matrix == NULL)
        return NULL;
    for (size_t i=0; i<sub_matrix->n_rows; ++i)
        for (size_t j=0; j<sub_matrix->n_cols; ++j)
            sub_matrix->items[i][j] = l->items[i][j] - r->items[i][j];
    return sub_matrix;
}

Matrix* mul(const Matrix* l, const Matrix* r)
{
    if (l->n_cols != r->n_rows)
        return NULL;

    Matrix *mul_matrix = create_matrix(l->n_rows, r->n_cols);
    if (mul_matrix == NULL)
        return NULL;
    for (size_t i=0; i<mul_matrix->n_rows; ++i)
        for (size_t j=0; j<mul_matrix->n_cols; ++j)
        {
            mul_matrix->items[i][j] = 0;
            for (size_t k = 0; k < l->n_cols; ++k)
                mul_matrix->items[i][j] += l->items[i][k] * r->items[k][j];
        }
    return mul_matrix;
}

// error_occured - маркер ошибки
static Matrix *get_minor(const Matrix *matrix, size_t row, size_t col) {
    Matrix *minor = create_matrix(matrix->n_rows - 1, matrix->n_rows - 1);
    if (minor != NULL)
    {
        size_t new_row, new_col;
        // Разбиты на несколько циклов для ускорения работы
        // вычеркивает строку row
        for (new_row = 0; new_row < row; ++new_row) {
            // вычеркивает столбец row
            for (new_col = 0; new_col < col; ++new_col)
                minor->items[new_row][new_col] = matrix->items[new_row][new_col];
            for (new_col = col + 1; new_col < matrix->n_cols; ++new_col)
                minor->items[new_row][new_col - 1] = matrix->items[new_row][new_col];
        }
        for (new_row = row + 1; new_row < matrix->n_rows; ++new_row) {
            for (new_col = 0; new_col < col; ++new_col)
                minor->items[new_row - 1][new_col] = matrix->items[new_row][new_col];
            for (new_col = col + 1; new_col < matrix->n_cols; ++new_col)
                minor->items[new_row - 1][new_col - 1] = matrix->items[new_row][new_col];
        }
    }
    return minor;
}

static double compute_det(const Matrix *matrix, int *error_occured)
{
    if (*error_occured)
        return 0;
    if (matrix->n_rows == 1)
        return matrix->items[0][0];

    size_t i;  //, new_row, new_col;
    double current_det = 0;
    for (i=0; i<matrix->n_cols; ++i)
    {
        // Выделение подматрицы вычеркиванием 0 строки и i-го столбца из исходной
        Matrix *submatrix = get_minor(matrix, 0, i);
        if (submatrix != NULL)
        {
            current_det += (i % 2) ? -matrix->items[0][i] * compute_det(submatrix, error_occured) :
                                      matrix->items[0][i] * compute_det(submatrix, error_occured);
            free_matrix(submatrix);
        }
        else
            *error_occured = 1;
    }
    return current_det;
}

int det(const Matrix* matrix, double* val)
{
    int error = 0;
    double result_det = compute_det(matrix, &error);
    if (error)
        return 1;
    *val = result_det;
    return 0;
}

Matrix* adj(const Matrix* matrix)
{
    if (matrix->n_rows != matrix->n_cols)
        return NULL;

    Matrix *adjoining_matrix = create_matrix(matrix->n_rows, matrix->n_cols);
    if (adjoining_matrix != NULL)
    {
        size_t i, j;
        int det_status;
        Matrix *current_minor;
        for (i = 0; i < adjoining_matrix->n_rows; ++i)
            for (j = 0; j < adjoining_matrix->n_cols; ++j)
            {
                current_minor = get_minor(matrix, i, j);
                if (current_minor == NULL)
                    return NULL;
                det_status = det(current_minor, &adjoining_matrix->items[j][i]);
                if (det_status)
                    return NULL;
                adjoining_matrix->items[j][i] *= ((i + j) % 2) ? (-1) : (1);
                free_matrix(current_minor);
            }
    }
    return adjoining_matrix;
}

Matrix* inv(const Matrix* matrix)
{
    if (matrix->n_rows != matrix->n_cols)
        return NULL;

    double cur_det;
    int det_status = det(matrix, &cur_det);
    if (det_status || cur_det == 0)
        return NULL;

    Matrix *mat_inverse = adj(matrix);
    if (mat_inverse != NULL)
    {
        mat_inverse = mul_scalar(mat_inverse, 1 / cur_det);
    }
    return mat_inverse;
}



// NOTE(stitaevskiy): Place your implementation here
