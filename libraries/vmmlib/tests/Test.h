#ifndef _Test_H_
#define _Test_H_

namespace vmml
{
class Test
{
public: 
    virtual ~Test() {}

    virtual bool test() = 0;
};

}; // namespace vmml

#endif
