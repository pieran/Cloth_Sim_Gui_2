
#pragma once

#include <glcore\Scene.h>
#include <glcore\SceneManager.h>
#include <glcore\NCLDebug.h>
#include <glcore\CommonUtils.h>

class TestScene : public Scene
{
public:
	TestScene(const std::string& friendly_name)
		: Scene(friendly_name)
	{
	}

	virtual void OnInitializeScene() override
	{
		SceneManager::Instance()->GetCamera()->SetPosition(Vector3(-3.0f, 10.0f, 10.0f));
		SceneManager::Instance()->GetCamera()->SetPitch(-20.f);
		//SceneManager::Instance()->SetShadowMapNum(1);
		//SceneManager::Instance()->SetShadowMapSize(1024);

		//Create Ground (..why not?)
		Object* ground = CommonUtils::BuildCuboidObject(
			"Ground",
			Vector3(0.0f, 0.0f, 0.0f),
			Vector3(20.0f, 1.0f, 20.0f),
			false,
			0.0f,
			false,
			false,
			Vector4(0.2f, 0.5f, 1.0f, 1.0f));

		this->AddGameObject(ground);


		//Create Hanging Ball
		{
			Object* handle = CommonUtils::BuildSphereObject("",
				Vector3(-7.f, 7.f, -5.0f),				//Position
				0.5f,									//Radius
				true,									//Has Physics Object
				0.0f,									//Infinite Mass
				false,									//No Collision Shape Yet
				true,									//Dragable by the user
				CommonUtils::GenColour(0.45f, 0.5f));	//Color

			Object* ball = CommonUtils::BuildSphereObject("",
				Vector3(-4.f, 7.f, -5.0f),				//Position
				0.5f,									//Radius
				true,									//Has Physics Object
				1.0f,									// Inverse Mass = 1 / 1kg mass
				false,									//No Collision Shape Yet
				true,									//Dragable by the user
				CommonUtils::GenColour(0.5f, 1.0f));	//Color

			this->AddGameObject(handle);
			this->AddGameObject(ball);
		}




		//Create Hanging Cube (Attached by corner)
		{
			Object* handle = CommonUtils::BuildSphereObject("",
				Vector3(4.f, 7.f, -5.0f),				//Position
				0.5f,									//Radius
				true,									//Has Physics Object
				0.0f,									//Infinite Mass
				false,									//No Collision Shape Yet
				true,									//Dragable by the user
				CommonUtils::GenColour(0.55f, 0.5f));	//Color

			Object* cube = CommonUtils::BuildCuboidObject("",
				Vector3(7.f, 7.f, -5.0f),				//Position
				Vector3(0.5f, 0.5f, 0.5f),				//Half Dimensions
				true,									//Has Physics Object
				1.0f,									//Infinite Mass
				false,									//No Collision Shape Yet
				true,									//Dragable by the user
				CommonUtils::GenColour(0.6f, 1.0f));	//Color

			this->AddGameObject(handle);
			this->AddGameObject(cube);
		}


	}

	virtual void OnUpdateScene(float dt) override
	{
		Scene::OnUpdateScene(dt);

		NCLDebug::AddStatusEntry(Vector4(1.0f, 0.9f, 0.8f, 1.0f), "Physics:");
	}
};