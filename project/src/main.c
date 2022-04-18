#include <stdio.h>
#include <string.h>


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

    if ((c = fgetc(datafile)) == EOF)
        return FILE_EOF;
    else if (c != opposite_c)
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
        // fgetc also skips line termination char
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
    return (c == EOF && !current_buff_length) ? c : current_buff_length;
}


//static rule_t transition_table[S_COUNT][L_COUNT] =
// //                  L_HNAME            L_COLON         L_HVALUE
///* S_BEGIN  */ {{{S_HNAME, NULL},  {S_BEGIN, NULL },  {S_BEGIN, NULL}},
///* S_HNAME  */  {{S_BEGIN, NULL},  {S_HVALUE, NULL}, {S_BEGIN, NULL}},
///* S_HVALUE */  {{S_BEGIN, NULL},  {S_BEGIN, NULL },  {S_BEGIN, NULL}},
///* S_END    */  {{S_END,   NULL},  {S_END, NULL   },  {S_END, NULL}}};



int main(int argc, const char **argv) {
    if (argc != 2) {
        return -1;
    }

    char current_lexem[BUFFSIZE];

    const char *path_to_eml = argv[1];
    FILE *email_data = fopen(path_to_eml, "r");

    int read_status;
    while (1) {
        read_status = get_header_name(email_data, current_lexem, BUFFSIZE);

        printf("%d <hSTART>%s<hEND>\n", read_status, current_lexem);
        if (read_status == EOF)
            break;

        read_status = get_header_value(email_data, current_lexem, BUFFSIZE);
        printf("%d <vSTART>%s<vEND>\n", read_status, current_lexem);
        if (read_status == EOF)
            break;
    }
    fclose(email_data);
    return 0;
}
