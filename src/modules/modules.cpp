#include "modules.hpp"
#include "../gui/gui.hpp"
#include <imgui.h>
#include "../utils/utils.hpp"
#include "../gui/render_info.hpp"
#include "../scheduler/scheduler.hpp"

static std::vector<modules::module*> _modules{};

void modules::init()
{
	jni::frame frame{};
	_modules =
	{
		new hitbox(),
		new aimassist(),
		new autoclick(),
		new triggerbot(),
		new glow_esp(),
		new keepsprint(),
		new block_esp()
	};
}

void modules::shutdown(::cache& cache)
{
	jni::frame frame{};
	for (modules::module* module : _modules)
	{
		if (module->enabled && cache.is_valid()) module->on_disable(cache);
		delete module;
	}
}

const std::vector<modules::module*>& modules::get_modules()
{
	return _modules;
}


modules::module::module(const char* name) : 
	name(name)
{
}

void modules::module::run(::cache& cache)
{
}

void modules::module::on_enable(::cache& cache)
{
}

void modules::module::on_disable(::cache& cache)
{
}

void modules::module::render()
{
}

void modules::module::render_options()
{
}

int modules::module::get_keybind()
{
	return keybind;
}

const char* modules::module::get_name()
{
	return name;
}

void modules::module::key_bind_selector()
{
	utils::null_string display = (scanning ? "scanning..." : keycode_to_string(keybind));
	if (ImGui::Button((utils::null_string("keybind: ") + display + "##" + name).data))
	{
		scanning = true;
		gui::reset_last_pressed_key();
	}

	if (scanning)
	{
		int last_pressed = gui::get_last_pressed_key();
		if (!last_pressed) return;

		if (last_pressed == VK_ESCAPE)
			last_pressed = 0;

		keybind = last_pressed;
		scanning = false;
	}
}

utils::null_string modules::module::keycode_to_string(int keycode)
{
	if (!keycode)
		return "none";

	switch (keycode)
	{
	case VK_MENU:
	case VK_LMENU:
		return "LALT";
	case VK_RMENU:
		return "RALT";
	case VK_SPACE:
		return "SPACE";
	case VK_LBUTTON:
		return "LBUTTON";
	case VK_RBUTTON:
		return "RBUTTON";
	case VK_MBUTTON:
		return "MBUTTON";
	case VK_TAB:
		return "TAB";
	}

	char character = MapVirtualKeyA(keybind, MAPVK_VK_TO_CHAR);
	if (!character)
		return "?";
		
	utils::null_string str{"a"};
	str.data[0] = character;
	return str;
}

void modules::block_esp::on_enable(::cache& cache)
{
	scheduler* main_scheduler = scheduler::get_main();

	render_data_task_id = main_scheduler->schedule
	({
		[this]()
		{
			jni::frame frame{};

			maps::MinecraftClient instance = maps::MinecraftClient{}.instance.get();
			if (!instance) return;
			maps::ClientWorld world = instance.world.get();
			if (!world) return;
			maps::ClientPlayerEntity player = instance.player.get();
			if (!player) return;

			std::vector<block> new_render_data{};
			std::vector<block_name_color> target_blocks{};
			{
				std::lock_guard lock{ blocks_mutex };
				target_blocks = blocks;
			}

			auto get_block_color = [&target_blocks](const std::string& block_name) -> std::optional<ImVec4>
			{
				for (const block_name_color& b : target_blocks)
					if (block_name.find(b.name.data()) != std::string::npos)
						return b.color;

				return std::nullopt;
			};

			glm::vec3 player_block_pos = glm::floor(player.pos.get().to_glm_vec3());

			int rad = radius;
			int x_start = (int)player_block_pos.x - rad;
			int x_end = (int)player_block_pos.x + rad + 1;
			int z_start = (int)player_block_pos.z - rad;
			int z_end = (int)(int)player_block_pos.z + rad + 1;
			int y_start = (int)player_block_pos.y - rad;
			int y_end = (int)player_block_pos.y + rad + 1;

			for (jint x = x_start; x < x_end; ++x)
			for (jint y = y_start; y < y_end; ++y)
			for (jint z = z_start; z < z_end; ++z)
			{
				jni::frame subframe{};
				maps::BlockPos blockPos = maps::BlockPos::new_object(&maps::BlockPos::constructor, x, y, z);
				std::string block_name = world.getBlockState(blockPos).toString().to_string();

				std::optional<ImVec4> block_color = get_block_color(block_name);
				if (!block_color.has_value())
					continue;
				new_render_data.push_back(block({ x, y, z }, block_color.value()));
			}

			{
				std::lock_guard lock{ mtx };
				render_data = std::move(new_render_data);
			}
		},
		std::chrono::milliseconds(100)
	});
}

void modules::block_esp::on_disable(::cache& cache)
{
	scheduler* main_scheduler = scheduler::get_main();

	main_scheduler->unschedule(render_data_task_id);

	std::lock_guard lock{ mtx };
	render_data.clear();
}

void modules::block_esp::render()
{
	if (!render_info::is_valid()) return;

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	std::vector<block> local_render_data{};
	{
		std::lock_guard lock{ mtx };
		local_render_data = render_data;
	}

	for (const block& b : local_render_data)
	{
		std::array<block::quad_2d, 6> faces = b.get_screen_faces();

		for (const block::quad_2d& face : faces)
		{
			if (!face.is_valid()) break;
			drawList->AddQuad(ImVec2(face.p0.x, face.p0.y), ImVec2(face.p1.x, face.p1.y), ImVec2(face.p2.x, face.p2.y), ImVec2(face.p3.x, face.p3.y), ImColor(b.color), 1.0f);
		}
	}
}

void modules::block_esp::render_options()
{
	int radius_temp = radius;
	ImGui::SliderInt("radius", &radius_temp, 1, 30);
	radius = radius_temp;

	std::vector<block_name_color> target_blocks{};
	{
		std::lock_guard lock{ blocks_mutex };
		target_blocks = blocks;
	}
	ImGui::SeparatorText("blocks");
	std::unique_ptr<bool[]> to_delete = std::make_unique<bool[]>(target_blocks.size());
	for (int i = 0; i < target_blocks.size(); ++i)
	{
		const block_name_color& b = target_blocks[i];
		ImGui::Text(b.name.data());
		ImGui::SameLine();
		ImGui::ColorButton(b.name.data(), b.color);
		ImGui::SameLine();
		if (ImGui::Button("x"))
			to_delete[i] = true;
	}
	for (int i = 0; i < target_blocks.size(); ++i)
	{
		if (!to_delete[i]) continue;
		target_blocks.erase(target_blocks.begin() + i);
	}
	{
		std::lock_guard lock{ blocks_mutex };
		blocks = std::move(target_blocks);
	}

	ImGui::SeparatorText("block adder");
	
	ImGui::InputText("block name contains", name_buff.data(), NAME_SIZE);
	ImGui::ColorEdit4("block color", color_buff);

	if (ImGui::Button("add") && name_buff[0] != '\0')
	{
		{
			std::lock_guard lock{ blocks_mutex };
			blocks.push_back(block_name_color{ name_buff, ImVec4(color_buff[0], color_buff[1], color_buff[2], color_buff[3]) });
		}
		memset(name_buff.data(), '\0', NAME_SIZE);
	}
}

modules::block_esp::block::block(const glm::vec3& blockPos, const ImVec4& color) :
	faces
	{
		{{blockPos.x, blockPos.y, blockPos.z}, {blockPos.x + 1, blockPos.y, blockPos.z}, {blockPos.x + 1, blockPos.y, blockPos.z + 1}, {blockPos.x, blockPos.y, blockPos.z + 1}},
		{{blockPos.x, blockPos.y + 1, blockPos.z}, {blockPos.x + 1, blockPos.y + 1, blockPos.z}, {blockPos.x + 1, blockPos.y + 1, blockPos.z + 1}, {blockPos.x, blockPos.y + 1, blockPos.z + 1}},
		{{blockPos.x, blockPos.y, blockPos.z}, {blockPos.x + 1, blockPos.y, blockPos.z}, {blockPos.x + 1, blockPos.y + 1, blockPos.z}, {blockPos.x, blockPos.y + 1, blockPos.z}},
		{{blockPos.x, blockPos.y, blockPos.z + 1}, {blockPos.x + 1, blockPos.y, blockPos.z + 1}, {blockPos.x + 1, blockPos.y + 1, blockPos.z + 1}, {blockPos.x, blockPos.y + 1, blockPos.z + 1}},
		{{blockPos.x, blockPos.y, blockPos.z}, {blockPos.x, blockPos.y, blockPos.z + 1}, {blockPos.x, blockPos.y + 1, blockPos.z + 1}, {blockPos.x, blockPos.y + 1, blockPos.z}},
		{{blockPos.x + 1, blockPos.y, blockPos.z}, {blockPos.x + 1, blockPos.y, blockPos.z + 1}, {blockPos.x + 1, blockPos.y + 1, blockPos.z + 1}, {blockPos.x + 1, blockPos.y + 1, blockPos.z}}
	},
	color(color)
{

}

std::array<modules::block_esp::block::quad_2d, 6> modules::block_esp::block::get_screen_faces() const
{
	std::array<quad_2d, 6> result{};
	for (int i = 0; i < 6; ++i)
		result[i] = faces[i].to_quad_2d();
	return result;
}

modules::block_esp::block::quad_2d modules::block_esp::block::quad::to_quad_2d() const
{
	using namespace render_info;
	return {world_to_screen(p0),
		world_to_screen(p1),
		world_to_screen(p2),
		world_to_screen(p3) };
}

bool modules::block_esp::block::quad_2d::is_valid() const
{

	auto all_positive =
		[](const glm::vec2& v) -> bool
		{
			return glm::all(glm::greaterThanEqual(v, glm::vec2(0.f)));
		}
	;
	return all_positive(p0) && all_positive(p1) && all_positive(p2) && all_positive(p3);
}
