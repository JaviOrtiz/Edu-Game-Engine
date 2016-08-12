#include "Globals.h"
#include "Application.h"
#include "ModuleResources.h"
#include "ModuleFileSystem.h"
#include "ModuleTextures.h"
#include "ModuleMeshes.h"
#include "ModuleAudio.h"
#include "ModuleSceneLoader.h"
#include "Event.h"
#include "ResourceTexture.h"
#include "ResourceMesh.h"
#include "ResourceAudio.h"
#include <string>

#define LAST_UID_FILE "LAST_UID"

using namespace std;

ModuleResources::ModuleResources(bool start_enabled) : Module("Resource Manager", start_enabled), asset_folder(ASSETS_FOLDER)
{
	// Exception: we are going to acces file system to obtain the last used unique id
	// so others modules can use resources starting from Init()
}

// Destructor
ModuleResources::~ModuleResources()
{
}

// Called before render is available
bool ModuleResources::Init(Config* config)
{
	LOG("Loading Resource Manager");
	bool ret = true;

	LoadUID();

	return ret;
}

// Called before quitting
bool ModuleResources::CleanUp()
{
	LOG("Unloading Resource Manager");

	for (map<UID, Resource*>::iterator it = resources.begin(); it != resources.end(); ++it)
		RELEASE(it->second);

	resources.clear();

	return true;
}

void ModuleResources::ReceiveEvent(const Event& event)
{
	switch (event.type)
	{
		case Event::file_dropped:
			LOG("File dropped: %s", event.string.ptr);
			ImportFileOutsideVFM(event.string.ptr);
		break;
	}
}

Resource::Type ModuleResources::TypeFromExtension(const char * extension) const
{
	Resource::Type ret = Resource::unknown;

	if (extension != nullptr)
	{
		if (_stricmp(extension, "wav") == 0)
			ret = Resource::audio;
		else if (_stricmp(extension, "ogg") == 0)
			ret = Resource::audio;
		else if (_stricmp(extension, "dds") == 0)
			ret = Resource::texture;
		else if (_stricmp(extension, "png") == 0)
			ret = Resource::texture;
		else if (_stricmp(extension, "jpg") == 0)
			ret = Resource::texture;
		else if (_stricmp(extension, "tga") == 0)
			ret = Resource::texture;
		else if (_stricmp(extension, "fbx") == 0)
			ret = Resource::scene;
		else if (_stricmp(extension, "dae") == 0)
			ret = Resource::scene;
	}

	return ret;
}

UID ModuleResources::Find(const char * file_in_assets) const
{
	string file(file_in_assets);
	App->fs->NormalizePath(file);

	for (map<UID, Resource*>::const_iterator it = resources.begin(); it != resources.end(); ++it)
	{
		if (it->second->file.compare(file) == 0)
			return it->first;
	}
	return 0;
}

UID ModuleResources::ImportFileOutsideVFM(const char * full_path)
{
	UID ret = 0;

	string final_path;

	App->fs->SplitFilePath(full_path, nullptr, &final_path);
	final_path = asset_folder + final_path;

	if(App->fs->CopyFromOutsideFS(full_path, final_path.c_str()) == true)
		ret = ImportFile(final_path.c_str());

	return ret;
}

UID ModuleResources::ImportFile(const char * new_file_in_assets)
{
	// Check is that file has been already exported
	UID ret = Find(new_file_in_assets);

	if (ret != 0)
		return ret;

	// Find out the type from the extension and send to the correct exporter
	string extension;
	App->fs->SplitFilePath(new_file_in_assets, nullptr, nullptr, &extension);

	Resource::Type type = TypeFromExtension(extension.c_str());

	bool import_ok = false;
	string written_file;

	switch (type)
	{
		case Resource::texture:
			import_ok = App->tex->Import(new_file_in_assets, "", written_file);
		break;
		case Resource::audio:
			import_ok = App->audio->Import(new_file_in_assets, written_file);
		break;

		default:
		break;
	}

	// If export was successfull, create a new resource
	if (import_ok == true)
	{
		Resource* res = CreateNewResource(type);
		res->file = new_file_in_assets;
		App->fs->NormalizePath(res->file);
		string file;
		App->fs->SplitFilePath(written_file.c_str(), nullptr, &file);
		res->exported_file = file.c_str();
		ret = res->uid;
	}

	return ret;
}

UID ModuleResources::ImportBuffer(const void * buffer, uint size, Resource::Type type, const char* source_file)
{
	UID ret = 0;

	bool import_ok = false;
	string output;

	switch (type)
	{
		case Resource::texture:
			import_ok = App->tex->Import(buffer, size, output);
		break;
		case Resource::mesh:
			// Old school trick: if it is a Mesh, buffer will be treated as an AiMesh*
			// TODO: this can go bad in so many ways :)
			import_ok = App->meshes->Import((aiMesh*) buffer, output);
		break;
	}

	// If export was successfull, create a new resource
	if (import_ok  == true)
	{
		Resource* res = CreateNewResource(type);
		if (source_file != nullptr) {
			res->file = source_file;
			App->fs->NormalizePath(res->file);
		}
		string file;
		App->fs->SplitFilePath(output.c_str(), nullptr, &file);
		res->exported_file = file;
		ret = res->uid;
	}

	return ret;
}

UID ModuleResources::GenerateNewUID()
{
	++last_uid;
	SaveUID();
	return last_uid;
}

const Resource * ModuleResources::Get(UID uid) const
{			   
	if(resources.find(uid) != resources.end())
		return resources.at(uid);
	return nullptr;
}

Resource * ModuleResources::CreateNewResource(Resource::Type type)
{
	Resource* ret = nullptr;
	switch (type)
	{
		case Resource::texture:
			ret = (Resource*) new ResourceTexture(GenerateNewUID());
		break;
		case Resource::mesh:
			ret = (Resource*) new ResourceMesh(GenerateNewUID());
		break;
		case Resource::audio:
			ret = (Resource*) new ResourceAudio(GenerateNewUID());
		break;
	}

	if(ret != nullptr)
		resources[ret->uid] = ret;

	return ret;
}

void ModuleResources::GatherResourceType(std::vector<const Resource*>& resources, Resource::Type type) const
{
	for (map<UID, Resource*>::const_iterator it = this->resources.begin(); it != this->resources.end(); ++it)
	{
		if (it->second->type == type)
			resources.push_back(it->second);
	}
}

void ModuleResources::LoadUID()
{
	string file(SETTINGS_FOLDER);
	file += LAST_UID_FILE;

	char *buf = nullptr;
	uint size = App->fs->Load(file.c_str(), &buf);

	if (size == sizeof(last_uid))
		last_uid = *((UID*)buf);
	else
	{
		LOG("WARNING! Cannot read resource UID from file [%s] - Generating a new one", file.c_str());
		SaveUID();
	}

	RELEASE(buf);
}

void ModuleResources::SaveUID() const
{
	string file(SETTINGS_FOLDER);
	file += LAST_UID_FILE;

	uint size = App->fs->Save(file.c_str(), (const char*) &last_uid, sizeof(last_uid));

	if (size != sizeof(last_uid))
		LOG("WARNING! Cannot write resource UID into file [%s]", file.c_str());
}