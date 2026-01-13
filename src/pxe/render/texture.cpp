// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/components/component.hpp>
#include <pxe/render/texture.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <format>
#include <fstream>
#include <spdlog/spdlog.h>
#include <string>

namespace pxe {

auto texture::init(const std::string &path) -> result<> {
	if(std::ifstream const font_file(path); !font_file.is_open()) {
		return error(std::format("can not load texture file: {}", path));
	}

	const auto loaded_texture = LoadTexture(path.c_str());
	if(loaded_texture.id == 0) {
		return error(std::format("failed to load texture from file {}", path));
	}

	SetTextureFilter(texture_, TEXTURE_FILTER_POINT);
	texture_ = loaded_texture;
	size_.width = static_cast<float>(loaded_texture.width);
	size_.height = static_cast<float>(loaded_texture.height);

	SPDLOG_DEBUG("texture: loaded from file: {} ({}x{})", path, size_.width, size_.height);

	return true;
}

auto texture::end() -> result<> {
	UnloadTexture(texture_);
	texture_ = Texture2D{};
	size_ = size{.width = 0, .height = 0};

	return true;
}

auto texture::draw(const Vector2 &pos) const -> result<> {
	if(texture_.id == 0) {
		return error("texture not initialized");
	}
	DrawTexture(texture_, static_cast<int>(pos.x), static_cast<int>(pos.y), WHITE);
	return true;
}

auto texture::draw(const Rectangle origin,
				   const Rectangle dest,
				   const Color tint,
				   const float rotation,
				   const Vector2 center) const -> result<> {
	if(texture_.id == 0) {
		return error("texture not initialized");
	}
	DrawTexturePro(texture_, origin, dest, center, rotation, tint);
	return true;
}

} // namespace pxe