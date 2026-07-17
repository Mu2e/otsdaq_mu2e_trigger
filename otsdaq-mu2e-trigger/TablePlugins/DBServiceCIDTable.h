#ifndef _ots_DBServiceCIDTable_h_
#define _ots_DBServiceCIDTable_h_

#include "otsdaq-mu2e-trigger/TablePlugins/DBServiceTable.h"

namespace ots
{
// clang-format off
class DBServiceCIDTable : public DBServiceTable
{
  public:
	DBServiceCIDTable(void);
	virtual ~DBServiceCIDTable(void);
};
// clang-format on
}  // namespace ots
#endif
