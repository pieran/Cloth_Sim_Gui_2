#include "Sim_Renderer.h"
#include <glcore\NCLDebug.h>
#include <glcore\Scene.h>
#include <glcore\SceneManager.h>
#include "utils.h"

Sim_Renderer::Sim_Renderer()
{
#if _DEBUG
	m_RenderMode = Sim_RenderMode_Vertices;
	m_LightingEnabled = false;
#else
	m_RenderMode = Sim_RenderMode_Stress;
	m_LightingEnabled = true;
#endif

	m_RenderExtraInfo = Sim_RenderExtraInfo_None;
	

	m_RenderSubdivisions = 17;

	m_AllVertices = NULL;
	m_VertexArrayObject = NULL;
	m_LineIndexBuffer = NULL;	
	m_TriIndexBuffer = NULL;
	m_VertexBuffer = NULL;

	m_RenderType = GL_TRIANGLES;

	m_RenderShader = new Shader(SHADERDIR"ClothSimple.vert", SHADERDIR"ClothSimple.frag");
	if (!m_RenderShader->LinkProgram())
		printf("ERROR: CLOTH SHADER COULD NOT COMPILE!");

	m_RenderShaderLighting = new Shader(SHADERDIR"ClothLighting.vert", SHADERDIR"ClothLighting.frag");
	if (!m_RenderShaderLighting->LinkProgram())
		printf("ERROR: CLOTH SHADER COULD NOT COMPILE!");
}

void Sim_Renderer::SetSimulation(Sim_Rendererable* sim)
{
	m_Sim = sim;
}

Sim_Renderer::~Sim_Renderer()
{
	if (m_AllVertices)
	{
		delete[] m_AllVertices;
		m_AllVertices = NULL;
	}

	if (m_VertexArrayObject)
	{
		glDeleteBuffers(1, &m_LineIndexBuffer);
		glDeleteBuffers(1, &m_TriIndexBuffer);
		glDeleteBuffers(1, &m_VertexBuffer);
		glDeleteVertexArrays(1, &m_VertexArrayObject);
	}

	if (m_RenderShaderLighting)
	{
		delete m_RenderShader;
		delete m_RenderShaderLighting;
		m_RenderShaderLighting = NULL;
	}
}

int Sim_Renderer::GetOffset(int triidx, int ix, int iy)
{
	int tidx = m_VertsPerTri * triidx + iy;

	if (ix > 0) 
		tidx += (int)(ix * (m_RenderSubdivisions - (ix - 1) * 0.5f));

	return tidx;
}

void Sim_Renderer::AllocateBuffers(Vector3* positions)
{
	if (m_AllVertices) delete[] m_AllVertices;

	m_AllocatedTris = m_Sim->GetNumTris();
	int half_upper = (int)ceilf((m_RenderSubdivisions) / 2.f);
	m_VertsPerTri = m_RenderSubdivisions * half_upper;

	m_NumLineIndices = m_AllocatedTris * (m_RenderSubdivisions * (m_RenderSubdivisions - 1)) * 3;
	m_NumTriIndices = m_AllocatedTris * (m_RenderSubdivisions - 1) * (m_RenderSubdivisions - 1) * 3;
	m_AllVertices = new Sim_RenderVertex[m_AllocatedTris * m_VertsPerTri];


	int* lineIndices = new int[m_NumLineIndices];
	int* triIndices = new int[m_NumTriIndices];

	//Generate Line Indices
	int l_index = 0;
	for (int tri_idx = 0; tri_idx < m_AllocatedTris; ++tri_idx)
	{
		for (int ix = 1; ix < m_RenderSubdivisions; ++ix)
		{
			for (int iy = 0; iy < (m_RenderSubdivisions - ix); ++iy)
			{
				lineIndices[l_index++] = GetOffset(tri_idx, ix, iy);
				lineIndices[l_index++] = GetOffset(tri_idx, ix - 1, iy);
			}
		}

		for (int ix = 0; ix < m_RenderSubdivisions; ++ix)
		{
			for (int iy = 0; iy < (m_RenderSubdivisions - ix - 1); ++iy)
			{
				lineIndices[l_index++] = GetOffset(tri_idx, ix, iy);
				lineIndices[l_index++] = GetOffset(tri_idx, ix, iy + 1);
			}
		}

		for (int ix = 1; ix < m_RenderSubdivisions; ++ix)
		{
			for (int iy = 0; iy < (m_RenderSubdivisions - ix); ++iy)
			{
				lineIndices[l_index++] = GetOffset(tri_idx, ix, iy);
				lineIndices[l_index++] = GetOffset(tri_idx, ix - 1, iy + 1);
			}
		}
	}


	//Generate Tri Indices
	l_index = 0;
	for (int tri_idx = 0; tri_idx < m_AllocatedTris; ++tri_idx)
	{
		for (int ix = 0; ix < m_RenderSubdivisions; ++ix)
		{
			for (int iy = 0; iy < (m_RenderSubdivisions - ix); ++iy)
			{				
				if (ix < m_RenderSubdivisions - 1
					&& iy < (m_RenderSubdivisions - ix - 1))
				{
					triIndices[l_index++] = GetOffset(tri_idx, ix, iy);
					triIndices[l_index++] = GetOffset(tri_idx, ix, iy + 1);
					triIndices[l_index++] = GetOffset(tri_idx, ix + 1, iy);
				}
				
				if (ix > 0 && iy > 0)
				{
					triIndices[l_index++] = GetOffset(tri_idx, ix, iy);
					triIndices[l_index++] = GetOffset(tri_idx, ix , iy - 1);
					triIndices[l_index++] = GetOffset(tri_idx, ix - 1, iy);
				}
			}
		}
	}

	//Copy buffers to gfx card
	if (!m_VertexArrayObject) glGenVertexArrays(1, &m_VertexArrayObject);
	if (!m_LineIndexBuffer) glGenBuffers(1, &m_LineIndexBuffer);	
	if (!m_TriIndexBuffer) glGenBuffers(1, &m_TriIndexBuffer);
	if (!m_VertexBuffer) glGenBuffers(1, &m_VertexBuffer);


	glBindVertexArray(m_VertexArrayObject);

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, m_AllocatedTris * m_VertsPerTri * sizeof(Sim_RenderVertex), m_AllVertices, GL_STREAM_DRAW);
	glVertexAttribPointer(VERTEX_BUFFER, 3, GL_FLOAT, GL_FALSE, sizeof(Sim_RenderVertex), 0);
	glVertexAttribPointer(COLOUR_BUFFER, 3, GL_FLOAT, GL_FALSE, sizeof(Sim_RenderVertex), (void*)sizeof(Vector3));
	glVertexAttribPointer(NORMAL_BUFFER, 3, GL_FLOAT, GL_TRUE, sizeof(Sim_RenderVertex), (void*)(2 * sizeof(Vector3)));
	glEnableVertexAttribArray(VERTEX_BUFFER);
	glEnableVertexAttribArray(COLOUR_BUFFER);
	glEnableVertexAttribArray(NORMAL_BUFFER);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_LineIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_NumLineIndices * sizeof(int), lineIndices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_TriIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_NumTriIndices * sizeof(int), triIndices, GL_STATIC_DRAW);

	glBindVertexArray(0);

	delete[] lineIndices;
	delete[] triIndices;
}

void Sim_Renderer::Render()
{
	SceneManager* ctxt = SceneManager::Instance();

	GLint old_pid; glGetIntegerv(GL_CURRENT_PROGRAM, &old_pid);
	
	if (m_RenderExtraInfo != Sim_RenderExtraInfo_None) glDepthMask(GL_FALSE);

	GLint pid = 0;
	if (m_LightingEnabled)
	{
		int uidx;
		pid = m_RenderShaderLighting->GetProgram();
		glUseProgram(pid);

		uidx = glGetUniformLocation(pid, "viewProjMatrix");
		glUniformMatrix4fv(uidx, 1, GL_FALSE, &ctxt->GetProjViewMatrix().values[0]);

		uidx = glGetUniformLocation(pid, "ambientColour");
		glUniform3fv(uidx, 1, &ctxt->GetAmbientColor().x);

		uidx = glGetUniformLocation(pid, "invLightDir");
		glUniform3fv(uidx, 1, &ctxt->GetInverseLightDirection().x);

		uidx = glGetUniformLocation(pid, "cameraPos");
		Vector3 campos = ctxt->GetCamera()->GetPosition();
		glUniform3fv(uidx, 1, &campos.x);

		uidx = glGetUniformLocation(pid, "specularIntensity");
		glUniform1f(uidx, ctxt->GetSpecularIntensity());
	}
	else
	{
		pid = m_RenderShader->GetProgram();
		glUseProgram(pid);
		int uidx = glGetUniformLocation(pid, "viewProjMatrix");
		glUniformMatrix4fv(uidx, 1, GL_FALSE, &ctxt->GetProjViewMatrix().values[0]);
	}
	
	glBindVertexArray(m_VertexArrayObject);
	
	if (m_RenderType == GL_TRIANGLES || m_RenderType == GL_POINTS)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_TriIndexBuffer);
		int uidx = glGetUniformLocation(pid, "normal_mult");

		if (m_RenderType == GL_POINTS)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			glPointSize(12.f);
		}
		glCullFace(GL_FRONT);
		if (uidx > -1) glUniform1f(uidx, 1.f);
		glDrawElements(GL_TRIANGLES, m_NumTriIndices, GL_UNSIGNED_INT, 0);
		glCullFace(GL_BACK);

		if (uidx > -1) glUniform1f(uidx, -1.f);
		glDrawElements(GL_TRIANGLES, m_NumTriIndices, GL_UNSIGNED_INT, 0);

		if (m_RenderType == GL_POINTS)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_LineIndexBuffer);
		glDrawElements(GL_LINES, m_NumLineIndices, GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);

	if (m_RenderExtraInfo != Sim_RenderExtraInfo_None) glDepthMask(GL_TRUE);
	
	glUseProgram(old_pid);
}

void Sim_Renderer::BuildVertexBuffer(Sim_Integrator* integrator)
{
	Vector3* positions = integrator->X();

	if (m_AllocatedTris != m_Sim->GetNumTris())
		AllocateBuffers(positions);


	const float step_interval = 1.f / float(m_RenderSubdivisions-1);

	Vector3 gp;
	Matrix3 rot;
	for (int tri_idx = 0; tri_idx < m_AllocatedTris; ++tri_idx)
	{
		for (int ix = 0; ix < m_RenderSubdivisions; ++ix)
		{
			for (int iy = 0; iy < (m_RenderSubdivisions - ix); ++iy)
			{
				gp.x = ix * step_interval;
				gp.y = iy * step_interval;
				gp.z = 1.f - (gp.x + gp.y);
				
				int tidx = GetOffset(tri_idx, ix, iy);

				Sim_RenderVertex& vert = m_AllVertices[tidx];
				m_Sim->GetVertexWsPos(tri_idx, gp, positions, vert.pos);
				 
				if (m_LightingEnabled || (m_RenderMode == Sim_RenderMode_Normals) || (m_RenderExtraInfo == Sim_RenderExtraInfo_Rotations))
				{
					m_Sim->GetVertexRotation(tri_idx, gp, positions, vert.pos, rot);
					
					Vector3 r_z = Vector3(rot._31, rot._32, rot._33);

					if (m_LightingEnabled) vert.normal = r_z;

					if (m_RenderExtraInfo == Sim_RenderExtraInfo_Rotations)
					{
						const float scalar = 0.02f;

						Vector3 r_x = Vector3(rot._11, rot._12, rot._13);
						Vector3 r_y = Vector3(rot._21, rot._22, rot._23);

						NCLDebug::DrawThickLine(vert.pos, vert.pos + r_x * scalar, 0.005f, Vector4(1.f, 0.f, 0.f, 1.f));
						NCLDebug::DrawThickLine(vert.pos, vert.pos + r_y * scalar, 0.005f, Vector4(0.f, 1.f, 0.f, 1.f));
						NCLDebug::DrawThickLine(vert.pos, vert.pos + r_z * scalar, 0.005f, Vector4(0.f, 0.f, 1.f, 1.f));
					}
					else if (m_RenderMode == Sim_RenderMode_Normals)
					{
						vert.col = r_z * 0.5f + 0.5f;
					}
				}
				else
					vert.normal = Vector3(0.f, 0.f, 0.f);

				if (m_RenderMode == Sim_RenderMode_Vertices)
				{
					vert.col = gp;
				}
				
				if (m_RenderMode == Sim_RenderMode_Stress  || m_RenderMode == Sim_RenderMode_Strain || (m_RenderExtraInfo == Sim_RenderExtraInfo_StressVector))
				{
					Vector3 stress, strain;
					m_Sim->GetVertexStressStrain(tri_idx, gp, positions, vert.pos, stress, strain);

					if (m_RenderMode == Sim_RenderMode_Stress)
					{
						vert.col = hsv2rgb(Vector3(stress.Length()  * 0.001f, 1.f, 1.f));
					}
					else if (m_RenderMode == Sim_RenderMode_Strain)//Strain
					{
						vert.col = hsv2rgb(Vector3(strain.Length(), 1.f, 1.f));
					}

					if (m_RenderExtraInfo == Sim_RenderExtraInfo_StressVector)
					{
						NCLDebug::DrawThickLine(vert.pos, vert.pos + stress * 0.0001f, 0.005f, Vector4(1.f, 0.f, 1.f, 1.f));
					}
				}

				if (m_RenderExtraInfo != Sim_RenderExtraInfo_None)
					vert.col = vert.col * 0.2f;
			}
		}

	}

	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, m_AllocatedTris * m_VertsPerTri * sizeof(Sim_RenderVertex), m_AllVertices, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}