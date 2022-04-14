#include <stdlib.h>
#include <stdio.h>

#include "matrix.h"

#define ERROR 1
#define NORMAL 0

// NOTE(stitaevskiy): Place your implementation here

Matrix *create_matrix(size_t rows, size_t cols) {
    if (rows == 0 || cols == 0)
        return NULL;

    Matrix *new_matrix = malloc(sizeof(Matrix));
    if (new_matrix == NULL) {
        return NULL;
    }
    new_matrix->n_rows = rows;
    new_matrix->n_cols = cols;

    // allocation of an array of pointers to double
    new_matrix->items = calloc(rows * cols, sizeof(double));
    if (new_matrix->items == NULL) {
        free_matrix(new_matrix);
        return NULL;
    }
    return new_matrix;
}

Matrix* create_matrix_from_file(const char* path_file) {
    FILE *matrix_data;
    matrix_data = fopen(path_file, "r");
    if (matrix_data == NULL)
        return NULL;

    size_t n_rows, n_cols;
    int scanning_results = fscanf(matrix_data, "%zu %zu", &n_rows, &n_cols);

    if (scanning_results != 2) {
        fclose(matrix_data);
        return NULL;
    }

    Matrix *read_matrix = create_matrix(n_rows, n_cols);
    if (read_matrix == NULL) {
        fclose(matrix_data);
        return NULL;
    }

    double scanned_item;
    for (size_t i = 0; i < n_rows; ++i)
        for (size_t j = 0; j < n_cols; ++j) {
            scanning_results = fscanf(matrix_data, "%lf", &scanned_item);
            if (!scanning_results) {
                free_matrix(read_matrix);
                fclose(matrix_data);
                return NULL;
            }
            set_elem(read_matrix, i, j, scanned_item);
        }
    fclose(matrix_data);
    return read_matrix;
}

void free_matrix(Matrix* matrix) {
    if (matrix != NULL) {
        free(matrix->items);
    }
    free(matrix);
}

int get_rows(const Matrix* matrix, size_t* rows) {
    if (matrix == NULL || rows == NULL)
        return ERROR;
    *rows = matrix->n_rows;
    return NORMAL;
}

int get_cols(const Matrix* matrix, size_t* cols) {
    if (matrix == NULL || cols == NULL)
        return ERROR;
    *cols = matrix->n_cols;
    return NORMAL;
}

int get_elem(const Matrix* matrix, size_t row, size_t col, double* val) {
    if (matrix == NULL || val == NULL || row >= matrix->n_rows || col >= matrix->n_cols)
        return ERROR;
    *val = *(matrix->items + row * matrix->n_cols + col);  // [row][col]
    return NORMAL;
}

int set_elem(Matrix* matrix, size_t row, size_t col, double val) {
    if (matrix == NULL || row >= matrix->n_rows || col >= matrix->n_cols)
        return ERROR;
    *(matrix->items + row * matrix->n_cols + col) = val;
    return NORMAL;
}

Matrix* mul_scalar(const Matrix* matrix, double val) {
    if (matrix == NULL)
        return NULL;

    Matrix *scaled_matrix = create_matrix(matrix->n_rows, matrix->n_cols);
    if (scaled_matrix == NULL)
        return NULL;

    double origin_matrix_item;
    for (size_t i=0; i < matrix->n_rows; ++i)
        for (size_t j=0; j < matrix->n_cols; ++j) {
            get_elem(matrix, i, j, &origin_matrix_item);
            set_elem(scaled_matrix, i, j, val * origin_matrix_item);
        }
    return scaled_matrix;
}

Matrix* transp(const Matrix* matrix) {
    if (matrix == NULL)
        return NULL;

    Matrix *transposed_matrix = create_matrix(matrix->n_cols, matrix->n_rows);
    if (transposed_matrix == NULL)
        return NULL;

    double origin_matrix_item;
    for (size_t i=0; i < matrix->n_rows; ++i)
        for (size_t j=0; j < matrix->n_cols; ++j) {
            get_elem(matrix, i, j, &origin_matrix_item);
            set_elem(transposed_matrix, j, i, origin_matrix_item);
        }
    return transposed_matrix;
}

static double summation_operator(double l, double r) {
    return l + r;
}

static double substraction_operator(double l, double r) {
    return l - r;
}

static Matrix *matrix_elementwise_operator(const Matrix *l, const Matrix *r,
                                           double (*operator)(double, double)) {
    if (l == NULL || r == NULL || l->n_rows != r->n_rows || l->n_cols != r->n_cols)
        return NULL;

    Matrix *result_mat = create_matrix(l->n_rows, l->n_cols);
    if (result_mat == NULL)
        return NULL;

    double l_item, r_item;
    for (size_t i=0; i < l->n_rows; ++i)
        for (size_t j=0; j < l->n_cols; ++j) {
            get_elem(l, i, j, &l_item);
            get_elem(r, i, j, &r_item);
            set_elem(result_mat, i, j, (*operator)(l_item, r_item));
        }
    return result_mat;
}

Matrix* sum(const Matrix* l, const Matrix* r) {
    return matrix_elementwise_operator(l, r, &summation_operator);;
}

Matrix* sub(const Matrix* l, const Matrix* r) {
    return matrix_elementwise_operator(l, r, &substraction_operator);;
}

Matrix* mul(const Matrix* l, const Matrix* r) {
    if (l == NULL || r == NULL || l->n_cols != r->n_rows)
        return NULL;

    Matrix *mul_matrix = create_matrix(l->n_rows, r->n_cols);
    if (mul_matrix == NULL)
        return NULL;

    for (size_t i=0; i < mul_matrix->n_rows; ++i)
        for (size_t j=0; j < mul_matrix->n_cols; ++j) {
            double l_item, r_item, result_item;
            result_item = 0;
            for (size_t k = 0; k < l->n_cols; ++k) {
                get_elem(l, i, k, &l_item);
                get_elem(r, k, j, &r_item);
                result_item += l_item * r_item;
            }
            set_elem(mul_matrix, i, j, result_item);
        }
    return mul_matrix;
}

static Matrix *get_minor(const Matrix *matrix, size_t row, size_t col) {
    if (matrix == NULL || matrix->n_rows <= 1 || matrix->n_cols <= 1)
        return NULL;

    Matrix *minor = create_matrix(matrix->n_rows - 1, matrix->n_cols - 1);
    if (minor == NULL)
        return NULL;

    double origin_item;
    // Разбиты на несколько циклов для ускорения работы
    // вычеркивает строку row
    for (size_t new_row = 0; new_row < row; ++new_row) {
        // вычеркивает столбец row
        for (size_t new_col = 0; new_col < col; ++new_col) {
            get_elem(matrix, new_row, new_col, &origin_item);
            set_elem(minor, new_row, new_col, origin_item);
        }
        for (size_t new_col = col + 1; new_col < matrix->n_cols; ++new_col) {
            get_elem(matrix, new_row, new_col, &origin_item);
            set_elem(minor, new_row, new_col - 1, origin_item);
        }
    }
    for (size_t new_row = row + 1; new_row < matrix->n_rows; ++new_row) {
        for (size_t new_col = 0; new_col < col; ++new_col) {
            get_elem(matrix, new_row, new_col, &origin_item);
            set_elem(minor, new_row - 1, new_col, origin_item);
        }
        for (size_t new_col = col + 1; new_col < matrix->n_cols; ++new_col) {
            get_elem(matrix, new_row, new_col, &origin_item);
            set_elem(minor, new_row - 1, new_col - 1, origin_item);
        }
    }
    return minor;
}

// error_occurred - маркер ошибки
static double compute_det(const Matrix *matrix, int *error_occurred) {
    if (matrix == NULL) {
        *error_occurred = ERROR;
        return 0;
    }

    double current_item = 0;
    if (matrix->n_rows == 1) {
        get_elem(matrix, 0, 0, &current_item);
        return current_item;
    }

    double current_det = 0;
    for (size_t i = 0; i < matrix->n_cols; ++i) {
        get_elem(matrix, 0, i, &current_item);
        if (!current_item)  // There is no sense in computing a det(submatrix) if corresponding equals to zero
            continue;
        if (i % 2)
            current_item *= -1;

        // Выделение подматрицы вычеркиванием 0 строки и i-го столбца из исходной
        Matrix *submatrix = get_minor(matrix, 0, i);
        if (submatrix != NULL && !(*error_occurred)) {
            current_det += current_item * compute_det(submatrix, error_occurred);
            free_matrix(submatrix);
        } else {
            *error_occurred = ERROR;
            break;
        }
    }
    return current_det;
}

int det(const Matrix* matrix, double* val) {
    if (matrix == NULL)
        return ERROR;

    int status = NORMAL;
    double result_det = compute_det(matrix, &status);
    if (status)
        return ERROR;
    *val = result_det;
    return NORMAL;
}

Matrix* adj(const Matrix* matrix) {
    if (matrix == NULL || matrix->n_rows != matrix->n_cols || matrix->n_rows == 1)
        return NULL;

    Matrix *adjugate_matrix = create_matrix(matrix->n_rows, matrix->n_cols);
    if (adjugate_matrix == NULL)
        return NULL;

    for (size_t i = 0; i < adjugate_matrix->n_rows; ++i)
        for (size_t j = 0; j < adjugate_matrix->n_cols; ++j) {
            Matrix *current_minor = get_minor(matrix, i, j);
            if (current_minor == NULL) {
                free_matrix(adjugate_matrix);
                return NULL;
            }

            double adj_item;
            int det_status = det(current_minor, &adj_item);
            if (det_status) {
                free_matrix(current_minor);
                free_matrix(adjugate_matrix);
                return NULL;
            }
            if ((i + j) % 2)
                adj_item *= -1;

            set_elem(adjugate_matrix, j, i, adj_item);
            free_matrix(current_minor);
        }
    return adjugate_matrix;
}

Matrix* inv(const Matrix* matrix) {
    if (matrix == NULL || matrix->n_rows != matrix->n_cols)
        return NULL;
    if (matrix->n_rows == 1) {
        Matrix *one_by_one_inv = create_matrix(1, 1);
        double origin_item = 1;
        get_elem(matrix, 0, 0, &origin_item);
        set_elem(one_by_one_inv, 0, 0, 1.0 / origin_item);
        return one_by_one_inv;
    }

    double cur_det;
    int det_status = det(matrix, &cur_det);
    if (det_status || cur_det == 0)
        return NULL;

    Matrix *inverse_matrix = NULL;
    Matrix *adj_matrix = adj(matrix);
    if (adj_matrix != NULL) {
        inverse_matrix = mul_scalar(adj_matrix, 1 / cur_det);
        free_matrix(adj_matrix);
    }
    return inverse_matrix;
}
