#include "ocudu/rohc/rohc_factory.h"
#ifdef ENABLE_ROHC_LIB
#include "rohc_lib/rohc_lib_compressor.h"
#include "rohc_lib/rohc_lib_decompressor.h"
#endif

using namespace ocudu;
using namespace ocudu::rohc;

std::unique_ptr<rohc_compressor> ocudu::rohc::create_rohc_compressor(const rohc_config& cfg)
{
#ifdef ENABLE_ROHC_LIB
  return std::make_unique<rohc_lib_compressor>(cfg);
#else
  return nullptr;
#endif
}

std::unique_ptr<rohc_decompressor> ocudu::rohc::create_rohc_decompressor(const rohc_config& cfg)
{
#ifdef ENABLE_ROHC_LIB
  return std::make_unique<rohc_lib_decompressor>(cfg);
#else
  return nullptr;
#endif
}
