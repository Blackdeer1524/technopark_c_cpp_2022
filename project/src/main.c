#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define BUFFSIZE 500

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


int next_line_checker(FILE *datafile, char *next_char) {
    int c = fgetc(datafile);
    int opposite_c;

    if (c == EOF)
        return FILE_EOF;
    else if (c == '\n')
        opposite_c = '\r';
    else if (c == '\r')
        opposite_c = '\n';
    else {
        *next_char = c;
        return FILE_OK;
    }

    if ((c = fgetc(datafile)) != opposite_c)
        ungetc(c, datafile);
    return FILE_BLOCK_TERM;
}

int header_name_end_checker(FILE *datafile, char *next_char) {
    int status = next_line_checker(datafile, next_char);
    if (status != FILE_OK)
        return status;

    if (*next_char == ':')
        return FILE_BLOCK_TERM;

    return FILE_OK;
}

typedef int (*block_termination_checker)(FILE *, char *);

void remove_whitespaces(FILE *email_data) {
    int c;
    while ((c = fgetc(email_data)) == ' ')
        ;
    ungetc(c, email_data);
}

// возвращает длину прочитанного; EOF если EOF И ничего не прочитал
int read_in_buffer(FILE *email_data,
                   char saving_buffer[],
                   int starting_position,
                   int limit,
                   block_termination_checker is_stop_char) {
    int i, block_terminator_status = FILE_OK;
    char c;
    for (i = starting_position;
         i < limit - 1 && (block_terminator_status = (*is_stop_char)(email_data, &c)) == FILE_OK;
         ++i)
        saving_buffer[i] = c;
    saving_buffer[i] = '\0';

    if (block_terminator_status == FILE_EOF && i == starting_position)
        return EOF;

    for (int j = i - 1; j >= starting_position && saving_buffer[j] == ' '; --j, --i)
        saving_buffer[j] = '\0';
    return i;
}

int get_header_name(FILE *email_data, char header_value_buf[], int limit) {
    return read_in_buffer(email_data, header_value_buf,  0, limit, header_name_end_checker);
}

int get_header_value(FILE *email_data, char header_value_buf[], int limit) {
    int c, current_buff_length = 0;
    remove_whitespaces(email_data);
    while (1) {
        current_buff_length = read_in_buffer(email_data, header_value_buf, current_buff_length, limit,
                                             next_line_checker);
        // c = fgetc(email_data) : getting the first char of the next line to check whether header value continues
        if (current_buff_length == EOF || current_buff_length == limit - 1 || (c = fgetc(email_data)) == EOF)
            break;

        // checks header value continuation
        if (c == ' ') {
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

typedef enum {
    L_OTHER_TAG,
    L_FROM_TAG,
    L_TO_TAG,
    L_CONTENT_TYPE_TAG,
    L_DATE_TAG,
    L_RECEIVED,
    L_COUNT
} lexem_t;

int lowercase_strncmp(const char *s, const char *t, int n) {
    int i;
    for (i=0; i < n && *s && *t && tolower(*s) == tolower(*t); ++i, ++t, ++s)
        ;
    if (i == n)
        return 1;
    return 0;
}

lexem_t header2lexem(const char given_header_name[]) {
    static struct {
        char *header_name;
        lexem_t lexem;
    } tag_names[] = {{"from", L_FROM_TAG},
                     {"to", L_TO_TAG},
                     {"date", L_DATE_TAG},
                     {"content-type", L_CONTENT_TYPE_TAG},
                     {"received", L_RECEIVED}};
    static int tag_names_length = sizeof (tag_names) / sizeof (tag_names[0]);

    int input_length = strlen(given_header_name);

    for (int i = 0; i < tag_names_length; ++i)
        if (lowercase_strncmp(tag_names[i].header_name, given_header_name, input_length))
            return tag_names[i].lexem;
    return L_OTHER_TAG;
}


//static rule_t transition_table[S_COUNT][L_COUNT] =
// //                  L_HNAME            L_COLON         L_HVALUE
///* S_BEGIN  */ {{{S_HNAME, NULL},  {S_BEGIN, NULL },  {S_BEGIN, NULL}},
///* S_HNAME  */  {{S_BEGIN, NULL},  {S_HVALUE, NULL}, {S_BEGIN, NULL}},
///* S_HVALUE */  {{S_BEGIN, NULL},  {S_BEGIN, NULL },  {S_BEGIN, NULL}},
///* S_END    */  {{S_END,   NULL},  {S_END, NULL   },  {S_END, NULL}}};


void test_lower_strncmp() {
    static struct {
        char *a;
        char *b;
        int n;
        int res;
    } test_array[] = {{"Test", "Test", 4, 1},
                      {"Test", "tEst", 4, 1},
                      {"Ttew", "tEst", 4, 0},
                      {"", "wetw", 4, 0},
                      {"", "wetw", 0, 1},
                      {"", "", 0, 1},
                      {"Wwww", "ww", 1, 1},
                      {"wetw", "wett", 5, 0},
                      {"wEtw", "wett", 3, 1}};

    for (int i = 0; i < sizeof (test_array) / sizeof (test_array[0]); ++i)
        printf("%d\n", test_array[i].res == lowercase_strncmp(test_array[i].a, test_array[i].b, test_array[i].n));
}



int main(int argc, const char **argv) {
    if (argc != 2) {
        return -1;
    }

    char str_lexem[BUFFSIZE];
    const char *path_to_eml = argv[1];
    FILE *email_data = fopen(path_to_eml, "r");

    int read_status;
    while (1) {
        read_status = get_header_name(email_data, str_lexem, BUFFSIZE);
        lexem_t lexem = header2lexem(str_lexem);
        
        printf("%d <hSTART>%s<hEND>\n", read_status, str_lexem);
        if (read_status == EOF)
            break;

        read_status = get_header_value(email_data, str_lexem, BUFFSIZE);
        printf("%d <vSTART>%s<vEND>\n", read_status, str_lexem);
        if (read_status == EOF)
            break;
    }
    fclose(email_data);
    return 0;
}
