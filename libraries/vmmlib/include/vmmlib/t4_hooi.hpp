/*
 * VMMLib - Tensor Classes
 *
 * @author Rafa Ballester
 *
 */

#ifndef __VMML__T4_HOOI__HPP__
#define __VMML__T4_HOOI__HPP__


#include <vmmlib/t4_hosvd.hpp>
#include <vmmlib/t3_ttm.hpp>
#include <vmmlib/t4_ttm.hpp>
#include <vmmlib/matrix_pseudoinverse.hpp>
#include <vmmlib/tensor_stats.hpp>

namespace vmml {

    template< size_t R1, size_t R2, size_t R3, size_t R4, size_t I1, size_t I2, size_t I3, size_t I4, typename T = float >
            class t4_hooi {
    public:

        typedef tensor4< I1, I2, I3, I4, T > t4_type;

        typedef tensor4< R1, R2, R3, R4, T > t4_core_type;

        typedef matrix< I1, R1, T > u1_type;
        typedef matrix< I2, R2, T > u2_type;
        typedef matrix< I3, R3, T > u3_type;
        typedef matrix< I4, R4, T > u4_type;

        typedef matrix< R1, I1, T > u1_t_type;
        typedef matrix< R2, I2, T > u2_t_type;
        typedef matrix< R3, I3, T > u3_t_type;
        typedef matrix< R4, I4, T > u4_t_type;

        template< typename T_init>
        static tensor_stats als(const t4_type& data_, u1_type& u1_, u2_type& u2_, u3_type& u3_, u4_type& u4_, T_init init, const double& max_f_norm_ = 0.0, const size_t max_iterations = 10, const float tolerance = 1e-04);
        
        template< typename T_init>
        static tensor_stats als(const t4_type& data_, u1_type& u1_, u2_type& u2_, u3_type& u3_, u4_type& u4_, t4_core_type& core_, T_init init, const double& max_f_norm_ = 0.0, const size_t max_iterations = 10, const float tolerance = 1e-04);

        // init functors

        struct init_hosvd {

            inline void operator()(const t4_type& data_, u1_type& u1_, u2_type& u2_, u3_type & u3_, u4_type & u4_) {
                t4_hosvd< R1, R2, R3, R4, I1, I2, I3, I4, T >::apply_mode2(data_, u2_);
                t4_hosvd< R1, R2, R3, R4, I1, I2, I3, I4, T >::apply_mode3(data_, u3_);
                t4_hosvd< R1, R2, R3, R4, I1, I2, I3, I4, T >::apply_mode4(data_, u4_);
            }
        };
        
        struct init_random {

            inline void operator()(const t4_type& data_, u1_type& u1_, u2_type& u2_, u3_type & u3_, u4_type & u4_) {
                srand(time(NULL));
                u2_.set_random();
                u3_.set_random();
                u4_.set_random();

                u2_ /= u2_.frobenius_norm();
                u3_ /= u3_.frobenius_norm();
                u4_ /= u4_.frobenius_norm();
            }
        };

    protected:


        static void optimize_mode1(const t4_type& data_, const u2_type& u2_, const u3_type& u3_, const u4_type& u4_,
                tensor4< I1, R2, R3, R4, T >& projection_);
        static void optimize_mode2(const t4_type& data_, const u1_type& u1_, const u3_type& u3_, const u4_type& u4_,
            tensor4< R1, I2, R3, R4, T >& projection_);
        static void optimize_mode3(const t4_type& data_, const u1_type& u1_, const u2_type& u2_, const u4_type& u4_,
            tensor4< R1, R2, I3, R4, T >& projection_);
        static void optimize_mode4(const t4_type& data_, const u1_type& u1_, const u2_type& u2_, const u3_type& u3_,
            tensor4< R1, R2, R3, I4, T >& projection_);


    }; //end class t3_hooi

#define VMML_TEMPLATE_STRING        template< size_t R1, size_t R2, size_t R3, size_t R4, size_t I1, size_t I2, size_t I3, size_t I4, typename T >
#define VMML_TEMPLATE_CLASSNAME     t4_hooi< R1, R2, R3, R4, I1, I2, I3, I4, T >

    VMML_TEMPLATE_STRING
    template< typename T_init>
    tensor_stats
    VMML_TEMPLATE_CLASSNAME::als(const t4_type& data_,
            u1_type& u1_, u2_type& u2_, u3_type& u3_, u4_type& u4_,
            T_init init,
            const double& max_f_norm_, const size_t max_iterations_, const float tolerance_) {
        t4_core_type core;
        core.zero();
        return als(data_, u1_, u2_, u3_, u4_, core, init, max_f_norm_, max_iterations_, tolerance_);
    }

    VMML_TEMPLATE_STRING
    template< typename T_init>
    tensor_stats
    VMML_TEMPLATE_CLASSNAME::als(const t4_type& data_,
            u1_type& u1_, u2_type& u2_, u3_type& u3_, u4_type& u4_,
            t4_core_type& core_,
            T_init init,
            const double& max_f_norm_, const size_t max_iterations_, const float tolerance_) {
        tensor_stats result;

        //intialize basis matrices
        init(data_, u1_, u2_, u3_, u4_);

        core_.zero();
        T max_f_norm = 0.0;
        T f_norm, fit, fitchange, fitold, normresidual;
        if (tolerance_ > 0) {
            max_f_norm = max_f_norm_;

            if (max_f_norm <= 0.0) {
                max_f_norm = data_.frobenius_norm();
            }
            fit = 0;
            //removed to save computation
            /*if ( (max_f_norm != 0) && (max_f_norm > f_norm) )
            {
                    fit = 1 - (normresidual / max_f_norm);
            } else {
                    fit = 1;
            }*/
            fitchange = 1;
            fitold = fit;
            normresidual = 0;
        }

        tensor4< I1, R2, R3, R4, T > projection1;
        tensor4< R1, I2, R3, R4, T > projection2;
        tensor4< R1, R2, I3, R4, T > projection3;
        tensor4< R1, R2, R3, I4, T > projection4;

#if TUCKER_LOG
        std::cout << "HOOI ALS (for tensor3) " << std::endl
                << "initial fit: " << fit << ", "
                << "frobenius norm original: " << max_f_norm << std::endl;
#endif
        size_t i = 0;
        while (i < max_iterations_ && (tolerance_ < 0 || fitchange >= tolerance_)) { //do until converges
            fitold = fit;

            //optimize modes
            optimize_mode1(data_, u2_, u3_, u4_, projection1);
            t4_hosvd< R1, R2, R3, R4, I1, R2, R3, R4, T >::apply_mode1(projection1, u1_);

            optimize_mode2(data_, u1_, u3_, u4_, projection2);
            t4_hosvd< R1, R2, R3, R4, R1, I2, R3, R4, T >::apply_mode2(projection2, u2_);

            optimize_mode3(data_, u1_, u2_, u4_, projection3);
            t4_hosvd< R1, R2, R3, R4, R1, R2, I3, R4, T >::apply_mode3(projection3, u3_);
            
            optimize_mode4(data_, u1_, u2_, u3_, projection4);
            t4_hosvd< R1, R2, R3, R4, R1, R2, R3, I4, T >::apply_mode4(projection4, u4_);

            t4_ttm::mode4_multiply_fwd(projection4, transpose(u4_), core_);

            if (tolerance_ > 0) {
                f_norm = core_.frobenius_norm();
                normresidual = sqrt( fabs(max_f_norm * max_f_norm - f_norm * f_norm) );
                fit = 1 - (normresidual / max_f_norm);
                fitchange = fabs(fitold - fit);
#if TUCKER_LOG
                std::cout << "iteration '" << i << "', fit: " << fit
                        << ", fitdelta: " << fitchange
                        << ", frobenius norm of core: " << f_norm << std::endl;
#endif
            }
            ++i;
        }
        result.set_n_iterations(i);
        return result;
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::optimize_mode1(const t4_type& data_, const u2_type& u2_, const u3_type& u3_, const u4_type& u4_,
            tensor4< I1, R2, R3, R4, T >& projection_) {
        u2_t_type* u2_inv = new u2_t_type;
        u3_t_type* u3_inv = new u3_t_type;
        u4_t_type* u4_inv = new u4_t_type;
        u2_.transpose_to(*u2_inv);
        u3_.transpose_to(*u3_inv);
        u4_.transpose_to(*u4_inv);

        //forward cyclic matricization/unfolding (after Kiers, 2000) -> memory optimized
        tensor4< I1, R2, I3, I4, T > tmp1_;
        tensor4< I1, R2, R3, I4, T > tmp2_;
        t4_ttm::mode2_multiply_fwd(data_, *u2_inv, tmp1_);
        t4_ttm::mode3_multiply_fwd(tmp1_, *u3_inv, tmp2_);
        t4_ttm::mode4_multiply_fwd(tmp2_, *u4_inv, projection_);

        delete u2_inv;
        delete u3_inv;
        delete u4_inv;
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::optimize_mode2(const t4_type& data_, const u1_type& u1_, const u3_type& u3_, const u4_type& u4_,
            tensor4< R1, I2, R3, R4, T >& projection_) {
        u1_t_type* u1_inv = new u1_t_type;
        u3_t_type* u3_inv = new u3_t_type;
        u4_t_type* u4_inv = new u4_t_type;
        u1_.transpose_to(*u1_inv);
        u3_.transpose_to(*u3_inv);
        u4_.transpose_to(*u4_inv);

        //forward cyclic matricization/unfolding (after Kiers, 2000) -> memory optimized
        tensor4< R1, I2, I3, I4, T > tmp1_;
        tensor4< R1, I2, R3, I4, T > tmp2_;
        t4_ttm::mode1_multiply_fwd(data_, *u1_inv, tmp1_);
        t4_ttm::mode3_multiply_fwd(tmp1_, *u3_inv, tmp2_);
        t4_ttm::mode4_multiply_fwd(tmp2_, *u4_inv, projection_);

        delete u1_inv;
        delete u3_inv;
        delete u4_inv;
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::optimize_mode3(const t4_type& data_, const u1_type& u1_, const u2_type& u2_, const u4_type& u4_,
            tensor4< R1, R2, I3, R4, T >& projection_) {
        u1_t_type* u1_inv = new u1_t_type;
        u2_t_type* u2_inv = new u2_t_type;
        u4_t_type* u4_inv = new u4_t_type;
        u1_.transpose_to(*u1_inv);
        u2_.transpose_to(*u2_inv);
        u4_.transpose_to(*u4_inv);

        //forward cyclic matricization/unfolding (after Kiers, 2000) -> memory optimized
        tensor4< R1, I2, I3, I4, T > tmp1_;
        tensor4< R1, R2, I3, I4, T > tmp2_;
        t4_ttm::mode1_multiply_fwd(data_, *u1_inv, tmp1_);
        t4_ttm::mode2_multiply_fwd(tmp1_, *u2_inv, tmp2_);
        t4_ttm::mode4_multiply_fwd(tmp2_, *u4_inv, projection_);

        delete u1_inv;
        delete u2_inv;
        delete u4_inv;
    }
    
    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::optimize_mode4(const t4_type& data_, const u1_type& u1_, const u2_type& u2_, const u3_type& u3_,
            tensor4< R1, R2, R3, I4, T >& projection_) {
        u1_t_type* u1_inv = new u1_t_type;
        u2_t_type* u2_inv = new u2_t_type;
        u3_t_type* u3_inv = new u3_t_type;
        u1_.transpose_to(*u1_inv);
        u2_.transpose_to(*u2_inv);
        u3_.transpose_to(*u3_inv);

        //forward cyclic matricization/unfolding (after Kiers, 2000) -> memory optimized
        tensor4< R1, I2, I3, I4, T > tmp1_;
        tensor4< R1, R2, I3, I4, T > tmp2_;
        t4_ttm::mode1_multiply_fwd(data_, *u1_inv, tmp1_);
        t4_ttm::mode2_multiply_fwd(tmp1_, *u2_inv, tmp2_);
        t4_ttm::mode3_multiply_fwd(tmp2_, *u3_inv, projection_);

        delete u1_inv;
        delete u2_inv;
        delete u3_inv;
    }


#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME


}//end vmml namespace

#endif

