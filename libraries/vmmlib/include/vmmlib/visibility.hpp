#ifndef __VMML__VISIBILITY__HPP__
#define __VMML__VISIBILITY__HPP__

namespace vmml
{

enum Visibility
{
    VISIBILITY_NONE     = 0,
    VISIBILITY_PARTIAL  = 1,
    VISIBILITY_FULL     = 2
};

inline std::ostream& operator<< ( std::ostream& os, const Visibility& v )
{
    return os << ( v == VISIBILITY_NONE ?    "not visible      " :
                   v == VISIBILITY_PARTIAL ? "partially visible" :
                   v == VISIBILITY_FULL ?    "fully visible    " : "ERROR" );
}

} // namespace vmml

#endif

