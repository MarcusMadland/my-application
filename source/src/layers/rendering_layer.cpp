#include "layers/rendering_layer.hpp"
#include "mapp/app.hpp"
#include "imgui_impl_mapp.hpp"
#include "mrender/imgui_impl_mrender.hpp"
#include "mcore/math.hpp"

#include <imgui.h>
#include <stb_image.h>
#include <iostream>

void RenderingLayer::onInit(mapp::AppContext& context)
{
	mAppContext = &context;

	// MRENDER
	mrender::RenderSettings renderSettings;
	renderSettings.mRendererName = "MyRenderer";
	renderSettings.mNativeDisplay = mAppContext->getWindow()->getNativeDisplay();
	renderSettings.mNativeWindow = mAppContext->getWindow()->getNativeWindow();
	renderSettings.mResolutionWidth = mAppContext->getWindow()->getParams().mWidth;
	renderSettings.mResolutionHeight = mAppContext->getWindow()->getParams().mHeight;
	renderSettings.mVSync = false;

	mRenderContext = mrender::createRenderContext();
	mRenderContext->initialize(renderSettings);

	// Clear color
	mRenderContext->setClearColor(0xFF00FFFF);

	// Shaders
	mRenderContext->loadShader("uber", "C:/Users/marcu/Dev/mengine/mrender/shaders/uber");

	// Geometry
	mrender::BufferLayout layout =
	{ {
		{ mrender::AttribType::Float, 3, mrender::Attrib::Position },
		{ mrender::AttribType::Uint8, 4, mrender::Attrib::Normal },
		{ mrender::AttribType::Uint8, 4, mrender::Attrib::Tangent },
		{ mrender::AttribType::Int16, 2, mrender::Attrib::TexCoord0 },
	} };
	std::shared_ptr<mrender::Geometry> cubeGeo = mRenderContext->createGeometry(layout, s_bunnyVertices.data(), static_cast<uint32_t>(s_bunnyVertices.size() * sizeof(Vertex)), s_bunnyTriList);

	// Materials
	std::shared_ptr<mrender::Material> textureMaterial = mRenderContext->createMaterial("uber");
	{
		{
			stbi_set_flip_vertically_on_load(true);
			int width = 0, height = 0, channels = 0;
			const uint8_t* data = stbi_load("C:/Users/marcu/Dev/mengine/resources/albedo.png", &width, &height,
				&channels, 4);
			static std::shared_ptr<mrender::Texture> texture = mRenderContext->createTexture(data, mrender::TextureFormat::RGBA8, 0, width, height, 4);
			
			textureMaterial->setUniform("u_albedo", mrender::UniformType::Sampler, texture);
		}
		{
			stbi_set_flip_vertically_on_load(true);
			int width = 0, height = 0, channels = 0;
			const uint8_t* data = stbi_load("C:/Users/marcu/Dev/mengine/resources/normal.png", &width, &height,
				&channels, 4);
			static std::shared_ptr<mrender::Texture> texture = mRenderContext->createTexture(data, mrender::TextureFormat::RGBA8, 0, width, height, 4);
			
			textureMaterial->setUniform("u_normal", mrender::UniformType::Sampler, texture);
		}
		{
			stbi_set_flip_vertically_on_load(true);
			int width = 0, height = 0, channels = 0;
			const uint8_t* data = stbi_load("C:/Users/marcu/Dev/mengine/resources/specular.png", &width, &height,
				&channels, 4);
			static std::shared_ptr<mrender::Texture> texture = mRenderContext->createTexture(data, mrender::TextureFormat::RGBA8, 0, width, height, 4);
			
			textureMaterial->setUniform("u_specular", mrender::UniformType::Sampler, texture);
		}
		
	}
	std::shared_ptr<mrender::Material> whiteMaterial = mRenderContext->createMaterial("uber");
	{
		{
			stbi_set_flip_vertically_on_load(true);
			int width = 0, height = 0, channels = 0;
			const uint8_t* data = stbi_load("C:/Users/marcu/Dev/mengine/resources/white.png", &width, &height,
				&channels, 4);
			static std::shared_ptr<mrender::Texture> texture = mRenderContext->createTexture(data, mrender::TextureFormat::RGBA8, 0, width, height, 4);
			whiteMaterial->setUniform("u_albedo", mrender::UniformType::Sampler, texture);
		}
	}

	// Renderables
	std::shared_ptr<mrender::Renderable> cubeRender1 = mRenderContext->createRenderable(cubeGeo, textureMaterial);
	std::shared_ptr<mrender::Renderable> cubeRender2 = mRenderContext->createRenderable(cubeGeo, textureMaterial);
	std::shared_ptr<mrender::Renderable> floorRender = mRenderContext->createRenderable(cubeGeo, whiteMaterial);

	mRenderContext->addRenderable(cubeRender1);
	mRenderContext->addRenderable(cubeRender2);
	mRenderContext->addRenderable(floorRender);

	// Camera
	mrender::CameraSettings cameraSettings;
	cameraSettings.mProjectionType = mrender::ProjectionType::Perspective;
	cameraSettings.mWidth = static_cast<float>(mRenderContext->getSettings().mResolutionWidth);
	cameraSettings.mHeight = static_cast<float>(mRenderContext->getSettings().mResolutionHeight);
	cameraSettings.mClipFar = 10000.0f;
	cameraSettings.mPosition[2] = -5.0f;
	auto camera = mRenderContext->createCamera(cameraSettings);
	mCamera = std::make_shared<CameraOrbitController>(camera);

	// ImGui
	imguiImplInit();
}

void RenderingLayer::onShutdown()
{
	mRenderContext->cleanup();

	imguiImplShutdown();
}

void RenderingLayer::onEvent(mapp::Event& event)
{
	ImGui_ImplMApp_ProcessEvent(event, mAppContext->getWindow());
	mCamera->onEvent(event);

	mapp::EventDispatcher dispatcher = mapp::EventDispatcher(event);

	dispatcher.dispatch<mapp::WindowResizeEvent>(
		[&](const mapp::WindowResizeEvent& e)
		{
			// Resize Renderer
			mrender::RenderSettings settings = mRenderContext->getSettings();
			settings.mResolutionWidth = e.getWidth();
			settings.mResolutionHeight = e.getHeight();
			mRenderContext->setSettings(settings);

			return 0;
		});
}

void RenderingLayer::onUpdate(const float& dt)
{
	mCamera->onUpdate(dt);
}

void RenderingLayer::onRender()
{
	float deltaTime = mAppContext->getApp()->getDeltaTime();
	static float rotationSpeed = 20.0f;
	static float accumulatedTime = 0.0f;
	static float rotationAngle = 0.0f;
	accumulatedTime += deltaTime;
	float targetRotationAngle = rotationSpeed * accumulatedTime;

	{
		mcore::Vector<float, 3> position = { -1.5f, 0.0f, 0.0f };
		mcore::Matrix4x4<float> translation = mcore::Matrix4x4<float>::identity();
		mcore::translate(translation, position);
		mcore::Matrix4x4<float> rotation = mcore::Matrix4x4<float>::identity();
		mcore::rotateX(rotation, targetRotationAngle);
		mcore::Matrix4x4<float> model = rotation * translation;
		mRenderContext->getRenderables()[0]->setTransform(&model[0]);
	}
	{
		mcore::Vector<float, 3> position = { 1.5f, 0.0f, 0.0f };
		mcore::Matrix4x4<float> translation = mcore::Matrix4x4<float>::identity();
		mcore::translate(translation, position);
		mcore::Matrix4x4<float> rotation = mcore::Matrix4x4<float>::identity();
		mcore::rotateY(rotation, targetRotationAngle);
		mcore::Matrix4x4<float> model = rotation * translation;
		mRenderContext->getRenderables()[1]->setTransform(&model[0]);
	}
	{
		mcore::Vector<float, 3> position = { 0.0f, -1.5f, 0.0f };
		mcore::Matrix4x4<float> translation = mcore::Matrix4x4<float>::identity();
		mcore::translate(translation, position);
		mcore::Matrix4x4<float> scale = mcore::Matrix4x4<float>::identity();
		mcore::scale(scale, {10.0f, 0.01f, 10.0f });
		mcore::Matrix4x4<float> model = scale * translation;
		mRenderContext->getRenderables()[2]->setTransform(&model[0]);

	}

	// Render
	mRenderContext->render(mCamera->getCamera());
	
	// IMGUI TEST
	imguiImplBegin();
	renderUserInterface();
	imguiImplEnd();

	// Debug text (application performance)
	{
		uint16_t y = 2;
		uint16_t x = 45;
		const float deltaTime = mAppContext->getApp()->getDeltaTime();
		static uint32_t counter = 0; counter++;
		static float fps = 0;
		static float ms = 0;
		static float msHighest = 0;
		if (!(counter % 10))
		{
			fps = 1 / deltaTime;
			ms = 1000 * deltaTime;
			if (ms > msHighest) msHighest = ms;
		}

		if (mDrawDebugText)
		{
			mRenderContext->submitDebugTextOnScreen(x, y, "cpu(application):  %.2f ms [%.2f ms]", ms, msHighest);
			mRenderContext->submitDebugTextOnScreen(x, y + 1, "cpu(mrender):      %.2f ms [%.2f ms]", 0, 0);
			mRenderContext->submitDebugTextOnScreen(x, y + 2, "gpu:               %.2f ms [%.2f ms]", 0, 0);
			mRenderContext->submitDebugTextOnScreen(x, y + 3, "framerate:         %.2f fps", fps);
			mRenderContext->submitDebugTextOnScreen(x, y + 4, "textures:          %.2f / 1454 MiB", 0);

			mRenderContext->submitDebugTextOnScreen(x - 20, y, mrender::Color::Red, true, false, "Too many meshes!!", 0);
		}
	}
	
	// Swap buffers
	mRenderContext->swapBuffers();
}

void RenderingLayer::renderUserInterface()
{
	ImGui::SetNextWindowPos(ImVec2(10, 10));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	if (ImGui::Begin(" MRender | Rendering Framework", (bool*)0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::Text("A 3D Rendering framework with support\nfor PBR and GI");
		if (ImGui::Button("Recompile & load shaders"))
		{
			const char* scriptPath = "C:/Users/marcu/Dev/mengine/compile-shaders-win.bat";
			int result = system(scriptPath);
			if (result == 0)
			{
				mRenderContext->reloadShaders();
			}
		}
		ImGui::Checkbox("Draw stats", &mDrawDebugText);
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen))
		{
			std::vector<std::string_view> allRenderers = mRenderContext->getRenderer()->getNames();
			static const char* currentItem = mRenderContext->getSettings().mRendererName.data();
			if (ImGui::BeginCombo("Renderer", currentItem))
			{
				for (int n = 0; n < allRenderers.size(); n++)
				{
					bool isSelected = (currentItem == allRenderers[n].data());
					if (ImGui::Selectable(allRenderers[n].data(), isSelected))
					{
						currentItem = allRenderers[n].data();
					}
					if (isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				mrender::RenderSettings settings = mRenderContext->getSettings();
				settings.mRendererName = currentItem;
				mRenderContext->setSettings(settings);

				ImGui::EndCombo();
			}

			ImGui::Text("Num render states	: %u", mRenderContext->getRenderStateCount());
			ImGui::Text("Num buffers	 : %u", mRenderContext->getBuffers().size());
			ImGui::Text("Num draw calls		: %u", 0);
			ImGui::Text("Render Resolution	: %ux%u", mRenderContext->getSettings().mResolutionWidth, mRenderContext->getSettings().mResolutionHeight);
			

			static bool vSync = mRenderContext->getSettings().mVSync;
			if (ImGui::Checkbox("VSync	", &vSync))
			{
				mrender::RenderSettings settings = mRenderContext->getSettings();
				settings.mVSync = vSync;
				mRenderContext->setSettings(settings);
			}

		}

		if (ImGui::CollapsingHeader("Render Systems", ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (uint32_t i = 0; i < mRenderContext->getRenderSystems().size(); i++)
			{
				ImGui::Text(mRenderContext->getRenderSystems()[i]->getName().data());
			}
		}

		if (ImGui::CollapsingHeader("Shaders", ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (auto& shader : mRenderContext->getShaders())
			{
				ImGui::Text(shader.first.data());
			}
		}
	}
	else
	{
		printf("Failed");
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

void RenderingLayer::imguiImplInit()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplMApp_Init(mAppContext->getWindow()->getNativeWindow());
	ImGui_ImplMRender_Init(50);
}

void RenderingLayer::imguiImplShutdown()
{
	ImGui_ImplMRender_Shutdown();
	ImGui_ImplMApp_Shutdown();
	ImGui::DestroyContext();
}

void RenderingLayer::imguiImplBegin()
{
	ImGui_ImplMRender_NewFrame();
	ImGui_ImplMApp_NewFrame();
	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(mAppContext->getWindow()->getParams().mWidth),
		static_cast<float>(mAppContext->getWindow()->getParams().mHeight));
}

void RenderingLayer::imguiImplEnd()
{
	ImGui::Render();
	ImGui::EndFrame();
	ImGui_ImplMRender_RenderDrawData(ImGui::GetDrawData());
}
