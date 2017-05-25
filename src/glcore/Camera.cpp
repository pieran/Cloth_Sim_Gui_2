#include "Camera.h"
#include <functional>
#include "Matrix3.h"
#include "Input.h"
#include "Window.h"

Camera::Camera(void) {
	position = Vector3(0.0f, 0.0f, 0.0f);
	yaw = 0.0f;
	pitch = 0.0f;
	yawspd = 0.f;
	pitchspd = 0.f;
	positionspd = Vector3(0.f, 0.f, 0.f);

	Input::AddMouseListener(this);
};

Camera::~Camera()
{
	Input::RemoveMouseListener(this);
}

bool Camera::OnUpdateMouseInput(bool hasFocus)
{
	if (Input::IsMouseDown(MOUSE_LEFT))
	{
		float aspect = Window::GetAspectRatio();

		Vector2 mouse = Input::GetMouseMovementClipSpace();
		pitchspd += (-mouse.y * 33.f - pitchspd) * 0.7f;
		yawspd += (-mouse.x * 33.f * aspect - yawspd) * 0.7f;

		return true;
	}

	return false;
}

/*
Polls the camera for keyboard / mouse movement.
Should be done once per frame! Pass it the msec since
last frame (default value is for simplicities sake...)
*/
void Camera::Update(float dt)
{
	float speed = 14.f * dt;

	Vector3 position_change = Vector3(0.f, 0.f, 0.f);
	if (!Input::IsKeyModifierActive(KeyModifier_Control))
	{
		if (Input::IsKeyDown(GLFW_KEY_W)) position_change.z = speed;
		if (Input::IsKeyDown(GLFW_KEY_S)) position_change.z = -speed;
		if (Input::IsKeyDown(GLFW_KEY_A)) position_change.x = speed;
		if (Input::IsKeyDown(GLFW_KEY_D)) position_change.x = -speed;
		if (Input::IsKeyDown(GLFW_KEY_Q)) position_change.y = -speed;
		if (Input::IsKeyDown(GLFW_KEY_E)) position_change.y = speed;
	}

	yaw += yawspd;

	//Bounds check the pitch, to be between straight up and straight down ;)
	pitch = max(min(pitch + pitchspd, 90.0f), - 90.0f);

	if (yaw < 0) {
		yaw += 360.0f;
	}
	if (yaw > 360.0f) {
		yaw -= 360.0f;
	}

	pitchspd = pitchspd * 0.5f;
	yawspd = yawspd * 0.5f;


	viewMatrix = Matrix4::Rotation(yaw, Vector3(0, 1, 0)) * Matrix4::Rotation(pitch, Vector3(1, 0, 0));

	position -= viewMatrix * position_change;

	viewMatrix.SetPositionVector(position);
	viewMatrix = Matrix4::Inverse(viewMatrix);
}

/*
Generates a view matrix for the camera's viewpoint. This matrix can be sent
straight to the shader...it's already an 'inverse camera' matrix.
*/
Matrix4 Camera::BuildViewMatrix()	{
	return viewMatrix;
	//Why do a complicated matrix inversion, when we can just generate the matrix
	//using the negative values ;). The matrix multiplication order is important!

};
