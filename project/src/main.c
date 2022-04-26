#include <string.h>
#include <ctype.h>

#include "utils.h"



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

void remove_whitespaces(FILE *email_data) {
    int c;
    while (isblank(c = fgetc(email_data)))
    {}
    ungetc(c, email_data);
}

// возвращает длину прочитанного; EOF если EOF И ничего не прочитал
int read_in_buffer(FILE *email_data,
                   char saving_buffer[],
                   bufflength_type starting_position,
                   bufflength_type limit,
                   block_termination_checker is_stop_char,
                   int *block_terminator_status) {
    if (!email_data || !limit || !block_terminator_status)
        return FILE_WRONG_PARAMS;

    *block_terminator_status = FILE_OK;
    bufflength_type i;
    char c = '\0';
    for (i = (starting_position > limit - 1) ? (limit - 1) : starting_position;
         i < limit - 1 && (*block_terminator_status = (*is_stop_char)(email_data, &c)) == FILE_OK;
         ++i)
        saving_buffer[i] = c;
    // Overflow check
    if (*block_terminator_status == FILE_OK)
        ungetc(c, email_data);

    for (bufflength_type j = i - 1; j >= starting_position && isblank(saving_buffer[j]); --j, --i)
    {}
    saving_buffer[i] = '\0';

    if (i == starting_position && *block_terminator_status == FILE_EOF)
        return EOF;
    return i;
}

int get_header_name(FILE *email_data,
                    char header_value_buf[],
                    bufflength_type limit,
                    int *block_terminator_status) {
    if (!email_data || !limit || !block_terminator_status)
        return FILE_WRONG_PARAMS;

    return read_in_buffer(email_data, header_value_buf,  0, limit,
                          header_name_end_checker, block_terminator_status);
}

int get_header_value(FILE *email_data, char header_value_buf[], bufflength_type limit, int *block_terminator_status) {
    if (!email_data || !limit || !block_terminator_status)
        return FILE_WRONG_PARAMS;

    bufflength_type current_buff_length = 0;
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

int skip_read_in_buffer(FILE *email_data,
                        block_termination_checker is_stop_char,
                        int *block_terminator_status) {
    if (!email_data || !block_terminator_status)
        return FILE_WRONG_PARAMS;

    bufflength_type i;
    *block_terminator_status = FILE_OK;
    char c;
    for (i = 0; (*block_terminator_status = (*is_stop_char)(email_data, &c)) == FILE_OK; ++i)
    {}

    if (*block_terminator_status == FILE_EOF && i == 0)
        return EOF;
    return i;
}

int skip_header_value(FILE *email_data, int *block_terminator_status) {
    if (!email_data || !block_terminator_status)
        return FILE_WRONG_PARAMS;

    bufflength_type current_buff_length = 0;
    remove_whitespaces(email_data);
    while (1) {
        *block_terminator_status = FILE_OK;
        current_buff_length = skip_read_in_buffer(email_data, next_line_checker, block_terminator_status);
        // c = fgetc(email_data) : getting the first char of the next
        // line to check whether header value continues
        int c;
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
    for (i = 0; i < n && *s && *t && tolower(*s) == tolower(*t); ++i, ++t, ++s) {}
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
    for (bufflength_type i = 0; i < tag_names_length; ++i)
        if (lowercase_strncmp(tag_names[i].header_name, given_header_name, input_length))
            return tag_names[i].header;
    return L_OTHER_HEADER;
}

char* stristr(const char* haystack, const char* needle) {
    do {
        const char* h = haystack;
        const char* n = needle;
        while (tolower((unsigned char) *h) == tolower((unsigned char) *n) && *n) {
            h++;
            n++;
        }
        if (!*n) {
            return (char *) haystack;
        }
    } while (*haystack++);
    return NULL;
}

void display_res(Results res) {
    if (!res.from)
        res.from = "";
    if (!res.to)
        res.to = "";
    if (!res.date)
        res.date = "";
    printf("%s|%s|%s|%d", res.from, res.to, res.date, res.n_parts);
}

int rewrite_charptr(char **dest, const char *src, size_t src_str_length) {
    if (!dest || !src)
        return 1;
    free(*dest);
    *dest = calloc(sizeof(char), src_str_length + 1);
    strncpy(*dest, src, src_str_length);
    (*dest)[src_str_length] = '\0';
    return 0;
}

int main(int argc, const char **argv) {
    if (argc != 2) {
        return FILE_WRONG_PARAMS;
    }
    const char *path_to_eml = argv[1];
    FILE *email_data = fopen(path_to_eml, "r");
    if (!email_data)
        return FILE_WRONG_PARAMS;

    Results res = {NULL, NULL, NULL, 0};
    bufflength_type read_status;
    char *boundary = NULL;
    bufflength_type boundary_length = 0;
    int block_terminator_status;
    char str_buffer[BUFFSIZE];

    while (1) {
        // trying to read header
        read_status = get_header_name(email_data, str_buffer, BUFFSIZE, &block_terminator_status);
        if (read_status == EOF) {
            break;
        } else if (read_status == BUFFSIZE - 1 && block_terminator_status == FILE_OK) {
            // skip header and its value if buffer overflow occurs
            skip_header_value(email_data, &block_terminator_status);
            continue;
        } else if (block_terminator_status == FILE_WRONG_TERM) {
            if (!read_status)  // start of a body part found
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
        if (read_status == EOF) {
            break;
        } else if (read_status == BUFFSIZE - 1 && block_terminator_status == FILE_OK) {
            skip_header_value(email_data, &block_terminator_status);
            continue;
        } else if (block_terminator_status == FILE_WRONG_TERM) {
            continue;
        }

        switch (cur_header) {
            case L_FROM_HEADER:
                rewrite_charptr(&res.from, str_buffer, read_status);
                break;
            case L_TO_HEADER:
                rewrite_charptr(&res.to, str_buffer, read_status);
                break;
            case L_DATE_HEADER:
                rewrite_charptr(&res.date, str_buffer, read_status);
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
                    for (boundary_length = 0; *(part_boundary+boundary_length) != '"' &&
                                              *(part_boundary+boundary_length) != '\0' &&
                                              *(part_boundary+boundary_length) != ';'; ++boundary_length)
                    {}
                    if (boundary_length)
                        rewrite_charptr(&boundary, part_boundary, boundary_length);
                    else
                        boundary = NULL;
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
    char *found_boundary_start;
    while (1) {
        read_status = read_in_buffer(email_data, str_buffer, 0, BUFFSIZE,
                                     next_line_checker, &block_terminator_status);
        if (read_status == EOF) {
            break;
        } else if (read_status == BUFFSIZE - 1 && block_terminator_status == FILE_OK) {
            skip_header_value(email_data, &block_terminator_status);
            continue;
        } else if (block_terminator_status == FILE_WRONG_TERM) {
            continue;
        }

        if ((found_boundary_start = stristr(str_buffer, boundary))) {
            if (*(found_boundary_start + boundary_length) != '\0')
                break;  // catches ending boundary that ends with <-->
            ++res.n_parts;
        }
    }

    free_space:
        display_res(res);
        free(boundary);
        free(res.date);
        free(res.from);
        free(res.to);
        fclose(email_data);
    return 0;
}
