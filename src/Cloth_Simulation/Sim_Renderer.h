#pragma once
#include "Sim_Integrator.h"
#include <glcore\Mesh.h>
#include <glcore\Shader.h>

class Scene;

enum Sim_RenderMode : int
{
	Sim_RenderMode_Stress = 0,
	Sim_RenderMode_Strain = 1,
	Sim_RenderMode_Vertices = 2,
	Sim_RenderMode_Normals = 3
};

enum Sim_RenderExtraInfo : int
{
	Sim_RenderExtraInfo_None = 0,
	Sim_RenderExtraInfo_Rotations = 1,
	Sim_RenderExtraInfo_StressVector = 2
};

struct Sim_RenderVertex
{
	Vector3 pos;
	Vector3 col;
	Vector3 normal;
};

class Sim_Rendererable
{
public:
	Sim_Rendererable() {};
	virtual ~Sim_Rendererable() {};

public:
	virtual int GetNumTris() = 0;
	virtual void GetVertexWsPos(int triidx, const Vector3& gauss_point, const Vector3* positions, Vector3& out_pos) = 0;
	virtual void GetVertexRotation(int triidx, const Vector3& gauss_point, const Vector3* positions, const Vector3& wspos, Matrix3& out_rot) = 0;
	virtual void GetVertexStressStrain(int triidx, const Vector3& gauss_point, const Vector3* positions, const Vector3& wspos, Vector3& out_stress, Vector3& out_strain) = 0;
};

class Sim_Renderer 
{
public:
	Sim_Renderer();
	~Sim_Renderer();

	void SetSimulation(Sim_Rendererable* sim);

	void AllocateBuffers(Vector3* positions);
	void BuildVertexBuffer(Sim_Integrator* integrator);
	void Render();
	
	void ToggleRenderType()
	{
		if (m_RenderType == GL_TRIANGLES)
			m_RenderType = GL_LINES;
		else
			m_RenderType = GL_TRIANGLES;
	}

	Sim_RenderMode& GetRenderMode() { return m_RenderMode; }
	Sim_RenderExtraInfo& GetRenderExtraInfo() { return m_RenderExtraInfo; }

	GLuint GetRenderType() { return m_RenderType; }
	void SetRenderType(GLuint type) { m_RenderType = type; }
	void SetRenderExtraInfo(Sim_RenderExtraInfo mode) {
		m_RenderExtraInfo = mode;
	}

	void SetRenderMode(Sim_RenderMode mode) { m_RenderMode = mode; }

	bool& GetLightingEnabled() { return m_LightingEnabled; }
	void SetLightingEnabled(bool enabled) { m_LightingEnabled = !m_LightingEnabled; }

protected:
	int GetOffset(int triidx, int ix, int iy);

private:
	bool m_LightingEnabled;
	
	Sim_RenderExtraInfo m_RenderExtraInfo;

	Sim_RenderMode m_RenderMode;
	GLuint m_RenderType;
	int m_RenderSubdivisions;
	Sim_Rendererable* m_Sim;

	int m_AllocatedTris;
	int m_VertsPerTri;
	Sim_RenderVertex* m_AllVertices;
	
	int m_NumLineIndices;
	int m_NumTriIndices;

	Shader* m_RenderShader;
	Shader* m_RenderShaderLighting;

	GLuint m_VertexArrayObject;
	GLuint m_LineIndexBuffer;
	GLuint m_TriIndexBuffer;
	GLuint m_VertexBuffer;	
};