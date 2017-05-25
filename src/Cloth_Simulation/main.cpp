#include <glcore\Window.h>
#include "MyScene.h"
#include <glcore\NCLDebug.h>
#include <glcore\SceneManager.h>
#include <glcore\Input.h>
#include <iomanip>

MyScene* scene = NULL;

int Quit(bool pause = false, const string &reason = "") {
	SceneManager::Release();
	Window::Destroy();

	if (pause) {
		std::cout << reason << std::endl;
		system("PAUSE");
	}

	return 0;
}

int main()
{
	//-------------------
	//--- MAIN ENGINE ---
	//-------------------

	//Initialise the Window
	if (!Window::Initialize(1366, 768, "Cloth Simulation"))
	{
		return Quit(true, "Window failed to initialise!");
	}

	if (!SceneManager::Instance()->Initialize())
	{
		return Quit(true, "SceneManager failed to initialise!");
	}

	//Initialise the Scene
	scene = new MyScene("FE Cloth");
	SceneManager::Instance()->EnqueueScene(scene);


	const float title_update_seconds = 0.33f;
	float deltaTime, lastTime = glfwGetTime();
	float time_accum = 0.0f, time_accum_cloth = 0.0f;
	int frames = 0;
	//Create main game-loop
	while (!Window::WindowShouldClose() && !Input::IsKeyDown(GLFW_KEY_ESCAPE)) {
		float cTime = glfwGetTime();
		deltaTime = cTime - lastTime;
		lastTime = cTime;

		
		Window::BeginFrame();
		SceneManager::Instance()->UpdateScene(deltaTime);
		SceneManager::Instance()->RenderScene();
		Window::EndFrame();
	}

	//Cleanup
	return Quit();
}