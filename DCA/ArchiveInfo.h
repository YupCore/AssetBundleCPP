#pragma once
#include "AssetBundle.h"
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/json.hpp>

namespace cereal
{
	//! Serializing for std::pair
	template <class Archive, class T1, class T2> inline
		void CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar, std::pair<T1, T2>& pair)
	{
		ar(CEREAL_NVP_("first", pair.first),
			CEREAL_NVP_("second", pair.second));
	}
} // namespace cereal

struct ArchiveInfo
{
public:
	std::string bundleName;
	unsigned int headerSize = 0;
	std::map<std::string, std::pair<unsigned long long, unsigned long long>> fileMap; // the first one is starting point, the second one is size

	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(CEREAL_NVP_("archiveName", bundleName), CEREAL_NVP_("fileMap", fileMap), CEREAL_NVP_("headerSize", headerSize));
	}
};