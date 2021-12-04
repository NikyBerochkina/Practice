#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>

#define MAX_PROC  10
int pids[MAX_PROC];
size_t pidsCount = 0;

#define PRINT_ERROR(fmt, ...) \
    fprintf(stderr, "%s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

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

// вывод слов (больше не нужно)
void writestringarray(const struct StringArray* words)
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

enum TokenType
{
    tNoToken,
    tOpeningBracket, // (
    tClosingBracket, // )
    tVerticalLine, // | 
    tSemicolon, // ; 
    tAmpersand, // &
    tLeftAngle, // < 
    tRightAngle, // > 
    tDoubleRightAngle, // >> 
    tDoubleAmpersand, // &&
    tDoubleVerticalLine // ||
};

int perform_redirection(struct StringArray* words, size_t start, size_t* end,
                         int* fd_in, int* fd_out, int* fd_buf, enum TokenType* type)
{
    *type = tNoToken;
    *fd_in = *fd_out = -1;
    if (*fd_buf >= 0)
    {
        *fd_in = *fd_buf;
        *fd_buf = -1;
    }

    *end = start;
    int plain = 1;
    while (*end < words->size)
    {
        plain = 0;
        if (!strcmp(words->array[*end], ";"))
        {
            *type = tSemicolon;
            break;
        }
        if (!strcmp(words->array[*end], "&"))
        {   
            *type = tAmpersand;
            break;
        }
        if (!strcmp(words->array[*end], "||"))
        {
            *type = tDoubleVerticalLine;
            break;
        }
        if (!strcmp(words->array[*end], "&&"))
        {
            *type = tDoubleAmpersand;
            break;
        }
        if (!strcmp(words->array[*end], "|"))
        {
            if (*fd_out != -1)
            {
                PRINT_ERROR("double override output");
                return 0;
            }
            int fds[2];
            if (!pipe(fds))
            {
                *fd_out = fds[1];
                *fd_buf = fds[0];
            }
            else 
            {
                PRINT_ERROR("pipe failed");
                return 0;
            }
            *type = tVerticalLine;
            break;
        }
        if (!strcmp(words->array[*end], "<"))
        {
            if (*end + 1 >= words->size || words->array[*end + 1] == NULL)
            {
                PRINT_ERROR("filename expected");
                return 0;
            }
            if (*fd_in != -1)
            {
                PRINT_ERROR("double override output");
                return 0;
            }
            int fd = open(words->array[*end + 1], O_RDONLY, 0666); 
            if (fd < 0) 
            {
                PRINT_ERROR("open failed");
                return 0;
            }
            *fd_in = fd;
            // no break;
        }
        else if (!strcmp(words->array[*end], ">"))
        {
            if (*end + 1 >= words->size || words->array[*end + 1] == NULL)
            {
                PRINT_ERROR("filename expected");
                return 0;
            }
            if (*fd_out != -1)
            {
                PRINT_ERROR("double override output");
                return 0;
            }
            int fd = open(words->array[*end + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666); 
            if (fd < 0) 
            {
                PRINT_ERROR("open failed");
                return 0;
            }
            *fd_out = fd;
            // no break;
        }
        else if (!strcmp(words->array[*end], ">>"))
        {
            if (*end + 1 >= words->size || words->array[*end + 1] == NULL)
            {
                PRINT_ERROR("filename expected");
                return 0;
            }
            if (*fd_out != -1)
            {
                PRINT_ERROR("double override output");
                return 0;
            }
            int fd = open(words->array[*end + 1], O_WRONLY | O_CREAT | O_APPEND, 0666); 
            if (fd < 0) 
            {
                PRINT_ERROR("open failed");
                return 0;
            }
            *fd_out = fd;
            // no break;
        }
        else
        {
            plain = 1;
        }

        if (!plain)
        {
            free(words->array[*end]);
            words->array[*end] = NULL;
            // avoid double free at exit
            plain = 1;
        }

        (*end)++;
    }
    if (!plain)
    {
        free(words->array[*end]);
        words->array[*end] = NULL;
        (*end)++;
    }
    if (*fd_in == -1)
    {
        *fd_in = STDIN_FILENO;
    }
    if (*fd_out == -1)
    {
        *fd_out = STDOUT_FILENO;
    }

    return 1;
}

int change_directory(const struct StringArray* words, size_t start)
{
    const char *path = NULL;
    if (start >= words->size || words->array[start + 1] == NULL)
    {
        path = getenv("HOME");
        if (path == NULL)
        {
            PRINT_ERROR("no HOME environment variable");
            return 0;
        }
    }
    else 
    {
        path = words->array[start + 1];
    }
    if(chdir(path) < 0)
    {
        perror(path);
        return 0;
    }

    char newdir[PATH_MAX];
    getcwd(newdir, PATH_MAX);
    printf("pwd: %s\n", newdir);
    return 1;
}

int child_exec(const struct StringArray* words, size_t start, int fd_in, int fd_out, int fd_buf)
{
    if (fd_in != STDIN_FILENO)
    {
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }
    if (fd_out != STDOUT_FILENO)
    {
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }
    if (fd_buf >= 0)
    {
        close(fd_buf);
    }
    execvp(words->array[start], (char* const*)(words->array + start));
    PRINT_ERROR("execvp failed");
    return 1;
}

int parent_cleanup(int pid, int background, int fd_in, int fd_out)
{
    if (fd_in != STDIN_FILENO)
    {
        close(fd_in);
    }
    if (fd_out != STDOUT_FILENO)
    {
        close(fd_out);
    }
    if (pid == -1)
    {
        return 1;
    }

    if (!background || pidsCount == MAX_PROC)
    {
        if (background)
        {
            PRINT_ERROR("it's impossible to do background mode");
        }
        int status;
        waitpid(pid, &status, 0);
    }
    else
    {
        for (size_t i = 0; i < MAX_PROC; i++)
        {
            if (pids[i] < 0)
            {
                pids[i] = pid;
                pidsCount++;
                break;
            }
        }
    }

    return 1;
}

int command_execution(struct StringArray* words)
{
    size_t start = 0;       // start of curr command
    size_t end = 0;         // end of curr command
    
    int fd_in, fd_out, fd_buf = -1;
    while (end != words->size)
    {
        enum TokenType type = tNoToken;
        if (!perform_redirection(words, start, &end, &fd_in, &fd_out, &fd_buf, &type))
        {
            PRINT_ERROR("refuse the whole line");
            return 0;
        }
        if (words->array[start] == NULL)
        {
            PRINT_ERROR("refuse the whole line");
            return 0;
        }

        int background = type == tAmpersand;
        if (!strcmp(words->array[start], "cd"))
        {
            parent_cleanup(-1, 0, fd_in, fd_out);
            change_directory(words, start);
        }
        else
        {
            int pid;
            if ((pid = fork()) == 0)
            {
                child_exec(words, start, fd_in, fd_out, fd_buf);
            }
            else if (pid > 0)
            {
                parent_cleanup(pid, background, fd_in, fd_out);
            }
            else
            {
                PRINT_ERROR("fork failed");
            }
        }
        start = end + 1;
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
    for (int i = 1; i < MAX_PROC; i++)
    {
        pids[i] = -1;
    }
    while ((status = readline(f_in, &array)) != RS_EOF)
    {
        for (int i = 1; i < MAX_PROC; i++)
        {
            if (pids[i] < 0 )
            {
                continue;
            }

            int backgroundStatus;
            if (waitpid(pids[i], &backgroundStatus, WNOHANG))
            {
                printf("exited with pid - %d, status - %d\n", pids[i], WEXITSTATUS(backgroundStatus));
                pids[i] = -1;
                pidsCount--;
            }
        }
        if (status == RS_ERROR)
        {
            PRINT_ERROR("error");
        }
        else
        {
            command_execution(array);
            destroystringarray(array);
        }
    }

    return 0;
}