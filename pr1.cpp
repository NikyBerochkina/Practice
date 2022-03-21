//      нужно ввести строку, состоящую из цифр 1 и 2, 
//      и отделить единицы от двоек
//      пример: ввели 112121221112
//      выведется: 1111111 и 22222

#include <iostream>
#include <fstream>
#include <cctype>

class Stack 
{
    public:
        struct node
        {
            char element;
            node *next;
        };
        node *first{}; 

        bool IsEmpty();
        void Push(char k);
        int Pop();
        void Print();
};

bool Stack::IsEmpty()
{
    return first ? false : true;
}

void Stack::Push(char k)
{
    node *curr = new node;
    curr->element = k;
    curr->next = first;
    first = curr;
}

int Stack::Pop()
{
    if (IsEmpty())
    {
        return -1;
    }
    char pres = first->element;
    node *curr = first;
    first = first->next;
    delete curr;
    return pres;
}

void Stack::Print()
{
    while(first)
    {
        std::cout << Pop();
    }
    std::cout << std::endl;
}


int main(int argc, char** argv)
{
    Stack first, second;
    char symbol{};

    std::fstream file;
    std::istream* p{};
    
    if (argc > 1) // reading from file
    {
        file = std::fstream(argv[1]);
        p = &file;
    }
    else
    {
        p = &std::cin;
    }
    std::istream& input = *p;
    input >> std::noskipws;
    
    while ((input >> symbol) && (symbol != '\n'))
    {
        switch (symbol)
        {
        case '1':
            first.Push(symbol);
            break;
        case '2':
            second.Push(symbol);
            break;
        default:
            break;
        }
    }


    if (!first.IsEmpty())
    {
        std::cout << "sequence consists in 1: ";
        first.Print();
        std::cout << std::endl;
    }
    else 
    {
        std::cout << "sequence with 1 is empty" << std::endl;
    }
    if (!second.IsEmpty())
    {
        std::cout << "sequence consists in 1: ";
        second.Print();
        std::cout << std::endl;
    }
    else 
    {
        std::cout << "sequence with 2 is empty" << std::endl;
    }

}