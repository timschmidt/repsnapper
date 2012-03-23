#ifndef __VMML__QR_DECOMPOSITION_TEST__HPP__
#define __VMML__QR_DECOMPOSITION_TEST__HPP__

#include "unit_test.hpp"

namespace vmml
{

class qr_decomposition_test : public unit_test
{
public:
	qr_decomposition_test() : unit_test( "QR decomposition" ) {}

    bool run();
protected:

}; // class qr_decomposition_test

} // namespace vmml

#endif

