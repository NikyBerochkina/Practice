#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct data
{
    char *word; // указатель на текст 
    int count; // число вхождений 
};

struct tnode 
{
    struct data body; 
    struct tnode *left; 
    struct tnode *right; 
};

// считывание слова в w
int readw(char **w, char* sign, FILE *f_in);

// добавление узла со словом w в р или ниже него 
struct tnode *addtree(struct tnode *p, char *w, size_t *n);

// извлечение слов из дерева
void extractwords(struct tnode *p, struct data *words);

// сравнение слов по числу вхождений в текст
int countcmp(const void *lhs, const void *rhs);

// очистка дерева
void freetree(struct tnode *p);

void usage(char* executable)
{
    printf("%s [-i input-file] [-o output-file]\n", executable);
}

int main(int argc, char** argv)
{
    // 0. определяем входной поток и поток на выход
    char *input = NULL, *output = NULL;
    int ch = 0;
    while ((ch = getopt(argc, argv, "i:o:")) != -1) 
    {
        switch (ch) 
        {
            case 'i':
                input = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case '?':
            default:
                usage(argv[0]);
        }
    }

    // 1. считываем, сохраняем в дерево
    FILE* f_in = input ? fopen(input, "r") : stdin;
    if (!f_in)
    {
        perror("failed to open input file");
        return EXIT_FAILURE;
    }
    struct tnode *root = NULL;
    char sign = 0;
    size_t n = 0;
    size_t max = 0; //всего слов во входном потоке
    for (char* w = NULL; readw(&w, &sign, f_in); max++)
    {
        root = addtree(root, w, &n);
        free(w);
    }
    if (input)
    {
        fclose(f_in);
    }

    // 2. извлекаем слова
    struct data *words = (struct data *)malloc(n * sizeof(struct data));
    extractwords(root, words);

    // 3. сортировка
    qsort(words, n, sizeof(struct data), &countcmp);

    // 4. распечатать
    FILE* f_out = output ? fopen(output, "w") : stdout;
    if (!f_out)
    {
        perror("failed to open output file");
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < n; ++i)
    {
        float freq = (float) words[i].count / max;
        fprintf(f_out, "'%s' %d %f\n", words[i].word, words[i].count, freq);
    }
    if (output != NULL)
    {
        fclose(f_out);
    }

    free(words);
    freetree(root);
}

int readw(char **w, char *sign, FILE *f_in)
{
    if (*sign != 0)
    {
        *w = (char *)malloc(2 * sizeof(char));
        (*w)[0] = *sign;
        (*w)[1] = '\0';
        *sign = 0;
        return 1;
    }

    const char *signs = ".,!&'’?()-<>*\\=[]+-%/:;{}#\"";
    int memory = 2, i = 0, c = 0, in = 0;
    *w = (char *)malloc(sizeof(char) * memory);
    while (( c = fgetc(f_in)) != EOF && (c == ' ' || c == '\n' || c == '\t'))
    {
    }
    if (c != EOF)
    {
        ungetc(c, f_in);
    }
    while (( c = fgetc(f_in)) != EOF && !isspace(c))
    {
        if (strchr(signs, c) == NULL)
        {
            (*w)[i++] = c;
            in = 1;
            if (i == memory)
            {
                memory *= 2;
                *w = (char *)realloc(*w, sizeof(char) * memory);
            }
        }
        else if (!in)
        {
            (*w)[0] = c;
            (*w)[1] = '\0';
            return 1;  //тк используем в качестве true в main
        }
        else
        {
            *sign = c;
            break;
        }
    }
    if (c == EOF && i == 0)
    {
        free(*w);
        *w = NULL;
        return 0; // тк используем в качестве false в main
    }
    *w = (char *)realloc(*w, (i+1) * sizeof(char));
    (*w)[i] = '\0';
    return 1; //тк используем в качестве true в main
}

struct tnode *addtree(struct tnode *p, char *w, size_t *n)
{
    int cond;
    if (p == NULL) /* слово встречается впервые */
    { 
        p = (struct tnode *)malloc(sizeof(struct tnode)); /* создается новый узел */
        p->body.word = strdup(w);
        p->body.count = 1;
        p->left = p->right = NULL;
        (*n)++;
    } 
    else if ((cond = strcmp(w, p->body.word)) == 0)
    {
        p->body.count++; /* это слово уже встречалось */
    }
    else if (cond < 0) /* меньше корня левого поддерева */
    {
        p->left = addtree(p->left, w, n);
    }
    else // больше корня правого поддерева
    {
        p->right = addtree(p->right, w, n);
    }
    return p;
}

void internalextractwords(struct tnode *p, struct data *words, size_t *i)
{
    if (p == NULL)
    {
        return;
    }

    words[*i].count = p->body.count;
    words[*i].word = p->body.word;
    
    (*i)++;

    internalextraсctwords(p->left, words, i);
    internalextractwords(p->right, words, i);
}

void extractwords(struct tnode *p, struct data *words)
{
    size_t i = 0;
    internalextractwords(p, words, &i);
}

int countcmp(const void *lhs, const void *rhs)
{
    const struct data* lhsd = (const struct data*)lhs;
    const struct data* rhsd = (const struct data*)rhs;
    
    // минус т.к. хотим невозрастающую последовательнсоть
    return -(lhsd->count - rhsd->count);
}

void freetree(struct tnode *p)
{
    if (p == NULL)
    {
        return;
    }
    freetree(p->left);
    freetree(p->right);
    free(p->body.word);
    free(p);
    return;
}