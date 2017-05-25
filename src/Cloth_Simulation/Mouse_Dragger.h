#pragma once
#include "Sim_Manager.h"
#include <glcore\Input.h>

struct Mouse_Dragger_TangentDescriptor
{
	uint tangent_idx;
	uint phyxel_idx;
	float tangent_mult;

	Vector3 tangent_position; //Computed Bu RenderDragables Each Frame
};


class Mouse_Dragger : public Input_MouseListener
{
public:
	Mouse_Dragger();
	~Mouse_Dragger();

	inline void SetSimulation(Sim_Manager* sim) { m_Sim = sim; UpdateTangentDescriptors(); }

	virtual bool OnUpdateMouseInput(bool hasFocus) override;

	void UpdateTangentDescriptors();
	void RenderDragables();

	bool& GetIs2DMode() { return m_Drag2D; }
	void SetIs2DMode(bool mode) { m_Drag2D = mode; }

	bool& GetIsTangents() { return m_DragTangents; }
	void SetIsTangents(bool mode);

	bool& GetDrawControlPoints() { return m_DrawControlPoints; }
protected:
	std::function<bool(const Ray& ray, uint* best_idx, float* distance)> IsMouseOverPoint;
	std::function<void(uint idx, const Vector3 position)> UpdatePoint;


	bool IsMouseOverPoint_Position(const Ray& ray, uint* best_idx, float* distance);
	void UpdatePoint_Position(uint idx, const Vector3& position);

	bool IsMouseOverPoint_Tangent(const Ray& ray, uint* out_idx, float* out_dist);
	void UpdatePoint_Tangent(uint idx, const Vector3& position);

	bool HandleMouseInputTangents(Sim_Manager* sim, const Matrix4& projView);

	
protected:
	Sim_Manager* m_Sim;

	bool m_DrawControlPoints = true;
	bool m_IsDragging = false;
	bool m_IsOrigStatic = false;
	bool m_DragTangents = false;
	bool m_Drag2D = false;
	uint m_DragIdx = 0;
	uint m_HoverIdx = 0;
	float m_MouseRayDepth = 0.f;
	Vector3 m_ClickOffset;

	
	std::vector<Mouse_Dragger_TangentDescriptor> m_TangentsDescriptors;
};