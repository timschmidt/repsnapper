/*
 * VMMLib - Tensor Classes
 *
 * @author Susanne Suter
 *
 * class to read/write and convert tensor3 files (from raw, to csv...)
 *
 */
#ifndef __VMML__T3_CONVERTER__HPP__
#define __VMML__T3_CONVERTER__HPP__

#include <vmmlib/tensor3.hpp>

namespace vmml {

    template< size_t I1, size_t I2, size_t I3, typename T = float >
    class t3_converter
    {
    public:
        typedef tensor3< I1, I2, I3, T > t3_t;
        typedef typename vmml::tensor3_iterator< tensor3< I1, I2, I3, T > > iterator;
        typedef typename vmml::tensor3_const_iterator< tensor3< I1, I2, I3, T > > const_iterator;

        template< typename T_convert >
        static void convert_raw(const std::string& dir_, const std::string& in_filename_, const std::string& out_filename_);

        //header size as bytes
        static void remove_uct_cylinder(const std::string& dir_,
                const std::string& in_filename_,
                const std::string& out_filename_,
                const double& sigma_,
                const size_t header_size_,
                const size_t radius_offset_,
                int seed_ = 0);

        static void export_to(const t3_t& input_, std::vector< T >& output_);
        static void import_from(const std::vector< T >& input_, t3_t& output_);
        static void write_to_raw(const t3_t& data_, const std::string& dir_, const std::string& filename_); // OK
        static void read_from_raw(t3_t& data_, const std::string& dir_, const std::string& filename_); // OK
        static void write_datfile(const std::string& dir_, const std::string& filename_);
        static void write_to_csv(const t3_t& data_, const std::string& dir_, const std::string& filename_);
        static void remove_normals_from_raw(const t3_t& data_, const std::string& dir_, const std::string& filename_);
        static double rmse_from_files(const std::string& dir_, const std::string& filename_a_, const std::string& filename_b_ );

        template< typename TT >
        static void quantize_to(const std::string& dir_,
                const std::string& in_filename_, const std::string& out_filename_,
                const T& min_value_, const T& max_value_);


    protected:

        static void concat_path(const std::string& dir_, const std::string& filename_, std::string& path_);

    }; //end t3_converter



#define VMML_TEMPLATE_STRING        template< size_t I1, size_t I2, size_t I3, typename T >
#define VMML_TEMPLATE_CLASSNAME     t3_converter< I1, I2, I3, T >

    VMML_TEMPLATE_STRING
    template< typename TT >
    void
    VMML_TEMPLATE_CLASSNAME::quantize_to(const std::string& dir_,
            const std::string& in_filename_, const std::string& out_filename_,
            const T& min_value_, const T& max_value_) {
        std::string path_in_raw = "";
        std::string path_out_raw = "";
        concat_path(dir_, in_filename_, path_in_raw);
        concat_path(dir_, out_filename_, path_out_raw);

        std::ofstream outfile;
        outfile.open(path_out_raw.c_str());

        std::ifstream infile;
        infile.open(path_in_raw.c_str(), std::ios::in);

        if (infile.is_open() && outfile.is_open()) {
            double max_tt_range = double((std::numeric_limits< TT >::max)());
            double min_tt_range = double((std::numeric_limits< TT >::min)());
            double tt_range = max_tt_range - min_tt_range;
            double t_range = max_value_ - min_value_;

            //std::cout << "tt min= " << min_tt_range << ", tt max= " << max_tt_range << ", t min= " << min_value_ << ", t max= " << max_value_ << std::endl;
            //std::cout << "tt range=" << tt_range << ", t range= " << t_range << std::endl;

            T* in_value;
            TT out_value;
            size_t len_in = sizeof (T);
            size_t len_out = sizeof (TT);
            char* data = new char[ len_in ];

            for (size_t i3 = 0; i3 < I3; ++i3) {
                for (size_t i1 = 0; i1 < I1; ++i1) {
                    for (size_t i2 = 0; i2 < I2; ++i2) {
                        //read value
                        infile.read(data, len_in);
                        in_value = (T*)&(data[0]);

                        //Quantize value
                        if (std::numeric_limits<TT>::is_signed) {
                            out_value = TT((std::min)((std::max)(min_tt_range, double((*in_value * tt_range / t_range) + 0.5)), max_tt_range));
                        } else {
                            out_value = TT((std::min)((std::max)(min_tt_range, double(((*in_value - min_value_) * tt_range / t_range) + 0.5)), max_tt_range));
                        }

                        //write_value
                        outfile.write((char*) &(out_value), len_out);
                    }
                }
            }

            infile.close();
            outfile.close();
        } else {
            infile.close();
            outfile.close();
            std::cout << "no file open" << std::endl;
        }
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::concat_path(const std::string& dir_, const std::string& filename_, std::string& path_) {
        int dir_length = dir_.size() - 1;
        int last_separator = dir_.find_last_of("/");
        path_ = dir_;
        if (last_separator < dir_length) {
            path_.append("/");
        }
        path_.append(filename_);

        //check for format
        if( filename_.find("raw", filename_.size() - 3) == std::string::npos )
        {
            path_.append(".");
            path_.append("raw");
        }
    }

    VMML_TEMPLATE_STRING
    template< typename T_convert >
    void
    VMML_TEMPLATE_CLASSNAME::convert_raw(const std::string& dir_, const std::string& in_filename_, const std::string& out_filename_) {
        std::string path_in_raw = "";
        std::string path_out_raw = "";
        concat_path(dir_, in_filename_, path_in_raw);
        concat_path(dir_, out_filename_, path_out_raw);

        std::ofstream outfile;
        outfile.open(path_out_raw.c_str());

        std::ifstream infile;
        infile.open(path_in_raw.c_str(), std::ios::in);

        if (infile.is_open() && outfile.is_open()) {
            T* in_value;
            T_convert out_value;
            size_t len_in = sizeof (T);
            size_t len_out = sizeof (T_convert);
            char* data = new char[ len_in ];

            for (size_t i3 = 0; i3 < I3; ++i3) {
                for (size_t i1 = 0; i1 < I1; ++i1) {
                    for (size_t i2 = 0; i2 < I2; ++i2) {
                        infile.read(data, len_in);
                        in_value = (T*)&(data[0]);
                        out_value = static_cast<T_convert> (*in_value);
                        outfile.write((char*) &(out_value), len_out);
                    }
                }
            }

            infile.close();
            outfile.close();
        } else {
            infile.close();
            outfile.close();
            std::cout << "no file open" << std::endl;
        }
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::remove_uct_cylinder(const std::string& dir_,
            const std::string& in_filename_,
            const std::string& out_filename_,
            const double& sigma_,
            const size_t header_size_,
            const size_t radius_offset_,
            int seed_) {
        std::string path_in_raw = "";
        std::string path_out_raw = "";
        concat_path(dir_, in_filename_, path_in_raw);
        concat_path(dir_, out_filename_, path_out_raw);

        std::ofstream outfile;
        outfile.open(path_out_raw.c_str());

        std::ifstream infile;
        infile.open(path_in_raw.c_str(), std::ios::in);

        //for noise adding in outer area
        if (seed_ >= 0)
            srand(seed_);

        double length = 0;
        double radius = (I1 - 1.0) / 2.0 - radius_offset_;
        radius *= radius;
        double k1 = 0;
        double k2 = 0;
        double fill_value = 0;

        if (infile.is_open() && outfile.is_open()) {
            T* in_value;
            T out_value;
            size_t len_val = sizeof (T);
            char* data = new char[ len_val ];

            //skip header
            infile.read(data, header_size_);

            //Read/write data
            for (size_t i3 = 0; i3 < I3; ++i3) {
                for (size_t i1 = 0; i1 < I1; ++i1) {
                    k1 = i1 - (I1 - 1.0) / 2.0;
                    for (size_t i2 = 0; i2 < I2; ++i2) {
                        infile.read(data, len_val);
                        in_value = (T*)&(data[0]);
                        fill_value = static_cast<T> (*in_value);

                        //check if value is outside cylinder
                        k2 = i2 - (I2 - 1.0) / 2.0;
                        length = k1 * k1 + k2*k2;
                        if (length >= radius) {
                            fill_value = rand();
                            fill_value /= RAND_MAX;
                            fill_value *= sigma_;
                        }

                        out_value = static_cast<T> (fill_value);
                        outfile.write((char*) &(out_value), len_val);
                    }
                }
            }

            infile.close();
            outfile.close();
        } else {
            infile.close();
            outfile.close();
            std::cout << "no file open" << std::endl;
        }
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::export_to(const t3_t& input_, std::vector< T >& output_) {
        output_.clear();
        const_iterator it = input_.begin(),
        it_end = input_.end();
        for (; it != it_end; ++it) {
            output_.push_back(*it);
        }
    }
    
    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::import_from(const std::vector< T >& input_, t3_t& output_) {
        size_t i = 0; //iterator over data_
        size_t input_size = input_.size();

        iterator it = output_.begin(),
        it_end = output_.end();
        for (; it != it_end; ++it, ++i) {
            if (i >= input_size)
                *it = static_cast<T> (0);
            else
                *it = input_.at(i);
        }
    }
    
    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::remove_normals_from_raw(const t3_t& data_, const std::string& dir_, const std::string& filename_) {
        int dir_length = dir_.size() - 1;
        int last_separator = dir_.find_last_of("/");
        std::string path = dir_;
        if (last_separator < dir_length) {
            path.append("/");
        }
        path.append(filename_);


        size_t max_file_len = 2147483648u - sizeof (T);
        size_t len_data = sizeof (T) * data_.SIZE;
        size_t len_value = sizeof (T) * 4; //three normals per scalar value
        size_t len_read = 0;
        char* data = new char[ len_data ];
        std::ifstream infile;
        infile.open(path.c_str(), std::ios::in);

        if (infile.is_open()) {
            tensor3_iterator<T> it = data_.begin(),
                    it_end = data_.end();

            size_t counter = 0;
            while (len_data > 0) {
                len_read = (len_data % max_file_len) > 0 ? len_data % max_file_len : len_data;
                len_data -= len_read;
                infile.read(data, len_read);

                T* T_ptr = (T*)&(data[0]);
                for (; (it != it_end) && (len_read > 0); ++it, len_read -= len_value) {
                    ++T_ptr;
                    ++T_ptr;
                    ++T_ptr;
                    *it = *T_ptr;
                    ++T_ptr;
                    ++counter;
                }
            }

            delete[] data;
            infile.close();
        } else {
            std::cout << "no file open" << std::endl;
        }

        std::cout << "converted normals" << std::endl;

        std::string filename = "";
        filename = filename_.substr(0, filename_.size() - 6);


        write_datfile(".", filename);
        write_to_raw(".", filename);
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::read_from_raw(t3_t& data_, const std::string& dir_, const std::string& filename_) {
        int dir_length = dir_.size() - 1;
        int last_separator = dir_.find_last_of("/");
        std::string path = dir_;
        if (last_separator < dir_length) {
            path.append("/");
        }
        path.append(filename_);

        size_t max_file_len = 2147483648u - sizeof (T);
        size_t len_data = sizeof (T) * data_.SIZE;
        size_t len_read = 0;
        char* data = new char[ len_data ];
        std::ifstream infile;
        infile.open(path.c_str(), std::ios::in);

        if (infile.is_open()) {
            tensor3_iterator<t3_t> it = data_.begin(),
                    it_end = data_.end();

            while (len_data > 0) {
                len_read = (len_data % max_file_len) > 0 ? len_data % max_file_len : len_data;
                len_data -= len_read;
                infile.read(data, len_read);

                T* T_ptr = (T*)&(data[0]);
                for (; (it != it_end) && (len_read > 0); ++it, len_read -= sizeof (T)) {
                    *it = *T_ptr;
                    ++T_ptr;
                }
            }

            delete[] data;
            infile.close();
        } else {
            std::cout << "no file open" << std::endl;
        }

    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::write_to_raw(const t3_t& data_, const std::string& dir_, const std::string& filename_) {
        int dir_length = dir_.size() - 1;
        int last_separator = dir_.find_last_of("/");
        std::string path = dir_;
        if (last_separator < dir_length) {
            path.append("/");
        }
        path.append(filename_);
        //check for format
        if (filename_.find("raw", filename_.size() - 3) == std::string::npos) {
            path.append(".");
            path.append("raw");
        }
        std::string path_raw = path;

        std::ofstream outfile;
        outfile.open(path_raw.c_str());
        if (outfile.is_open()) {
            size_t len_slice = sizeof (T) * I1 * I2;
            for (size_t index = 0; index < I3; ++index) {
                outfile.write((char*) &(data_.get_frontal_slice_fwd(index)), len_slice);
            }
            outfile.close();
        } else {
            outfile.close();
            std::cout << "no file open" << std::endl;
        }
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::write_datfile(const std::string& dir_, const std::string& filename_) {
        int dir_length = dir_.size() - 1;
        int last_separator = dir_.find_last_of("/");
        std::string path = dir_;
        if (last_separator < dir_length) {
            path.append("/");
        }

        std::string filename = filename_;
        const size_t pos = filename_.size();
        if ((filename_.find(".raw", pos) + 4 == pos) ||
            (filename_.find(".dat", pos) + 4 == pos))
        {
            filename = filename_.substr(0, filename_.size() - 4);
        }
        path.append(filename);
        //check for format
        if (filename_.find("dat", filename_.size() - 3) == std::string::npos) {
            path.append(".");
            path.append("dat");
        }

        std::string path_dat = path;

        const char* format = (sizeof (T) == 2) ? "USHORT" : "UCHAR";

        FILE* datfile = fopen(path_dat.c_str(), "w");
        fprintf(datfile, "ObjectFileName:\t%s.raw\n", filename.c_str());
        fprintf(datfile, "TaggedFileName:\t---\nResolution:\t%i %i %i\n", int(I1), int(I2), int(I3));
        fprintf(datfile, "SliceThickness:\t1.0 1.0 1.0\n");
        fprintf(datfile, "Format:\t%s\nNbrTags:\t0\n", format);
        fprintf(datfile, "ObjectType:\tTEXTURE_VOLUME_OBJECT\nObjectModel:\tI\nGridType:\tEQUIDISTANT\n");
        fprintf(datfile, "Modality:\tunknown\nTimeStep:\t0\n");
        fclose(datfile);
    }

    VMML_TEMPLATE_STRING
    void
    VMML_TEMPLATE_CLASSNAME::write_to_csv(const t3_t& data_, const std::string& dir_, const std::string& filename_) {
        int dir_length = dir_.size() - 1;
        int last_separator = dir_.find_last_of("/");
        std::string path = dir_;
        if (last_separator < dir_length) {
            path.append("/");
        }
        path.append(filename_);
        //check for format
        if (filename_.find("csv", filename_.size() - 3) == std::string::npos)
        {
            path.append(".");
            path.append("csv");
        }

        std::ofstream outfile;
        outfile.open(path.c_str());
        if (outfile.is_open()) {

            for (size_t i = 0; i < I3; ++i) {
                outfile << data_.get_frontal_slice_fwd(i) << std::endl;
            }

            outfile.close();
        } else {
            std::cout << "no file open" << std::endl;
        }

    }

    VMML_TEMPLATE_STRING
    double
    VMML_TEMPLATE_CLASSNAME::rmse_from_files(const std::string& dir_,
const std::string& filename_a_, const std::string& filename_b_ ) {
        std::string path_a_raw = "";
        std::string path_b_raw = "";
        concat_path( dir_, filename_a_, path_a_raw );
        concat_path( dir_, filename_b_, path_b_raw );
		
        std::ifstream afile;
        afile.open( path_a_raw.c_str(), std::ios::in);
		
        std::ifstream bfile;
        bfile.open( path_b_raw.c_str(), std::ios::in);
		
		double mse_val = 0.0f;
		double mse_val_avg = 0.0f;
		double mse_val_i3 = 0.0f;
		double diff = 0.0f;
		
        if (afile.is_open() && bfile.is_open()) {
            T* a_value;
            T* b_value;
			double a_value_f;
			double b_value_f;
			
            size_t value_len = sizeof (T);
            char* data_a = new char[ value_len ];
            char* data_b = new char[ value_len ];
			
            for (size_t i3 = 0; i3 < I3; ++i3) {
				mse_val = 0.0f;
                for (size_t i1 = 0; i1 < I1; ++i1) {
                    for (size_t i2 = 0; i2 < I2; ++i2) {
                        afile.read( data_a, value_len);
                        a_value = (T*)&(data_a[0]);
                        a_value_f = static_cast<double> (*a_value);
						
						bfile.read( data_b, value_len);
                        b_value = (T*)&(data_b[0]);
                        b_value_f = static_cast<double> (*b_value);
						
						diff = fabs(b_value_f - a_value_f);
						diff *= diff;
						mse_val += diff;
					}
				}
				mse_val_avg = mse_val;
				mse_val_avg /= double(I1);
				mse_val_avg /= double(I2);
				mse_val_i3 += mse_val_avg;
				
            }
			
			
            afile.close();
            bfile.close();
        } else {
            afile.close();
            bfile.close();
            std::cout << "no file open" << std::endl;
        }
		
		
		mse_val_i3 /= double(I3);
		return sqrt(mse_val_i3);
    }


#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME


}//end vmml namespace

#endif
