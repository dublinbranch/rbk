#include "sqlrowv2.h"

SqlResultV2::SqlResultV2() {
	columns = std::make_shared<SqlResV2::TypeMap>();
}
