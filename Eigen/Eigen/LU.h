#ifndef EIGEN_LU_MODULE_H
#define EIGEN_LU_MODULE_H

#include "Core.h"

#include "src/Core/util/DisableStupidWarnings.h"

/** \defgroup LU_Module LU module
  * This module includes %LU decomposition and related notions such as matrix inversion and determinant.
  * This module defines the following MatrixBase methods:
  *  - MatrixBase::inverse()
  *  - MatrixBase::determinant()
  *
  * \code
  * #include <Eigen/LU>
  * \endcode
  */

#include "src/misc/Kernel.h"
#include "src/misc/Image.h"
#include "src/LU/FullPivLU.h"
#include "src/LU/PartialPivLU.h"
#ifdef EIGEN_USE_LAPACKE
#include "src/LU/PartialPivLU_MKL.h"
#endif
#include "src/LU/Determinant.h"
#include "src/LU/InverseImpl.h"

// Use the SSE optimized version whenever possible. At the moment the
// SSE version doesn't compile when AVX is enabled
#if defined EIGEN_VECTORIZE_SSE && !defined EIGEN_VECTORIZE_AVX
  #include "src/LU/arch/Inverse_SSE.h"
#endif

#include "src/Core/util/ReenableStupidWarnings.h"

#endif // EIGEN_LU_MODULE_H
/* vim: set filetype=cpp et sw=2 ts=2 ai: */
