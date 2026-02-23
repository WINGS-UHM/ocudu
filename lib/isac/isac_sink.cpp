#include "ocudu/isac/isac_sink.h"

namespace ocudu {
isac_sink& get_isac_sink()
{
  static isac_sink s;
  return s;
}
} // namespace ocudu