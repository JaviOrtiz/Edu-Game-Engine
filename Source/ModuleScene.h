#ifndef __MODULE_SCENE_H__
#define __MODULE_SCENE_H__

#include <string>
#include "Module.h"
#include "Math.h"

struct aiScene;
struct aiNode;
struct aiMaterial;
struct aiMetadata;
class GameObject;

class ModuleScene : public Module
{
public:

	ModuleScene(bool start_enabled = true);
	~ModuleScene();

	bool Init(Config* config = nullptr) override;
	bool Start(Config* config) override;
	bool CleanUp() override;

	bool LoadScene(const char* file);
	void LoadMetaData(aiMetadata* const meta);

private:

	void RecursiveCreateGameObjects(const aiNode* node, GameObject* parent, const std::string& basePath);

	const struct aiScene* scene = nullptr;
};

#endif // __MODULE_SCENE_H__