/*
 * VMMLib - Tensor Classes
 *
 * @author Rafael Ballester
 *
 * incremental version of higher-order orthogonal iteration (HOOI), see t3_hooi.hpp
 *
 */


#ifndef __VMML__T3_IHOOI_HPP__
#define	__VMML__T3_IHOOI_HPP__

#include <vmmlib/t3_hopm.hpp>
#include <vmmlib/t3_hooi.hpp>

namespace vmml {

    template< size_t R1, size_t R2, size_t R3, size_t NBLOCKS, size_t I1, size_t I2, size_t I3, typename T_val = float, typename T_coeff = double >
            class t3_ihooi {
    public:

        typedef tensor3< I1, I2, I3, T_val > t3_type;
        typedef tensor3< I1, I2, I3, T_coeff > t3_coeff_type;

        // The output data
        typedef tensor3< R1, R2, R3, T_val > t3_core_type;
        typedef matrix< I1, R1, T_val > u1_type;
        typedef matrix< I2, R2, T_val > u2_type;
        typedef matrix< I3, R3, T_val > u3_type;

        // The incremental data, it is iteratively filled and later cast into the output
        typedef tensor3< R1, R2, R3, T_coeff > t3_core_incr_type;
        typedef matrix< I1, R1, T_coeff > u1_incr_type;
        typedef matrix< I2, R2, T_coeff > u2_incr_type;
        typedef matrix< I3, R3, T_coeff > u3_incr_type;

        // The pieces that are used at each iteration
        typedef tensor3< R1 / NBLOCKS, R2 / NBLOCKS, R3 / NBLOCKS, T_coeff > t3_core_tmp_type;
        typedef matrix< I1, R1 / NBLOCKS, T_coeff > u1_tmp_type;
        typedef matrix< I2, R2 / NBLOCKS, T_coeff > u2_tmp_type;
        typedef matrix< I3, R3 / NBLOCKS, T_coeff > u3_tmp_type;

        // Used to "transport" individual columns between the tmp_type's and the incr_type's
        typedef matrix< I1, 1, T_coeff > u1_1col_type;
        typedef matrix< I2, 1, T_coeff > u2_1col_type;
        typedef matrix< I3, 1, T_coeff > u3_1col_type;

        // Incremental Tucker-ALS
        template< typename T_init >
        static tensor_stats i_als(const t3_type& data_, u1_type& u1_, u2_type& u2_, u3_type& u3_, t3_core_type& core_, T_init init, const size_t max_iterations_ = 50, const float tolerance = 1e-04);

        // Incremental CP-Tucker-ALS: at each iteration, R-rank CP is performed, but a (R1,R2,R3)-Tucker core (R1,R2,R3 <= R) is computed from the resulting matrices.
        template< size_t R, typename T_init >
        static tensor_stats i_cp_als(const t3_type& data_, u1_type& u1_, u2_type& u2_, u3_type& u3_, t3_core_type& core_, T_init init, const size_t max_iterations_ = 50, const float tolerance = 1e-04);
    };



#define VMML_TEMPLATE_STRING		template< size_t R1, size_t R2, size_t R3, size_t NBLOCKS, size_t I1, size_t I2, size_t I3, typename T_val, typename T_coeff >
#define VMML_TEMPLATE_CLASSNAME		t3_ihooi< R1, R2, R3, NBLOCKS, I1, I2, I3, T_val, T_coeff >

    VMML_TEMPLATE_STRING
    template< typename T_init>
    tensor_stats
    VMML_TEMPLATE_CLASSNAME::i_als(const t3_type& data_, u1_type& u1_, u2_type& u2_, u3_type& u3_, t3_core_type& core_, T_init init, const size_t max_iterations_, const float tolerance) {
        tensor_stats result;

        if ((R1 % NBLOCKS != 0) or (R2 % NBLOCKS != 0) or (R3 % NBLOCKS != 0)) {
            std::ostringstream convert1, convert2, convert3, convert4;
            convert1 << R1;
            convert2 << R2;
            convert3 << R3;
            convert4 << NBLOCKS;
            VMMLIB_ERROR("In incremental Tucker, (R1,R2,R3) = (" + convert1.str() + "," + convert2.str() + "," + convert3.str() + "), NBLOCKS = " + convert2.str() + " (must be divisible)", VMMLIB_HERE);
        }
        t3_coeff_type* approx_data = new t3_coeff_type;
        approx_data->zero();
        t3_coeff_type* residual_data = new t3_coeff_type;
        residual_data->cast_from(data_);

        t3_core_tmp_type* t3_core_tmp = new t3_core_tmp_type;
        t3_core_tmp->zero();
        u1_tmp_type* u1_tmp = new u1_tmp_type;
        u2_tmp_type* u2_tmp = new u2_tmp_type;
        u3_tmp_type* u3_tmp = new u3_tmp_type;

        t3_core_incr_type* t3_core_incr = new t3_core_incr_type;
        t3_core_incr->zero();
        u1_incr_type* u1_incr = new u1_incr_type;
        u1_incr->zero();
        u2_incr_type* u2_incr = new u2_incr_type;
        u2_incr->zero();
        u3_incr_type* u3_incr = new u3_incr_type;
        u3_incr->zero();

        u1_1col_type* u1_1col = new u1_1col_type;
        u2_1col_type* u2_1col = new u2_1col_type;
        u3_1col_type* u3_1col = new u3_1col_type;

        typedef t3_hooi < R1 / NBLOCKS, R2 / NBLOCKS, R3 / NBLOCKS, I1, I2, I3, T_coeff > hooi_type;

        for (size_t i = 0; i < NBLOCKS; ++i) {
#ifdef TUCKER_LOG
            std::cout << "Incremental Tucker: block number '" << i << "'" << std::endl;
#endif

            // Do Tucker-ALS for this block
            result += hooi_type::als(*residual_data, *u1_tmp, *u2_tmp, *u3_tmp, *t3_core_tmp, typename hooi_type::init_hosvd(), tolerance);

            // Copy the newly obtained columns into ui_incr
            for (size_t r = 0; r < R1 / NBLOCKS; ++r) {
                u1_tmp->get_column(r, *u1_1col);
                u1_incr->set_column(i * R1 / NBLOCKS + r, *u1_1col);
            }
            for (size_t r = 0; r < R2 / NBLOCKS; ++r) {
                u2_tmp->get_column(r, *u2_1col);
                u2_incr->set_column(i * R2 / NBLOCKS + r, *u2_1col);
            }
            for (size_t r = 0; r < R3 / NBLOCKS; ++r) {
                u3_tmp->get_column(r, *u3_1col);
                u3_incr->set_column(i * R3 / NBLOCKS + r, *u3_1col);
            }

            // Copy the newly obtained block core into t3_core_incr
            t3_core_incr->set_sub_tensor3(*t3_core_tmp, i * R1 / NBLOCKS, i * R2 / NBLOCKS, i * R3 / NBLOCKS);

            // Reconstruct this t3_core_tmp to set a new residual value.
            t3_ttm::full_tensor3_matrix_multiplication(*t3_core_tmp, *u1_tmp, *u2_tmp, *u3_tmp, *approx_data);
            *residual_data = *residual_data - *approx_data;
        }

        u1_.cast_from(*u1_incr);
        u2_.cast_from(*u2_incr);
        u3_.cast_from(*u3_incr);

        core_.cast_from(*t3_core_incr);

        delete u1_1col;
        delete u2_1col;
        delete u3_1col;
        delete t3_core_tmp;
        delete u1_tmp;
        delete u2_tmp;
        delete u3_tmp;
        delete t3_core_incr;
        delete u1_incr;
        delete u2_incr;
        delete u3_incr;
        delete residual_data;
        delete approx_data;

        return result;
    }

    VMML_TEMPLATE_STRING
    template< size_t R, typename T_init>
    tensor_stats
    VMML_TEMPLATE_CLASSNAME::i_cp_als(const t3_type& data_, u1_type& u1_, u2_type& u2_, u3_type& u3_, t3_core_type& core_, T_init init, const size_t max_iterations_, const float tolerance) {
        tensor_stats result;

        if ((R1 % NBLOCKS != 0) or (R2 % NBLOCKS != 0) or (R3 % NBLOCKS != 0)) {
            std::ostringstream convert1, convert2, convert3, convert4, convert5;
            convert1 << R;
            convert2 << R1;
            convert3 << R2;
            convert4 << R3;
            convert5 << NBLOCKS;
            VMMLIB_ERROR("In incremental CP-Tucker, R = " + convert1.str() + ", (R1,R2,R3) = (" + convert2.str() + "," + convert3.str() + "," + convert4.str() + "), NBLOCKS = " + convert5.str() + " (must be divisible, and R1,R2,R3 <= R)", VMMLIB_HERE);
        }

        t3_coeff_type* approx_data = new t3_coeff_type;
        approx_data->zero();
        t3_coeff_type* residual_data = new t3_coeff_type;
        residual_data->cast_from(data_);

        t3_core_tmp_type* t3_core_tmp = new t3_core_tmp_type;
        t3_core_tmp->zero();
        u1_tmp_type* u1_tmp = new u1_tmp_type;
        u1_tmp_type* u1_tmp2 = new u1_tmp_type;
        u2_tmp_type* u2_tmp = new u2_tmp_type;
        u2_tmp_type* u2_tmp2 = new u2_tmp_type;
        u3_tmp_type* u3_tmp = new u3_tmp_type;
        u3_tmp_type* u3_tmp2 = new u3_tmp_type;

        t3_core_incr_type* t3_core_incr = new t3_core_incr_type;
        t3_core_incr->zero();
        u1_incr_type* u1_incr = new u1_incr_type;
        u1_incr->zero();
        u2_incr_type* u2_incr = new u2_incr_type;
        u2_incr->zero();
        u3_incr_type* u3_incr = new u3_incr_type;
        u3_incr->zero();

        u1_1col_type* u1_1col = new u1_1col_type;
        u2_1col_type* u2_1col = new u2_1col_type;
        u3_1col_type* u3_1col = new u3_1col_type;

        typedef t3_hopm < R / NBLOCKS, I1, I2, I3, T_coeff > hopm_type;
        typedef vector < R / NBLOCKS, T_coeff > lambda_tmp_type;
        lambda_tmp_type* lambdas_tmp = new lambda_tmp_type;
        typedef matrix < I1, R / NBLOCKS, T_coeff > u1_cp_tmp_type;
        u1_cp_tmp_type* u1_cp_tmp = new u1_cp_tmp_type;
        typedef matrix < I2, R / NBLOCKS, T_coeff > u2_cp_tmp_type;
        u2_cp_tmp_type* u2_cp_tmp = new u2_cp_tmp_type;
        typedef matrix < I3, R / NBLOCKS, T_coeff > u3_cp_tmp_type;
        u3_cp_tmp_type* u3_cp_tmp = new u3_cp_tmp_type;

        typedef matrix < R1 / NBLOCKS, I1, T_coeff > u1_inv_tmp_type;
        u1_inv_tmp_type* u1_inv_tmp = new u1_inv_tmp_type;
        typedef matrix < R2 / NBLOCKS, I2, T_coeff > u2_inv_tmp_type;
        u2_inv_tmp_type* u2_inv_tmp = new u2_inv_tmp_type;
        typedef matrix < R3 / NBLOCKS, I3, T_coeff > u3_inv_tmp_type;
        u3_inv_tmp_type* u3_inv_tmp = new u3_inv_tmp_type;

        for (size_t i = 0; i < NBLOCKS; ++i) {
#ifdef TUCKER_LOG
            std::cout << "Incremental CP-Tucker: block number '" << i << "'" << std::endl;
#endif

            // Do CP-ALS for this block
            result += hopm_type::als(*residual_data, *u1_cp_tmp, *u2_cp_tmp, *u3_cp_tmp, *lambdas_tmp, typename hopm_type::init_hosvd(), max_iterations_, tolerance);

            // Compute the pseudoinverses
            u1_cp_tmp->get_sub_matrix(*u1_tmp, 0, 0);
            compute_pseudoinverse< u1_tmp_type > compute_pinv1;
            compute_pinv1(*u1_tmp, *u1_tmp2);
            u1_tmp2->transpose_to(*u1_inv_tmp);

            u2_cp_tmp->get_sub_matrix(*u2_tmp, 0, 0);
            compute_pseudoinverse< u2_tmp_type > compute_pinv2;
            compute_pinv2(*u2_tmp, *u2_tmp2);
            u2_tmp2->transpose_to(*u2_inv_tmp);

            u3_cp_tmp->get_sub_matrix(*u3_tmp, 0, 0);
            compute_pseudoinverse< u3_tmp_type > compute_pinv3;
            compute_pinv3(*u3_tmp, *u3_tmp2);
            u3_tmp2->transpose_to(*u3_inv_tmp);
            //            hooi_type::als(*residual_data, *u1_tmp, *u2_tmp, *u3_tmp, *t3_core_tmp, typename hooi_type::init_hosvd());

            // Project the initial data onto the pseudoinverses to get the core
            t3_ttm::full_tensor3_matrix_multiplication(*residual_data, *u1_inv_tmp, *u2_inv_tmp, *u3_inv_tmp, *t3_core_tmp);

            // Copy the newly obtained columns into ui_incr
            for (size_t r = 0; r < R1 / NBLOCKS; ++r) {
                u1_tmp->get_column(r, *u1_1col);
                u1_incr->set_column(i * R1 / NBLOCKS + r, *u1_1col);
            }
            for (size_t r = 0; r < R2 / NBLOCKS; ++r) {
                u2_tmp->get_column(r, *u2_1col);
                u2_incr->set_column(i * R2 / NBLOCKS + r, *u2_1col);
            }
            for (size_t r = 0; r < R3 / NBLOCKS; ++r) {
                u3_tmp->get_column(r, *u3_1col);
                u3_incr->set_column(i * R3 / NBLOCKS + r, *u3_1col);
            }

            // Copy the newly obtained block core into t3_core_incr
            t3_core_incr->set_sub_tensor3(*t3_core_tmp, i * R1 / NBLOCKS, i * R2 / NBLOCKS, i * R3 / NBLOCKS);

            // Reconstruct this t3_core_tmp to set a new residual value.
            t3_ttm::full_tensor3_matrix_multiplication(*t3_core_tmp, *u1_tmp, *u2_tmp, *u3_tmp, *approx_data);
            *residual_data = *residual_data - *approx_data;
        }

        u1_.cast_from(*u1_incr);
        u2_.cast_from(*u2_incr);
        u3_.cast_from(*u3_incr);

        core_.cast_from(*t3_core_incr);

        delete u1_1col;
        delete u2_1col;
        delete u3_1col;
        delete t3_core_tmp;
        delete u1_tmp;
        delete u2_tmp;
        delete u3_tmp;
        delete t3_core_incr;
        delete u1_incr;
        delete u2_incr;
        delete u3_incr;
        delete residual_data;
        delete approx_data;

        return result;
    }

#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME

}//end vmml namespace

#endif	/* T3_IHOOI_HPP */
