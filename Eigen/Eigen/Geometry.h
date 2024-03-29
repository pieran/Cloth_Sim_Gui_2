#ifndef EIGEN_GEOMETRY_MODULE_H
#define EIGEN_GEOMETRY_MODULE_H

#include "Core.h"

#include "src/Core/util/DisableStupidWarnings.h"

#include "SVD.h"
#include "LU.h"
#include <limits>

/** \defgroup Geometry_Module Geometry module
  *
  *
  *
  * This module provides support for:
  *  - fixed-size homogeneous transformations
  *  - translation, scaling, 2D and 3D rotations
  *  - quaternions
  *  - \ref MatrixBase::cross() "cross product"
  *  - \ref MatrixBase::unitOrthogonal() "orthognal vector generation"
  *  - some linear components: parametrized-lines and hyperplanes
  *
  * \code
  * #include <Eigen/Geometry>
  * \endcode
  */

#include "src/Geometry/OrthoMethods.h"
#include "src/Geometry/EulerAngles.h"

#include "src/Geometry/Homogeneous.h"
#include "src/Geometry/RotationBase.h"
#include "src/Geometry/Rotation2D.h"
#include "src/Geometry/Quaternion.h"
#include "src/Geometry/AngleAxis.h"
#include "src/Geometry/Transform.h"
#include "src/Geometry/Translation.h"
#include "src/Geometry/Scaling.h"
#include "src/Geometry/Hyperplane.h"
#include "src/Geometry/ParametrizedLine.h"
#include "src/Geometry/AlignedBox.h"
#include "src/Geometry/Umeyama.h"

// Use the SSE optimized version whenever possible. At the moment the
// SSE version doesn't compile when AVX is enabled
#if defined EIGEN_VECTORIZE_SSE && !defined EIGEN_VECTORIZE_AVX
#include "src/Geometry/arch/Geometry_SSE.h"
#endif

#include "src/Core/util/ReenableStupidWarnings.h"

#endif // EIGEN_GEOMETRY_MODULE_H
/* vim: set filetype=cpp et sw=2 ts=2 ai: */

