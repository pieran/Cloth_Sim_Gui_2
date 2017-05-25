
#pragma once

#define USE_FEM_C1 0

#include <glcore\Mesh.h>
#include <glcore\Scene.h>
#include <glcore\ObjectMesh.h>

#include "GraphObject.h"
#include "Sim_Manager.h"
#include "Mouse_Dragger.h"


#include "Sim_6NodedC0.h"
#include "Sim_6NodedC1.h"
#include "Sim_PBD.h"

#define MAX_HUD_VISIBILITY_TYPES 4
#define CLOTHVIDEODIR "../../media/clothvideos/"

class MyScene : public Scene
{
public:
	MyScene(const std::string& friendly_name);
	~MyScene();

	void ConfigureGraphObjects();
	void SetSimulationSubdivisions();

	void OnInitializeScene()					override;
	void OnUpdateScene(float dt)				override;
	void OnSceneResize(int width, int height)	override;

	void HandleSimulationOptions_ImGui();
	void HandleSimulationOptions();
	void UpdateSimulationGraphs();

	void SaveCameraData();
	void LoadCameraData();

	GLuint   m_glResetTexture;
	GLuint   m_glPlayTextures[4];

	Sim_Manager*		m_Sim;
	Mouse_Dragger		m_MouseDragger;
	int m_SceneGridSize = 1;
	bool m_GeneratorInvalidated = false;

	float m_SimTimestep;
	bool m_SimPaused;

	float           m_ClothUpdateMs;
	GraphObject*	m_GraphObject;
	GraphObject*	m_GraphObjectSolver;

	bool m_GraphsVisible;
};