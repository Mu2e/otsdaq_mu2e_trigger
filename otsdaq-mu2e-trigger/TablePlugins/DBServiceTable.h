#ifndef _ots_DBServiceTable_h_
#define _ots_DBServiceTable_h_

#include <fstream>  // std::fstream
#include <iostream>
#include <string>
#include "otsdaq/ConfigurationInterface/ConfigurationManager.h"
#include "otsdaq/TableCore/TableBase.h"

namespace ots
{
// clang-format off
class DBServiceTable : public TableBase
{
  public:
	DBServiceTable(void);
	virtual ~DBServiceTable(void);

  protected:
	DBServiceTable(const std::string& tableName);

	// Methods
	void 				init							(ConfigurationManager* configManager);
	void 				createTriggerFcl				(std::ostream& fclFileOut, const ConfigurationManager* configManager) const;
	std::string	     	getFclValueForARTDAQ			(const ConfigurationManager* configManager, const std::string& field = "") const override;
	std::string         getStructureAsJSON		        (const ConfigurationManager* configManager) override;
};
// clang-format on
}  // namespace ots
#endif
