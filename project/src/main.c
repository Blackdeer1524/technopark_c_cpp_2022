#include <stdlib.h>
#include <stdio.h>

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

/*
 STR, COLON, WHITESPACES, LINESEP
 */

static char current_lexem[BUFFSIZE];

typedef enum {
    L_HNAME,
    L_COLON,
    L_HVALUE,
    L_COUNT
} lexem_t;

typedef enum {
    S_BEGIN,
    S_HNAME,
    S_HVALUE,
    S_END,
    S_COUNT
} state_t;

typedef int (*lexem_getter)(FILE *);

typedef struct {
    state_t next_state;
    lexem_getter get_next_lexem;
} rule_t;

int next_line_checker(char current_char) {
    return (current_char == '\n');
}

int hname_stop_chars_checker(char current_char) {
    return (current_char == ':' || next_line_checker(current_char));
}

// возвращает длину прочитанного
int read_in_buffer(FILE *email_data,
                   char saving_buffer[],
                   int starting_position,
                   int limit,
                   int (*is_stop_char)(char current_char)) {
    int c, i, stop_flag = 0;
    for (i = starting_position;
         i < limit - 1 && (c = fgetc(email_data)) != EOF && !(stop_flag = (*is_stop_char)(c));
         ++i)
        saving_buffer[i] = c;

    if (stop_flag) {
        ungetc(c, email_data);
    }
    else if (i == starting_position && i < limit - 1)   // EOF check
        return EOF;

    saving_buffer[i] = '\0';
    return i;
}

void remove_whitespaces(FILE *email_data) {
    int c;
    while ((c = fgetc(email_data)) == ' ')
        ;
}

int get_colon(FILE *email_data) {
    remove_whitespaces(email_data);
    int c = fgetc(email_data);
    if (c == ':')
        return 0;
    else if (c == EOF)
        return -1;
    return 1;
}

int get_header_value(FILE *email_data, char header_value_buf[], int limit) {
    int current_buff_length = 0;
    int c;

    while (1) {
        current_buff_length = read_in_buffer(email_data, header_value_buf, current_buff_length, limit,
                                             next_line_checker);
        // fgetc also skips line termination char
        // c = fgetc(email_data) : getting the first char of the next line to check whether header value continues
        if (current_buff_length == EOF || current_buff_length == limit - 1 ||
            fgetc(email_data) == EOF || (c = fgetc(email_data)) == EOF)
            break;

        if (c == ' ') {
            remove_whitespaces(email_data);
            header_value_buf[current_buff_length] = ' ';
            ++current_buff_length;
        } else {
            ungetc(c, email_data);
            break;
        }
    }
    return (c == EOF) ? c : current_buff_length;
}


static rule_t transition_table[S_COUNT][L_COUNT] =
 //                  L_HNAME            L_COLON         L_HVALUE
/* S_BEGIN  */ {{{S_HNAME, NULL},  {S_BEGIN, NULL },  {S_BEGIN, NULL}},
/* S_HNAME  */  {{S_BEGIN, NULL},  {S_HVALUE, NULL}, {S_BEGIN, NULL}},
/* S_HVALUE */  {{S_BEGIN, NULL},  {S_BEGIN, NULL },  {S_BEGIN, NULL}},
/* S_END    */  {{S_END,   NULL},  {S_END, NULL   },  {S_END, NULL}}};


int main(int argc, const char **argv) {
    if (argc != 2) {
        return -1;
    }

    const char *path_to_eml = argv[1];
    FILE *email_data = fopen(path_to_eml, "r");

    get_header_value(email_data, current_lexem, BUFFSIZE);
    printf("%s\n", current_lexem);
    fclose(email_data);
//
//    char *s;
//    int c;
//    state_t current_state = S_BEGIN;
//    int current_buffer_length;
//    while ((current_buffer_length = read_next_lexem(email_data)) != -1 && current_buffer_length != BUFFSIZE - 1) {
//        // \n or :
//        if (current_state == S_BEGIN) {
//            if (current_buffer_length == 0) {
//                if ((c = fgetc(email_data)) == ':')
//                    current_state = S_HBEGIN;
//                ungetc(c, email_data);
//        }
//
//        }
//    }
    return 0;
}
