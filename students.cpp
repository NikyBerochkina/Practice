/*
 * Студент: 
 * свойства: имя, группа, очник/не очник
 * методы: получение описания
 * 
 * 
 * 
 */

#include <string>
#include <sstream>
#include <iostream>
#include <vector>

class StudentBase
{
public:
    StudentBase(const std::string& name, uint32_t group)
        : m_name{name}
        , m_group{group}
    {
    }

    StudentBase(const StudentBase& rhs)
        : m_name{rhs.m_name}
        , m_group{rhs.m_group}
    {
    }

    StudentBase& operator = (const StudentBase& rhs)
    {
        if (this != &rhs)
        {
            return *this;
        }

        m_name = rhs.m_name;
        m_group = rhs.m_group;
        return *this;
    }

    virtual ~StudentBase() = default;

    std::string GetName() const
    {
        return m_name;
    }

    uint32_t GetGroup() const
    {
        return m_group;
    }

    virtual std::string GetPresenseForm() const = 0;

private:
    std::string m_name;
    uint32_t m_group;
};

std::ostream& operator << (std::ostream& os, const StudentBase& student)
{
    os << "Student " << student.GetName() << " from group " << student.GetGroup()
       << " with " << student.GetPresenseForm() << " presence form";
    return os;
}

class InPersonStudent
    : public StudentBase
{
public:
    InPersonStudent(const std::string& name, uint32_t group)
        : StudentBase{name, group}
    {
    }

    InPersonStudent(const InPersonStudent& rhs)
        : StudentBase{rhs}
    {
    }

    InPersonStudent& operator = (const InPersonStudent& rhs)
    {
        return dynamic_cast<InPersonStudent&>(StudentBase::operator=(rhs));
    }

    std::string GetPresenseForm() const override
    {
        return "InPerson";
    }
};

class InAbsentiaStudent
    : public StudentBase
{
public:
    InAbsentiaStudent(const std::string& name, uint32_t group)
        : StudentBase{name, group}
    {
    }

    InAbsentiaStudent(const InAbsentiaStudent& rhs)
        : StudentBase{rhs}
    {
    }

    InAbsentiaStudent& operator = (const InAbsentiaStudent& rhs)
    {
        return dynamic_cast<InAbsentiaStudent&>(StudentBase::operator=(rhs));
    }

    std::string GetPresenseForm() const override
    {
        return "InAbsentia";
    }
};

int main()
{
    InPersonStudent* s1 = new InPersonStudent("Nika", 214);
    std::cout << *s1 << std::endl;

    *s1 = *s1;

    InPersonStudent* s2 = new InPersonStudent{InPersonStudent("Lina", 215)};
    std::cout << *s2 << std::endl;

    InAbsentiaStudent* s3 = new InAbsentiaStudent("Alex", 216);
    std::cout << *s3 << std::endl;

    delete s1;
    delete s2;

    return 0;
}
