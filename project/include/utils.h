//
// Created by blackdeer on 4/25/22.
//

#ifndef PROJECT_INCLUDE_UTILS_H_
#define PROJECT_INCLUDE_UTILS_H_

#include <stdio.h>
#include <stdlib.h>

typedef int bufflength_type;

#define BUFFSIZE 2400000
#define FILE_WRONG_PARAMS (-2)
#define FILE_EOF (-1)
#define FILE_OK 0
#define FILE_BLOCK_TERM 1
#define FILE_WRONG_TERM 2


int next_line_checker(FILE *datafile, char *next_char);

int header_name_end_checker(FILE *datafile, char *next_char);

typedef int (*block_termination_checker)(FILE *, char *);

typedef enum {
    L_OTHER_HEADER,
    L_FROM_HEADER,
    L_TO_HEADER,
    L_CONTENT_TYPE_HEADER,
    L_DATE_HEADER
} header_t;

void remove_whitespaces(FILE *email_data);

// возвращает длину прочитанного; EOF если EOF И ничего не прочитал
int read_in_buffer(FILE *email_data,
                   char saving_buffer[],
                   int starting_position,
                   int limit,
                   block_termination_checker is_stop_char,
                   int *block_terminator_status);

int get_header_name(FILE *email_data,
                    char header_value_buf[],
                    int limit,
                    int *block_terminator_status);

int get_header_value(FILE *email_data, char header_value_buf[], int limit, int *block_terminator_status);

int skip_read_in_buffer(FILE *email_data,
                        block_termination_checker is_stop_char,
                        int *block_terminator_status);

int skip_header_value(FILE *email_data, int *block_terminator_status);

int lowercase_strncmp(const char *s, const char *t, unsigned long n);

header_t header2lexem(const char given_header_name[]);

typedef struct {
    char *from;
    char *to;
    char *date;
    int n_parts;
} Results;


char* stristr(const char* haystack, const char* needle);

void display_res(Results res);

#endif  // PROJECT_INCLUDE_UTILS_H_
