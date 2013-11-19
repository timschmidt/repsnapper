/*
 * VMMLib - Tensor Classes
 *
 * @author David Klaper
 * @author Susanne Suter
 *
 * class to read/write and convert tensor4 files (from raw, to csv...)
 *
 */

#ifndef __VMML__T4_CONVERTER__HPP__
#define __VMML__T4_CONVERTER__HPP__

#include "tensor4.hpp"

namespace vmml
{

	template< size_t I1, size_t I2, size_t I3, size_t I4, typename T = float >
	class t4_converter
	{

	public:


		typedef tensor4< I1, I2, I3, I4, T > t4_t;

		static void export_to( std::vector< T >& data_ ) ;
		static void import_from( const std::vector< T >& data_ ) ;

		static void write_to_raw( const t4_t& data_, const std::string& dir_, const std::string& filename_ ); //TODO: DK done
		static void read_from_raw( t4_t& data_, const std::string& dir_, const std::string& filename_ ) ; //TODO: DK done

		static void write_datfile( const std::string& dir_, const std::string& filename_ );
		static void write_to_csv( const t4_t& data_, const std::string& dir_, const std::string& filename_ ); //TODO: DK done


	protected:

		static void concat_path( const std::string& dir_, const std::string& filename_, std::string& path_ );

	}; //end t4_converter



#define VMML_TEMPLATE_STRING        template< size_t I1, size_t I2, size_t I3, size_t I4, typename T >
#define VMML_TEMPLATE_CLASSNAME     t4_converter< I1, I2, I3, I4, T >


	VMML_TEMPLATE_STRING
	void
	VMML_TEMPLATE_CLASSNAME::concat_path( const std::string& dir_, const std::string& filename_, std::string& path_ )
	{
		int dir_length = dir_.size() -1;
		int last_separator = dir_.find_last_of( "/");
		path_ = dir_;
		if (last_separator < dir_length ) {
			path_.append( "/" );
		}
		path_.append( filename_ );

		//check for format
		if( filename_.find( "raw", filename_.size()-3 ) == std::string::npos)
        {
			path_.append( ".");
			path_.append( "raw" );
		}
	}


	VMML_TEMPLATE_STRING
	void
	VMML_TEMPLATE_CLASSNAME::write_to_csv( const t4_t& data_, const std::string& dir_, const std::string& filename_ )
	{
		std::string path_raw;
		concat_path(dir_, filename_, path_raw);
		path_raw.replace(path_raw.size() - 3, 3, "csv");

		std::ofstream outfile(path_raw.c_str());
		if(outfile.is_open())
		{
			for( size_t i4 = 0; i4 < data_.T3S; ++i4 )
			{
				for( size_t i3 = 0; i3 < data_.SLICES; ++i3 )
				{
					for( size_t i1 = 0; i1 < data_.ROWS; ++i1 )
					{
						for( size_t i2 = 0; i2 < data_.COLS; ++i2 )
						{
							outfile << data_(i1,i2,i3,i4) << ", ";
						}
						outfile.seekp( -2, std::ios_base::cur); // remove superfluous comma signs
						outfile << std::endl;
					}
					outfile << std::endl;
				}
				outfile << std::endl;
			}

			outfile.close();
		}else {
			outfile.close();
			std::cout << "no file open" << std::endl;
		}

	}


	VMML_TEMPLATE_STRING
	void
	VMML_TEMPLATE_CLASSNAME::read_from_raw( t4_t& data_, const std::string& dir_, const std::string& filename_ )
	{
		data_.zero();
		std::string path_raw;
		concat_path(dir_, filename_, path_raw);
		std::ifstream infile;
		infile.open( path_raw.c_str(), std::ios::in );
		if( infile.is_open() ) {

			size_t max_file_len = (std::numeric_limits<std::streamsize>::max)() - sizeof(T);
			size_t len_data = data_.size();
			size_t len_read = 0;
			T* dataptr = data_.get_array_ptr();
			char* chardat = new char[ len_data*sizeof(T)];
			size_t offset = 0;

			while(len_data > 0)
			{

				if (len_data*sizeof(T) > max_file_len)
				{
					size_t mod = max_file_len % sizeof(T);
					len_read = (max_file_len-mod)/sizeof(T);
				}else
				{
					len_read = len_data;
				}

				infile.read( chardat, len_read*sizeof(T) );

				for(size_t index = 0; index+offset < data_.size() && index < len_read; ++index)
				{
					T* data = (T*)&(chardat[index*sizeof(T)]);
					dataptr[index+offset] = *data;
				}
				offset += len_read;
				len_data -= len_read;

			}
			delete[] chardat;
			infile.close();
		} else {
			infile.close();
			std::cout << "no file open" << std::endl;
		}

	}




	VMML_TEMPLATE_STRING
	void
	VMML_TEMPLATE_CLASSNAME::write_to_raw( const t4_t& data_, const std::string& dir_, const std::string& filename_ )
	{
		std::string path_raw;
		concat_path(dir_, filename_, path_raw);

		std::ofstream outfile;
		outfile.open( path_raw.c_str() );
		if( outfile.is_open() ) {
			size_t type_size = sizeof(T);
			size_t chunk_size = 5; // How many data values are written at once
			const T* dataptr = data_.get_array_ptr();
			for( size_t index = 0; index < data_.size(); index += chunk_size )
			{
				if(chunk_size + index > data_.size()) // only write remaining values
				{
					outfile.write( (char*)dataptr+index*type_size, type_size*(data_.size()-index) );
				}else // write whole chunk
				{
					outfile.write( (char*)dataptr+index*type_size, chunk_size*type_size );
				}
			}
			outfile.close();
		} else {
			outfile.close();
			std::cout << "no file open" << std::endl;
		}
	}




#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME


}//end vmml namespace

#endif
