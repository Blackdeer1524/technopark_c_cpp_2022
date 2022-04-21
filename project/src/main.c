#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define BUFFSIZE 2400000

// <От кого письмо>|<Кому письмо>|<Время получения письма>|<Количество партов письма>
/*
 Нужно приводить имена заголовка к нижнему регистру
 При выводе заголовка в STDOUT необходимо заменить символы переноса строки и пробелы
     перед значением заголовка на новой строке на один пробел.

 Могут быть в разнобой или отсутствовать
 From: L_SENDER
 To: L_RECEIVER, ..., L_RECEIVER
 Content-Type: L_CONTENT_TYPE; L_BOUNDARY
   // Если не найден Content-Type ИЛИ multipart, то только один парт. Если нет тела - 0.
 Date: L_DATE   // Время получения письма

 могут быть многострочными: тогда обязательно начинаются с хотя бы одного пробела.

 Считать символом переноса строки в том числе символ \r (специфика Windows-систем).
 Допустима последовательность \r\n и наоборот.
 */

#define FILE_EOF (-1)
#define FILE_OK 0
#define FILE_BLOCK_TERM 1
#define FILE_WRONG_TERM 2

int next_line_checker(FILE *datafile, char *next_char) {
    int c = fgetc(datafile);
    int opposite_c;

    if (c == EOF) {
        return FILE_EOF;
    } else if (c == '\n') {
        opposite_c = '\r';
    } else if (c == '\r') {
        opposite_c = '\n';
    } else {
        *next_char = c;
        return FILE_OK;
    }

    if ((c = fgetc(datafile)) != opposite_c)
        ungetc(c, datafile);
    return FILE_BLOCK_TERM;
}

int header_name_end_checker(FILE *datafile, char *next_char) {
    int status = next_line_checker(datafile, next_char);
    if (status == FILE_BLOCK_TERM)
        return FILE_WRONG_TERM;

    if (status == FILE_EOF)
        return FILE_EOF;

    if (*next_char == ':')
        return FILE_BLOCK_TERM;

    return FILE_OK;
}

typedef int (*block_termination_checker)(FILE *, char *);

typedef enum {
    L_OTHER_HEADER,
    L_FROM_HEADER,
    L_TO_HEADER,
    L_CONTENT_TYPE_HEADER,
    L_DATE_HEADER
} header_t;

void remove_whitespaces(FILE *email_data) {
    int c;
    while (isblank(c = fgetc(email_data)))
    {}
    ungetc(c, email_data);
}

// возвращает длину прочитанного; EOF если EOF И ничего не прочитал
int read_in_buffer(FILE *email_data,
                   char saving_buffer[],
                   int starting_position,
                   int limit,
                   block_termination_checker is_stop_char,
                   int *block_terminator_status) {
    *block_terminator_status = FILE_OK;
    int i;
    char c = '\0';
    for (i = starting_position;
         i < limit - 1 && (*block_terminator_status = (*is_stop_char)(email_data, &c)) == FILE_OK;
         ++i)
        saving_buffer[i] = c;
    saving_buffer[i] = '\0';

    if (*block_terminator_status == FILE_EOF && i == starting_position)
        return EOF;
    for (int j = i - 1; j >= starting_position && isblank(saving_buffer[j]); --j, --i)
        saving_buffer[j] = '\0';
    return i;
}

int get_header_name(FILE *email_data, char header_value_buf[], int limit, int *block_terminator_status) {
    return read_in_buffer(email_data, header_value_buf,  0, limit, header_name_end_checker, block_terminator_status);
}

int get_header_value(FILE *email_data, char header_value_buf[], int limit, int *block_terminator_status) {
    int current_buff_length = 0;
    remove_whitespaces(email_data);
    while (1) {
        int c;
        *block_terminator_status = FILE_OK;
        current_buff_length = read_in_buffer(email_data, header_value_buf, current_buff_length, limit,
                                             next_line_checker, block_terminator_status);
        // c = fgetc(email_data) : getting the first char of
        // the next line to check whether header value continues
        if (*block_terminator_status == FILE_WRONG_TERM || current_buff_length == EOF ||
            current_buff_length == limit - 1 || (c = fgetc(email_data)) == EOF)
            break;

        // checks header value continuation
        if (isblank(c)) {
            remove_whitespaces(email_data);
            header_value_buf[current_buff_length] = ' ';
            ++current_buff_length;
        } else {
            ungetc(c, email_data);
            break;
        }
    }
    return current_buff_length;
}

int skip_read_in_buffer(FILE *email_data, block_termination_checker is_stop_char, int *block_terminator_status) {
    int i;
    *block_terminator_status = FILE_OK;
    char c;
    for (i = 0; (*block_terminator_status = (*is_stop_char)(email_data, &c)) == FILE_OK; ++i)
    {}

    if (*block_terminator_status == FILE_EOF && i == 0)
        return EOF;
    return i;
}

int skip_header_value(FILE *email_data, int *block_terminator_status) {
    int current_buff_length = 0;
    remove_whitespaces(email_data);
    while (1) {
        int c;
        *block_terminator_status = FILE_OK;
        current_buff_length = skip_read_in_buffer(email_data, next_line_checker, block_terminator_status);
        // c = fgetc(email_data) : getting the first char of the next
        // line to check whether header value continues
        if (*block_terminator_status == FILE_WRONG_TERM || current_buff_length == EOF
            || (c = fgetc(email_data)) == EOF)
            break;

        // checks header value continuation
        if (isblank(c)) {
            remove_whitespaces(email_data);
            ++current_buff_length;
        } else {
            ungetc(c, email_data);
            break;
        }
    }
    return current_buff_length;
}

int lowercase_strncmp(const char *s, const char *t, unsigned long n) {
    unsigned long i;
    for (i=0; i < n && *s && *t && tolower(*s) == tolower(*t); ++i, ++t, ++s)
    {}
    if (i == n)
        return 1;
    return 0;
}

header_t header2lexem(const char given_header_name[]) {
    static struct {
        char *header_name;
        header_t header;
    } tag_names[] = {{"from", L_FROM_HEADER},
                     {"to", L_TO_HEADER},
                     {"date", L_DATE_HEADER},
                     {"content-type", L_CONTENT_TYPE_HEADER}};
    static int tag_names_length = sizeof (tag_names) / sizeof (tag_names[0]);

    unsigned long input_length = strlen(given_header_name);
    for (int i = 0; i < tag_names_length; ++i)
        if (lowercase_strncmp(tag_names[i].header_name, given_header_name, input_length))
            return tag_names[i].header;
    return L_OTHER_HEADER;
}

typedef struct {
    char *from;
    char *to;
    char *date;
    int n_parts;
} Results;


//  void test_lower_strncmp() {
//    static struct {
//        char *a;
//        char *b;
//        int n;
//        int res;
//    } test_array[] = {{"Test", "Test", 4, 1},
//                      {"Test", "tEst", 4, 1},
//                      {"Ttew", "tEst", 4, 0},
//                      {"", "wetw", 4, 0},
//                      {"", "wetw", 0, 1},
//                      {"", "", 0, 1},
//                      {"Wwww", "ww", 1, 1},
//                      {"wetw", "wett", 5, 0},
//                      {"wEtw", "wett", 3, 1}};
//
//    for (size_t i = 0; i < sizeof (test_array) / sizeof (test_array[0]); ++i)
//        printf("%d\n", test_array[i].res ==
//                       lowercase_strncmp(test_array[i].a, test_array[i].b, test_array[i].n));
//  }

void free_res(Results res) {
    free(res.date);
    free(res.from);
    free(res.to);
}

char* stristr(const char* haystack, const char* needle) {
    do {

   const char* h = haystack;
        const char* n = needle;
        while (tolower((unsigned char) *h) == tolower((unsigned char ) *n) && *n) {
            h++;
            n++;
        }
        if (!*n) {
            return (char *) haystack;
        }
    } while (*haystack++);
    return NULL;
}


//  void manual_printf(const char *s) {
//      for (; *s; ++s)
//          putchar(*s);
//  }

void display_res(Results res) {
    char test[BUFFSIZE];
    if (!res.from)
        res.from = "";
    if (!res.to)
        res.to = "";
    if (!res.date)
        res.date = "";
    sprintf(test, "%s|%s|%s|%d", res.from, res.to, res.date, res.n_parts);
    puts(test);
}

int rewrite_charptr(char **dest, const char *src, size_t n) {
    if (!dest || !src)
        return 1;
    free(*dest);
    *dest = calloc(sizeof(char), n);
    strncpy(*dest, src, n);
    return 0;
}

int main(int argc, const char **argv) {
    if (argc != 2) {
        return -1;
    }
    const char *path_to_eml = argv[1];
    FILE *email_data = fopen(path_to_eml, "r");
    if (!email_data)
        return -1;

    Results res = {NULL, NULL, NULL, 0};
    int read_status;
    char *boundary = NULL;
    int block_terminator_status;
    char str_buffer[BUFFSIZE];

    while (1) {
        // trying to read header
        read_status = get_header_name(email_data, str_buffer, BUFFSIZE, &block_terminator_status);
        if (read_status == EOF)
            break;
        else if (read_status == BUFFSIZE - 1 && block_terminator_status == FILE_OK) {
            // skip header and its value if buffer overflow occurs
            skip_header_value(email_data, &block_terminator_status);
            continue;
        } else if (block_terminator_status == FILE_WRONG_TERM) {
            if (!read_status)  // finds a start of a body part
                break;
            continue;
        }

        // differentiate one header from the other
        header_t cur_header = header2lexem(str_buffer);
        if (cur_header == L_OTHER_HEADER) {
            skip_header_value(email_data, &block_terminator_status);
            continue;
        }

        // trying to read header value
        read_status = get_header_value(email_data, str_buffer, BUFFSIZE, &block_terminator_status);
        if (read_status == EOF)
            break;
        else if (read_status == BUFFSIZE - 1 && block_terminator_status == FILE_OK) {
            skip_header_value(email_data, &block_terminator_status);
            continue;
        } else if (block_terminator_status == FILE_WRONG_TERM) {
            continue;
        }

        switch (cur_header) {
            case L_FROM_HEADER:
                rewrite_charptr(&res.from, str_buffer, read_status + 1);
                break;
            case L_TO_HEADER:
                rewrite_charptr(&res.to, str_buffer, read_status + 1);
                break;
            case L_DATE_HEADER:
                rewrite_charptr(&res.date, str_buffer, read_status + 1);
                break;
            case L_CONTENT_TYPE_HEADER: {
                char *multipart_start;
                if ((multipart_start = stristr(str_buffer, "multipart"))) {
                    char *part_boundary = stristr(multipart_start, "boundary");
                    // checks whether found boundary tag is precisely "boundary"
                    if (ispunct(*(part_boundary - 1)) || isblank(*(part_boundary - 1)))
                        part_boundary += 8;
                    else
                        continue;

                    // parses <boundary = blablabla>
                    while (isblank(*part_boundary))
                        ++part_boundary;
                    ++part_boundary;
                    while (isblank(*part_boundary))
                        ++part_boundary;

                    if (*part_boundary == '\"')
                        ++part_boundary;
                    int boundary_length;
                    for (boundary_length = 0; *(part_boundary+boundary_length) != '"' &&
                                              *(part_boundary+boundary_length) != '\0' &&
                                              *(part_boundary+boundary_length) != ';'; ++boundary_length)
                    {}
                    if (boundary_length) {
                        rewrite_charptr(&boundary, part_boundary, boundary_length + 1);
                        boundary[boundary_length] = '\0';
                    } else {
                        boundary = NULL;
                    }
                }
            }
                break;
            default: break;
        }
    }
    if (!boundary) {
        res.n_parts = 1;
        goto free_space;
    }
    res.n_parts = 0;
    // body part starts with boundary
    do {
        read_status = read_in_buffer(email_data, str_buffer, 0, BUFFSIZE,
                                     next_line_checker, &block_terminator_status);
        if (read_status == EOF)
            break;
        else if (read_status == BUFFSIZE - 1 && block_terminator_status == FILE_OK) {
            skip_header_value(email_data, &block_terminator_status);
            continue;
        } else if (block_terminator_status == FILE_WRONG_TERM) {
            continue;
        }
    } while (!stristr(str_buffer, boundary));
    remove_whitespaces(email_data);
    int c;
    if (read_status == EOF || (c = fgetc(email_data)) == EOF) {
        goto free_space;
    }
    ungetc(c, email_data);

    res.n_parts = 1;
    int contains_sth = 0;
    while (1) {
        read_status = read_in_buffer(email_data, str_buffer, 0, BUFFSIZE,
                                     next_line_checker, &block_terminator_status);
        if (read_status == EOF)
            break;
        else if (read_status == BUFFSIZE - 1 && block_terminator_status == FILE_OK) {
            skip_header_value(email_data, &block_terminator_status);
            continue;
        } else if (block_terminator_status == FILE_WRONG_TERM) {
            continue;
        }

        contains_sth |= read_status;
        if (stristr(str_buffer, boundary)) {
            ++res.n_parts;
            contains_sth = 0;
        }
    }
    if (!contains_sth)
        --res.n_parts;

    free_space:
        display_res(res);
        free(boundary);
        free_res(res);
        fclose(email_data);
    return 0;
}
