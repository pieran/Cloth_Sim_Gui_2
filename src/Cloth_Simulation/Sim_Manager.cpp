#include "Sim_Manager.h"
#include "Sim_6NodedC0.h"
#include "Sim_6NodedC1.h"
#include "Sim_6NodedC1_v2.h"
#include "Sim_PBD.h"
#include "Generator_Square_Grid.h"

Sim_Manager::Sim_Manager(const std::string& friendly_name) 
	: Object(friendly_name)
	, m_Renderer(NULL)
	, m_Integrator(NULL)
	, m_Simulation(NULL)
{
	m_Renderer = new Sim_Renderer();
	m_Integrator = new Sim_Integrator();
	m_Generator = new Generator_Square_Grid();
	m_Generator->Generate(m_BaseConfiguration);
	m_SimType = Sim_Type_NULL;

	m_Integrator->SetOnStepCompleteCallback(std::bind(&Sim_Renderer::BuildVertexBuffer, m_Renderer, std::placeholders::_1));

	SetSimType(Sim_Type_FE6NodedC1);

	this->SetBoundingRadius(FLT_MAX);
}

Sim_Manager::~Sim_Manager()
{
	if (m_Renderer)
	{
		delete m_Renderer;
		m_Renderer = NULL;
	}
	if (m_Integrator)
	{
		delete m_Integrator;
		m_Integrator = NULL;
	}
	if (m_Simulation)
	{
		delete m_Simulation;
		m_Simulation = NULL;
	}
}

void Sim_Manager::SetSimType(Sim_Type type)
{
	if (m_Simulation != NULL)
	{
		delete m_Simulation;
		m_Simulation = NULL;

		m_SimType = Sim_Type_NULL;
	}

	switch (type)
	{
	case Sim_Type_FE6NodedC0:
		m_Simulation = new Sim_6NodedC0();
		break;

	case Sim_Type_FE6NodedC1:
		m_Simulation =  new Sim_6NodedC1();
		break;

	case Sim_Type_FE6NodedC1_v2:
		m_Simulation = new Sim_6NodedC1_v2();
		break;

	case Sim_Type_PBD3NodedC0:
		m_Simulation = new Sim_PBD();
		break;

	default:
		m_Simulation = NULL;
	}
	m_SimType = type;

	Reset();
}



void Sim_Manager::SetGenerator(Sim_Generator* generator)
{
	if (generator != NULL && generator != m_Generator)
	{
		delete m_Generator;
		m_Generator = generator;
		m_BaseConfiguration.Release();
		Generate();
	}
}

void Sim_Manager::Generate()
{
	if (m_Simulation != NULL)
	{
		m_BaseConfiguration.Release();
		m_Generator->Generate(m_BaseConfiguration);

		Reset();
	}
}

void Sim_Manager::Reset()
{
	if (m_Simulation != NULL)
	{
		m_Simulation->Initialize(m_BaseConfiguration);
		m_Integrator->Initialize(m_Simulation, m_BaseConfiguration);

		m_Renderer->SetSimulation(m_Simulation);
		m_Renderer->AllocateBuffers(m_Integrator->X());
		m_Renderer->BuildVertexBuffer(m_Integrator);
	}
}

void Sim_Manager::OnRenderObject()
{
	if (m_Simulation != NULL)
	{
		m_Renderer->Render();
	}
}

void Sim_Manager::OnUpdateObject(float dt)
{
	if (m_Simulation != NULL)
	{
		//m_Renderer->BuildVertexBuffer(m_Integrator->X());
	}
}