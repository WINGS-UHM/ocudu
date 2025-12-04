#include "ocudu/rohc/rohc_support.h"

using namespace ocudu;
using namespace ocudu::rohc;

bool ocudu::rohc::rohc_supported()
{
#ifdef ENABLE_ROHC_LIB
  return true;
#else
  return false;
#endif
}
