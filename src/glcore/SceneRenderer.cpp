#include "SceneRenderer.h"
#include "NCLDebug.h"
#include "CommonMeshes.h"
#include "BoundingBox.h"
#include "Window.h"


SceneRenderer::SceneRenderer()
	: m_Scene(NULL)
	, m_ScreenFBO(NULL)
	, m_ShadowFBO(NULL)
	, m_VsyncEnabled(true)
	, m_ShadowMapsInvalidated(true)
	, m_ShadowMapNum(4)
	, m_ShadowMapSize(2048)
	, m_BackgroundColour(0.7f, 0.7f, 0.7f)
	, m_AmbientColour(0.4f, 0.4f, 0.4f)
	, m_InvLightDirection(0.5f, 1.0f, -0.8f)
	, m_SpecularIntensity(64.0f)
	, m_GammaCorrection(1.0f / 2.2f)
{
	m_InvLightDirection.Normalise();

	m_ScreenTex[0] = NULL;
	m_ShadowTex[0] = NULL;

	//We do not have an OGLContext at this point so nothing can be loaded properly, see InitializeOGLContext for all shader/resource loading
	if (!RenderList::AllocateNewRenderList(&m_FrameRenderList, true))
	{
		NCLERROR("Unable to allocate scene render list! - Try using less shadow maps");
	}

	//Initialize the shadow render lists
	for (uint i = 0; i < m_ShadowMapNum; ++i)
	{
		if (!RenderList::AllocateNewRenderList(&m_ShadowRenderLists[i], true))
		{
			NCLERROR("Unable to allocate shadow render list %d! - Try using less shadow maps", i);
		}
	}
}

SceneRenderer::~SceneRenderer()
{
	if (m_ShadowTex[0])
	{
		glDeleteTextures(m_ShadowMapNum, m_ShadowTex);
		glDeleteFramebuffers(1, &m_ShadowFBO);
		m_ShadowTex[0] = NULL;

		for (uint i = 0; i < m_ShadowMapNum; ++i)
			delete m_ShadowRenderLists[i];
	}

	if (m_FrameRenderList)
	{
		delete m_FrameRenderList;
		m_FrameRenderList = NULL;
	}

	if (m_ShaderColNorm)
	{
		delete m_ShaderColNorm;
		delete m_ShaderLightDir;
		delete m_ShaderCombineLighting;
		delete m_ShaderPresentToWindow;
		delete m_ShaderShadow;
		delete m_ShaderForwardLighting;
		m_ShaderColNorm = NULL;
	}

	if (m_Camera)
	{
		delete m_Camera;
		m_Camera = NULL;
	}

	if (m_ScreenTex[0])
	{
		glDeleteTextures(SCREENTEX_MAX, m_ScreenTex);
		glDeleteFramebuffers(1, &m_ScreenFBO);
		m_ScreenTex[0] = NULL;
	}

	if (m_Scene)
	{
		m_Scene->OnCleanupScene();
		delete m_Scene;
		m_Scene = NULL;
	}

	NCLDebug::ReleaseShaders();
	CommonMeshes::ReleaseMeshes();
}

bool SceneRenderer::Initialize()
{
	CommonMeshes::InitializeMeshes();

	m_Camera = new Camera();

	BuildFBOs();

	NCLDebug::LoadShaders();

	InitializeDefaults();

	if (!ShadersLoad())
		return false;

	return true;
}

void SceneRenderer::InitializeDefaults()
{
	NCLDebug::ClearLog();

	m_Camera->SetPosition(Vector3(-3.0f, 10.0f, 15.0f));
	m_Camera->SetYaw(-10.f);
	m_Camera->SetPitch(-30.f);

	m_BackgroundColour = Vector3(0.8f, 0.8f, 0.8f);
	m_AmbientColour = Vector3(0.4f, 0.4f, 0.4f);
	m_InvLightDirection = Vector3(0.5f, 1.0f, -0.8f); m_InvLightDirection.Normalise();
	m_SpecularIntensity = 64.0f;

	SetShadowMapNum(4);
	SetShadowMapSize(2048);
	m_NumSuperSamples = 4.0f;
	m_GammaCorrection = 1.0f / 2.2f;
}

void SceneRenderer::SetShadowMapNum(uint num) {
	if (m_ShadowMapNum != num && num <= SHADOWMAP_MAX)
	{
		if (m_ShadowMapNum > 0)
		{
			for (int i = m_ShadowMapNum - 1; i >= (int)num; --i)
			{
				glDeleteTextures(1, &m_ShadowTex[i]);
				delete m_ShadowRenderLists[i];
				m_ShadowRenderLists[i] = NULL;
				m_ShadowTex[i] = NULL;
			}
		}
		for (uint i = m_ShadowMapNum; i < num; i++)
		{
			glGenTextures(1, &m_ShadowTex[i]);
			RenderList::AllocateNewRenderList(&m_ShadowRenderLists[i], true);
		}
		m_ShadowMapNum = num;


		m_ShadowMapsInvalidated = true;
	}
}

void SceneRenderer::SetShadowMapSize(uint size) {
	if (!m_ShadowMapsInvalidated)
		m_ShadowMapsInvalidated = (size != m_ShadowMapSize);

	m_ShadowMapSize = size;
}

void SceneRenderer::RenderScene()
{
	if (m_Scene == NULL)
		return;

	//Check to see if the window has been resized
	BuildFBOs();
	
	//Reset all varying data
	Mesh::Reset();
	
	m_ProjMatrix = Matrix4::Perspective(PROJ_NEAR, PROJ_FAR, (float)m_ScreenTexWidth / (float)m_ScreenTexHeight, PROJ_FOV);
	m_FrameFrustum.FromMatrix(m_ProjMatrix * m_ViewMatrix);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	glEnable(GL_STENCIL_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glDepthFunc(GL_LEQUAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	//Update all Object's World Transform
	m_Scene->BuildWorldMatrices();
	NCLDebug::SetDebugDrawData(m_ProjMatrix * m_ViewMatrix, m_Camera->GetPosition());


	//Setup default shader uniforms
	ShadersSetDefaults();

	//Render Shadow Maps
	RenderShadowMaps();

	//Build Scene Render List
	m_FrameRenderList->UpdateCameraWorldPos(m_Camera->GetPosition());
	m_FrameRenderList->RemoveExcessObjects(m_FrameFrustum);
	m_FrameRenderList->SortLists();
	m_Scene->InsertToRenderList(m_FrameRenderList, m_FrameFrustum);


	//Main Render Window
	{
		//Render Opaque Objects via deferred rendering (Quicker)
		DeferredRenderOpaqueObjects();

		//Render Transparent Objects via forward rendering - slower but only way :[
		glStencilFunc(GL_ALWAYS, 1, 1);
		ForwardRenderTransparentObjects();

		//Render Debug Data (NCLDebug)
		NCLDebug::SortDebugLists();
		NCLDebug::DrawDebugLists();	
	}

	//Downsample and present our complete image to the window
	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	int width, height;
	Window::GetRenderDimensions(&width, &height);
	glViewport(0, 0, width, height);
	glUseProgram(m_ShaderPresentToWindow->GetProgram());
	glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_COLOUR1]);
	glDrawArrays(GL_POINTS, 0, 1);

	//Finally draw any non-anti aliasing HUD elements
	NCLDebug::DrawDebubHUD();
	NCLDebug::ClearDebugLists();
}

void SceneRenderer::UpdateScene(float dt)
{
	if (m_Scene != NULL)
	{
		m_Camera->Update(dt);
		m_ViewMatrix = m_Camera->BuildViewMatrix();
		m_Scene->OnUpdateScene(dt);
	}		
}





void SceneRenderer::RenderShadowMaps()
{
	if (m_Scene != NULL)
	{
		const float proj_range = PROJ_FAR - PROJ_NEAR;
		Matrix4 view = Matrix4::BuildViewMatrix(Vector3(0.0f, 0.0f, 0.0f), m_InvLightDirection);
		Matrix4 invCamProjView = Matrix4::Inverse(m_ProjMatrix * m_ViewMatrix);


		auto compute_depth = [&](float x)
		{
			float proj_start = -(proj_range * x + PROJ_NEAR);
			return (proj_start*m_ProjMatrix[10] + m_ProjMatrix[14]) / (proj_start*m_ProjMatrix[11]);
		};


		float itr_factor = 9.0f / float(m_ShadowMapNum);	//Causes i to go from 0-9 

#pragma omp parallel for
		for (int i = 0; i < (int)m_ShadowMapNum; ++i)
		{

			float factor_n = 1.0f - log10(i * itr_factor + 1.0f);
			float factor_f = 1.0f - log10((i + 1) * itr_factor + 1.0f);

			float near_depth = compute_depth(factor_n);
			float far_depth = compute_depth(factor_f);

			//Build Bounding Box around frustum section
			BoundingBox bb;
			bb.ExpandToFit(invCamProjView * Vector3(-1.0f, -1.0f, near_depth));
			bb.ExpandToFit(invCamProjView * Vector3(-1.0f, 1.0f, near_depth));
			bb.ExpandToFit(invCamProjView * Vector3(1.0f, -1.0f, near_depth));
			bb.ExpandToFit(invCamProjView * Vector3(1.0f, 1.0f, near_depth));
			bb.ExpandToFit(invCamProjView * Vector3(-1.0f, -1.0f, far_depth));
			bb.ExpandToFit(invCamProjView * Vector3(-1.0f, 1.0f, far_depth));
			bb.ExpandToFit(invCamProjView * Vector3(1.0f, -1.0f, far_depth));
			bb.ExpandToFit(invCamProjView * Vector3(1.0f, 1.0f, far_depth));


			//Rotate bounding box so it's orientated in the lights direction
			Vector3 centre = (bb.maxPoints + bb.minPoints) * 0.5f;
			Matrix4 localView = view* Matrix4::Translation(-centre);

			bb = bb.Transform(localView);
			bb.minPoints.z = min(bb.minPoints.z, -m_Scene->GetWorldRadius());
			bb.maxPoints.z = max(bb.maxPoints.z, m_Scene->GetWorldRadius());

			//Build Light Projection		
			m_ShadowProj[i] = Matrix4::Orthographic(bb.maxPoints.z, bb.minPoints.z, bb.minPoints.x, bb.maxPoints.x, bb.maxPoints.y, bb.minPoints.y);
			m_ShadowProjView[i] = m_ShadowProj[i] * localView ;

			//Construct Shadow RenderList 
			Vector3 top_mid = centre + view * Vector3(0.0f, 0.0f, bb.maxPoints.z);
			Frustum f; f.FromMatrix(m_ShadowProjView[i]);
			m_ShadowRenderLists[i]->UpdateCameraWorldPos(top_mid);
			m_ShadowRenderLists[i]->RemoveExcessObjects(f);
			m_ShadowRenderLists[i]->SortLists();
			m_Scene->InsertToRenderList(m_ShadowRenderLists[i], f);
		}



		//Render Shadow Maps (OpenGL cannot be multithreaded easily)
		glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowFBO);
		glViewport(0, 0, m_ShadowMapSize, m_ShadowMapSize);
		glUseProgram(m_ShaderColNorm->GetProgram());
		Matrix4 identity = Matrix4();
		for (uint i = 0; i < m_ShadowMapNum; ++i)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ShadowTex[i], 0);		
			glClear(GL_DEPTH_BUFFER_BIT);
			
			glUniformMatrix4fv(glGetUniformLocation(m_ShaderColNorm->GetProgram(), "viewMatrix"), 1, false, (float*)&identity);
			glUniformMatrix4fv(glGetUniformLocation(m_ShaderColNorm->GetProgram(), "projMatrix"), 1, false, (float*)&m_ShadowProjView[i]);


			GLint uniloc_modelMatrix = glGetUniformLocation(m_ShaderColNorm->GetProgram(), "modelMatrix");
			auto per_obj_render = [&](Object* obj)
			{
				glUniformMatrix4fv(uniloc_modelMatrix, 1, false, (float*)&obj->m_WorldTransform);
			};
			m_ShadowRenderLists[i]->RenderOpaqueObjects(per_obj_render);
			m_ShadowRenderLists[i]->RenderTransparentObjects(per_obj_render);
		}		
	}

}

void SceneRenderer::BuildFBOs()
{
	//Util Function
	auto build_texture = [&](uint texID, GLint internalformat, uint width, uint height, bool depth, bool samplerShadow)
	{
		glBindTexture(GL_TEXTURE_2D, texID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (samplerShadow)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
		}

		if (depth)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		}
		else
			glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	};

	//Construct Scene GBuffer
	int width, height;
	Window::GetRenderDimensions(&width, &height);

	if (m_ScreenTexWidth != uint(width * m_NumSuperSamples) || m_ScreenTexHeight != uint(height * m_NumSuperSamples))
	{
		if (m_Scene)
			m_Scene->OnSceneResize(width, height);

		m_ScreenTexWidth = uint(width * m_NumSuperSamples);
		m_ScreenTexHeight = uint(height * m_NumSuperSamples);

		if (!m_ScreenTex[0]) glGenTextures(SCREENTEX_MAX, m_ScreenTex);

		//Generate our Scene Depth Texture
		build_texture(m_ScreenTex[SCREENTEX_DEPTH], GL_DEPTH24_STENCIL8, m_ScreenTexWidth, m_ScreenTexHeight, true, false);
		build_texture(m_ScreenTex[SCREENTEX_COLOUR0], GL_SRGB8, m_ScreenTexWidth, m_ScreenTexHeight, false, false);
		build_texture(m_ScreenTex[SCREENTEX_COLOUR1], GL_SRGB8, m_ScreenTexWidth, m_ScreenTexHeight, false, false);
		build_texture(m_ScreenTex[SCREENTEX_NORMALS], GL_RGB8, m_ScreenTexWidth, m_ScreenTexHeight, false, false);
		build_texture(m_ScreenTex[SCREENTEX_LIGHTING], GL_RGB8, m_ScreenTexWidth, m_ScreenTexHeight, false, false);

		//Generate our Framebuffer
		if (!m_ScreenFBO) glGenFramebuffers(1, &m_ScreenFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, m_ScreenFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_DEPTH], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_DEPTH], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_COLOUR0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_COLOUR1], 0);

		//Validate our framebuffer
		GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			NCLERROR("Unable to create Screen Framebuffer! StatusCode: %x", status);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}


	//Construct our Shadow Maps
	if (m_ShadowMapsInvalidated && m_ShadowMapNum > 0)
	{	
		m_ShadowMapsInvalidated = false;

		for (uint i = 0; i < m_ShadowMapNum; ++i)
		{
			if (!m_ShadowTex[i]) glGenTextures(1, &m_ShadowTex[i]);
			build_texture(m_ShadowTex[i], GL_DEPTH_COMPONENT32, m_ShadowMapSize, m_ShadowMapSize, true, true);
		}
		
		if (!m_ShadowFBO) glGenFramebuffers(1, &m_ShadowFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ShadowTex[0], 0);
		glDrawBuffers(0, GL_NONE);

		GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			NCLERROR("Unable to create Shadow Framebuffer! StatusCode: %x", status);
		}
	}
}


bool SceneRenderer::ShadersLoad()
{
	auto SHADERERROR = [&](const char* name) {
		NCLERROR("Could not link shader: %s", name);
	};

	m_ShaderColNorm = new Shader(
		SHADERDIR"SceneRenderer/TechVertexFull.glsl",
		SHADERDIR"SceneRenderer/TechFragDeferred.glsl");
	if (!m_ShaderColNorm->LinkProgram()){
		SHADERERROR("Default Deferred Render");
		return false;
	}

	m_ShaderLightDir = new Shader(
		SHADERDIR"Common/EmptyVertex.glsl",
		SHADERDIR"SceneRenderer/TechFragDeferredDirLight.glsl",
		SHADERDIR"Common/FullScreenQuadGeometry.glsl");
	if (!m_ShaderLightDir->LinkProgram()){
		SHADERERROR("Deferred Render Light Computation");
		return false;
	}

	m_ShaderCombineLighting = new Shader(
		SHADERDIR"Common/EmptyVertex.glsl",
		SHADERDIR"SceneRenderer/TechFragDeferredCombine.glsl",
		SHADERDIR"Common/FullScreenQuadGeometry.glsl");
	if (!m_ShaderCombineLighting->LinkProgram()){
		SHADERERROR("Deferred Render Light Combination");
		return false;
	}

	m_ShaderPresentToWindow = new Shader(
		SHADERDIR"Common/EmptyVertex.glsl",
		SHADERDIR"SceneRenderer/TechFragSuperSample.glsl",
		SHADERDIR"Common/FullScreenQuadGeometry.glsl");
	if (!m_ShaderPresentToWindow->LinkProgram()){
		SHADERERROR("Present to window / SuperSampling");
		return false;
	}

	m_ShaderShadow = new Shader(
		SHADERDIR"SceneRenderer/TechVertexSimple.glsl",
		SHADERDIR"Common/EmptyFragment.glsl");
	if (!m_ShaderShadow->LinkProgram()){
		SHADERERROR("Shadow Shader");
		return false;
	}

	m_ShaderForwardLighting = new Shader(
		SHADERDIR"SceneRenderer/TechVertexFull.glsl",
		SHADERDIR"SceneRenderer/TechFragForwardRender.glsl");
	if (!m_ShaderForwardLighting->LinkProgram()){
		SHADERERROR("Forward Renderer");
		return false;
	}

	return true;
}

void SceneRenderer::UpdateDefaultShaderMatrices(GLuint pidx)
{
	if (pidx) {
		Matrix4 identity = Matrix4();
		glUniformMatrix4fv(glGetUniformLocation(pidx, "modelMatrix"), 1, false, (float*)&identity);
		glUniformMatrix4fv(glGetUniformLocation(pidx, "viewMatrix"), 1, false, (float*)&m_ViewMatrix);
		glUniformMatrix4fv(glGetUniformLocation(pidx, "projMatrix"), 1, false, (float*)&m_ProjMatrix);
		glUniformMatrix4fv(glGetUniformLocation(pidx, "textureMatrix"), 1, false, (float*)&identity);
	}
}

void SceneRenderer::ShadersSetDefaults()
{
	Vector3 camPos = m_Camera->GetPosition();
	Matrix4 clipspaceToWorldspace = Matrix4::Inverse(m_ProjMatrix * m_ViewMatrix);
	int shadowmapindicies[] {8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 };

	glUseProgram(m_ShaderColNorm->GetProgram());
	UpdateDefaultShaderMatrices(m_ShaderColNorm->GetProgram());
	glUniform1i(glGetUniformLocation(m_ShaderColNorm->GetProgram(), "diffuseTex"), 0);


	//SetCurrentShader(m_ShaderLightDir);
	glUseProgram(m_ShaderLightDir->GetProgram());
	glUniform1i(glGetUniformLocation(m_ShaderLightDir->GetProgram(), "depthTex"), 5);
	glUniform1i(glGetUniformLocation(m_ShaderLightDir->GetProgram(), "colourTex"), 6);
	glUniform1i(glGetUniformLocation(m_ShaderLightDir->GetProgram(), "normalTex"), 7);
	glUniform1iv(glGetUniformLocation(m_ShaderLightDir->GetProgram(), "shadowTex[0]"), m_ShadowMapNum, &shadowmapindicies[0]);
	glUniform1i(glGetUniformLocation(m_ShaderLightDir->GetProgram(), "shadowNum"), m_ShadowMapNum);
	glUniform2f(glGetUniformLocation(m_ShaderLightDir->GetProgram(), "shadowSinglePixel"), 1.0f / m_ShadowMapSize, 1.0f / m_ShadowMapSize);
	glUniformMatrix4fv(glGetUniformLocation(m_ShaderLightDir->GetProgram(), "clipToWorldTransform"), 1, GL_FALSE, &clipspaceToWorldspace.values[0]);
	glUniform3fv(glGetUniformLocation(m_ShaderLightDir->GetProgram(), "invLightDir"), 1, &m_InvLightDirection.x);
	glUniform3fv(glGetUniformLocation(m_ShaderLightDir->GetProgram(), "cameraPos"), 1, &camPos.x);
	glUniform1f(glGetUniformLocation(m_ShaderLightDir->GetProgram(), "specularIntensity"), m_SpecularIntensity);


	//SetCurrentShader(m_ShaderForwardLighting);
	glUseProgram(m_ShaderForwardLighting->GetProgram());
	UpdateDefaultShaderMatrices(m_ShaderForwardLighting->GetProgram());
	glUniform1i(glGetUniformLocation(m_ShaderForwardLighting->GetProgram(), "diffuseTex"), 0);
	glUniform1iv(glGetUniformLocation(m_ShaderForwardLighting->GetProgram(), "shadowTex[0]"), m_ShadowMapNum, &shadowmapindicies[0]);
	glUniform1i(glGetUniformLocation(m_ShaderForwardLighting->GetProgram(), "shadowNum"), m_ShadowMapNum);
	glUniform2f(glGetUniformLocation(m_ShaderForwardLighting->GetProgram(), "shadowSinglePixel"), 1.0f / m_ShadowMapSize, 1.0f / m_ShadowMapSize);
	glUniformMatrix4fv(glGetUniformLocation(m_ShaderForwardLighting->GetProgram(), "clipToWorldTransform"), 1, GL_FALSE, &clipspaceToWorldspace.values[0]);
	glUniform3fv(glGetUniformLocation(m_ShaderForwardLighting->GetProgram(), "invLightDir"), 1, &m_InvLightDirection.x);
	glUniform3fv(glGetUniformLocation(m_ShaderForwardLighting->GetProgram(), "cameraPos"), 1, &camPos.x);
	glUniform1f(glGetUniformLocation(m_ShaderForwardLighting->GetProgram(), "specularIntensity"), m_SpecularIntensity);
	glUniform3fv(glGetUniformLocation(m_ShaderForwardLighting->GetProgram(), "ambientColour"), 1, &m_AmbientColour.x);


	//SetCurrentShader(m_ShaderCombineLighting);
	glUseProgram(m_ShaderCombineLighting->GetProgram());
	glUniform3fv(glGetUniformLocation(m_ShaderCombineLighting->GetProgram(), "ambientColour"), 1, &m_AmbientColour.x);
	glUniform1i(glGetUniformLocation(m_ShaderCombineLighting->GetProgram(), "colourTex"), 5);
	glUniform1i(glGetUniformLocation(m_ShaderCombineLighting->GetProgram(), "lightingTex"), 6);



	//SetCurrentShader(m_ShaderPresentToWindow);
	glUseProgram(m_ShaderPresentToWindow->GetProgram());
	glUniform2f(glGetUniformLocation(m_ShaderPresentToWindow->GetProgram(), "singlepixel"), 1.0f / m_ScreenTexWidth, 1.0f / m_ScreenTexHeight);
	glUniform1f(glGetUniformLocation(m_ShaderPresentToWindow->GetProgram(), "numsamples"), m_NumSuperSamples);
	glUniform1f(glGetUniformLocation(m_ShaderPresentToWindow->GetProgram(), "gammaCorrection"), 1.0f / m_GammaCorrection);
	glUniform1i(glGetUniformLocation(m_ShaderPresentToWindow->GetProgram(), "colourTex"), 5);
}

void SceneRenderer::DeferredRenderOpaqueObjects()
{
	const GLenum drawbuffers_1[1] = { GL_COLOR_ATTACHMENT0 };
	const GLenum drawbuffers_2[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };


	//Render the Scene to Depth/Color/Normal textures
	glBindFramebuffer(GL_FRAMEBUFFER, m_ScreenFBO);
	glViewport(0, 0, m_ScreenTexWidth, m_ScreenTexHeight);
	glClearColor(m_BackgroundColour.x, m_BackgroundColour.y, m_BackgroundColour.z, 0.0f);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_COLOUR0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_COLOUR1], 0);
	glDrawBuffers(2, drawbuffers_2);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glUseProgram(m_ShaderColNorm->GetProgram());
	glUniformMatrix4fv(glGetUniformLocation(m_ShaderColNorm->GetProgram(), "viewMatrix"), 1, false, (float*)&m_ViewMatrix);
	glUniformMatrix4fv(glGetUniformLocation(m_ShaderColNorm->GetProgram(), "projMatrix"), 1, false, (float*)&m_ProjMatrix);

	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_ZERO, GL_KEEP, GL_REPLACE);

	GLint uniloc_modelMatrix = glGetUniformLocation(m_ShaderColNorm->GetProgram(), "modelMatrix");
	GLint uniloc_nodeColour = glGetUniformLocation(m_ShaderColNorm->GetProgram(), "nodeColour");
	m_FrameRenderList->RenderOpaqueObjects([&](Object* obj)
	{
		glUniformMatrix4fv(uniloc_modelMatrix, 1, false, (float*)&obj->m_WorldTransform);
		if (uniloc_nodeColour > -1) glUniform4fv(uniloc_nodeColour, 1, (float*)&obj->GetColour());
	});


	//Compute Lighting
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_LIGHTING], 0);
	glDrawBuffers(1, drawbuffers_1);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	glUseProgram(m_ShaderLightDir->GetProgram());
	glUniformMatrix4fv(glGetUniformLocation(m_ShaderLightDir->GetProgram(), "shadowTransform[0]"), m_ShadowMapNum, GL_FALSE, &m_ShadowProjView[0].values[0]);
	glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_DEPTH]);
	glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_COLOUR0]);
	glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_COLOUR1]);

	for (uint i = 0; i < m_ShadowMapNum; ++i)
	{
		glActiveTexture(GL_TEXTURE8 + i);
		glBindTexture(GL_TEXTURE_2D, m_ShadowTex[i]);
	}

	
	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glDrawArrays(GL_POINTS, 0, 1);

	//Finally Combine all the colours/lighting/shadows etc into a single texture
	glBindFramebuffer(GL_FRAMEBUFFER, m_ScreenFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_COLOUR1], 0);
	

	glUseProgram(m_ShaderCombineLighting->GetProgram());
	glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_COLOUR0]);
	glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_LIGHTING]);
	glDrawArrays(GL_POINTS, 0, 1);
	
	glEnable(GL_DEPTH_TEST);
}

void SceneRenderer::ForwardRenderTransparentObjects()
{
	//Draw all transparent objects onto scene using forward-rendering
	glUseProgram(m_ShaderForwardLighting->GetProgram());
	glUniformMatrix4fv(glGetUniformLocation(m_ShaderForwardLighting->GetProgram(), "shadowTransform[0]"), m_ShadowMapNum, GL_FALSE, &m_ShadowProjView[0].values[0]);
	glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_DEPTH]);
	glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_COLOUR0]);
	glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, m_ScreenTex[SCREENTEX_COLOUR1]);

	for (uint i = 0; i < m_ShadowMapNum; ++i)
	{
		glActiveTexture(GL_TEXTURE8 + i);
		glBindTexture(GL_TEXTURE_2D, m_ShadowTex[i]);
	}

	GLint uniloc_modelMatrix = glGetUniformLocation(m_ShaderForwardLighting->GetProgram(), "modelMatrix");
	GLint uniloc_nodeColour = glGetUniformLocation(m_ShaderForwardLighting->GetProgram(), "nodeColour");
	m_FrameRenderList->RenderTransparentObjects([&](Object* obj)
	{
		glUniformMatrix4fv(uniloc_modelMatrix, 1, false, (float*)&obj->m_WorldTransform);
		if (uniloc_nodeColour > -1) glUniform4fv(uniloc_nodeColour, 1, (float*)&obj->GetColour());
	});
}