#ifndef EIGEN_METISSUPPORT_MODULE_H
#define EIGEN_METISSUPPORT_MODULE_H

#include "SparseCore.h"

#include "src/Core/util/DisableStupidWarnings.h"

extern "C" {
#include <metis.h>
}


/** \ingroup Support_modules
  * \defgroup MetisSupport_Module MetisSupport module
  *
  * \code
  * #include <Eigen/MetisSupport>
  * \endcode
  * This module defines an interface to the METIS reordering package (http://glaros.dtc.umn.edu/gkhome/views/metis). 
  * It can be used just as any other built-in method as explained in \link OrderingMethods_Module here. \endlink
  */


#include "src/MetisSupport/MetisSupport.h"

#include "src/Core/util/ReenableStupidWarnings.h"

#endif // EIGEN_METISSUPPORT_MODULE_H
