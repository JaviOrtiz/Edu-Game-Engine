#include "Globals.h"
#include "Application.h"
#include "ComponentSteering.h"
#include "GameObject.h"
#include "Component.h"
#include "ModuleAI.h"
#include "DebugDraw.h"
#include "ModuleEditor.h"
#include "PanelProperties.h"
#include "Imgui/imgui.h"

using namespace std;

// ---------------------------------------------------------
ComponentSteering::ComponentSteering(GameObject* container) : Component(container, Types::Steering)
{
}

// ---------------------------------------------------------
ComponentSteering::~ComponentSteering()
{
}

// ---------------------------------------------------------
void ComponentSteering::OnSave(Config& config) const
{
}

// ---------------------------------------------------------
void ComponentSteering::OnLoad(Config * config)
{
}

// ---------------------------------------------------------
void ComponentSteering::OnPlay()
{
}

float RandomBinomial()
{
	return ((float)rand() / (float)RAND_MAX) - ((float)rand() / (float)RAND_MAX);
}

// ---------------------------------------------------------
void ComponentSteering::OnUpdate(float dt)
{
	if (goal != nullptr)
	{
		float3 my_pos = game_object->GetGlobalPosition();
		float3 goal_pos = goal->GetGlobalPosition();
		float distance = goal_pos.Distance(my_pos);
		
		float3 direction = goal_pos - my_pos;
		direction.Normalize();
		Quat my_rot = game_object->GetLocalRotationQ();
		float3 my_front = game_object->GetGlobalTransformation().WorldZ();
		float3 my_right = game_object->GetGlobalTransformation().WorldX();

		float speed_dt = max_mov_speed * dt;

		switch (behaviour)
		{
			case Behaviour::seek:
			{
				if (distance < max_distance && distance > min_distance) 
				{
					game_object->SetLocalPosition(my_pos + (direction * speed_dt));
				}
			} break;

			case Behaviour::flee:
			{
				if (distance < max_distance) 
				{
					direction = my_pos - goal_pos;
					direction.Normalize();
					game_object->SetLocalPosition(my_pos + (direction * speed_dt));
				}
			} break;

			case Behaviour::wander:
			{
				float r = RandomBinomial();
				direction = Quat::RotateY(r).Transform(my_front);
				game_object->SetLocalPosition(my_pos + (direction * speed_dt));
			} break;
		}

		// rotate to look in our current movement direction (Z)
		float angle = direction.AngleBetweenNorm(my_right);

		if (angle > 0.01f)
		{
			bool left = direction.Dot(my_right) >= 0.f;
			float rotation = MIN(angle*dt, max_rot_speed*dt);
			if (!left)
				rotation = -rotation;
			game_object->SetLocalRotation(my_rot * Quat::RotateY(rotation));
		}
	}
}

// ---------------------------------------------------------
void ComponentSteering::OnStop()
{
}

// ---------------------------------------------------------
void ComponentSteering::OnDebugDraw() const
{
	float3 pos = game_object->GetLocalPosition();
	float3 dir = game_object->GetLocalTransform().WorldZ();
	DebugDraw(Cylinder(LineSegment(pos, pos + float3::unitY), 0.5f), Green, game_object->GetGlobalTransformation());
	DebugDraw(Cone(LineSegment(pos, pos + float3::unitY), 0.5f), Red, game_object->GetGlobalTransformation());
	DebugDraw(LineSegment(pos, dir), Blue, true);

	if (goal != nullptr)
	{
		DebugDraw(Sphere(float3::zero, min_distance), Green, goal->GetGlobalTransformation());
		DebugDraw(Sphere(float3::zero, max_distance), Red, goal->GetGlobalTransformation());
	}
}

// ---------------------------------------------------------
void ComponentSteering::SetBehaviour(Behaviour new_behaviour)
{
	if (new_behaviour != behaviour)
	{
		behaviour = new_behaviour;
	}
}

// ---------------------------------------------------------
ComponentSteering::Behaviour ComponentSteering::GetBehaviour() const
{
	return behaviour;
}

// ---------------------------------------------------------
void ComponentSteering::DrawEditor()
{
	static_assert(Behaviour::unknown == 3, "code needs update");

	static const char* behaviours[] = { "Seek", "Flee", "Wander", "Unknown" };

	int behaviour_type = behaviour;
	if (ImGui::Combo("Behaviour", &behaviour_type, behaviours, 3))
		SetBehaviour((Behaviour) behaviour_type);

	ImGui::Text("Target:");
	const GameObject* selected = App->editor->props->PickGameObject(goal);
	if (selected != nullptr)
		goal = selected;

	ImGui::DragFloat("Movement Vel", &max_mov_speed, 0.1f);
	ImGui::SliderAngle("Rotation Vel", &max_rot_speed, 0.01f);
	ImGui::DragFloatRange2("Range", &min_distance, &max_distance, 0.1f, 0.1f);

	static ImVec2 foo[10] = { {-1,0} };
	//foo[0].x = -1; // init data so editor knows to take it from here
	if (ImGui::Curve("Curve Editor", ImGui::GetContentRegionAvail(), 10, foo))
	{
		// curve changed
		LOG("At 0.5 we have a %f", ImGui::CurveValue(0.5f, 10, foo));
	}
}
