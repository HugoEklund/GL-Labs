#include "assignment5.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <tinyfiledialogs.h>
#include <cstdlib>
#include <clocale>
#include <stdexcept>
#include <glm\gtc\type_ptr.hpp>

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment5::~Assignment5()
{
	bonobo::deinit();
}

void
edaf80::Assignment5::run()
{
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(0.0f); // 3 m/s => 10.8 km/h

	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram("Skybox",
											{ { ShaderType::vertex, "EDAF80/skybox.vert" },
											  { ShaderType::fragment, "EDAF80/skybox.frag" } },
											skybox_shader);
	if (skybox_shader == 0u)
		LogError("Failed to load skybox shader");

	GLuint phong_shader = 0u;
	program_manager.CreateAndRegisterProgram("Phong",
		{ { ShaderType::vertex, "EDAF80/phong.vert" },
		  { ShaderType::fragment, "EDAF80/phong.frag" } },
		phong_shader);
	if (phong_shader == 0u)
		LogError("Failed to load phong shader");



	float elapsed_time_s = 0.0f;
	auto const fps_uniforms = [&elapsed_time_s](GLuint program)
	{
		glUniform1f(glGetUniformLocation(program, "elapsed_time_s"), elapsed_time_s);
	};

	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program)
	{
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	//auto skybox_shape = parametric_shapes::createSphere(20.0f, 100u, 100u);



	GLuint cubemap = bonobo::loadTextureCubeMap(
		config::resources_path("cubemaps/Stairs/posx.jpg"),
		config::resources_path("cubemaps/Stairs/negx.jpg"),
		config::resources_path("cubemaps/Stairs/posy.jpg"),
		config::resources_path("cubemaps/Stairs/negy.jpg"),	
		config::resources_path("cubemaps/Stairs/posz.jpg"),
		config::resources_path("cubemaps/Stairs/negz.jpg"));

	Node skybox;
	//skybox.set_geometry(skybox_shape);
	skybox.add_texture("cubemap", cubemap, GL_TEXTURE_CUBE_MAP);
	skybox.set_program(&skybox_shader, set_uniforms);

	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	std::vector<bonobo::mesh_data> loaded_objects = bonobo::loadObjects("C:/Users/raz12/source/repos/CG_LabsThisLast/res/cannon/models/cannonModel/cannon.obj");
	auto boat = Node();
	boat.set_geometry(loaded_objects[0]);
	boat.set_material_constants(demo_material);
	boat.set_program(&phong_shader, phong_set_uniforms);
	boat.add_texture("my_diffuse", diffuse_id, GL_TEXTURE_2D);
	boat.add_texture("my_specular", specular_id, GL_TEXTURE_2D);
	boat.add_texture("my_normal", normal_id, GL_TEXTURE_2D);

	std::vector<std::unique_ptr<Node>> parts;
	parts.reserve(loaded_objects.size() - 1);

	for (int i = 1; i < loaded_objects.size(); i++) {
		parts.push_back(std::make_unique<Node>());
		parts[i - 1]->set_geometry(loaded_objects[i]);
		parts[i - 1]->set_material_constants(demo_material);
		parts[i - 1]->set_program(&phong_shader, phong_set_uniforms);
		parts[i - 1]->add_texture("my_diffuse", diffuse_id, GL_TEXTURE_2D);
		parts[i - 1]->add_texture("my_specular", specular_id, GL_TEXTURE_2D);
		parts[i - 1]->add_texture("my_normal", normal_id, GL_TEXTURE_2D);

		boat.add_child(parts[i - 1].get());
	}

	std::string filename = config::resources_path("gun/gun.obj");
	std::vector<bonobo::mesh_data> meshes = bonobo::loadObjects(filename);

	if (meshes.empty()) {
		std::cerr << "Failed to load object!" << std::endl;
	}
	else {
		std::cout << "Object loaded successfully!" << std::endl;
	}

	auto lastTime = std::chrono::high_resolution_clock::now();
	bool pause_animation = true;
	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;

	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;
		if (!pause_animation) {
			elapsed_time_s += std::chrono::duration<float>(deltaTimeUs).count();
		}

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(deltaTimeUs, inputHandler);
		glm::vec2 mousePos = inputHandler.GetMousePosition();


		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);


		//
		// Todo: If you need to handle inputs, you can do it here
		//


		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


		if (!shader_reload_failed) {
			glDepthFunc(GL_LEQUAL);
			skybox.render(mCamera.GetWorldToClipMatrix());

			glDepthFunc(GL_LESS);
		}


		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//
		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
		if (opened) {
			ImGui::Checkbox("Show basis", &show_basis);
			ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
			ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
		}
		ImGui::End();

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
	}
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try {
		edaf80::Assignment5 assignment5(framework.GetWindowManager());
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	return 0;
}
