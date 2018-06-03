/* =================================================================================================

(c - GPLv3) T.W.J. de Geus (Tom) | tom@geus.me | www.geus.me | github.com/tdegeus/GooseEYE

================================================================================================= */

#ifndef GOOSEEYE_PRIVATE_H
#define GOOSEEYE_PRIVATE_H

// =================================================================================================

#include "GooseEYE.h"

// =================================================================================================

namespace GooseEYE {
namespace Private {

// =================================================================================================

std::vector<int> midpoint(const std::vector<size_t> &shape, size_t nd, const std::string &fname);

template<class T>
std::vector<size_t> shape(const cppmat::array<T> &A, size_t nd, const std::string &fname);

// =================================================================================================

}} // namespace ...

// =================================================================================================

#endif
