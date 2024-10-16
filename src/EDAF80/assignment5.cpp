
#include "assignment5.hpp"
#include "interpolation.hpp"
#include "interpolation.cpp"
#include "parametric_shapes.hpp"
#include "parametric_shapes.cpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/node.hpp" 
#include "core/ShaderProgramManager.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>
#include <cstdlib>
#include <random>

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
		static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
		0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0 };

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
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 0.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(0.0f); // 3 m/s => 10.8 km/h

	ShaderProgramManager program_manager;

	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram("Fallback",
		{ { ShaderType::vertex, "common/fallback.vert" },
		  { ShaderType::fragment, "common/fallback.frag" } },
		fallback_shader);

	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram("skybox",
		{ { ShaderType::vertex, "EDAF80/skybox.vert" },
		  { ShaderType::fragment, "EDAF80/skybox.frag" } },
		skybox_shader);

	GLuint phong_shader = 0u;
	program_manager.CreateAndRegisterProgram("phong",
		{ { ShaderType::vertex, "EDAF80/phong.vert" },
		  { ShaderType::fragment, "EDAF80/phong.frag" } },
		phong_shader);

	GLuint water_shader = 0u;
	program_manager.CreateAndRegisterProgram("Water",
		{ { ShaderType::vertex, "EDAF80/water.vert" },
		  { ShaderType::fragment, "EDAF80/water.frag" } },
		water_shader);


	bonobo::material_data demo_material;
	demo_material.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	demo_material.diffuse = glm::vec3(0.3f, 0.3f, 0.3f);
	demo_material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
	demo_material.shininess = 10.0f;

	float skyboxRad = 200.0f;
	float elapsed_time_s = 0.0f;
	bool firing = false;
	float count = 0;
	glm::mat4 viewMatrix;
	glm::mat3 rotMatrix;
	glm::vec3 forward;
	glm::vec3 bulletDir;
	glm::vec3 bulletPos;
	glm::mat4 thaiTranslate;
	glm::mat4 thaiRotate;

	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	bool use_normal_mapping = true;
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto const uniforms = [&use_normal_mapping, &light_position, &camera_position, &demo_material, &elapsed_time_s](GLuint program) {
		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));

		glUniform1f(glGetUniformLocation(program, "elapsed_time_s"), elapsed_time_s);
		glUniform1i(glGetUniformLocation(program, "normalMap"), use_normal_mapping ? 1 : 0);
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "cameraPos"), 1, glm::value_ptr(camera_position));

		glUniform3fv(glGetUniformLocation(program, "material.ambient"), 1, glm::value_ptr(demo_material.ambient));
		glUniform3fv(glGetUniformLocation(program, "material.diffuse"), 1, glm::value_ptr(demo_material.diffuse));
		glUniform3fv(glGetUniformLocation(program, "material.specular"), 1, glm::value_ptr(demo_material.specular));
		glUniform1f(glGetUniformLocation(program, "material.shininess"), demo_material.shininess);
	};

#pragma region Skybox
	auto skybox_shape = parametric_shapes::createSphere(skyboxRad, 100u, 100u);

	GLuint cubemap = bonobo::loadTextureCubeMap(
		config::resources_path("cubemaps/Stairs/posx.jpg"),
		config::resources_path("cubemaps/Stairs/negx.jpg"),
		config::resources_path("cubemaps/Stairs/posy.jpg"),
		config::resources_path("cubemaps/Stairs/negy.jpg"),
		config::resources_path("cubemaps/Stairs/posz.jpg"),
		config::resources_path("cubemaps/Stairs/negz.jpg"));

	auto skybox = Node();
	skybox.set_program(&skybox_shader, uniforms);
	skybox.set_geometry(skybox_shape);
	skybox.add_texture("skybox", cubemap, GL_TEXTURE_CUBE_MAP);

#pragma endregion

#pragma region Gun

	GLuint const gun_diffuse = bonobo::loadTexture2D(config::resources_path("textures/asiimov.jpg"));
	GLuint const gun_specular = bonobo::loadTexture2D(config::resources_path("textures/leather_red_02_rough_2k.jpg"));
	GLuint const gun_normal = bonobo::loadTexture2D(config::resources_path("textures/leather_red_02_nor_2k.jpg"));

	auto cross = Node();

	cross.set_geometry(parametric_shapes::createSphere(0.0001f, 8u, 8u));
	cross.set_material_constants(demo_material);
	cross.set_program(&fallback_shader, uniforms);

	std::vector<bonobo::mesh_data> loaded_objects = bonobo::loadObjects(config::resources_path("gun/gun.obj"));

	auto gun = Node();
	gun.set_geometry(loaded_objects[0]);
	gun.set_material_constants(demo_material);
	gun.set_program(&phong_shader, uniforms);
	gun.add_texture("my_diffuse", gun_diffuse, GL_TEXTURE_2D);
	gun.add_texture("my_specular", gun_specular, GL_TEXTURE_2D);
	gun.add_texture("my_normal", gun_normal, GL_TEXTURE_2D);

	std::vector<std::unique_ptr<Node>> parts;
	parts.reserve(loaded_objects.size() - 1);

	for (int i = 1; i < loaded_objects.size(); i++) {
		parts.push_back(std::make_unique<Node>());
		parts[i - 1]->set_geometry(loaded_objects[i]);
		parts[i - 1]->set_material_constants(demo_material);
		parts[i - 1]->set_program(&phong_shader, uniforms);
		parts[i - 1]->add_texture("my_diffuse", gun_diffuse, GL_TEXTURE_2D);
		parts[i - 1]->add_texture("my_specular", gun_specular, GL_TEXTURE_2D);
		parts[i - 1]->add_texture("my_normal", gun_normal, GL_TEXTURE_2D);

		gun.add_child(parts[i - 1].get());
	}
#pragma endregion

#pragma region Enemies
	std::vector<std::unique_ptr<Node>> enemyVec;
	std::random_device rng;
	std::mt19937 gen(rng());
	std::uniform_real_distribution<float> circum(0.0f, glm::two_pi<float>());
	std::uniform_real_distribution<float> height(glm::radians(-10.0f), glm::radians(10.0f));
	int spawnDelay = 2;

	GLuint const enemy_diffuse = bonobo::loadTexture2D(config::resources_path("textures/leather_red_02_coll1_2k.jpg"));
	GLuint const enemy_specular = bonobo::loadTexture2D(config::resources_path("textures/leather_red_02_rough_2k.jpg"));
	GLuint const enemy_normal = bonobo::loadTexture2D(config::resources_path("textures/leather_red_02_nor_2k.jpg"));
#pragma endregion

#pragma region Bullet
	auto tempBullet = Node();

	GLuint waterTexture = bonobo::loadTexture2D(config::resources_path("textures/waves.png"));

	tempBullet.set_geometry(parametric_shapes::createSphere(0.4f, 20u, 20u));
	tempBullet.set_material_constants(demo_material);
	tempBullet.add_texture("water", waterTexture, GL_TEXTURE_2D);
	tempBullet.add_texture("cubemap", cubemap, GL_TEXTURE_CUBE_MAP);
	tempBullet.set_program(&water_shader, uniforms);
#pragma endregion


	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	auto lastTime = std::chrono::high_resolution_clock::now();
	bool use_orbit_camera = false;
	std::int32_t demo_sphere_program_index = 0;
	auto cull_mode = bonobo::cull_mode_t::disabled;
	auto polygon_mode = bonobo::polygon_mode_t::fill;
	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;
	int score = 0;

	changeCullMode(cull_mode);

	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;
		elapsed_time_s += std::chrono::duration<float>(deltaTimeUs).count();

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(deltaTimeUs, inputHandler);
		if (use_orbit_camera) {
			mCamera.mWorld.LookAt(glm::vec3(0.0f));
		}
		camera_position = mCamera.mWorld.GetTranslation();

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

		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		bonobo::changePolygonMode(polygon_mode);
		glDisable(GL_DEPTH_TEST);

		skybox.get_transform().SetTranslate(camera_position);
		skybox.render(mCamera.GetWorldToClipMatrix());
		glEnable(GL_DEPTH_TEST);
		//demo_sphere.render(mCamera.GetWorldToClipMatrix());

		cross.render(mCamera.GetViewToClipMatrix(), glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.001f, -0.1f)));

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		bool opened = ImGui::Begin("Scene Control", nullptr, ImGuiWindowFlags_None);
		if (opened) {
			auto const cull_mode_changed = bonobo::uiSelectCullMode("Cull mode", cull_mode);
			if (cull_mode_changed) {
				changeCullMode(cull_mode);
			}
			bonobo::uiSelectPolygonMode("Polygon mode", polygon_mode);
			/*auto demo_sphere_selection_result = program_manager.SelectProgram("Demo sphere", demo_sphere_program_index);
			if (demo_sphere_selection_result.was_selection_changed) {
				demo_sphere.set_program(demo_sphere_selection_result.program, phong_set_uniforms);
			}*/

			ImGui::Separator();
			ImGui::Checkbox("Use normal mapping", &use_normal_mapping);
			ImGui::ColorEdit3("Ambient", glm::value_ptr(demo_material.ambient));
			ImGui::ColorEdit3("Diffuse", glm::value_ptr(demo_material.diffuse));
			ImGui::ColorEdit3("Specular", glm::value_ptr(demo_material.specular));
			ImGui::SliderFloat("Shininess", &demo_material.shininess, 1.0f, 1000.0f);
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -20.0f, 20.0f);
			ImGui::Separator();
			ImGui::Checkbox("Use orbit camera", &use_orbit_camera);
			ImGui::Separator();
			ImGui::Checkbox("Show basis", &show_basis);
			ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
			ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
		}
		ImGui::End();

		glm::mat4 thaiScale = glm::mat4(0.005f);
		glm::mat4 thaiGun = thaiScale * thaiTranslate * thaiRotate;


		gun.render(mCamera.GetViewToClipMatrix(), thaiGun);
		for (int i = 0; i < loaded_objects.size() - 1; i++) {

			Node const* bit = gun.get_child(i);
			bit->render(mCamera.GetViewToClipMatrix(), thaiGun * gun.get_transform().GetMatrix());
		}

		if (inputHandler.GetKeycodeState(GLFW_KEY_SPACE) & JUST_PRESSED && count < elapsed_time_s)
		{
			firing = true;
			count = elapsed_time_s + 0.5;
		}

		if (firing)
		{
			float x = count - elapsed_time_s;
			float shotForward = (1 - (x / 0.5)) * 10;
			float change = (x / 3) * 30;

			thaiTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, -30.0f, -100.0f - change));
			thaiRotate = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, -1.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(70.0f), glm::vec3(-1.0f, 0.0f, 0.0f));

			bulletPos += bulletDir * shotForward;
			tempBullet.get_transform().SetTranslate(bulletPos);
			glm::mat4 bulletMove = glm::translate(glm::mat4(1.0f), bulletPos);
			tempBullet.render(mCamera.GetWorldToClipMatrix(), bulletMove);

			if (count < elapsed_time_s)
			{
				firing = false;
				count = 0.0f;
			}
		}
		else
		{
			thaiTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(50.0f, -40.0f, -100.0f));
			thaiRotate = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, -1.0f, 0.0f));

			viewMatrix = mCamera.mWorld.GetMatrix();
			rotMatrix = glm::mat3(viewMatrix);
			forward = -glm::normalize(rotMatrix[2]);
			bulletDir = forward;
			bulletPos = glm::vec3(0.0f, -1.0f, 0.0f);
		}

#pragma region EnemyManager
		static float lastSpawnTime = -spawnDelay;
		if (elapsed_time_s - lastSpawnTime >= spawnDelay && enemyVec.size() < 20)
		{
			lastSpawnTime = elapsed_time_s;

			auto tempEnemy = std::make_unique<Node>();

			glm::vec3 enemyPos;
			enemyPos.x = skyboxRad * cos(height(gen)) * cos(circum(gen));
			enemyPos.y = skyboxRad * sin(height(gen));
			enemyPos.z = skyboxRad * sin(circum(gen) * cos(height(gen)));

			tempEnemy->set_geometry(parametric_shapes::createSphere(1.0f, 30u, 30u));
			tempEnemy->get_transform().SetTranslate(enemyPos);
			tempEnemy->set_material_constants(demo_material);
			tempEnemy->add_texture("my_diffuse", enemy_diffuse, GL_TEXTURE_2D);
			tempEnemy->add_texture("my_specular", enemy_specular, GL_TEXTURE_2D);
			tempEnemy->add_texture("my_normal", enemy_normal, GL_TEXTURE_2D);
			tempEnemy->set_program(&phong_shader, uniforms);
			enemyVec.emplace_back(std::move(tempEnemy));
		}

		enemyVec.erase(
			std::remove_if(enemyVec.begin(), enemyVec.end(),
				[&](auto& currEnemy) {
			glm::vec3 startPos = currEnemy->get_transform().GetTranslation();
			glm::vec3 newPos = interpolation::evalLERP(startPos, camera_position, 0.001);

			float distance = glm::distance2(newPos, bulletPos);
			float radiusSum = 1.0f + 0.4f;

			if (distance >= radiusSum * radiusSum) {
				currEnemy->get_transform().SetTranslate(newPos);
				currEnemy->render(mCamera.GetWorldToClipMatrix());
				return false;  // Keep this enemy
			}
			score += 1;
			return true;  // Remove this enemy
		}),
			enemyVec.end());
#pragma endregion

		bool const open = ImGui::Begin("Score", nullptr, ImGuiWindowFlags_None);
		if (open) {

			ImGui::Text("Current number: %d", score);
		}
		ImGui::End();

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());

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
	}
	catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
