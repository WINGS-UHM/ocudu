#include "rohc_lib_engine.h"
#include "rohc/rohc.h"
#include "ocudu/ocudulog/ocudulog.h"

using namespace ocudu;
using namespace ocudu::rohc;

rohc_lib_engine::rohc_lib_engine() : logger(ocudulog::fetch_basic_logger("ROHC"))
{
  logger.info("Created ROHC engine with ROHC library version {}", rohc_version());
}
