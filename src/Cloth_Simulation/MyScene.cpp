#include "MyScene.h"

#include <glcore\ObjectMesh.h>
#include <glcore\CommonUtils.h>
#include <glcore\SceneManager.h>
#include <glcore\Input.h>
#include <glcore\NCLDebug.h>
#include <glcore\Window.h>
#include <glcore\imgui_wrapper.h>
#include <glcore\external\SOIL.h>

#include "Generator_Square_Grid.h"
#include "Generator_Square_Grid_BendTest.h"

#include <fstream>

MyScene::MyScene(const std::string& friendly_name)
	: Scene(friendly_name)
	, m_Sim(NULL)
	, m_SimTimestep(1.0f / 60.0f)
	, m_SimPaused(true)
	, m_glResetTexture(NULL)
{
	memset(m_glPlayTextures, 0, 4 * sizeof(GLuint));
	m_GraphsVisible = true;
}

MyScene::~MyScene()
{
	if (m_glResetTexture)
		glDeleteTextures(1, &m_glResetTexture);
}

void MyScene::OnSceneResize(int width, int height)
{
	//Profiling
	m_GraphObject->Parameters().graph_width = 300;
	m_GraphObject->Parameters().graph_height = 150;
	m_GraphObject->SetPosition(width - 10, 70);


	m_GraphObjectSolver->Parameters().graph_width = 200;
	m_GraphObjectSolver->Parameters().graph_height = 100;
	m_GraphObjectSolver->SetPosition(width - 10, 320);
}

void MyScene::ConfigureGraphObjects()
{
	const int num_graph_colours = 4;
	const Vector4 graph_colours[] = {
		Vector4(0.7f, 0.5f, 1.0f, 1.0f),
		Vector4(1.0f, 0.5f, 0.8f, 1.0f),
		Vector4(0.5f, 0.8f, 1.0f, 1.0f),
		Vector4(0.8f, 1.0f, 0.5f, 1.0f),
	};

	m_GraphObject->Parameters().title_text = "Simulation";
	m_GraphObject->ClearSeries();
	for (int i = 0; i < m_Sim->Simulation()->GetNumSubProfilers(); ++i)
	{
		m_GraphObject->AddSeries(m_Sim->Simulation()->GetSubProfiler(i).GetAlias(), graph_colours[i % num_graph_colours]);
	}
	
	m_GraphObject->Parameters().axis_y_span_label_interval = 1;
	m_GraphObject->Parameters().axis_y_span_sub_interval = 0.5;
	m_GraphObject->Parameters().axis_y_span_max = 2;
	m_GraphObject->Parameters().axis_y_span_max_isdynamic = true;
	m_GraphObject->Parameters().axis_y_label_subfix = "ms";


	m_GraphObjectSolver->Parameters().title_text = "Solver";
	m_GraphObjectSolver->ClearSeries();
	m_GraphObjectSolver->AddSeries("Init", Vector4(0.7f, 0.5f, 1.0f, 1.0f));
	m_GraphObjectSolver->AddSeries("A Mtx Mult", Vector4(1.0f, 0.5f, 0.8f, 1.0f));
	m_GraphObjectSolver->AddSeries("Dot Products", Vector4(0.5f, 0.8f, 1.0f, 1.0f));

	m_GraphObjectSolver->Parameters().axis_y_span_label_interval = 0.5;
	m_GraphObjectSolver->Parameters().axis_y_span_sub_interval = 0.25;
	m_GraphObjectSolver->Parameters().axis_y_span_max = 1;
	m_GraphObjectSolver->Parameters().axis_y_span_max_isdynamic = true;
	m_GraphObjectSolver->Parameters().axis_y_label_subfix = "ms";

	int width, height;
	Window::GetRenderDimensions(&width, &height);
	OnSceneResize(width, height);
}

uint visual_subdivisions = 1;
void MyScene::OnInitializeScene()
{
	Vector3 light_dir = Vector3(0.5f, 1.0f, 1.0f); light_dir.Normalise();
	SceneManager::Instance()->SetInverseLightDirection(light_dir);

	SceneManager::Instance()->GetCamera()->SetPosition(Vector3(-3.0f, 10.0f, 10.0f));
	SceneManager::Instance()->GetCamera()->SetPitch(-20.f);
	SceneManager::Instance()->GetCamera()->SetYaw(0.0f);


	m_glResetTexture = SOIL_load_OGL_texture(TEXTUREDIR"reset.png", 0, m_glResetTexture, 0);
	m_glPlayTextures[0] = SOIL_load_OGL_texture(TEXTUREDIR"play.png", 0, m_glPlayTextures[0], 0);
	m_glPlayTextures[1] = SOIL_load_OGL_texture(TEXTUREDIR"pause.png", 0, m_glPlayTextures[1], 0);
	m_glPlayTextures[2] = SOIL_load_OGL_texture(TEXTUREDIR"steponce.png", 0, m_glPlayTextures[2], 0);
	m_glPlayTextures[3] = SOIL_load_OGL_texture(TEXTUREDIR"resetsim.png", 0, m_glPlayTextures[3], 0);

	//Profiling
	m_GraphObject = new GraphObject();
	this->AddGameObject(m_GraphObject);

	m_GraphObjectSolver = new GraphObject();
	this->AddGameObject(m_GraphObjectSolver);

	m_ClothUpdateMs = 0.0f;


	m_Sim = new Sim_Manager("FE_Simulation");
	//m_Sim->SetGenerator(new Generator_Square_Grid_BendTest());
	this->AddGameObject(m_Sim);
	SetSimulationSubdivisions();
	m_MouseDragger.SetSimulation(m_Sim);


	//Create Ground
	Object* ground = CommonUtils::BuildCuboidObject("Ground", Vector3(0.0f, -1.001f, 0.0f), Vector3(20.0f, 1.0f, 20.0f), false, 0.0f, false, false, Vector4(0.4f, 0.8f, 1.f, 1.f));
	this->AddGameObject(ground);

	ConfigureGraphObjects();
	LoadCameraData();
}

void MyScene::SetSimulationSubdivisions()
{
	Generator_Square_Grid* gen = dynamic_cast<Generator_Square_Grid*>(m_Sim->Generator());
	if (gen != NULL)
	{
		if (m_GeneratorInvalidated || gen->GetVisualSubdivisions() != m_SceneGridSize)
		{
			gen->SetVisualSubdivisions(m_SceneGridSize);
			m_Sim->Generate();
			m_MouseDragger.UpdateTangentDescriptors();
		}
		else
		{
			//Keep track of static vertices
			std::vector<uint> static_phyxels; static_phyxels.reserve(30);

			uint num_total = m_Sim->BaseConfig().NumVertices + m_Sim->BaseConfig().NumTangents;
			for (unsigned int i = 0; i < num_total; ++i)
			{
				if (m_Sim->Simulation()->GetIsStatic(i))
					static_phyxels.push_back(i);
			}

			m_Sim->Generate(); //Only effects rotation

			for (unsigned int i = 0; i < static_phyxels.size(); ++i)
			{
				m_Sim->Simulation()->SetIsStatic(static_phyxels[i], true);
			}
		}
		
		
		ConfigureGraphObjects();
		m_GeneratorInvalidated = false;
		
	}
}


void MyScene::OnUpdateScene(float dt)
{
	UpdateSimulationGraphs();
	HandleSimulationOptions_ImGui();

	if (!m_SimPaused)
		m_Sim->Integrator()->UpdateSimulation(m_SimTimestep);

	m_MouseDragger.RenderDragables();

	Scene::OnUpdateScene(dt);
}

#define _BEGIN_TABLE_ ImGui::Indent(16.0f); ImGui::Columns(2); 
#define _END_TABLE_ ImGui::Columns(1); ImGui::Unindent(16.0f);
#define _ROW_START_(x, ...) ImGui::PushID(x); ImGui::Text(x, __VA_ARGS__); ImGui::NextColumn(); ImGui::PushItemWidth(-1);
#define _ROW_END_ while(ImGui::GetColumnIndex() != 0) {ImGui::NextColumn();} ImGui::PopID(); ImGui::Separator();
#define _RESET_BUTTON_ ImGui::ImageButton((void*)m_glResetTexture, ImVec2(16.f, 16.f), ImVec2(0, 0), ImVec2(1, 1), 0)
#define _RESSET_BUTTON_SIZE_ 40
#define _SIZING_FOR_RESET_ ImGui::PushItemWidth(ImGui::GetColumnWidth() - _RESSET_BUTTON_SIZE_);

int ColouredButton(const char* x, const ImVec2& y, const Vector4& colour) // Name, Number, Size, Color, HoveredColor, ActiveColor
{
	//ImGui::PushID(x);
	ImGui::PushStyleColor(ImGuiCol_Button, (const ImVec4&)colour);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (const ImVec4&)(colour + Vector4(0.1f, 0.1f, 0.1f, 0.0f)));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, (const ImVec4&)(colour + Vector4(0.2f, 0.2f, 0.2f, 0.1f)));
	if (ImGui::Button(x, y))
	{
		ImGui::PopStyleColor(3);
		//ImGui::PopID();
		return 1;
	}

	ImGui::PopStyleColor(3);
	return 0;	
}

void MyScene::HandleSimulationOptions_ImGui()
{
	
	static bool overlay_visible = true;

	int width, height;
	Window::GetRenderDimensions(&width, &height);

	ImGui::SetNextWindowPos(ImVec2(-5, 25));
	ImGui::SetNextWindowSizeConstraints(ImVec2(300, -1), ImVec2(FLT_MAX, -1));
	if (!ImGui::Begin("Properties", &overlay_visible, ImVec2(400, height - 25), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar))
	{
		ImGui::End();
		return;
	}
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	
	if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen))
	{	
		float total_time = (m_SimPaused) ? 0.0f : m_Sim->Integrator()->GetTotalTimer().GetTimedMilliSeconds();
		float solver_total = (m_SimPaused) ? 0.0f : m_Sim->Simulation()->GetTotalTimer().GetTimedMilliSeconds();
		m_Sim->Simulation()->GetTotalTimer().ResetTotalMs();
		int num_solver_calls = (m_SimPaused) ? 0 : m_Sim->Integrator()->GetNumSolverCalls();

		float single_solve_time = (m_SimPaused) ? 0.0f : solver_total / (float)num_solver_calls;
		float overhead_time = total_time - solver_total;
		if (overhead_time < 0.f) overhead_time = 0.f; //some floating point and timing issues for near zero overhead times

		_BEGIN_TABLE_;
		{
			_SIZING_FOR_RESET_;
			_ROW_START_("Simulation Method");
			static int simulation_type = m_Sim->GetSimType();
			ImGui::Combo("##SimulationType", &simulation_type, "FE 6 Noded C0\0FE 6 Noded C1\0FE 6 Noded C1 V2\0PBD 3 Noded C0");
			ImGui::SameLine();
			if (_RESET_BUTTON_) m_Sim->SetSimType(Sim_Type_FE6NodedC1);
			_ROW_END_;

			_ROW_START_("Integration Method");
			_SIZING_FOR_RESET_;
			static int integration_type = m_Sim->Integrator()->GetIntergrationType();
			ImGui::Combo("##IntegrationMethod", &integration_type, "Explicit Euler\0Runge Kutta 2\0Runge Kutta 4");
			ImGui::SameLine();
			if (_RESET_BUTTON_) m_Sim->Integrator()->SetIntegrationType(Sim_Integrator_Type_RK2);
			_ROW_END_;

			_ROW_START_("Simulation timestep");
			_SIZING_FOR_RESET_;
			ImGui::DragFloat("##UpdatesPerRender", &m_Sim->Integrator()->GetSubTimestep(), 0.000001f, 0.0001f, 1.f / 60.f, "%.5fms", 1.0f);
			ImGui::SameLine();
			if (_RESET_BUTTON_) m_Sim->Integrator()->SetSubTimestep(DEFAULT_SUB_TIMESTEP);
			_ROW_END_;
	
			_ROW_START_("Updates Per Render");
			ImGui::Text("~ %.1f", (m_SimTimestep / m_Sim->Integrator()->GetSubTimestep()));
			_ROW_END_;

			_ROW_START_("Time Total");
			ImGui::Text("%.2fms", total_time);
			_ROW_END_;

			_ROW_START_("Integrator Overhead");
			ImGui::Text("%.2fms", overhead_time);
			_ROW_END_;

			_ROW_START_("Time Single Solve");
			ImGui::Text("%.2fms x %d", single_solve_time, num_solver_calls);
			_ROW_END_;

			_ROW_START_("Show Profiling Graphs");
			ImGui::Checkbox("##profilinggraphs", &m_GraphsVisible);
			m_GraphObject->SetVisibility(m_GraphsVisible);
			m_GraphObjectSolver->SetVisibility(m_GraphsVisible);
			_ROW_END_;

			static bool gavityEnabled = true;
			_ROW_START_("Gravity");
			ImGui::Checkbox("##gravity", &gavityEnabled);
			m_Sim->Integrator()->SetGravity(gavityEnabled ? Vector3(0.f, -9.81f, 0.f) : Vector3(0.f, 0.f, 0.f));
			_ROW_END_;

			if (integration_type != m_Sim->Integrator()->GetIntergrationType())
			{
				m_Sim->Integrator()->SetIntegrationType((Sim_Integrator_Type)integration_type);
			}
			if (simulation_type != m_Sim->GetSimType())
			{
				m_Sim->SetSimType((Sim_Type)simulation_type);
			}
		}
		_END_TABLE_;


	}
	if (ImGui::CollapsingHeader("Generator", ImGuiTreeNodeFlags_DefaultOpen))
	{
		Generator_Square_Grid* gen = dynamic_cast<Generator_Square_Grid*>(m_Sim->Generator());

		static int grid_size = m_SceneGridSize;
		static Vector4 rotation = Vector4(90.f, 1.f, 0.f, 0.f);
		
	


		_BEGIN_TABLE_;
		{
			static int generator_type = 0;
			_ROW_START_("Generate");
			if (ImGui::Button("GENERATE", ImVec2(150, 16)) && gen != NULL)
			{
				gen->Transform = Matrix4::Rotation(rotation.x, Vector3(rotation.y, rotation.z, rotation.w));
				m_SceneGridSize = grid_size;
				SetSimulationSubdivisions();
			}
			_ROW_END_;

			_ROW_START_("Generator Scheme");
			_SIZING_FOR_RESET_;
			auto update_generator_type = [&]() {
				Sim_Generator* gen = NULL;
				switch (generator_type)
				{
				case 0:
					gen = new Generator_Square_Grid();
					break;
				case 1:
					gen = new Generator_Square_Grid_BendTest();
					break;
				}
				gen->Transform = Matrix4::Rotation(rotation.x, Vector3(rotation.y, rotation.z, rotation.w));
				gen->SubDivisions = grid_size * 2 + 1;
				m_Sim->SetGenerator(gen);
				m_GeneratorInvalidated = true;
			};

			if (ImGui::Combo("##genscheme", &generator_type, "Square Grid\0Square Grid Bending Test")) update_generator_type();
			ImGui::SameLine();
			if (_RESET_BUTTON_)
			{
				generator_type = 0;
				update_generator_type();
			}
			_ROW_END_;

			_ROW_START_("Grid Subdivisions");
			_SIZING_FOR_RESET_;
			ImGui::SliderInt("##SimulationSubdivisions", &grid_size, 1, 10);
			ImGui::SameLine();
			if (_RESET_BUTTON_) grid_size = 1;
			_ROW_END_;

			_ROW_START_("Initial Rotation");
			_SIZING_FOR_RESET_;
			ImGui::DragFloat("##rotation", &rotation.x, 1.f, 0.f, 360.f, "%4.1f degrees");
			ImGui::SameLine();
			if (_RESET_BUTTON_) rotation.x = 90.f;
			_ROW_END_;

			_ROW_START_("Initial Rotation Axis"); 
			_SIZING_FOR_RESET_;
			if (ImGui::InputFloat3("##rotationaxis", &rotation.y, -1, ImGuiInputTextFlags_EnterReturnsTrue))
				((Vector3*)&rotation.y)->Normalise();
			ImGui::SameLine();
			if (_RESET_BUTTON_) rotation = Vector4(rotation.x, 1.f, 0.f, 0.f);
			_ROW_END_;
		}
		_END_TABLE_;


	}

	if (ImGui::CollapsingHeader("Runtime", ImGuiTreeNodeFlags_DefaultOpen))
	{
		_BEGIN_TABLE_;
		{
			_ROW_START_("State: %s", m_SimPaused ? "Paused" : "Running");
			ImGui::SameLine(50);
			if (ImGui::ImageButton((void*)m_glPlayTextures[3], ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), 0))
			{
				m_SimPaused = true;
				Window::GetVideoEncoder()->EndEncoding();

				SetSimulationSubdivisions();
			}

			ImGui::SameLine();

			if (ImGui::ImageButton((void*)m_glPlayTextures[(int)m_SimPaused], ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0, 1.f, 0.f, m_SimPaused ? 0 : 0.3f)))
			{
				m_SimPaused = !m_SimPaused;
				if (m_SimPaused)
					Window::GetVideoEncoder()->EndEncoding();
			}
			
			ImGui::SameLine();

			if (ImGui::ImageButton((void*)m_glPlayTextures[2], ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), 0))
			{
				m_SimPaused = true;
				Window::GetVideoEncoder()->EndEncoding();

				for (int i = 0; i < 5; ++i)
					m_Sim->Integrator()->UpdateSimulation(m_Sim->Integrator()->GetSubTimestep());
			}		

			_ROW_END_;


			_ROW_START_("Elapsed Time (mm:ss.xxx)");
			float mins = floor(m_Sim->Integrator()->GetElapsedTime() / 60.f);
			float secs = fmod(m_Sim->Integrator()->GetElapsedTime(), 60.f);

			ImGui::Text("%02.0f:%06.3f", mins, secs);
			_ROW_END_;

			_ROW_START_("Recording");
			if (Window::GetVideoEncoder()->IsEncoding())
			{
				if (ColouredButton("Stop Recording", ImVec2(150, 24), Vector4(1.f, 0.f, 0.f, 0.5f)))
				{
					Window::GetVideoEncoder()->EndEncoding();
					m_SimPaused = true;
				}
			}
			else
			{
				if (ColouredButton("Start Recording", ImVec2(150, 24), Vector4(0.f, 0.7f, 0.f, 0.5f)))
				{
					char filename[1024];
					auto now = time(NULL);
					struct tm buf;
					if (gmtime_s(&buf, &now))
					{
						printf("ERROR: Unable to get current date/time!\n");
					}
					else
					{
						strftime(filename, 1024, VIDEOSDIR"Cloth/Cloth %d_%m_%Y %H-%M-%S.mp4", &buf);
						Window::GetVideoEncoder()->BeginEncoding(filename);
						m_SimPaused = false;
					}
				}
			}

			_ROW_END_;
		}

		_END_TABLE_;
	}


	if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
	{
		_BEGIN_TABLE_;
		{
			static int is2DMode = (int)m_MouseDragger.GetIs2DMode();
			static int isTangents = (int)m_MouseDragger.GetIsTangents();



			_ROW_START_("Drag Axis");
			ImGui::Combo("##dragaxis", &is2DMode, "XYZ\0XY");
			m_MouseDragger.SetIs2DMode(is2DMode ? true : false);
			_ROW_END_;

			_ROW_START_("Drag Type");
			ImGui::Combo("##dragtype", &isTangents, "Vertices\0Tangents");
			m_MouseDragger.SetIsTangents(isTangents ? true : false);
			_ROW_END_;

			
			_ROW_START_("Render Type");
			ImGui::Combo("##rendertype", (int*)&m_Sim->Renderer()->GetRenderMode(), "Stress 1000:1\0Strain 1:1\0Form Function\0Normals");
			_ROW_END_;

			_ROW_START_("Extra Information");
			ImGui::Combo("##extrainfo", (int*)&m_Sim->Renderer()->GetRenderExtraInfo(), "None\0Rotations\0Stress Vector");
			_ROW_END_;

			_ROW_START_("Fill Mode");
			static int render_type = m_Sim->Renderer()->GetRenderType();
			if (render_type > 1) render_type = 2;
			ImGui::Combo("##fillmode", &render_type, "Points\0Lines\0Filled");
			m_Sim->Renderer()->SetRenderType(render_type > 1 ? GL_TRIANGLES : render_type);
			_ROW_END_;

			_ROW_START_("Lighting");
			ImGui::Checkbox("##lightingenabled", &m_Sim->Renderer()->GetLightingEnabled());
			_ROW_END_;

			_ROW_START_("Control Points");
			ImGui::Checkbox("##controlpoints", &m_MouseDragger.GetDrawControlPoints());
			_ROW_END_;

		}
		_END_TABLE_;
	}
	ImGui::PopStyleVar(1);
	ImGui::End();
}

void MyScene::HandleSimulationOptions()
{
	const Vector4 status_colors[]{
		Vector4(1.f, 0.9f, 0.9f, 1.f),
		Vector4(1.f, 1.0f, 1.f, 1.f),
		Vector4(1.f, 0.5f, 0.5f, 1.f),
		Vector4(0.4f, 1.0f, 0.4f, 1.f),
	};
	const Vector4 status_colors_switches[]{
		Vector4(1.f, 1.0f, 0.6f, 1.f),
		Vector4(0.6f, 1.0f, 1.0f, 1.f),
		Vector4(1.0f, 0.6f, 1.0f, 1.f),
		Vector4(0.3f, 1.0f, 0.8f, 1.f),
	};

	//NCLDebug::AddStatusEntry(status_colors[1], "Toggle HUD Elements: %d/%d [ctrl + H]", m_HudVisibility + 1, MAX_HUD_VISIBILITY_TYPES);
//	if (m_HudVisibility < 3)
	{
		int n_steps = (floor)(m_SimTimestep / m_Sim->Integrator()->GetSubTimestep());

		float total_time = (m_SimPaused) ? 0.0f : m_Sim->Simulation()->GetTotalTimer().GetTimedMilliSeconds();
		NCLDebug::AddStatusEntry(status_colors[1], "No. FE Elements    : %d", m_Sim->BaseConfig().Triangles.size());
		NCLDebug::AddStatusEntry(status_colors[1], "Time Step          : %fms  (~%d updates per render)", m_Sim->Integrator()->GetSubTimestep(), n_steps);
		NCLDebug::AddStatusEntry(status_colors[1], "Cloth Update       : %5.2fms", total_time);
		NCLDebug::AddStatusEntry(status_colors[0], "");
		NCLDebug::AddStatusEntry(status_colors[0], "    - Step Simulation Once [R]");
		NCLDebug::AddStatusEntry(status_colors[m_SimPaused ? 3 : 2], "    - Toggle Simulation - %s [G]", m_SimPaused ? "Paused" : "Running");
		NCLDebug::AddStatusEntry(status_colors_switches[m_MouseDragger.GetIs2DMode() ? 2 : 1], "    - Drag Mode: %s [ctrl + E]", m_MouseDragger.GetIs2DMode() ? "2D" : "3D");
		NCLDebug::AddStatusEntry(status_colors_switches[m_MouseDragger.GetIsTangents() ? 2 : 1], "    - Drag Mode: %s [ctrl + T]", m_MouseDragger.GetIsTangents() ? "TANGENTS" : "VERTICES");
		NCLDebug::AddStatusEntry(status_colors[0], "");
		NCLDebug::AddStatusEntry(status_colors[0], "    - Scene: %d / %d    [ctrl + space]", visual_subdivisions, 2);
		NCLDebug::AddStatusEntry(status_colors[0], "    - Reset Simulation [ctrl + R]");
		NCLDebug::AddStatusEntry(status_colors[0], "");

		std::string render_colour = "UNKNOWN";
		switch (m_Sim->Renderer()->GetRenderMode())
		{
		case Sim_RenderMode_Stress:
			render_colour = "STRESS 1000:1";
			break;
		case Sim_RenderMode_Strain:
			render_colour = "STRAIN 1:1";
			break;
		case Sim_RenderMode_Vertices:
			render_colour = "TRIANGLES";
			break;
		case Sim_RenderMode_Normals:
			render_colour = "NORMALS";
			break;
		default:
			render_colour = "UNKNOWN";
			break;
		}

		std::string render_extra = "UNKNOWN";
		switch (m_Sim->Renderer()->GetRenderExtraInfo())
		{
		case Sim_RenderExtraInfo_None:
			render_extra = "TRIANGLES";
			break;
		case Sim_RenderExtraInfo_Rotations:
			render_extra = "ROTATIONS";
			break;
		case Sim_RenderExtraInfo_StressVector:
			render_extra = "STRESS_VECTOR";
			break;
		default:
			render_extra = "UNKNOWN";
			break;
		}


		NCLDebug::AddStatusEntry(status_colors_switches[m_Sim->Renderer()->GetRenderMode()], "    - Render Colouring: %s [ctrl + X]", render_colour.c_str());
		NCLDebug::AddStatusEntry(status_colors_switches[m_Sim->Renderer()->GetRenderType() == GL_TRIANGLES ? 0 : 1], "    - Render Type     : %s [ctrl + J]", m_Sim->Renderer()->GetRenderType() == GL_TRIANGLES ? "TRIANGLES" : "LINES");
		NCLDebug::AddStatusEntry(status_colors_switches[m_Sim->Renderer()->GetRenderExtraInfo()], "    - Render Extra: %s [ctrl + Z]", render_extra.c_str());
		NCLDebug::AddStatusEntry(status_colors_switches[m_Sim->Renderer()->GetLightingEnabled() ? 0 : 1], "    - Render Lighting: %s [ctrl + L]", m_Sim->Renderer()->GetLightingEnabled() ? "ENABLED" : "DISABLED");


		NCLDebug::AddStatusEntry(status_colors[0], "");

		if (m_SimPaused)
		{
			NCLDebug::AddStatusEntry(status_colors[3], "    - Start Simulation with Recording [ctrl + P]");
		}
	}

	if (Input::IsKeyToggled(GLFW_KEY_G))
	{
		m_SimPaused = !m_SimPaused;

	}

	if (Input::IsKeyToggled(GLFW_KEY_R))
	{
		for (int i = 0; i < 5; ++i)
			m_Sim->Integrator()->UpdateSimulation(m_Sim->Integrator()->GetSubTimestep());
	}


	if (Input::IsKeyModifierActive(KeyModifier_Control))
	{
		if (Input::IsKeyToggled(GLFW_KEY_X))
		{
			m_Sim->Renderer()->SetRenderMode((Sim_RenderMode)((m_Sim->Renderer()->GetRenderMode() + 1) % 4));
		}
		if (Input::IsKeyToggled(GLFW_KEY_Z))
		{
			m_Sim->Renderer()->SetRenderExtraInfo((Sim_RenderExtraInfo)((m_Sim->Renderer()->GetRenderExtraInfo() + 1) % 3));
		}
		if (Input::IsKeyToggled(GLFW_KEY_J))
		{
			m_Sim->Renderer()->ToggleRenderType();
		}
		if (Input::IsKeyToggled(GLFW_KEY_L))
		{
			m_Sim->Renderer()->SetLightingEnabled(!m_Sim->Renderer()->GetLightingEnabled());
		}
		if (Input::IsKeyToggled(GLFW_KEY_E))
		{
			m_MouseDragger.SetIs2DMode(!m_MouseDragger.GetIs2DMode());
		}
		if (Input::IsKeyToggled(GLFW_KEY_T))
		{
			m_MouseDragger.SetIsTangents(!m_MouseDragger.GetIsTangents());
		}
		if (Input::IsKeyToggled(GLFW_KEY_SPACE))
		{
			m_SceneGridSize = (m_SceneGridSize + 1) % 4;
			SetSimulationSubdivisions();
		}

		if (Input::IsKeyToggled(GLFW_KEY_R))
		{
			SetSimulationSubdivisions();
		}

		if (Input::IsKeyToggled(GLFW_KEY_C))
		{
			LoadCameraData();
			NCLDebug::Log(Vector3(0.0f, 0.0f, 0.0f), "Loaded Camera Location");
		}
		if (Input::IsKeyToggled(GLFW_KEY_S))
		{
			SaveCameraData();
			NCLDebug::Log(Vector3(1.0f, 0.0f, 0.0f), "Saved Camera Location");
		}

		//if (Input::IsKeyToggled(GLFW_KEY_H))
		//{
		//	m_HudVisibility = (m_HudVisibility + 1) % MAX_HUD_VISIBILITY_TYPES;
		//	m_GraphObject->SetVisibility(m_HudVisibility < 2);
		//	m_GraphObjectSolver->SetVisibility(m_HudVisibility < 1);
		//}
	}
}

void MyScene::UpdateSimulationGraphs()
{
	if (!m_SimPaused)
	{
		m_GraphObject->UpdateYScale(1.0f / 60.0f);
		m_ClothUpdateMs = m_SimTimestep;
		float val = 0.f;
		for (int i = 0; i < m_Sim->Simulation()->GetNumSubProfilers(); ++i)
		{
			val += m_Sim->Simulation()->GetSubProfiler(i).GetTimedMilliSeconds();
			m_GraphObject->AddDataEntry(i, val);
		}


		if (m_Sim->Simulation()->Solver())
		{
			m_GraphObjectSolver->UpdateYScale(1.0f / 60.0f);
			float s_init_time = m_Sim->Simulation()->Solver()->m_ProfilingInitialization.GetTimedMilliSeconds();
			float s_upper_time = m_Sim->Simulation()->Solver()->m_ProfilingUpper.GetTimedMilliSeconds();
			float s_lower_time = m_Sim->Simulation()->Solver()->m_ProfilingLower.GetTimedMilliSeconds();

			val = s_init_time; m_GraphObjectSolver->AddDataEntry(0, val);
			val += s_upper_time; m_GraphObjectSolver->AddDataEntry(1, val);
			val += s_lower_time;  m_GraphObjectSolver->AddDataEntry(2, val);
		}

		//NCLDebug::Log(Vector3(1.0f, 1.0f, 1.0f), "Solver Time: %5.2fms\t Iterations: %5.2f", s_total_time, m_ClothRenderer->m_Cloth->m_Solver.GetAverageIterations());
		//NCLDebug::Log(Vector3(1.0f, 1.0f, 1.0f), "KMatrix Time: %5.2fms", matricies_time);
	}
}





void MyScene::SaveCameraData()
{
	ofstream myfile;
	myfile.open("screen_info.txt");
	if (myfile.is_open())
	{
		Vector3 pos = SceneManager::Instance()->GetCamera()->GetPosition();
		float pitch = SceneManager::Instance()->GetCamera()->GetPitch();
		float yaw = SceneManager::Instance()->GetCamera()->GetYaw();

		int width, height;
		Window::GetRenderDimensions(&width, &height);

		myfile << width << "\n";
		myfile << height << "\n";
		myfile << pos.x << "\n";
		myfile << pos.y << "\n";
		myfile << pos.z << "\n";
		myfile << pitch << "\n";
		myfile << yaw;

		myfile.close();
	}
}

void MyScene::LoadCameraData()
{
	ifstream myfile;
	myfile.open("screen_info.txt");
	if (myfile.is_open())
	{
		Vector3 pos;
		float pitch, yaw;

		int width, height;
		myfile >> width;
		myfile >> height;

		Window::SetRenderDimensions(width, height);

		myfile >> pos.x;
		myfile >> pos.y;
		myfile >> pos.z;
		myfile >> pitch;
		myfile >> yaw;

		SceneManager::Instance()->GetCamera()->SetPosition(pos);
		SceneManager::Instance()->GetCamera()->SetPitch(pitch);
		SceneManager::Instance()->GetCamera()->SetYaw(yaw);
		myfile.close();
	}
}