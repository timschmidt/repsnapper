#ifndef __VMML__MATRIX_FUNCTORS__HPP__
#define __VMML__MATRIX_FUNCTORS__HPP__

namespace vmml
{

template< typename matrix_t >
struct matrix_functor
{
    virtual void operator()( matrix_t& matrix_ ) const
    {}
    
    virtual ~matrix_functor() {};
};

template< typename matrix_t >
struct set_to_identity : public matrix_functor< matrix_t >
{
    virtual void operator()( matrix_t& matrix_ ) const
    {
        size_t M = matrix_.getM();
        if ( M == matrix_.getN() )
        {
            matrix_ = 0.0;
            for( size_t index = 0; index < M; ++index )
            {
                matrix_.at( index, index ) = 1.0;
            } 
        }
    }
}; // struct set_to_identity


template< typename matrix_t >
struct set_to_zero : public matrix_functor< matrix_t >
{
    virtual void operator()( matrix_t& matrix_ ) const
    {
        matrix_ = 0.0;
    }
}; // struct set_to_identity

} // namespace vmml

#endif

