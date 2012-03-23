/* 
 * VMMLib - Tensor Classes
 *  
 * @author Susanne Suter
 *
 * Import tool for Tucker3 tensor and quantized Tucker3 tensor
 * 
 */


#ifndef __VMML__TUCK3_IMPORTER__HPP__
#define __VMML__TUCK3_IMPORTER__HPP__

#include <vmmlib/qtucker3_tensor.hpp>

/* FIXME:
 *
 * - T_internal
 */


namespace vmml
{
	
	template< size_t R1, size_t R2, size_t R3, size_t I1, size_t I2, size_t I3, typename T_value = float, typename T_coeff = float >
	class tucker3_importer
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
		
		typedef matrix< R1, R2, T_coeff > front_core_slice_type; //fwd: forward cylcling (after kiers et al., 2000)

		typedef tensor3< R1, R2, R3, char > t3_core_signs_type;
		
		template< typename T >
		static void import_from( const std::vector< T >& data_, tucker3_type& tuck3_data_ );
		
		//previous version, but works only with 16bit quantization
		static void import_quantized_from( const std::vector<unsigned char>& data_in_, qtucker3_type& tuck3_data_  );
		
		//use this version, works with a better quantization for the core tensor:
		//logarithmic quantization and separate high energy core vale
		//suitable for voxelwise reconstruction
		static void import_hot_quantized_from( const std::vector<unsigned char>& data_in_, qtucker3_type& tuck3_data_  );
		
		//use this version for the ttm export/import (core: backward cyclic), without plain hot value 
		static void import_ttm_quantized_from( const std::vector<unsigned char>& data_in_, qtucker3_type& tuck3_data_  );
		
		
	}; //end tucker3 importer class
	
#define VMML_TEMPLATE_STRING        template< size_t R1, size_t R2, size_t R3, size_t I1, size_t I2, size_t I3, typename T_value, typename T_coeff >
#define VMML_TEMPLATE_CLASSNAME     tucker3_importer< R1, R2, R3, I1, I2, I3, T_value, T_coeff >
	
	
VMML_TEMPLATE_STRING
template< typename T >
void
VMML_TEMPLATE_CLASSNAME::import_from( const std::vector< T >& data_, tucker3_type& tuck3_data_  )
{
	size_t i = 0; //iterator over data_
	size_t data_size = (size_t) data_.size();
	
	if ( data_size != tuck3_data_.SIZE  )
		VMMLIB_ERROR( "import_from: the input data must have the size R1xR2xR3 + R1xI1 + R2xI2 + R3xI3 ", VMMLIB_HERE );
	
	u1_type* u1 = new u1_type;
	u2_type* u2 = new u2_type;
	u3_type* u3 = new u3_type;
	t3_core_type core;
	
	tuck3_data_.get_u1( *u1 );
	tuck3_data_.get_u2( *u2 );
	tuck3_data_.get_u3( *u3 );
	tuck3_data_.get_core( core );
	
	u1_iterator  it = u1->begin(),
	it_end = u1->end();
	for( ; it != it_end; ++it, ++i )
	{
		*it = static_cast< T >( data_.at(i));
	}
	
	
	u2_iterator  u2_it = u2->begin(),
	u2_it_end = u2->end();
	for( ; u2_it != u2_it_end; ++u2_it, ++i )
	{
		*u2_it = static_cast< T >( data_.at(i));
	}
	
	u3_iterator  u3_it = u3->begin(),
	u3_it_end = u3->end();
	for( ; u3_it != u3_it_end; ++u3_it, ++i )
	{
		*u3_it = static_cast< T >( data_.at(i));
	}
	
	t3_core_iterator  it_core = core.begin(),
	it_core_end = core.end();
	for( ; it_core != it_core_end; ++it_core, ++i )
	{
		*it_core = static_cast< T >( data_.at(i));
	}
	
	tuck3_data_.set_u1( *u1 );
	tuck3_data_.set_u2( *u2 );
	tuck3_data_.set_u3( *u3 );
	tuck3_data_.set_core( core );
	
	tuck3_data_.cast_comp_members();
	
	delete u1;
	delete u2;
	delete u3;
}
	

VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::import_quantized_from( const std::vector<unsigned char>& data_in_, qtucker3_type& tuck3_data_   )
{
	size_t end_data = 0;
	size_t len_t_comp = sizeof( T_internal );
	size_t len_export_data = tuck3_data_.SIZE * sizeof(T_coeff) + 8 * len_t_comp;
	unsigned char * data = new unsigned char[ len_export_data ]; 
	for( size_t byte = 0; byte < len_export_data; ++byte )
	{
		data[byte] = data_in_.at(byte);
	}
	
	//copy min and max values: u1_min, u1_max, u2_min, u2_max, u3_min, u3_max, core_min, core_max
	T_internal u_min = 0; T_internal u_max = 0;
	memcpy( &u_min, data, len_t_comp ); end_data = len_t_comp;
	memcpy( &u_max, data + end_data, len_t_comp ); end_data += len_t_comp;
	
	T_internal core_min = 0; T_internal core_max = 0;
	memcpy( &core_min, data + end_data, len_t_comp ); end_data += len_t_comp;
	memcpy( &core_max, data + end_data, len_t_comp ); end_data += len_t_comp;
	
	u1_type* u1 = new u1_type;
	u2_type* u2 = new u2_type;
	u3_type* u3 = new u3_type;
	t3_core_type core;
	
	tuck3_data_.get_u1( *u1 );
	tuck3_data_.get_u2( *u2 );
	tuck3_data_.get_u3( *u3 );
	tuck3_data_.get_core( core );

	//copy data to u1
	size_t len_u1 = I1 * R1 * sizeof( T_coeff );
	memcpy( *u1, data + end_data, len_u1 ); end_data += len_u1;
	
	//copy data to u2
	size_t len_u2 = I2 * R2 * sizeof( T_coeff );
	memcpy( *u2, data + end_data, len_u2 ); end_data += len_u2;
	
	//copy data to u3
	size_t len_u3 = I3 * R3 * sizeof( T_coeff );
	memcpy( *u3, data + end_data, len_u3 ); end_data += len_u3;
	
	//copy data to core
	size_t len_core_slice = R1 * R2 * sizeof( T_coeff );
	front_core_slice_type* slice = new front_core_slice_type();
	for (size_t r3 = 0; r3 < R3; ++r3 ) {
		memcpy( slice, data + end_data, len_core_slice );
		core.set_frontal_slice_fwd( r3, *slice );
		end_data += len_core_slice;
	}
	
	tuck3_data_.set_u1( *u1 );
	tuck3_data_.set_u2( *u2 );
	tuck3_data_.set_u3( *u3 );
	tuck3_data_.set_core( core );
	
	//dequantize tucker3 components (u1-u3 and core)
	tuck3_data_.dequantize_basis_matrices( u_min, u_max, u_min, u_max, u_min, u_max  );
	tuck3_data_.dequantize_core( core_min, core_max );	
	
	delete slice;
	delete[] data;
	delete u1;
	delete u2;
	delete u3;
}
	
	

VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::import_hot_quantized_from( const std::vector<unsigned char>& data_in_, qtucker3_type& tuck3_data_   )
{
	tuck3_data_.enable_quantify_hot();
	size_t end_data = 0;
	size_t len_t_comp = sizeof( T_internal );
	size_t len_export_data = R1*R2*R3 + (R1*I1 + R2*I2 + R3*I3) * sizeof(T_coeff) + 4 * len_t_comp;
	unsigned char * data = new unsigned char[ len_export_data ];
	for( size_t byte = 0; byte < len_export_data; ++byte )
	{
		data[byte] = data_in_.at(byte);
	}
	
	//copy min and max values: u1_min, u1_max, u2_min, u2_max, u3_min, u3_max, core_min, core_max
	T_internal u_min = 0; T_internal u_max = 0;
	memcpy( &u_min, data, len_t_comp ); end_data = len_t_comp;
	memcpy( &u_max, data + end_data, len_t_comp ); end_data += len_t_comp;
	
	T_internal core_min = 0; T_internal core_max = 0; //core_min is 0
	//memcpy( &core_min, data + end_data, len_t_comp ); end_data += len_t_comp;
	memcpy( &core_max, data + end_data, len_t_comp ); end_data += len_t_comp;
	//copy first value of core tensor separately as a float
	T_internal hottest_value = 0;
	memcpy( &hottest_value, data + end_data, len_t_comp ); end_data += len_t_comp;
	tuck3_data_.set_hottest_value( hottest_value );
	
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
	
	//copy data to u1
	size_t len_u1 = I1 * R1 * sizeof( T_coeff );
	memcpy( *u1, data + end_data, len_u1 ); end_data += len_u1;
	
	//copy data to u2
	size_t len_u2 = I2 * R2 * sizeof( T_coeff );
	memcpy( *u2, data + end_data, len_u2 ); end_data += len_u2;
	
	//copy data to u3
	size_t len_u3 = I3 * R3 * sizeof( T_coeff );
	memcpy( *u3, data + end_data, len_u3 ); end_data += len_u3;
	
	//copy data to core
	size_t len_core_el = 1; //currently 1 bit for sign and 7 bit for values
	
	unsigned char core_el;
	for (size_t r3 = 0; r3 < R3; ++r3 ) {
		for (size_t r2 = 0; r2 < R2; ++r2 ) {
			for (size_t r1 = 0; r1 < R1; ++r1 ) {
				memcpy( &core_el, data + end_data, len_core_el );
				signs.at( r1, r2, r3 ) = (core_el & 0x80)/128;
				core.at( r1, r2, r3 ) = core_el & 0x7f ;
				++end_data;
			}
		}
	} 
	
	tuck3_data_.set_u1( *u1 );
	tuck3_data_.set_u2( *u2 );
	tuck3_data_.set_u3( *u3 );
	tuck3_data_.set_core( core );
	tuck3_data_.set_core_signs( signs );

	//dequantize tucker3 components (u1-u3 and core)
	tuck3_data_.dequantize_basis_matrices( u_min, u_max, u_min, u_max, u_min, u_max  );
	tuck3_data_.dequantize_core( core_min, core_max );	

	delete[] data;
	delete u1;
	delete u2;
	delete u3;
}
	
	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::import_ttm_quantized_from( const std::vector<unsigned char>& data_in_, qtucker3_type& tuck3_data_   )
{
	tuck3_data_.enable_quantify_log();
	size_t end_data = 0;
	size_t len_t_comp = sizeof( T_internal );
	size_t len_export_data = R1*R2*R3 + (R1*I1 + R2*I2 + R3*I3) * sizeof(T_coeff) + 3 * len_t_comp;
	unsigned char * data = new unsigned char[ len_export_data ];
	for( size_t byte = 0; byte < len_export_data; ++byte )
	{
		data[byte] = data_in_.at(byte);
	}
	
	//copy min and max values: u1_min, u1_max, u2_min, u2_max, u3_min, u3_max, core_min, core_max
	T_internal u_min = 0; T_internal u_max = 0;
	memcpy( &u_min, data, len_t_comp ); end_data = len_t_comp;
	memcpy( &u_max, data + end_data, len_t_comp ); end_data += len_t_comp;
	
	T_internal core_min = 0; T_internal core_max = 0; //core_min is 0
	//memcpy( &core_min, data + end_data, len_t_comp ); end_data += len_t_comp;
	memcpy( &core_max, data + end_data, len_t_comp ); end_data += len_t_comp;
	
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
	
	//copy data to u1
	size_t len_u1 = I1 * R1 * sizeof( T_coeff );
	memcpy( *u1, data + end_data, len_u1 ); end_data += len_u1;
	
	//copy data to u2
	size_t len_u2 = I2 * R2 * sizeof( T_coeff );
	memcpy( *u2, data + end_data, len_u2 ); end_data += len_u2;
	
	//copy data to u3
	size_t len_u3 = I3 * R3 * sizeof( T_coeff );
	memcpy( *u3, data + end_data, len_u3 ); end_data += len_u3;
	
	//copy data to core
	size_t len_core_el = 1; //currently 1 bit for sign and 7 bit for values
	
	//backward cyclic after lathauwer et al. 
	unsigned char core_el;
	for (size_t r2 = 0; r2 < R2; ++r2 ) {
		for (size_t r3 = 0; r3 < R3; ++r3 ) {
			for (size_t r1 = 0; r1 < R1; ++r1 ) {
				memcpy( &core_el, data + end_data, len_core_el );
				signs.at( r1, r2, r3 ) = (core_el & 0x80)/128;
				core.at( r1, r2, r3 ) = core_el & 0x7f ;
				++end_data;
			}
		}
	} 
	//std::cout << "signs: " << _signs << std::endl;
	//std::cout << "_core: " << _core << std::endl;
	
	delete[] data;
	
	tuck3_data_.set_u1( *u1 );
	tuck3_data_.set_u2( *u2 );
	tuck3_data_.set_u3( *u3 );
	tuck3_data_.set_core( core );
	tuck3_data_.set_core_signs( signs );

	//dequantize tucker3 components (u1-u3 and core)
	tuck3_data_.dequantize_basis_matrices( u_min, u_max, u_min, u_max, u_min, u_max  );
	tuck3_data_.dequantize_core( core_min, core_max );	

	delete u1;
	delete u2;
	delete u3;	
}
	
#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME

	
} // namespace vmml

	
	
#endif

