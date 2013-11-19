/* 
 * VMMLib - Tensor Classes
 * 
 * @author Rafael Ballester
 */

#ifndef __VMML__TUCKER4_TENSOR__HPP__
#define __VMML__TUCKER4_TENSOR__HPP__

#include <vmmlib/t4_hooi.hpp>

namespace vmml {

    // T is the input type for the tensor
    template< size_t R1, size_t R2, size_t R3, size_t R4, size_t I1, size_t I2, size_t I3, size_t I4, typename T = float>
            class tucker4_tensor {
    public:
        typedef double T_internal;

        typedef tucker4_tensor< R1, R2, R3, R4, I1, I2, I3, I4, T > tucker4_type;
        typedef t4_hooi< R1, R2, R3, R4, I1, I2, I3, I4, T > t4_hooi_type;
        
        typedef tensor4< I1, I2, I3, I4, T > t4_type;
        typedef tensor4< R1, R2, R3, R4, T_internal > t4_core_type;
        typedef matrix< I1, R1, T_internal > u1_type;
        typedef matrix< I2, R2, T_internal > u2_type;
        typedef matrix< I3, R3, T_internal > u3_type;
        typedef matrix< I4, R4, T_internal > u4_type;

        tucker4_tensor();
        ~tucker4_tensor();
        size_t nnz_core() const;
        size_t size_core() const;
        void reconstruct(t4_type& data_);
        
    	template< typename T_init>
    	tensor_stats decompose(const t4_type& data_, T_init init, const size_t max_iterations = 10, const float tolerance = 10);
        
        template< typename T_init>
        tensor_stats tucker_als(const t4_type& data_, T_init init, const size_t max_iterations = 10, const float tolerance = 1e-04);

    private:
        //t3_core_type* _core ;
        u1_type* _u1;
        u2_type* _u2;
        u3_type* _u3;
        u4_type* _u4;
        t4_core_type _core;

    }; // class tucker3_tensor



#define VMML_TEMPLATE_STRING        template< size_t R1, size_t R2, size_t R3, size_t R4, size_t I1, size_t I2, size_t I3, size_t I4, typename T >
#define VMML_TEMPLATE_CLASSNAME     tucker4_tensor< R1, R2, R3, R4, I1, I2, I3, I4, T >

    VMML_TEMPLATE_STRING
    VMML_TEMPLATE_CLASSNAME::tucker4_tensor() {
        _core.zero();
        _u1 = new u1_type();
        _u1->zero();
        _u2 = new u2_type();
        _u2->zero();
        _u3 = new u3_type();
        _u3->zero();
        _u4 = new u4_type();
        _u4->zero();
    }

    VMML_TEMPLATE_STRING
    size_t
    VMML_TEMPLATE_CLASSNAME::nnz_core() const {
        return _core.nnz();
    }

    VMML_TEMPLATE_STRING
    size_t
    VMML_TEMPLATE_CLASSNAME::size_core() const {
        return _core.size();
    }

    VMML_TEMPLATE_STRING
    VMML_TEMPLATE_CLASSNAME::~tucker4_tensor() {
        delete _u1;
        delete _u2;
        delete _u3;
        delete _u4;
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::reconstruct(t4_type& data_) {
        // TODO (maybe try with the reordered summation formula)
        t4_ttm::full_tensor4_matrix_multiplication(_core, *_u1, *_u2, *_u3, *_u4, data_);
    }
    
	VMML_TEMPLATE_STRING
	template< typename T_init>
	tensor_stats
	VMML_TEMPLATE_CLASSNAME::decompose(const t4_type& data_, T_init init, const size_t max_iterations, const float tolerance)	{
		return tucker_als(data_, init, max_iterations, tolerance);
	}
    
    VMML_TEMPLATE_STRING
    template< typename T_init>
    tensor_stats
    VMML_TEMPLATE_CLASSNAME::tucker_als(const t4_type& data_, T_init init, const size_t max_iterations, const float tolerance) {
        tensor_stats result;

        typedef t4_hooi< R1, R2, R3, R4, I1, I2, I3, I4, T_internal > hooi_type;
        result += hooi_type::als(data_, *_u1, *_u2, *_u3, *_u4, _core, init, 0, max_iterations, tolerance);

        return result;
    }

#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME

} // namespace vmml

#endif	/* TUCKER4_TENSOR_HPP */
