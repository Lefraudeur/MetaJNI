#include "render_info.hpp"
#include "../mappings.hpp"
#include <imgui.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>

static bool first_update = true;
static bool _valid = false;

struct jni_refs
{
	maps::MinecraftClient instance{ nullptr, jni::GLOBAL_REF };
	maps::GameRenderer gameRenderer{ nullptr, jni::GLOBAL_REF };
	maps::RenderTickCounter renderTickCounter{ nullptr, jni::GLOBAL_REF };
	maps::SimpleOption bobView{ nullptr , jni::GLOBAL_REF };
	maps::SimpleOption fov{ nullptr, jni::GLOBAL_REF };
};
static alignas(alignof(jni_refs)) uint8_t refs_data[sizeof(jni_refs)]{};
static jni_refs* refs = nullptr;

static glm::mat4 projection_rot_mat{};
static glm::vec3 cam_pos{};
static glm::vec2 window_size{};


void render_info::update()
{
	_valid = false;
	jni::frame frame{};

	if (first_update)
	{
		refs = std::construct_at<jni_refs>((jni_refs*)&refs_data);
		maps::MinecraftClient MinecraftClient{};
		refs->instance = MinecraftClient.instance.get();
		refs->gameRenderer = refs->instance.gameRenderer.get();
		refs->renderTickCounter = refs->instance.renderTickCounter.get();
		maps::GameOptions options = refs->instance.options.get();
		refs->bobView = options.bobView.get();
		refs->fov = options.fov.get();
		first_update = false;
	}

	maps::ClientPlayerEntity player = refs->instance.player.get();
	if (!player)
		return;

	jfloat tickDelta = refs->renderTickCounter.tickDelta.get();

	jfloat fov = maps::Integer(refs->fov.value.get()).intValue();
	jfloat lastFovMultiplier = refs->gameRenderer.lastFovMultiplier.get();
	jfloat fovMultiplier = refs->gameRenderer.fovMultiplier.get();
	fov *= lastFovMultiplier + (fovMultiplier - lastFovMultiplier) * tickDelta;

	glm::mat4 projection_matrix = refs->gameRenderer.getBasicProjectionMatrix(fov).to_glm_mat4(); // TODO get FOV from options
	bool bob_view = maps::Boolean(refs->bobView.value.get()).booleanValue();
	if (bob_view)
	{
		jfloat horizontal_speed = player.horizontalSpeed.get();
		jfloat prev_stride_distance = player.prevStrideDistance.get();
		jfloat stride_distance = player.strideDistance.get();
		float g = horizontal_speed - player.prevHorizontalSpeed.get();
		float h = -(horizontal_speed + g * tickDelta);
		float i = prev_stride_distance + (stride_distance - prev_stride_distance) * tickDelta;
		constexpr float pi = glm::pi<float>();

		projection_matrix = glm::translate(projection_matrix, glm::vec3(glm::sin(h * pi) * i * 0.5f, -glm::abs(glm::cos(h * pi) * i), 0.f));
		projection_matrix *= glm::mat4_cast(glm::angleAxis(glm::radians(glm::sin(h * pi) * i * 3.f), glm::vec3{ 0.f, 0.f, 1.f }));
		projection_matrix *= glm::mat4_cast(glm::angleAxis(glm::radians(glm::abs(glm::cos(h * pi - 0.2f) * i) * 5.f), glm::vec3{ 1.f, 0.f, 0.f }));
	}

	glm::vec3 pos = player.pos.get().to_glm_vec3();
	glm::vec3 last_camera_pos = player.get_last_render_glm_vec3_pos();
	cam_pos = last_camera_pos + (pos - last_camera_pos) * tickDelta;
	cam_pos.y += player.getEyeHeight(player.getPose());

	glm::vec2 rotation = player.get_rotation_glm_vec2();

	glm::mat4 rotationMatrix = glm::mat4_cast(glm::angleAxis(glm::radians(rotation.x), glm::vec3(1.f, 0.f, 0.f)));
	rotationMatrix *= glm::mat4_cast(glm::angleAxis(glm::radians(rotation.y + 180.f), glm::vec3(0.f, 1.f, 0.f)));

	projection_rot_mat = projection_matrix * rotationMatrix;

	ImVec2 wsize = ImGui::GetIO().DisplaySize;
	window_size = { wsize.x, wsize.y };

	_valid = true;
}

void render_info::shutdown()
{
	std::destroy_at(refs);
}

bool render_info::is_valid()
{
	return _valid;
}

glm::vec2 render_info::world_to_screen(const glm::vec3& world_pos)
{
	glm::vec3 camera_relative = world_pos - cam_pos;
	glm::vec4 clip_space_pos = projection_rot_mat * glm::vec4(camera_relative, 1.f);
	if (clip_space_pos.z <= 0.f) return glm::vec2(-1.f);

	if (clip_space_pos.w == 0.0f) return glm::vec2(-1.f);
	glm::vec3 ndc = glm::vec3(clip_space_pos) / clip_space_pos.w;

	float x = (ndc.x * 0.5f + 0.5f) * window_size.x;
	float y = (1.0f - (ndc.y * 0.5f + 0.5f)) * window_size.y;

	return glm::floor(glm::vec2(x, y));
}
