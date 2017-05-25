#include "Mouse_Dragger.h"
#include <glcore\NCLDebug.h>
#include <glcore\Input.h>
#include <glcore\Window.h>
#include <glcore\imgui_wrapper.h>

Mouse_Dragger::Mouse_Dragger()
	: m_Sim(NULL)
{

	m_DragTangents = true;
	SetIsTangents(false);
	Input::AddMouseListener(this);
}

Mouse_Dragger::~Mouse_Dragger()
{
	Input::RemoveMouseListener(this);
	m_Sim = NULL;
}

void Mouse_Dragger::SetIsTangents(bool mode)
{ 
	if (mode != m_DragTangents)
	{
		m_DragTangents = mode;

		if (m_IsDragging)
		{
			m_Sim->Simulation()->SetIsStatic(m_DragIdx, m_IsOrigStatic);
			m_IsDragging = false;
			m_HoverIdx = -1;
		}
		
		using namespace std::placeholders;
		if (m_DragTangents)
		{
			IsMouseOverPoint = std::bind(&Mouse_Dragger::IsMouseOverPoint_Tangent, this, _1, _2, _3);
			UpdatePoint = std::bind(&Mouse_Dragger::UpdatePoint_Tangent, this, _1, _2);
		}
		else
		{
			IsMouseOverPoint = std::bind(&Mouse_Dragger::IsMouseOverPoint_Position, this, _1, _2, _3);
			UpdatePoint = std::bind(&Mouse_Dragger::UpdatePoint_Position, this, _1, _2);
		}
	}
}

void Mouse_Dragger::UpdateTangentDescriptors()
{
	if (!m_Sim)
		return;

	unsigned int num_phyxels = m_Sim->BaseConfig().NumVertices;

	uint num_tangents = m_Sim->BaseConfig().NumTangents;

	m_TangentsDescriptors.clear();

	auto add_tangent = [&](const FETriangle& tri, uint tan_idx, uint vert_idx)
	{
		uint tidx = tri.tangents[tan_idx];

		Vector3 tan = m_Sim->Integrator()->X()[tidx + num_phyxels] * tri.tan_multipliers[tan_idx];
		Vector3 pos = m_Sim->Integrator()->X()[tri.phyxels[vert_idx]];

		Mouse_Dragger_TangentDescriptor desc;
		desc.phyxel_idx = tri.phyxels[vert_idx];
		desc.tangent_idx = tri.tangents[tan_idx] + num_phyxels;
		desc.tangent_mult = tri.tan_multipliers[tan_idx];
		desc.tangent_position = m_Sim->Integrator()->X()[tri.phyxels[vert_idx]] + (m_Sim->Integrator()->X()[desc.tangent_idx] * desc.tangent_mult) * 0.25f;

		m_TangentsDescriptors.push_back(desc);
	};

	for (auto& tri : m_Sim->BaseConfig().Triangles)
	{
		add_tangent(tri, 0, 0);
		add_tangent(tri, 1, 0);
		add_tangent(tri, 2, 1);
		add_tangent(tri, 3, 1);
		add_tangent(tri, 4, 2);
		add_tangent(tri, 5, 2);

		add_tangent(tri, 6, 3);
		add_tangent(tri, 7, 4);
		add_tangent(tri, 8, 5);
	}
	
}

void Mouse_Dragger::RenderDragables()
{
	if (!m_Sim || !m_DrawControlPoints)
		return;

	unsigned int num_phyxels = m_Sim->BaseConfig().NumVertices;
	if (m_DragTangents)
	{
		for (uint i = 0; i < m_TangentsDescriptors.size(); ++i)
		{
			auto& dtan = m_TangentsDescriptors[i];

			Vector3 tan = m_Sim->Integrator()->X()[dtan.tangent_idx] * dtan.tangent_mult;
			Vector3 pos = m_Sim->Integrator()->X()[dtan.phyxel_idx];
			dtan.tangent_position = pos + tan * 0.25f;

			NCLDebug::DrawThickLineNDT(pos, dtan.tangent_position, 0.02f, Vector4(1.0f, 0.f, 1.0f, 0.3f));

			if (i == m_HoverIdx)
			{
				NCLDebug::DrawPointNDT(dtan.tangent_position, 0.02f, Vector4(1.f, 0.8f, 0.5f, 1.f));
			}
			else
			{
				if (m_Sim->Simulation()->GetIsStatic(dtan.tangent_idx))
					NCLDebug::DrawPointNDT(dtan.tangent_position, 0.02f, Vector4(0.0f, 0.0f, 0.0f, 0.9f));
				else
					NCLDebug::DrawPointNDT(dtan.tangent_position, 0.02f, Vector4(0.0f, 0.0f, 0.0f, 0.3f));
			}
		}
	}
	else
	{		
		for (unsigned int i = 0; i < num_phyxels; ++i)
		{
			if (i == m_HoverIdx)
			{
				NCLDebug::DrawPointNDT(m_Sim->Integrator()->X()[i], 0.02f, Vector4(1.f, 0.8f, 0.5f, 1.f));
			}
			else
			{
				if (m_Sim->Simulation()->GetIsStatic(i))
					NCLDebug::DrawPointNDT(m_Sim->Integrator()->X()[i], 0.02f, Vector4(0.0f, 0.0f, 0.0f, 0.9f));
				else
					NCLDebug::DrawPointNDT(m_Sim->Integrator()->X()[i], 0.02f, Vector4(0.0f, 0.0f, 0.0f, 0.3f));
			}
		}
	}
}


bool Mouse_Dragger::OnUpdateMouseInput(bool hasFocus)
{
	if (!m_Sim || !m_DrawControlPoints)
		return false;


	m_IsDragging = hasFocus;
	bool isHovering = m_IsDragging;

	Ray ray = Input::GetMouseWSRay();

	uint point_idx = m_DragIdx;
	float distance = m_MouseRayDepth;
	if (!isHovering)
	{
		isHovering = IsMouseOverPoint(ray, &point_idx, &distance);
	}

	if (isHovering)
	{
		m_HoverIdx = point_idx;
		uint idx = (m_DragTangents) ? m_TangentsDescriptors[point_idx].tangent_idx : point_idx;

		ImGui_Wrapper::SetCursor(Cursor_Move);

		if (Input::IsMouseToggled(GLFW_MOUSE_BUTTON_RIGHT))
		{
			bool is_static = !m_Sim->Simulation()->GetIsStatic(idx);
			m_Sim->Simulation()->SetIsStatic(idx, is_static);
			m_Sim->Integrator()->DxDt()[idx] = Vector3(0.f, 0.f, 0.f);
		}

		if (Input::IsMouseDown(GLFW_MOUSE_BUTTON_LEFT))
		{
			Vector2 scroll = Input::GetMouseScroll();
			m_MouseRayDepth = distance * 1.f + scroll.y * 0.02f;

			Vector3 new_pos = ray.GetPointOnRay(m_MouseRayDepth);
			UpdatePoint(point_idx, new_pos);
			m_Sim->Renderer()->BuildVertexBuffer(m_Sim->Integrator());
		}
		else if (m_IsDragging)
		{
			m_IsDragging = false;
			m_Sim->Simulation()->SetIsStatic(idx, m_IsOrigStatic);
		}
	}
	else
	{
		m_HoverIdx = -1;
	}
	
	return m_IsDragging;
}

bool Mouse_Dragger::IsMouseOverPoint_Position(const Ray& ray, uint* out_idx, float* out_dist)
{
	float best_dist = FLT_MAX, current_dist;
	uint best_idx = UINT_MAX;

	uint numverts = m_Sim->BaseConfig().NumVertices;
	for (uint i = 0; i < numverts; ++i)
	{	
		if (ray.intersectsSphere(m_Sim->Integrator()->X()[i], 0.02f, &current_dist))
		{
			if (current_dist < best_dist)
			{
				best_dist = current_dist;
				best_idx = i;
			}
		}
	}

	if (out_idx) *out_idx = best_idx;
	if (out_dist) *out_dist = best_dist;

	return (best_idx < UINT_MAX);
}

void Mouse_Dragger::UpdatePoint_Position(uint idx, const Vector3& position)
{
	if (m_IsDragging)
	{
		Vector3 new_pos = position + m_ClickOffset;
		
		if (m_Drag2D)
			new_pos.z = m_Sim->Integrator()->X()[idx].z;

		m_Sim->Integrator()->X()[idx] = new_pos;
	}
	else if (!Input::IsMouseHeld(GLFW_MOUSE_BUTTON_LEFT))
	{
		m_IsDragging = true;
		m_DragIdx = idx;
		
		m_IsOrigStatic = m_Sim->Simulation()->GetIsStatic(idx);
		m_ClickOffset = m_Sim->Integrator()->X()[idx] - position;
		m_Sim->Simulation()->SetIsStatic(idx, true);
	}
}

bool Mouse_Dragger::IsMouseOverPoint_Tangent(const Ray& ray, uint* out_idx, float* out_dist)
{
	float best_dist = FLT_MAX, current_dist;
	uint best_idx = UINT_MAX;

	uint numverts = m_Sim->BaseConfig().NumVertices;
	for (uint i = 0; i < m_TangentsDescriptors.size(); ++i)
	{
		if (ray.intersectsSphere(m_TangentsDescriptors[i].tangent_position, 0.02f, &current_dist))
		{
			if (current_dist < best_dist)
			{
				best_dist = current_dist;
				best_idx = i;
			}
		}
	}

	if (out_idx) *out_idx = best_idx;
	if (out_dist) *out_dist = best_dist;

	return (best_idx < UINT_MAX);
}

void Mouse_Dragger::UpdatePoint_Tangent(uint idx, const Vector3& position)
{
	auto& tangent = m_TangentsDescriptors[idx];

	if (m_IsDragging)
	{
		Vector3 new_tan = (position + m_ClickOffset - m_Sim->Integrator()->X()[tangent.phyxel_idx]) * 4.f * tangent.tangent_mult;

		if (m_Drag2D)
			new_tan.z = m_Sim->Integrator()->X()[tangent.tangent_idx].z;
		
		m_Sim->Integrator()->X()[tangent.tangent_idx] = new_tan;
	}
	else if (!Input::IsMouseHeld(GLFW_MOUSE_BUTTON_LEFT))
	{
		m_IsDragging = true;
		m_DragIdx = idx;

		m_IsOrigStatic = m_Sim->Simulation()->GetIsStatic(tangent.tangent_idx);
		m_ClickOffset = tangent.tangent_position - position;
		m_Sim->Simulation()->SetIsStatic(tangent.tangent_idx, true);
	}
}

bool Mouse_Dragger::HandleMouseInputTangents(Sim_Manager* sim, const Matrix4& projView)
{

	Vector2 mousePos = Input::GetMousePos();
	int width, height;
	Window::GetRenderDimensions(&width, &height);
	Vector2 windowTrans = Vector2((float)width, (float)height);
	mousePos.y = windowTrans.y - mousePos.y;


	const float epsilon = 10.f * 10.f;

	if (!m_IsDragging || !Input::IsMouseDown(GLFW_MOUSE_BUTTON_LEFT))
	{
		if (m_IsDragging && !m_IsOrigStatic)
		{
			sim->Simulation()->SetIsStatic(m_TangentsDescriptors[m_DragIdx].tangent_idx, false);
		}

		m_IsDragging = false;

		int best = -1;
		float best_depth = FLT_MAX;
		for (uint i = 0; i < m_TangentsDescriptors.size(); ++i)
		{
			Vector3 cs = projView * m_TangentsDescriptors[i].tangent_position;
			Vector2 wins = (Vector2(cs.x, cs.y) * 0.5f + 0.5f) * windowTrans;
			if ((mousePos - wins).LengthSquared() < epsilon)
			{
				if (cs.z < best_depth)
				{
					best = i;
					best_depth = cs.z;
				}
			}
		}

		if (best >= 0)
		{
			if (Input::IsMouseToggled(GLFW_MOUSE_BUTTON_RIGHT))
			{
				bool is_static = !sim->Simulation()->GetIsStatic(m_TangentsDescriptors[best].tangent_idx);
				sim->Simulation()->SetIsStatic(m_TangentsDescriptors[best].tangent_idx, is_static);
				sim->Integrator()->DxDt()[m_TangentsDescriptors[best].tangent_idx] = Vector3(0.f, 0.f, 0.f);
			}
			else if (Input::IsMouseToggled(GLFW_MOUSE_BUTTON_LEFT))
			{
				m_IsDragging = true;
			//	m_DragZDepth = best_depth;
				m_DragIdx = best;
				m_IsOrigStatic = sim->Simulation()->GetIsStatic(m_TangentsDescriptors[m_DragIdx].tangent_idx);
				sim->Integrator()->DxDt()[m_TangentsDescriptors[m_DragIdx].tangent_idx] = Vector3(0.f, 0.f, 0.f);

				Vector3 cs = projView * m_TangentsDescriptors[m_DragIdx].tangent_position;
				Vector2 wins = (Vector2(cs.x, cs.y) * 0.5f + 0.5f) * windowTrans;
				//m_ClickOffset = mousePos - wins;
			}
			NCLDebug::DrawPoint(m_TangentsDescriptors[best].tangent_position, 0.03f, Vector4(1.f, 1.f, 1.f, 1.f));
		}

		return false;
	}
	else
	{
		Vector2 ms_cs = (mousePos /*- m_ClickOffset*/) / windowTrans * 2.f - 1.f;
		Vector3 cs = Vector3(ms_cs.x, ms_cs.y, 0.f);// m_DragZDepth);
		Vector3 new_pos = Matrix4::Inverse(projView) * cs;

		Vector3 new_tan = (new_pos - sim->Integrator()->X()[m_TangentsDescriptors[m_DragIdx].phyxel_idx]) * 4.f;
		new_tan = new_tan * m_TangentsDescriptors[m_DragIdx].tangent_mult;

		unsigned int tan_idx = m_TangentsDescriptors[m_DragIdx].tangent_idx;

		sim->Integrator()->X()[tan_idx].x = new_tan.x;
		sim->Integrator()->X()[tan_idx].y = new_tan.y;

		if (!m_Drag2D)
			sim->Integrator()->X()[tan_idx].z = new_tan.z;

		if (Input::IsMouseToggled(GLFW_MOUSE_BUTTON_RIGHT))
		{
			m_IsOrigStatic = !m_IsOrigStatic;
			sim->Simulation()->SetIsStatic(m_TangentsDescriptors[m_DragIdx].tangent_idx, m_IsOrigStatic);
			sim->Integrator()->DxDt()[m_TangentsDescriptors[m_DragIdx].tangent_idx] = Vector3(0.f, 0.f, 0.f);
		}

		NCLDebug::DrawPoint(m_TangentsDescriptors[m_DragIdx].tangent_position, 0.03f, Vector4(1.f, 0.8f, 0.5f, 1.f));
	}

	return true;
}