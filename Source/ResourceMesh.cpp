#include "ResourceMesh.h"
#include "Application.h"
#include "ModuleMeshes.h"

// ---------------------------------------------------------
ResourceMesh::ResourceMesh(UID uid) : Resource(uid, Resource::Type::mesh)
{} 

// ---------------------------------------------------------
ResourceMesh::~ResourceMesh()
{
	// TODO: deallocate opengl buffers
	RELEASE_ARRAY(indices);
	RELEASE_ARRAY(vertices);
	RELEASE_ARRAY(colors);
	RELEASE_ARRAY(normals);
	RELEASE_ARRAY(texture_coords);
}

// ---------------------------------------------------------
bool ResourceMesh::LoadInMemory()
{
	return App->meshes->Load(this);
}

// ---------------------------------------------------------
void ResourceMesh::Save(Config & config) const
{
	Resource::Save(config);
}

// ---------------------------------------------------------
void ResourceMesh::Load(const Config & config)
{
	Resource::Load(config);
}

// ---------------------------------------------------------
void ResourceMesh::CreateDeformableVersion(const ResourceMesh * original)
{
	if (original != nullptr)
	{
		num_vertices = original->num_vertices;
		vertices = new float[num_vertices * 3];
		indices = new uint[original->num_indices]; // will use it to track number of bones per index

		if (original->normals != nullptr)
			normals = new float[num_vertices * 3];
	}
}
