#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

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

const char* skip_spaces(const char* p, const char* end)
{
    while (p != end && isspace(*p))
    {
        p++;
    }
    return p;
}

const char* read_symbols(const char* p, const char* end, char** word)
{
    const char *doubsigns = "&|>";
    size_t len = 1;
    // если найден спец знак, то проверяем следующий знак и формируем слово
    if (p + 1 != end && *p == *(p + 1) && strchr(doubsigns, *p) != NULL)
    {
        len = 2;
    }
    *word = (char*)malloc((len + 1) * sizeof(char));
    memcpy(*word, p, len * sizeof(char));
    (*word)[len] = '\0';
    return p + len;
}

void append_to_word(const char* p, size_t len, char** word, size_t* full_len)
{
    *word = (char*)realloc(*word, (*full_len + len + 1) * sizeof(char));
    memcpy(*word + *full_len, p, len);
    *full_len += len;
    (*word)[*full_len] = '\0';
}

// чтение слова из строки
const char* readword(const char* start, const char* end, char** word)
{
    *word = NULL;
    if ((start = skip_spaces(start, end)) == end)
    {
        return end;
    }

    const char *signs = "&|>;<()";
    if (strchr(signs, *start) != NULL)
    {
        return read_symbols(start, end, word);
    }

    const char* now = start;
    size_t full_len = 0;
    do
    {
        while (now != end && strchr(" \t\"&|>;<()", *now) == NULL)
        {
            now++;
        }

        append_to_word(start, now - start, word, &full_len);
        if (now == end || *now != '"')
        {
            return now;
        }
        assert(now < end && *now == '"');
        
        start = now + 1;
        now = strchr(start, '"');
        if (now == NULL)
        {
            if (*word)
            {
                free(*word);
                *word = NULL;
            }
            return end;
        }

        append_to_word(start, now - start, word, &full_len);
        start = now + 1;
        now = start;
    } while (1);

    assert(0);
    return NULL;
}

// допустим добавление слова в массив
int addstrtostringarray(struct StringArray* p, char *word)
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
    const char* copy = strdup(word);
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
    printf("write array: %p\n", words);
    for (int i = 0; i < words->size; i++)
    {
        printf("%2d: %s\n", i + 1, words->array[i]);
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
    free(words->array);
    free(words);
}

// считывание построчно
enum ReadStatus readline(FILE* f_in, struct StringArray** array)
{
    char* line = NULL;
    size_t unused = 0;
    ssize_t size = 0; 
    if ((size = getline(&line, &unused, f_in)) < 0)
    {
        free(line);
        return RS_EOF;
    }

    *array = createstringarray();

    const char* p = line;
    const char* end = line[size - 1] == '\n'
        ? line + size - 1
        : line + size;
    do
    { //считываем слова и создаем массив слов
        char* word = NULL;
        
        p = readword(p, end, &word);
        if (word)
        {
            addstrtostringarray(*array, word);
            free(word);
        }
    } while (p != end);

    free(line);
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