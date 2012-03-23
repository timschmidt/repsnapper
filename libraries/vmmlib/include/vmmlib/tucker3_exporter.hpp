/* 
 * VMMLib - Tensor Classes
 *  
 * @author Susanne Suter
 *
 * Export tool for Tucker3 tensor and quantized Tucker3 tensor
 * 
 */


#ifndef __VMML__TUCK3_EXPORTER__HPP__
#define __VMML__TUCK3_EXPORTER__HPP__

#include <vmmlib/qtucker3_tensor.hpp>


/* FIXME:
 *
 * - T_internal
 * - const input argument for tucker3 data
 */

namespace vmml
{
	
	template< size_t R1, size_t R2, size_t R3, size_t I1, size_t I2, size_t I3, typename T_value = float, typename T_coeff = float >
	class tucker3_exporter
	{
	public:  
		
		typedef float T_internal; //FIXME! should match with tucker3 tensor
				
		typedef tucker3_tensor< R1, R2, R3, I1, I2, I3, T_value, T_coeff > tucker3_type;
		typedef qtucker3_tensor< R1, R2, R3, I1, I2, I3, T_value, T_coeff > qtucker3_type;
		
		typedef tensor3< R1, R2, R3, T_coeff > t3_core_type;
		typedef typename t3_core_type::iterator t3_core_iterator;
		typedef typename t3_core_type::const_iterator t3_core_const_iterator;
				
		typedef matrix< I1, R1, T_coeff > u1_type;
		typedef typename u1_type::iterator u1_iterator;
		typedef typename u1_type::const_iterator u1_const_iterator;
		
		typedef matrix< I2, R2, T_coeff > u2_type;
		typedef typename u2_type::iterator u2_iterator;
		typedef typename u2_type::const_iterator u2_const_iterator;
		
		typedef matrix< I3, R3, T_coeff > u3_type;
		typedef typename u3_type::iterator u3_iterator;
		typedef typename u3_type::const_iterator u3_const_iterator;
		
		typedef tensor3< R1, R2, R3, char > t3_core_signs_type;
		
		template< typename T >
		static void export_to( std::vector< T >& data_, tucker3_type& tuck3_data_ );
		
		//previous version, but works only with 16bit quantization
		static void export_quantized_to(  std::vector<unsigned char>& data_out_, qtucker3_type& tuck3_data_   );
		
		//use this version, works with a better quantization for the core tensor:
		//logarithmic quantization and separate high energy core vale
		//suitable for voxelwise reconstruction
		static void export_hot_quantized_to(  std::vector<unsigned char>& data_out_, qtucker3_type& tuck3_data_   );
		
		//use this version for the ttm export/import (core: backward cyclic), without plain hot value 
		static void export_ttm_quantized_to(  std::vector<unsigned char>& data_out_, qtucker3_type& tuck3_data_   );
		
	
	}; //end tucker3 exporter class
	
#define VMML_TEMPLATE_STRING        template< size_t R1, size_t R2, size_t R3, size_t I1, size_t I2, size_t I3, typename T_value, typename T_coeff >
#define VMML_TEMPLATE_CLASSNAME     tucker3_exporter< R1, R2, R3, I1, I2, I3, T_value, T_coeff >

	
VMML_TEMPLATE_STRING
template< typename T >
void
VMML_TEMPLATE_CLASSNAME::export_to( std::vector< T >& data_, tucker3_type& tuck3_data_  )
{
	
	data_.clear();
	
	u1_type* u1 = new u1_type;
	u2_type* u2 = new u2_type;
	u3_type* u3 = new u3_type;
	t3_core_type core;
	
	tuck3_data_.get_u1( *u1 );
	tuck3_data_.get_u2( *u2 );
	tuck3_data_.get_u3( *u3 );
	tuck3_data_.get_core( core );
	
	tuck3_data_.cast_members();
	
	u1_const_iterator  it = u1->begin(),
	it_end = u1->end();
	for( ; it != it_end; ++it )
	{
		data_.push_back( static_cast< T >( *it) );
	}
	
	u2_const_iterator  u2_it = u2->begin(),
	u2_it_end = u2->end();
	for( ; u2_it != u2_it_end; ++u2_it )
	{
		data_.push_back(static_cast< T >(*u2_it) );
	}
	
	u3_const_iterator  u3_it = u3->begin(),
	u3_it_end = u3->end();
	for( ; u3_it != u3_it_end; ++u3_it )
	{
		data_.push_back(static_cast< T >( *u3_it) );
	}
	
	t3_core_iterator  it_core = core.begin(),
	it_core_end = core.end();
	for( ; it_core != it_core_end; ++it_core )
	{
		data_.push_back(static_cast< T >( *it_core) );
	}
	
	delete u1;
	delete u2;
	delete u3;
}
	
	
	
	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::export_quantized_to( std::vector<unsigned char>& data_out_, qtucker3_type& tuck3_data_  )
{
	//quantize tucker3 components (u1-u3 and core)
	size_t len_t_comp = sizeof( T_internal );
	size_t len_export_data = tuck3_data_.SIZE * sizeof(T_coeff) + 8 * len_t_comp;
	char * data = new char[ len_export_data ];
	size_t end_data = 0;
	
	//quantize basis matrices and copy min-max values
	T_internal u_min, u_max;
	tuck3_data_.quantize_basis_matrices( u_min, u_max);
	memcpy( data, &u_min, len_t_comp ); end_data = len_t_comp;
	memcpy( data + end_data, &u_max, len_t_comp ); end_data += len_t_comp;
	
	u1_type* u1 = new u1_type;
	u2_type* u2 = new u2_type;
	u3_type* u3 = new u3_type;
	t3_core_type core;
	
	tuck3_data_.get_u1( *u1 );
	tuck3_data_.get_u2( *u2 );
	tuck3_data_.get_u3( *u3 );
	tuck3_data_.get_core( core );
	
	//quantize core and copy min-max values
	T_internal core_min, core_max;
	tuck3_data_.quantize_core( core_min, core_max );		
	memcpy( data + end_data, &core_min, len_t_comp ); end_data += len_t_comp;
	memcpy( data + end_data, &core_max, len_t_comp ); end_data += len_t_comp;
	
	//copy data for u1
	size_t len_u1 = I1 * R1 * sizeof( T_coeff );
	memcpy( data + end_data, *u1, len_u1 ); end_data += len_u1;
	
	//copy data for u2
	size_t len_u2 = I2 * R2 * sizeof( T_coeff );
	memcpy( data + end_data, *u2, len_u2 ); end_data += len_u2;
	
	//copy data for u3
	size_t len_u3 = I3 * R3 * sizeof( T_coeff );
	memcpy( data + end_data, *u3, len_u3 ); end_data += len_u3;
	
	//copy data for core
	size_t len_core_slice = R1 * R2 * sizeof( T_coeff );
	for (size_t r3 = 0; r3 < R3; ++r3 ) {
		memcpy( data + end_data, core.get_frontal_slice_fwd( r3 ), len_core_slice );
		end_data += len_core_slice;
	}
	
	data_out_.clear();
	for( size_t byte = 0; byte < len_export_data; ++byte )
	{
		data_out_.push_back( data[byte] );
	}
	delete[] data;
	
	delete u1;
	delete u2;
	delete u3;
}
	
	
	
	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::export_hot_quantized_to( std::vector<unsigned char>& data_out_, qtucker3_type& tuck3_data_  )
{
	tuck3_data_.enable_quantify_hot();
	//quantize tucker3 components (u1-u3 and core)
	size_t len_t_comp = sizeof( T_internal );
	size_t len_export_data = R1*R2*R3 + (R1*I1 + R2*I2 + R3*I3) * sizeof(T_coeff) + 4 * len_t_comp;
	char * data = new char[ len_export_data ];
	size_t end_data = 0;
	
	//quantize basis matrices and copy min-max values
	T_internal u_min, u_max;
	tuck3_data_.quantize_basis_matrices( u_min, u_max);
	memcpy( data, &u_min, len_t_comp ); end_data = len_t_comp;
	memcpy( data + end_data, &u_max, len_t_comp ); end_data += len_t_comp;
	
	//quantize core and copy min-max values
	T_internal core_min, core_max;
	tuck3_data_.quantize_core( core_min, core_max );		
	//memcpy( data + end_data, &core_min, len_t_comp ); end_data += len_t_comp; min_value is always zero in log quant
	memcpy( data + end_data, &core_max, len_t_comp ); end_data += len_t_comp;
	
	u1_type* u1 = new u1_type;
	u2_type* u2 = new u2_type;
	u3_type* u3 = new u3_type;
	t3_core_type core;
	t3_core_signs_type signs;
	
	tuck3_data_.get_u1( *u1 );
	tuck3_data_.get_u2( *u2 );
	tuck3_data_.get_u3( *u3 );
	tuck3_data_.get_core( core );
	tuck3_data_.get_core_signs( signs );
	T_internal hottest_value = tuck3_data_.get_hottest_value();

	//copy first value of core tensor separately as a float
	memcpy( data + end_data, &hottest_value, len_t_comp ); end_data += len_t_comp;
	
	//copy data for u1
	size_t len_u1 = I1 * R1 * sizeof( T_coeff );
	memcpy( data + end_data, *u1, len_u1 ); end_data += len_u1;
	
	//copy data for u2
	size_t len_u2 = I2 * R2 * sizeof( T_coeff );
	memcpy( data + end_data, *u2, len_u2 ); end_data += len_u2;
	
	//copy data for u3
	size_t len_u3 = I3 * R3 * sizeof( T_coeff );
	memcpy( data + end_data, *u3, len_u3 ); end_data += len_u3;
	
	//copy data for core
	size_t len_core_el = 1; //currently 1 bit for sign and 7 bit for values
	
	//colume-first iteration
	unsigned char core_el;
	for (size_t r3 = 0; r3 < R3; ++r3 ) {
		for (size_t r2 = 0; r2 < R2; ++r2 ) {
			for (size_t r1 = 0; r1 < R1; ++r1 ) {
				core_el = (core.at( r1, r2, r3 ) | (signs.at( r1, r2, r3) * 0x80 ));
				/*std::cout << "value: " << int(_core.at( r1, r2, r3 )) << " bit " << int( core_el ) 
				 << " sign: " << int(_signs.at( r1, r2, r3)) << std::endl;*/
				memcpy( data + end_data, &core_el, len_core_el );
				++end_data;
			}
		}
	} 
	
	data_out_.clear();
	for( size_t byte = 0; byte < len_export_data; ++byte )
	{
		data_out_.push_back( data[byte] );
	}
	
	delete[] data;
	delete u1;
	delete u2;
	delete u3;
}
	
	
	
	
	
	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::export_ttm_quantized_to( std::vector<unsigned char>& data_out_, qtucker3_type& tuck3_data_  )
{
	tuck3_data_.enable_quantify_log();
	//quantize tucker3 components (u1-u3 and core)
	size_t len_t_comp = sizeof( T_internal );
	size_t len_export_data = R1*R2*R3 + (R1*I1 + R2*I2 + R3*I3) * sizeof(T_coeff) + 3 *len_t_comp;
	char * data = new char[ len_export_data ];
	size_t end_data = 0;
	
	//quantize basis matrices and copy min-max values
	T_internal u_min, u_max;
	tuck3_data_.quantize_basis_matrices( u_min, u_max);
	memcpy( data, &u_min, len_t_comp ); end_data = len_t_comp;
	memcpy( data + end_data, &u_max, len_t_comp ); end_data += len_t_comp;
	
	//quantize core and copy min-max values
	T_internal core_min, core_max;
	tuck3_data_.quantize_core( core_min, core_max );		
	//memcpy( data + end_data, &core_min, len_t_comp ); end_data += len_t_comp; min_value is always zero in log quant
	memcpy( data + end_data, &core_max, len_t_comp ); end_data += len_t_comp;
	
	u1_type* u1 = new u1_type;
	u2_type* u2 = new u2_type;
	u3_type* u3 = new u3_type;
	t3_core_type core;
	t3_core_signs_type signs;
	
	tuck3_data_.get_u1( *u1 );
	tuck3_data_.get_u2( *u2 );
	tuck3_data_.get_u3( *u3 );
	tuck3_data_.get_core( core );
	tuck3_data_.get_core_signs( signs );

	//copy data for u1
	size_t len_u1 = I1 * R1 * sizeof( T_coeff );
	memcpy( data + end_data, *u1, len_u1 ); end_data += len_u1;
	
	//copy data for u2
	size_t len_u2 = I2 * R2 * sizeof( T_coeff );
	memcpy( data + end_data, *u2, len_u2 ); end_data += len_u2;
	
	//copy data for u3
	size_t len_u3 = I3 * R3 * sizeof( T_coeff );
	memcpy( data + end_data, *u3, len_u3 ); end_data += len_u3;
	
	//copy data for core
	size_t len_core_el = 1; //currently 1 bit for sign and 7 bit for values
	
	//colume-first iteration
	//backward cylcling after lathauwer et al. 
	unsigned char core_el;
	for (size_t r2 = 0; r2 < R2; ++r2 ) {
		for (size_t r3 = 0; r3 < R3; ++r3 ) {
			for (size_t r1 = 0; r1 < R1; ++r1 ) {
				core_el = (core.at( r1, r2, r3 ) | (signs.at( r1, r2, r3) * 0x80 ));
				/*std::cout << "value: " << int(_core.at( r1, r2, r3 )) << " bit " << int( core_el ) 
				 << " sign: " << int(_signs.at( r1, r2, r3)) << std::endl;*/
				memcpy( data + end_data, &core_el, len_core_el );
				++end_data;
			}
		}
	} 
	
	data_out_.clear();
	for( size_t byte = 0; byte < len_export_data; ++byte )
	{
		data_out_.push_back( data[byte] );
	}
	
	delete[] data;
	delete u1;
	delete u2;
	delete u3;
}
	
#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME

	
} // namespace vmml


#endif

