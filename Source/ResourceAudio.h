#ifndef __RESOURCE_AUDIO_H__
#define __RESOURCE_AUDIO_H__

#include "Resource.h"

class ResourceAudio : public Resource
{
	friend class ModuleMeshes;

public:
	enum Format
	{
		sample,
		stream,
		unknown
	};

public:
	ResourceAudio(UID id);
	virtual ~ResourceAudio();

	Format GetFormat() const;
	const char* GetFormatStr() const;

public:
	ulong audio_id = 0;
	Format format = unknown;
};

#endif // __RESOURCE_AUDIO_H__