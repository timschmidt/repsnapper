/*
 * VMMLib - Tensor Classes
 *
 * @author Susanne Suter
 * @author Jonas Boesch
 * @author Rafael Ballester
 *
 * The cp3 tensor class is consists of three basis matrices u1-u3 and R lambda values for a given rank-R approximation
 * CP stands for Candecomp/Parafac (1970)
 * - Carroll & Chang, 1970: Analysis of Individual Differences in Multidimensional Scaling via an N-way generalization of ``Eckart--Young'' decompositions, Psychometrika.
 * - Harshman, 1970: Foundations of the PARAFAC procedure: Models and conditions for an 'explanatory' multi-modal factor analysis,UCLA Working Papers in Phonetics.
 * - De Lathauwer, De Moor, Vandewalle, 2000: A multilinear singular value decomposition, SIAM J. Matrix Anal. Appl.
 * - Kolda & Bader, 2009: Tensor Decompositions and Applications, SIAM Review.
 *
 */

#ifndef __VMML__CP3_TENSOR__HPP__
#define __VMML__CP3_TENSOR__HPP__

#include <vmmlib/matrix_pseudoinverse.hpp>
#include <vmmlib/t3_hopm.hpp>
#include <vmmlib/t3_ihopm.hpp>
#include <vmmlib/tensor3_iterator.hpp>


namespace vmml {

    template< size_t R, size_t I1, size_t I2, size_t I3,
              typename T_value = double, typename T_coeff = float >
    class cp3_tensor
    {
    public:
        typedef float T_internal;

        typedef tensor3< I1, I2, I3, T_value > t3_type;
        typedef typename t3_type::iterator t3_iterator;
        typedef typename t3_type::const_iterator t3_const_iterator;

        typedef tensor3< I1, I2, I3, T_internal > t3_comp_type;

        typedef tensor3< I1, I2, I3, T_coeff > t3_coeff_type;
        typedef typename t3_coeff_type::iterator t3_coeff_iterator;
        typedef typename t3_coeff_type::const_iterator t3_coeff_const_iterator;

        typedef matrix< I1, R, T_coeff > u1_type;
        typedef typename u1_type::iterator u1_iterator;
        typedef typename u1_type::const_iterator u1_const_iterator;

        typedef matrix< I2, R, T_coeff > u2_type;
        typedef typename u2_type::iterator u2_iterator;
        typedef typename u2_type::const_iterator u2_const_iterator;

        typedef matrix< I3, R, T_coeff > u3_type;
        typedef typename u3_type::iterator u3_iterator;
        typedef typename u3_type::const_iterator u3_const_iterator;

        typedef matrix< I1, R, T_internal > u1_comp_type;
        typedef matrix< I2, R, T_internal > u2_comp_type;
        typedef matrix< I3, R, T_internal > u3_comp_type;

        typedef vector< R, T_internal > lambda_comp_type;
        typedef vector< R, T_coeff > lambda_type;

        cp3_tensor(u1_type& U1, u2_type& U2, u3_type& U3, lambda_type& lambdas_);
        cp3_tensor();
        ~cp3_tensor();

        void get_lambdas(lambda_type& data_) const {
            data_ = *_lambdas;
        };

        void get_u1(u1_type& U1) const {
            U1 = *_u1;
        };

        void get_u2(u2_type& U2) const {
            U2 = *_u2;
        };

        void get_u3(u3_type& U3) const {
            U3 = *_u3;
        };

        void set_lambdas(lambda_type& lambdas) {
            *_lambdas = lambdas;
            _lambdas_comp->cast_from(lambdas);
        };

        void set_u1(u1_type& U1) {
            *_u1 = U1;
            _u1_comp->cast_from(U1);
        };

        void set_u2(u2_type& U2) {
            *_u2 = U2;
            _u1_comp->cast_from(U2);
        };

        void set_u3(u3_type& U3) {
            *_u3 = U3;
            _u1_comp->cast_from(U3);
        };

        void set_lambda_comp(lambda_comp_type& lambdas) {
            *_lambdas_comp = lambdas;
            _lambdas->cast_from(lambdas);
        };

        void set_u1_comp(u1_comp_type& U1) {
            *_u1_comp = U1;
            _u1->cast_from(U1);
        };

        void set_u2_comp(u2_comp_type& U2) {
            *_u2_comp = U2;
            _u1->cast_from(U2);
        };

        void set_u3_comp(u3_comp_type& U3) {
            *_u3_comp = U3;
            _u1->cast_from(U3);
        };

        void get_lambda_comp(lambda_comp_type& data_) const {
            data_ = _lambdas_comp;
        };

        void get_u1_comp(u1_comp_type& U1) const {
            U1 = *_u1_comp;
        };

        void get_u2_comp(u2_comp_type& U2) const {
            U2 = *_u2_comp;
        };

        void get_u3_comp(u3_comp_type& U3) const {
            U3 = *_u3_comp;
        };

        void export_to(std::vector< T_coeff >& data_) const;
        void import_from(std::vector< T_coeff >& data_);
        void reconstruct(t3_type& data_) const;
        double error(t3_type& data_) const;
        size_t nnz() const;

        template< typename T_init >
        tensor_stats decompose(const t3_type& data_, T_init init, const size_t max_iterations_ = 50, const float tolerance = 1e-04);
        
        template< typename T_init >
        tensor_stats cp_als(const t3_type& data_, T_init init, const size_t max_iterations_ = 50, const float tolerance = 1e-04);

        template< size_t NBLOCKS, typename T_init >
        tensor_stats i_cp_als(const t3_type& data_, T_init init, const size_t max_iterations_ = 50, const float tolerance = 1e-04);

        template< size_t K >
        void reduce_ranks(const cp3_tensor< K, I1, I2, I3, T_value, T_coeff >& other);

    protected:

        cp3_tensor(const cp3_tensor< R, I1, I1, I1, T_value, T_coeff >& other) {
        };

        cp3_tensor< R, I1, I1, I1, T_value, T_coeff > operator=(const cp3_tensor< R, I1, I1, I1, T_value, T_coeff >& other) {
            return *this;
        };
        
        typedef cp3_tensor< R, I1, I2, I3, T_value, T_coeff > cp3_type;
	    friend std::ostream& operator <<(std::ostream& os, const cp3_type& dec_ ) {
            lambda_type lambdas;
            dec_.get_lambdas(lambdas);
            u1_type* u1 = new u1_type;
            dec_.get_u1(*u1);
            u2_type* u2 = new u2_type;
            dec_.get_u2(*u2);
            u3_type* u3 = new u3_type;
            dec_.get_u3(*u3);
			
            os << "U1: " << std::endl << *u1 << std::endl
			<< "U2: " << std::endl << *u2 << std::endl
			<< "U3: " << std::endl << *u3 << std::endl
			<< "lambdas: " << std::endl << lambdas << std::endl;
			
            delete u1;
            delete u2;
            delete u3;
            return os;
        }

        void cast_members();
        void cast_comp_members();

    private:

        lambda_type* _lambdas;
        u1_type* _u1;
        u2_type* _u2;
        u3_type* _u3;

        lambda_comp_type* _lambdas_comp;
        u1_comp_type* _u1_comp;
        u2_comp_type* _u2_comp;
        u3_comp_type* _u3_comp;

    }; // class cp3_tensor


#define VMML_TEMPLATE_STRING			template< size_t R, size_t I1, size_t I2, size_t I3, typename T_value, typename T_coeff >
#define VMML_TEMPLATE_CLASSNAME			cp3_tensor< R, I1, I2, I3, T_value, T_coeff >

    VMML_TEMPLATE_STRING
    VMML_TEMPLATE_CLASSNAME::cp3_tensor(u1_type& U1, u2_type& U2, u3_type& U3, lambda_type& lambdas_) {
        set_lambdas(lambdas_);
        set_u1(U1);
        set_u2(U2);
        set_u3(U3);
    }

    VMML_TEMPLATE_STRING
    VMML_TEMPLATE_CLASSNAME::cp3_tensor() {
        _lambdas = new vector< R, T_coeff > ();
        _lambdas->set(0);
        _u1 = new u1_type();
        _u1->zero();
        _u2 = new u2_type();
        _u2->zero();
        _u3 = new u3_type();
        _u3->zero();
        _lambdas_comp = new vector< R, T_internal>;
        _lambdas_comp->set(0);
        _u1_comp = new u1_comp_type;
        _u1_comp->zero();
        _u2_comp = new u2_comp_type;
        _u2_comp->zero();
        _u3_comp = new u3_comp_type;
        _u3_comp->zero();
    }

    VMML_TEMPLATE_STRING
    VMML_TEMPLATE_CLASSNAME::~cp3_tensor() {
        delete _u1;
        delete _u2;
        delete _u3;
        delete _lambdas;
        delete _u1_comp;
        delete _u2_comp;
        delete _u3_comp;
        delete _lambdas_comp;
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::cast_members() {
        _u1->cast_from(*_u1_comp);
        _u2->cast_from(*_u2_comp);
        _u3->cast_from(*_u3_comp);
        _lambdas->cast_from(*_lambdas_comp);
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::cast_comp_members() {
        _u1_comp->cast_from(*_u1);
        _u2_comp->cast_from(*_u2);
        _u3_comp->cast_from(*_u3);
        _lambdas_comp->cast_from(_lambdas);
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::reconstruct(t3_type& data_) const {

        //FIXME: check data types
        t3_comp_type data;
        data.cast_from(data_);

        typedef t3_hopm< R, I1, I2, I3, T_internal > hopm_type;
        hopm_type::reconstruct(data, *_u1_comp, *_u2_comp, *_u3_comp, *_lambdas_comp);

        //convert reconstructed data, which is in type T_internal (double, float) to T_value (uint8 or uint16)
        if ((sizeof (T_value) == 1) || (sizeof (T_value) == 2)) {
            data_.float_t_to_uint_t(data);
        } else {
            data_.cast_from(data);
        }

    }

    VMML_TEMPLATE_STRING
    double
    VMML_TEMPLATE_CLASSNAME::error(t3_type& original) const {
        typedef t3_hopm< R, I1, I2, I3, T_internal > hopm_type;
        t3_comp_type data;
        hopm_type::reconstruct(data, *_u1_comp, *_u2_comp, *_u3_comp, *_lambdas_comp);
        double err = data.frobenius_norm(original) / original.frobenius_norm() * 100;
        return err;
    }

    VMML_TEMPLATE_STRING
    template< typename T_init >
    tensor_stats
    VMML_TEMPLATE_CLASSNAME::decompose(const t3_type& data_, T_init init, const size_t max_iterations_, const float tolerance) {
		return cp_als(data_, init, max_iterations_, tolerance );
    }
    
    VMML_TEMPLATE_STRING
    template< typename T_init >
    tensor_stats
    VMML_TEMPLATE_CLASSNAME::cp_als(const t3_type& data_, T_init init, const size_t max_iterations_, const float tolerance) {
        tensor_stats result;

        t3_comp_type data;
        data.cast_from(data_);

        typedef t3_hopm< R, I1, I2, I3, T_internal > hopm_type;
        result += hopm_type::als(data, *_u1_comp, *_u2_comp, *_u3_comp, *_lambdas_comp, init, max_iterations_, tolerance);

        cast_members();
        return result;
    }

    VMML_TEMPLATE_STRING
    template< size_t NBLOCKS, typename T_init >
    tensor_stats
    VMML_TEMPLATE_CLASSNAME::i_cp_als(const t3_type& data_, T_init init, const size_t max_iterations_, const float tolerance) {
        tensor_stats result;

        t3_comp_type data;
        data.cast_from(data_);

        typedef t3_ihopm< R, NBLOCKS, I1, I2, I3, T_internal > ihopm_type;
        result += ihopm_type::incremental_als(data, *_u1_comp, *_u2_comp, *_u3_comp, *_lambdas_comp, init, max_iterations_, tolerance);


        cast_members();

        return result;
    }

    VMML_TEMPLATE_STRING
    template< size_t K >
    void
    VMML_TEMPLATE_CLASSNAME::reduce_ranks(const cp3_tensor< K, I1, I2, I3, T_value, T_coeff >& other)
    // K -> R; I1, I2, I3 stay the same
    {
        assert(R <= K);

        //reduce basis matrices
        matrix< I1, K, T_coeff >* u1 = new matrix< I1, K, T_coeff > ();
        other.get_u1(*u1);
        matrix< I2, K, T_coeff >* u2 = new matrix< I2, K, T_coeff > ();
        other.get_u2(*u2);
        matrix< I3, K, T_coeff >* u3 = new matrix< I3, K, T_coeff > ();
        other.get_u3(*u3);
        for (size_t r = 0; r < R; ++r) {
            _u1->set_column(r, u1->get_column(r));
            _u2->set_column(r, u2->get_column(r));
            _u3->set_column(r, u3->get_column(r));
        }
        // reduce vector of lambdas
        vector< K, T_coeff >* lambdas;
        other.get_lambdas(lambdas);
        _lambdas_comp->set(lambdas);

        cast_comp_members();

        delete u1;
        delete u2;
        delete u3;
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::export_to(std::vector< T_coeff >& data_) const {
        u1_const_iterator it = _u1->begin(),
                it_end = _u1->end();
        for (; it != it_end; ++it) {
            data_.push_back(*it);
        }

        u2_const_iterator u2_it = _u2->begin(),
                u2_it_end = _u2->end();
        for (; u2_it != u2_it_end; ++u2_it) {
            data_.push_back(*u2_it);
        }

        u3_const_iterator u3_it = _u3->begin(),
                u3_it_end = _u3->end();
        for (; u3_it != u3_it_end; ++u3_it) {
            data_.push_back(*u3_it);
        }

        //TODO: iterate over lambdas
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::import_from(std::vector< T_coeff >& data_) {
        size_t i = 0; //iterator over data_

        u1_iterator it = _u1->begin(),
                it_end = _u1->end();
        for (; it != it_end; ++it, ++i) {
            *it = data_.at(i);
        }

        u2_iterator u2_it = _u2->begin(),
                u2_it_end = _u2->end();
        for (; u2_it != u2_it_end; ++u2_it, ++i) {
            *u2_it = data_.at(i);
        }

        u3_iterator u3_it = _u3->begin(),
                u3_it_end = _u3->end();
        for (; u3_it != u3_it_end; ++u3_it, ++i) {
            *u3_it = data_.at(i);
        }

        //TODO: import lambdas

    }

    VMML_TEMPLATE_STRING
    size_t
    VMML_TEMPLATE_CLASSNAME::nnz() const {
        size_t counter = 0;

        counter += _u1_comp->nnz();
        counter += _u2_comp->nnz();
        counter += _u3_comp->nnz();
        counter += _lambdas_comp->nnz();

        return counter;
    }

#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME

} // namespace vmml

#endif

