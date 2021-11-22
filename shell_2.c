#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <limits.h>

struct StringArray
{
    char** array;
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
    result->array = (char**)malloc(result->capacity * sizeof(char*));
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

// добавление слова в массив
int addstrtostringarray(struct StringArray* p, char *word)
{
    if (!p || !word)
    {
        return 0;
    }
    if (p->size + 2 > p->capacity)
    {
        size_t capacity = p->capacity * 2;
        char** temp = (char**)realloc(p->array, capacity * sizeof(char*));
        if (!temp)
        {
            return 0;
        }
        p->array = temp;
        p->capacity = capacity;
    }
    char* copy = strdup(word);
    if (!copy)
    {
        return 0;
    }

    p->array[(p->size)++] = copy;
    p->array[p->size] = NULL;
    return 1;
}

// вывод слов 
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
    char newdir[PATH_MAX];
    getcwd(newdir, sizeof(newdir));
    printf("%s> ", newdir);
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

int command_execution(struct StringArray* words)
{
    const char *path;
    char newdir[PATH_MAX];
    getcwd(newdir, sizeof(newdir));
    
    
    if (words->array[0])
    {
        if (strcmp(words->array[0], "cd"))
        {

            int pid;
            if ((pid = fork()) == 0)
            {
                execvp(words->array[0], (char* const*)words->array);
                fprintf(stderr, "error\n");
                return 1;
            }
            int s;
            wait(&s);
            if (pid == 1)
            {
                printf("error\n");
                return 1;
            }
        }
        else 
        {
            if (words->array[1] == NULL)
            {
                path = getenv("HOME");
                if (path == NULL)
                {
                    fprintf(stderr, "no HOME environment variable\n");
                    return 1;
                }
            }
            else 
            {
                path = (char*)words->array[1];
            }
            if(chdir(path) < 0)
            {
                perror(path);
            }
            getcwd(newdir, PATH_MAX);
            printf("pwd: %s\n", newdir);
        }
    }
    return 0;
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
            command_execution(array);
            destroystringarray(array);
        }
    }

    return 0;
}