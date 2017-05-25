#pragma once
#include "Sim_Renderer.h"
#include "Sim_Integrator.h"
#include "Sim_Generator.h"
#include <glcore\Object.h>
#include "mpcg.h"

enum Sim_Type
{
	Sim_Type_FE6NodedC0 = 0,
	Sim_Type_FE6NodedC1,
	Sim_Type_FE6NodedC1_v2,
	Sim_Type_PBD3NodedC0,
	Sim_Type_NULL,
	Sim_Type_UNKNOWN
};

class Sim_Simulation : public Sim_Integratable, public Sim_Rendererable
{
public:

	virtual void Initialize(const Sim_Generator_Output& configuration) = 0;

	virtual MPCG<SparseRowMatrix<Matrix3>>* Solver() = 0;

	virtual bool GetIsStatic(uint idx) = 0;
	virtual void SetIsStatic(uint idx, bool is_static) = 0;

	//Profiling
	virtual ProfilingTimer& GetTotalTimer() = 0;

	virtual int GetNumSubProfilers() = 0;
	virtual const ProfilingTimer& GetSubProfiler(int idx) = 0;
};

class Sim_Manager : public Object
{
public:
	Sim_Manager(const std::string& friendly_name);
	~Sim_Manager();

	Sim_Type GetSimType() { return m_SimType; }
	void SetSimType(Sim_Type type);

	void SetGenerator(Sim_Generator* generator);
	
	void Generate();
	void Reset();

	Sim_Renderer* Renderer()			{ return m_Renderer; }
	Sim_Integrator* Integrator()		{ return m_Integrator; }
	Sim_Simulation* Simulation()		{ return m_Simulation; }
	Sim_Generator* Generator()			{ return m_Generator; }
	Sim_Generator_Output& BaseConfig()	{ return m_BaseConfiguration; }
	
	
protected:
	virtual void OnRenderObject();				//Handles OpenGL calls to Render the object
	virtual void OnUpdateObject(float dt);		//Override to handle things like AI etc on update loop

protected:
	float           m_ElapsedTime;

	Sim_Type        m_SimType;
	Sim_Renderer*	m_Renderer;
	Sim_Integrator* m_Integrator;
	Sim_Simulation* m_Simulation;
	Sim_Generator*  m_Generator;

	Sim_Generator_Output m_BaseConfiguration;
};