#pragma once

#include <obs.hpp>

#include <string>

namespace xbotgo {

class SourceObserver {
	OBSWeakSource source;
	std::string id;

public:
	explicit SourceObserver(obs_source_t *source_) : source(OBSGetWeakRef(source_))
	{
		const char *sourceId = source_ ? obs_source_get_uuid(source_) : nullptr;
		if (sourceId) {
			id = sourceId;
		}
	}

	OBSSource Lock() const { return OBSGetStrongRef(source); }
	const std::string &Id() const { return id; }
};

} // namespace xbotgo
