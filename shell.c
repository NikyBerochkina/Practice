#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct StringArray
{
    const char** array;
    size_t capacity;
    size_t size;
};

enum ReadStatus
{
    RS_OK = 0,
    RS_ERROR,
    RS_EOF
};

struct Token
{
    char* word;
};

struct StringArray* createstringarray()
{
    struct StringArray* result = (struct StringArray*)malloc(sizeof(struct StringArray));
    if (!result)
    {
        return NULL;
    }

    result->capacity = 2;
    result->size = 0;
    result->array = (const char**)malloc(result->capacity * sizeof(const char*));
    if (!result->array)
    {
        free(result);
        return NULL;
    }
    return result;
}


// чтение слова из строки
const char* readword(const char* p, const char* end, char** word)
{
    while (p != end && isspace(*p))
    {
        p++;
    }
    if (p == end) //если конец файла
    {
        *word = NULL;
        return p;
    }

    char * f = 0;
    if (*p == '"')// если найдена открывающаяся кавычка,то ищем вторую, и формируем слово
    {
        if ((f = strchr(p+1, '"')) == NULL)
        {
            p = NULL;
            return p;
        }
        size_t len = (f - 1) - (p + 1) + 1;
        *word = (char*)malloc((len + 1) * sizeof(char));
        memcpy(*word, p + 1, len * sizeof(char));
        (*word)[len] = '\0';
        return f+1;
    }

    const char *signs = "&|>;<()";
    const char *doubsigns = "&|>";
    if (strchr(signs, *p) != NULL)// если найден спец знак, то проверяем следующий знак и формируем слово
    {
        ssize_t len = 2;
        if (strchr(doubsigns, *p) != NULL && *(p + 1) != EOF && p == p + 1)
        {
            len = 3;
        }
        *word = (char*)malloc(len * sizeof(char));
        memcpy(*word, p, (len - 1) * sizeof(char));
        (*word)[len] = '\0';
        return p + len - 1;
    }

    // ПРОВЕРИТЬ КАК РАБОТАЕТ ЭТА ЧАСТЬ КОДА!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1

    const char* now = p + 1;
    size_t len = 0;
    *word = (char*)malloc(2 * sizeof(char));
    while (now < end && !isspace(*now)) // не конец строки или файла, не табуляция, не пробел (если кавычка, но без пробела впереди или позади, то это все еще то же слово)
    {
        if (*now != '"')
        {
            len ++;
        }
        else
        {
            *word = (char*)realloc(*word, (len + 1) * sizeof(char));
            memcpy(*word, p + 1, len * sizeof(char));
            p = now + 1;
            len = 0;
        }

        now++;
    }
    *word = (char*)realloc(*word, (len + 1) * sizeof(char));
    (*word)[len] = '\0';
    return now + 1;

}

// допустим добавление слова в массив
int addstrtostringarray(struct StringArray* p, char **word)
{
    if (!p || !word)
    {
        return 0;
    }
    if (p->size + 1 > p->capacity)
    {
        size_t capacity = p->capacity * 2;
        const char** temp = (const char**)realloc(p->array, capacity * sizeof(const char*));
        if (!temp)
        {
            return 0;
        }
        p->array = temp;
        p->capacity = capacity;
    }
    const char* copy = strdup(*word);
    if (!copy)
    {
        return 0;
    }

    p->array[(p->size)++] = copy;
    return 1;
}

//вывод слов 
void writestringarray(struct StringArray* words)
{
    for (int i = 0; i < words->size; i++)
    {
        printf("%s\n", words->array[i]);
    }
}

// удаление массива
void destroystringarray(struct StringArray* words)
{
    if (!words)
    {
        return;
    }
    for (size_t i = 0; i < words->size; i++)
    {
        free((char *)words->array[i]);
    }
    free(words);
}

// считывание построчно
enum ReadStatus readline(FILE* f_in, struct StringArray** array)
{
    char* line = NULL;
    size_t size = 0;
    if (getline(&line, &size, f_in) < 0)
    {
        return RS_EOF;
    }

    //*words = createstringarray(); 
    const char* p = line;
    const char* end = line + size; 
    *array = createstringarray();
    do
    { //считываем слова и создаем массив слов
        char* word = NULL;
        
        p = readword(p, end, &word);
        if (word)
        {
            addstrtostringarray(*array, &word);
            free(word);
        }
    } while (p != end && p != NULL);

    if (p == NULL)
    {
        destroystringarray(*array);
        array = NULL;
        return RS_ERROR;
    }
    
    return RS_OK;
}

int main(int argc, char **argv)
{
    FILE* f_in = (argc > 1) ? fopen(argv[1], "r") : stdin;
    if (!f_in)
    {
        perror("failed to open input file");
        return EXIT_FAILURE;
    }

    struct StringArray* array = NULL;
    enum ReadStatus status = RS_OK;
    while ((status = readline(f_in, &array)) != RS_EOF)
    {
        if (status == RS_ERROR)
        {
            fprintf(stderr, "error\n");
        }
        else
        {
            writestringarray(array);
            destroystringarray(array);
        }
    }

    return 0;
}
