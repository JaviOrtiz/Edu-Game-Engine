#include "PanelProperties.h"
#include "Application.h"
#include "Imgui/imgui.h"
#include "GameObject.h"
#include "Component.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ComponentAudioSource.h"
#include "ComponentAudioListener.h"
#include "ComponentCamera.h"
#include "ModuleMeshes.h"
#include "ModuleLevelManager.h"
#include "ModuleTextures.h"
#include "ModuleEditor.h"
#include "DebugDraw.h"
#include <list>

using namespace std;

// ---------------------------------------------------------
PanelProperties::PanelProperties() : Panel("Properties", SDL_SCANCODE_F3)
{
	width = 325;
	height = 578;
	posx = 956;
	posy = 21;
}

// ---------------------------------------------------------
PanelProperties::~PanelProperties()
{}

// ---------------------------------------------------------
void PanelProperties::Draw()
{
	GameObject* selected = App->editor->selected;
    ImGui::Begin("Properties", &active, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoFocusOnAppearing);

	if (ImGui::BeginMenu("Options"))
	{
		if (ImGui::MenuItem("Reset Transform", nullptr, nullptr, (selected != nullptr)))
		{
			selected->SetLocalPosition(float3::zero);
			selected->SetLocalScale(float3::one);
			selected->SetLocalRotation(float3::zero);
		}

        if (ImGui::BeginMenu("New Component", (selected != nullptr)))
        {
			if (ImGui::MenuItem("Audio Listener"))
				selected->CreateComponent(ComponentTypes::AudioListener);
			if (ImGui::MenuItem("Audio Source"))
				selected->CreateComponent(ComponentTypes::AudioSource);
			if (ImGui::MenuItem("Geometry"))
				selected->CreateComponent(ComponentTypes::Geometry);
			if (ImGui::MenuItem("Material"))
				selected->CreateComponent(ComponentTypes::Material);
			if (ImGui::MenuItem("Camera"))
				selected->CreateComponent(ComponentTypes::Camera);
            ImGui::EndMenu();
        }

		ImGui::EndMenu();
	}

	if (selected != nullptr )
	{

		// Active check box
		bool active = selected->IsActive();
		ImGui::Checkbox(" ", &active);
		selected->SetActive(active);

		ImGui::SameLine();

		// Text Input for the name
		char name[50];
		strcpy_s(name, 50, selected->name.c_str());
		if (ImGui::InputText("", name, 50,
			ImGuiInputTextFlags_AutoSelectAll |
			ImGuiInputTextFlags_EnterReturnsTrue))
			selected->name = name;

		// Transform section ============================================
		if (ImGui::CollapsingHeader("Local Transformation", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float3 pos = selected->GetLocalPosition();
			float3 rot = selected->GetLocalRotation();
			float3 scale = selected->GetLocalScale();;

			if (ImGui::DragFloat3("Position", (float*)&pos, 0.25f))
				selected->SetLocalPosition(pos);

			if(ImGui::SliderFloat3("Rotation", (float*)&rot, -PI, PI))
				selected->SetLocalRotation(rot);

			if (ImGui::DragFloat3("Scale", (float*)&scale, 0.05f))
				selected->SetLocalScale(scale);

			DebugDraw(selected->GetGlobalTransformation());

			ImGui::Text("Bounding Box:");
			ImGui::SameLine();
			if (selected->global_bbox.IsFinite())
			{
				float3 size = selected->local_bbox.Size();
				ImGui::TextColored(IMGUI_YELLOW, "%.2f, %.2f %.2f", size.x, size.y, size.x);
			}
			else
				ImGui::TextColored(IMGUI_YELLOW, "- not generated -");
		}

		// Iterate all components and draw
		for (list<Component*>::iterator it = selected->components.begin(); it != selected->components.end(); ++it)
		{
			switch ((*it)->GetType())
			{
				case ComponentTypes::Geometry:
				{
					if(InitComponentDraw(*it, "Geometry Mesh"))
						DrawMeshComponent((ComponentMesh*)(*it));
				}	break;
				case ComponentTypes::Material:
				{
					if(InitComponentDraw(*it, "Material"))
						DrawMaterialComponent((ComponentMaterial*)(*it));
				}	break;
				case ComponentTypes::AudioSource:
				{
					if(InitComponentDraw(*it, "Audio Source"))
						DrawAudioSourceComponent((ComponentAudioSource*)(*it));
				}	break;
				case ComponentTypes::AudioListener:
				{
					if(InitComponentDraw(*it, "Audio Listener"))
						DrawAudioListenerComponent((ComponentAudioListener*)(*it));
				}	break;
				case ComponentTypes::Camera:
				{
					if(InitComponentDraw(*it, "Camera"))
						DrawCameraComponent((ComponentCamera*)(*it));
				}	break;
				default:
				{
					InitComponentDraw(*it, "Unknown");
				}
			};
		}

	}

    ImGui::End();
}

bool PanelProperties::InitComponentDraw(Component* component, const char * name)
{
	bool ret = false;

	if (ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen))
	{
		bool active = component->IsActive();
		if(ImGui::Checkbox("Active", &active))
			component->SetActive(active);
		ret = true;
	}

	return ret;
}

void PanelProperties::DrawMeshComponent(ComponentMesh * component)
{
	const Mesh* mesh = component->GetMesh();
	if (mesh == nullptr)
		return;

    ImGui::TextColored(ImVec4(1,1,0,1), "%u Triangles (%u indices %u vertices)",
		mesh->num_indices / 3,
		mesh->num_indices,
		mesh->num_vertices);

	bool uvs = mesh->texture_coords != nullptr;
	bool normals = mesh->normals != nullptr;
	bool colors = mesh->colors != nullptr;

	ImGui::Checkbox("UVs", &uvs);
	ImGui::SameLine();
	ImGui::Checkbox("Normals", &normals);
	ImGui::SameLine();
	ImGui::Checkbox("Colors", &colors);
}

void PanelProperties::DrawAudioSourceComponent(ComponentAudioSource * component)
{
	const char* file = component->GetFile();

	ImGui::Text("File: ");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(1, 1, 0, 1), (file) ? file : "No file loaded");
	ImGui::SameLine();
	ImGui::Checkbox("Is 2D", &component->is_2d);

	ImGui::SliderFloat("Fade In", (float*)&component->fade_in, 0.0f, 10.0f);
	ImGui::SliderFloat("Fade Out", (float*)&component->fade_out, 0.0f, 10.0f);
	ImGui::DragFloat("Min Distance", (float*)&component->min_distance, 0.1f, 0.1f, 10000.0f);
	ImGui::DragFloat("Max Distance", (float*)&component->max_distance, 0.1f, 0.1f, 10000.0f);
	ImGui::SliderInt("Cone In", (int*)&component->cone_angle_in, 0, 360);
	ImGui::SliderInt("Cone Out", (int*)&component->cone_angle_out, 0, 360);
	ImGui::SliderFloat("Vol Out Cone", (float*)&component->out_cone_vol, 0.0f, 1.0f);
	
	static const char * states[] = { 
		"Not Loaded", 
		"Stopped",
		"About to Play",
		"Playing",
		"About to Pause",
		"Pause",
		"About to Unpause",
		"About to Stop"
	};

	ImGui::Text("Current State: ");
	ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", states[component->GetCurrentState()]);

	if (ImGui::Button("Play"))
		component->Play();

	ImGui::SameLine();
	if (ImGui::Button("Pause"))
		component->Pause();

	ImGui::SameLine();
	if (ImGui::Button("Unpause"))
		component->UnPause();

	ImGui::SameLine();
	if (ImGui::Button("Stop"))
		component->Stop();
}

void PanelProperties::DrawAudioListenerComponent(ComponentAudioListener * component)
{
	ImGui::DragFloat("Distance", (float*)&component->distance, 0.1f, 0.1f, 10000.0f);
	ImGui::SliderFloat("Roll Off", (float*)&component->roll_off, 0.0f, 10.0f);
	ImGui::SliderFloat("Doppler", (float*)&component->doppler, 0.0f, 10.0f);
}

void PanelProperties::DrawCameraComponent(ComponentCamera * component)
{
	ImGui::Checkbox("Frustum Culling", &component->frustum_culling);

	float near_plane = component->GetNearPlaneDist();
	if (ImGui::DragFloat("Near Plane", &near_plane, 0.1f, 0.1f, 1000.0f))
		component->SetNearPlaneDist(near_plane);

	float far_plane = component->GetFarPlaneDist();
	if (ImGui::DragFloat("Far Plane", &far_plane, 10.0f, 25.f, 10000.0f))
		component->SetFarPlaneDist(far_plane);

	float fov = component->GetFOV();
	if (ImGui::SliderFloat("Field of View", &fov, 1.0f, 179.0f))
		component->SetFOV(fov);

	float aspect_ratio = component->GetAspectRatio();
	if (ImGui::DragFloat("Aspect Ratio", &aspect_ratio, 0.1f, 0.1f, 10000.0f))
		component->SetAspectRatio(aspect_ratio);

	ImGui::ColorEdit3("Background", &component->background);
}

void PanelProperties::DrawMaterialComponent(ComponentMaterial * component)
{
	const TextureInfo* info = component->texture;

	if (info == nullptr)
		return;

	ImGui::TextColored(IMGUI_YELLOW, info->name);
	ImGui::SameLine();
	ImGui::Text("(%u,%u) %0.1f Mb", info->width, info->height, info->bytes / (1024.f*1024.f));
	ImGui::Text("Format: %s Depth: %u Bpp: %u Mips: %u", texture_formats[info->format], info->depth, info->bpp, info->mips);

	ImVec2 size((float)info->width, (float)info->height);
	float max_size = 250.f;

	if (size.x > max_size || size.y > max_size)
	{
		if (size.x > size.y)
		{
			size.y *= max_size / size.x;
			size.x = max_size;
		}
		else
		{
			size.x *= max_size / size.y;
			size.y = max_size;
		}
	}

	ImGui::Image((ImTextureID) info->gpu_id, size, ImVec2(0,1), ImVec2(1,0), ImColor(255, 255, 255, 128), ImColor(255, 255, 255, 128));
}
