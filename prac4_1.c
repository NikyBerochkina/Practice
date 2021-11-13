#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct list 
{
    char *word; // указатель на текст
    struct list *next; 
};

// считывание и разделение строк на слова
int readw(char **w, char *sign, FILE *f_in, int *in_k)
{
    const char *signs = "&|>;<()";
    const char *doubsigns = "&|>";
    int capacity = 2, i = 0, c = 0;
    
    
    *w = (char *)malloc(sizeof(char) * capacity);

    if (*sign != 0) //создаем новое слово с специальным символом/символами
    {
        *w = (char *)malloc(2 * sizeof(char));
        (*w)[0] = *sign;
        *sign = 0;
        c = fgetc(f_in);

        if (strchr(doubsigns, (*w)[0]) != NULL && c == (*w)[0]) 
        {
            (*w)[1] = c;
            *w = (char *)realloc(*w, 3 * sizeof(char));
            (*w)[2] = '\0';
            c = fgetc(f_in);
            if (strchr(signs, c))
            {
                *sign = c;
            }
            else if (c != EOF)
            {
                ungetc(c, f_in); // возращаем символ во входной поток
            }
        } 
        else
        {
            (*w)[1] = '\0';
            if (strchr(signs, c))
            {
                *sign = c;
            }
            else if (c != EOF)
            {
                ungetc(c, f_in); // возращаем символ во входной поток
            }
        }
        return 1;
    }
    
    while (( c = fgetc(f_in)) != EOF && isspace(c))
    {
        // пропускаем все пробелы, табуляции и переходы на новые строки между словами
    }
    if (c != EOF)
    {
        ungetc(c, f_in); // возращаем символ во входной поток
    }

    while (( c = fgetc(f_in)) != EOF)
    {
        if (c == '"') //если встретили кавычку - меняем флаг in_k
        {
            *in_k = !(*in_k); // 1 - мы в кавычках, 0 - вне
        }
        else if (*in_k || (strchr(signs, c) == NULL && !isspace(c)))
        /* иначе добавляем нынешний символ в слово, если мы находимся в кавычках
           или если мы вне кавычек, но это и не специальный символ, и не пробельный символ */
        {
            (*w)[i++] = c;
            if (i == capacity) // если надо - расширяем память
            {
                capacity *= 2;
                *w = (char *)realloc(*w, sizeof(char) * capacity);
            }
        } 
        else if (isspace(c)) 
        {
            break;
        }
        else if (strchr(signs, c) != NULL ) // если это специальный символ
        {
            *sign = c;
            break;
        }
    }

    if (c == EOF && i == 0)
    {
        if (*in_k)
        {
            printf("unexpected EOF instead of '\"'\n");
            exit(EXIT_FAILURE);
        }
        free(*w);
        *w = NULL;
        return 0; // тк используем в качестве false в main
    }
    *w = (char *)realloc(*w, (i+1) * sizeof(char));
    (*w)[i] = '\0';
    return 1; //тк используем в качестве true в main
}

// добавление слова в список
struct list *addlist(struct list *p, char *w)
{   
    if (p == NULL)
    {
    p = (struct list *)malloc(sizeof(struct list)); /* создается новый узел */
    p->word = strdup(w);
    p->next = NULL;
    }
    else p->next = addlist(p->next, w);
    return p;
}

// печать списка
void writelist(struct list *p)
{
    while (p)
    {
        printf("'%s'\n", p->word);
        p = p->next;
    }
}

int main(int argc, char **argv)
{
    int in_k = 0; // флаг, показывающий в кавычках мы сейчас, или нет
    // 1. определение потока ввода
    FILE* f_in = (argc > 1) ? fopen(argv[1], "r") : stdin;
    if (!f_in)
    {
        perror("failed to open input file");
        return EXIT_FAILURE;
    }
    struct list *root = NULL;
    char sign = 0;
    // 2. считывание слов и добавление их в список

    for (char* w = NULL; readw(&w, &sign, f_in, &in_k);)
    {
        root = addlist(root, w);
        free(w);
    }

    if (argv[1])
    {
        fclose(f_in);
    }
    // 3. печать и очистка списка
    writelist(root);
    free(root);
}