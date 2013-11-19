/*
 * VMMLib - Tensor Classes
 *
 * @author Rafael Ballester
 *
 * Class for encapsulating several tensor experimental properties, so that they can be easily returned, manipulated and printed (approximation error, decomposition and reconstruction times, etc.).
 *
 */
#include <string>

#ifndef __VMML__TENSOR_STATS__HPP__
#define	__VMML__TENSOR_STATS__HPP__

namespace vmml {

    class tensor_stats {
    public:

        tensor_stats() {
            ranks = n_iterations = error = nnz = dec_time = rec_time = 0;
        }

        tensor_stats(const tensor_stats& other) {
            description = other.description;
            ranks = other.ranks;
            n_iterations = other.n_iterations;
            error = other.error;
            nnz = other.nnz;
            dec_time = other.dec_time;
            rec_time = other.rec_time;
        }

        inline tensor_stats operator+(const tensor_stats& other) const {
            tensor_stats result(*this);
            result += other;
            return result;
        }

        void operator+=(const tensor_stats& other) {
            ranks += other.ranks;
            n_iterations += other.n_iterations;
            error += other.error;
            nnz += other.nnz;
            dec_time += other.dec_time;
            rec_time += other.rec_time;
        }

        friend std::ostream& operator <<(std::ostream& os,
                const tensor_stats& stats) {
            os << stats.description << " " << stats.ranks << " " << stats.n_iterations << " " << stats.error << " " << stats.nnz << " " << stats.dec_time << " " << stats.rec_time;
            return os;
        }

        std::string get_description() {
            return description;
        }

        std::string get_short_description() {
            return short_description;
        }

        int get_ranks() {
            return ranks;
        }

        int get_n_iterations() {
            return n_iterations;
        }

        double get_error() {
            return error;
        }

        int get_nnz() {
            return nnz;
        }

        double get_dec_time() {
            return dec_time;
        }

        double get_rec_time() {
            return rec_time;
        }

        void set_description( const std::string& desc ) {
            description = desc;
        }

        void set_short_description( const std::string& short_desc ) {
            short_description = short_desc;
        }

        void set_ranks( const int r ) { ranks = r; }

        void set_n_iterations( const int n_it ) { n_iterations = n_it; }

        void set_error( const double err ) { error = err; }

        void set_nnz( const int nnz_ ) { nnz = nnz_; }

        void set_dec_time( const double dec ) { dec_time = dec; }

        void set_rec_time( const double rec ) { rec_time = rec; }

        static std::string get_content() {
            return "description ranks n_iterations error nnz dec_time rec_time";
        }

    private:
        std::string description;
        std::string short_description; // The description, without rank sizes
        int ranks;
        int n_iterations;
        double error;
        int nnz;
        double dec_time;
        double rec_time;

    };

} // namespace vmml

#endif

