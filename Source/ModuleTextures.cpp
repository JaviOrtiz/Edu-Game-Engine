#include "Globals.h"
#include "Application.h"
#include "ModuleTextures.h"
#include "ModuleFileSystem.h"
#include "Devil/include/il.h"
#include "Devil/include/ilut.h"

#pragma comment( lib, "Devil/libx86/DevIL.lib" )
#pragma comment( lib, "Devil/libx86/ILU.lib" )
#pragma comment( lib, "Devil/libx86/ILUT.lib" )

using namespace std;

ModuleTextures::ModuleTextures(bool start_enabled) : Module("Textures", start_enabled)
{
	ilInit();
	iluInit();
	ilutInit();
	ilutRenderer(ILUT_OPENGL);
}

// Destructor
ModuleTextures::~ModuleTextures()
{
	ilShutDown();
}

// Called before render is available
bool ModuleTextures::Init(Config* config)
{
	LOG("Init Image library using DevIL lib version %d", IL_VERSION);

	return true;
}

// Called before quitting
bool ModuleTextures::CleanUp()
{
	LOG("Freeing textures and Image library");

	// TODO: Free all textures
	for (vector<TextureInfo*>::iterator it = textures.begin(); it != textures.end(); ++it)
		RELEASE(*it);
					  
	textures.clear();
	return true;
}

// Import new texture from file path
const char* ModuleTextures::Import(const char* file, const char* path)
{
	const char* ret = nullptr;

	std::string sPath(path);
	std::string sFile(file);

	char* buffer = nullptr;
	uint size = App->fs->Load((char*) (sPath + sFile).c_str(), &buffer);

	if (buffer)
		ret = Import(buffer, size);

	RELEASE(buffer);

	if(ret == nullptr)
		LOG("Cannot load texture %s from path %s", file, path);

	return ret;
}

const char* ModuleTextures::Import(const void * buffer, uint size)
{
	static uint id = 0;
	const char* ret = nullptr;
	static char n[25];

	if (buffer)
	{
		sprintf_s(n, 25, "tex_%u.dds", ++id);
		std::string name(n);

		ILuint ImageName;				  
		ilGenImages(1, &ImageName);
		ilBindImage(ImageName);

		if (ilLoadL(IL_TYPE_UNKNOWN, (const void*)buffer, size))
		{
			ilEnable(IL_FILE_OVERWRITE);

		    ILuint   size;
		    ILubyte *data; 
			// To pick a specific DXT compression use ilSetInteger(IL_DXTC_FORMAT, IL_DXT5);
		    size = ilSaveL(IL_DDS, NULL, 0 ); // Get the size of the data buffer
			if(size > 0) 
			{
				data = new ILubyte[size]; // allocate data buffer
				if (ilSaveL(IL_DDS, data, size) > 0) // Save with the ilSaveIL function
				{
					App->fs->Save((TEXTURE_PATH + name).c_str(), (const char*)data, size);
					ret = n;
				}
				RELEASE(data);
			}
			ilDeleteImages(1, &ImageName);
		}
	}

	if (ret == nullptr)
		LOG("Cannot load texture from buffer of size %u", size);

	return ret;
}

// Load new texture from file path
const TextureInfo* ModuleTextures::Load(const char* file)
{
	static uint id = 0;
	TextureInfo* info = nullptr;

	if (file != nullptr)
	{
		std::string sFile(file);

		char* buffer = nullptr;
		uint size = App->fs->Load((TEXTURE_PATH + sFile).c_str(), &buffer);

		if (buffer)
		{
			ILuint ImageName;				  
			ilGenImages(1, &ImageName);
			ilBindImage(ImageName);

			if (ilLoadL(IL_DDS, (const void*)buffer, size))
			{
				ILinfo i;
				iluGetImageInfo(&i);
				info = new TextureInfo;
				strcpy_s(info->name, 15, file);
				info->width = i.Width;
				info->height = i.Height;
				info->bpp = (uint) i.Bpp;
				info->depth = i.Depth;
				info->mips = i.NumMips;
				info->bytes = i.SizeOfData;

				switch (i.Format)
				{
					case IL_COLOUR_INDEX:
						info->format = TextureInfo::color_index;
					break;
					case IL_RGB:
						info->format = TextureInfo::rgb;
					break;
					case IL_RGBA:
						info->format = TextureInfo::rgba;
					break;
					case IL_BGR:
						info->format = TextureInfo::bgr;
					break;
					case IL_BGRA:
						info->format = TextureInfo::bgra;
					break;
					case IL_LUMINANCE:
						info->format = TextureInfo::luminance;
					break;
					default:
						info->format = TextureInfo::unknown;
					break;
				}

				info->gpu_id = ilutGLBindTexImage();
				ilDeleteImages(1, &ImageName);
			}
		}

		RELEASE(buffer);
	}

	if (info != nullptr)
		textures.push_back(info);
	else
		LOG("Cannot load texture %s", file);

	return info;
}