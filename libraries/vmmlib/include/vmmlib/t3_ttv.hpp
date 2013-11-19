/*
 * VMMLib - Tensor Classes
 *
 * @author Rafa Ballester
 *
 * Tensor times vector multiplication for tensor3 (t3)
 * Only mode 1 is implemented. This is used for fast inner product calculation. The basic idea is that the inner product between a tensor X and a 1-rank tensor (expressed as the outer product of three vectors u, v and w) is the same as ((X x u).v).w .
 *
 */

#ifndef __VMML__T3_TTV__HPP__
#define __VMML__T3_TTV__HPP__

#include <vmmlib/tensor3.hpp>

#ifdef VMMLIB_USE_OPENMP
#  include <omp.h>
#endif

namespace vmml
{
    class t3_ttv
    {
    public:

        template< size_t I1, size_t I2, size_t I3, typename T >
        static void multiply_first_mode(const tensor3< I1, I2, I3, T >& t3_in_, const vector< I1, T >& u, matrix< I2, I3, T >& m_res_);

    protected:
    };

    template< size_t I1, size_t I2, size_t I3, typename T >
    void t3_ttv::multiply_first_mode(const tensor3< I1, I2, I3, T >& t3_in_,
                                     const vector< I1, T >& u,
                                     matrix< I2, I3, T >& m_res_ )
    {
        for( unsigned j = 0; j < I2; ++j)
        {
            for( unsigned k = 0; k < I3; ++k)
            {
                T vtv = 0;
                for( unsigned i = 0; i < I1; ++i)
                    vtv += t3_in_.at(i, j, k) * u.at(i);

                m_res_.at(j, k) = vtv;
            }
        }

    }

} //end vmml namespace

#endif

