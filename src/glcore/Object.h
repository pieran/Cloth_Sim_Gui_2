/******************************************************************************
Class:GameObject
Implements:
Author: Rich Davison	<richard.davison4@newcastle.ac.uk> and YOU!
Description: 

This is the base class for all of the objects in your game - the
player character, enemies, pickups etc would all be derived classes of the
GameObject.

This class 'wraps up' all of the communication between the subsystems of your
game - it has a renderer component (Similar to the SceneNode you are used to
using), an optional 'physics' component (The PhysicsNode you've been introduced
to in this code download), and eventually you'll add a 'sound' component, in the
short sound workshop in semester 2.

The sub systems handle the updating of the 'sub components' of a GameEntity,
so in the Update function of this class will just be the 'gameplay' specific
type functionality (has the player picked up some health / has he shot an enemy
and so forth).


-_-_-_-_-_-_-_,------,
_-_-_-_-_-_-_-|   /\_/\   NYANYANYAN
-_-_-_-_-_-_-~|__( ^ .^) /
_-_-_-_-_-_-_-""  ""

*//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Matrix4.h"
#include <vector>

class Scene;
class PhysicsEngine;
class RenderList;
class SceneRenderer;

class Object
{
	//This are the only class that can manually set the world transform
	friend class Scene;
	friend class RenderList;
	friend class SceneRenderer;
	friend class PhysicsEngine;
	friend class ScreenPicker;

public:
	Object(const std::string& name = "");
	virtual ~Object();

//<--------- SCENE TREE ---------->
	//Get a list of all child scene-tree objects
	std::vector<Object*>& GetChildren()		{ return m_Children; }

	//Recursively search this and all children for an object with the given name (returns NULL if none found)
	Object*				FindGameObject(const std::string& name);

	//Add a child object to this scene node
	void				AddChildObject(Object* child);


//<------- Object Parameters ------>
	//Get the name of this object (if set)
	const std::string&	GetName()			{ return m_Name; }

	//Returns if this object should be treated as opaque or transparent
	//	- Should be overrided if the object's transparency is not based on it's colour
	virtual bool IsOpaque()					{ return m_Colour.w >= 0.999f; }

	//Gets/Sets object colour
	void			SetColour(const Vector4& colour)	{ m_Colour = colour; }
	const Vector4&	GetColour()							{ return m_Colour; }

	//Gets/Sets object bounding radius
	//	- Used entirely for graphics frustum culling, and should be set for each object
	void			SetBoundingRadius(float radius)		{ m_BoundingRadius = radius; }
	float			GetBoundingRadius()					{ return m_BoundingRadius; }

	//Gets/Sets the local transform
	//	- This will be multiplied by the transform provided by the physicsObject to form
	//	  the world transform used for rendering. If no physicsObject exists, this contains
	//	  the position of the object relevant to it's parent.
	void			SetLocalTransform(const Matrix4& transform)			{ m_LocalTransform = transform; }
	const Matrix4&  GetLocalTransform()									{ return m_LocalTransform; }

	//Get the final world transform of the object, this is recalculated each render pass
	// - This is premultiplied by the objects parent (if any) and is the combination of it's physics transform (if any) and local transform.
	const Matrix4&  GetWorldTransform()					{ return m_WorldTransform; }


	//Get the frustum flags used to identify which RenderList's this object is currently visible within
	uint			GetFrustumCullFlags()				{ return m_FrustumCullFlags; }

	//Get the screen picker unique ID used to identify where the object exists (if it exsists) within the screen picker array
	uint			GetScreenPickerIdx()				{ return m_ScreenPickerIdx; }
protected:

//<------ OVERRIDABLE FUNCTIONALITY ---->
	//Called when the object should be renderered
	//	- This can be called multiple times per screen render, or none at all if the object is not in any visible view frustum
	virtual void OnRenderObject()				{};	

	//Called when the object should be updated
	//	- This is called once per frame regardless of screen-visibility and should handle all object logic
	virtual void OnUpdateObject(float dt)		{};	

//<----- MOUSE INTERACTIVITY --------->
//	- To Enable mouse interactivity the object must call "ScreenPicker::Instance()->RegisterObject(this)"

	//Called when the mouse cursor hovers over the obejct
	virtual void OnMouseEnter(float dt)			{};

	//Called when the mouse cursor stops hovering over the object
	virtual void OnMouseLeave(float dt)			{};

	//Called when the user clicks on the object
	virtual void OnMouseDown(float dt, const Vector3& worldPos)									{};

	//Called each frame the user has clicked (but not released) the object
	virtual void OnMouseMove(float dt, const Vector3& worldPos, const Vector3& worldChange)		{};

	//Called when the user releases the mouse after clicking the object
	virtual void OnMouseUp(float dt, const Vector3& worldPos)									{};

protected:
	//Scene Tree 
	std::string					m_Name;
	Scene*						m_Scene;
	Object*						m_Parent;
	std::vector<Object*>		m_Children;

	//Object Parameters
	Vector4						m_Colour;
	float						m_BoundingRadius;	
	Matrix4						m_LocalTransform;
	Matrix4						m_WorldTransform;

	//Misc Parameters
	uint						m_FrustumCullFlags;
	uint						m_ScreenPickerIdx;  
};