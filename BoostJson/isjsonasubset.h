#ifndef ISJSONASUBSET_H
#define ISJSONASUBSET_H

#include <boost/json/fwd.hpp>

/**
 * @brief recurse will check if subset is contained inside outer
 * @param outer
 * @param subset
 * @param basePath
 */
void isJsonASubset(boost::json::value const& outer, boost::json::value const& subset, const std::string& basePath = "");


#endif // ISJSONASUBSET_H
