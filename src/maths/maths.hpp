#pragma once
#include <concepts>
#include <numbers>
#include <array>
#include <chrono>

namespace maths
{
	struct vector3d;

	struct angles
	{
		vector3d get_vector() const;
		angles mod360() const;

		angles operator-(const angles& other) const;
		angles operator+(const angles& other) const;
		angles operator*(float coefficient) const;

		float yaw = 0.f, pitch = 0.f;
	};
	inline constexpr std::floating_point auto to_radians(std::floating_point auto angle)
	{
		constexpr double coef = std::numbers::pi / 180.0;
		return angle * coef;
	}

	inline constexpr std::floating_point auto to_degrees(std::floating_point auto angle)
	{
		constexpr double coef = 180.0 / std::numbers::pi;
		return angle * coef;
	}
	float mod360(float angle);

	struct vector3d
	{
		double length() const;
		double squared_length() const;

		vector3d normalized() const;

		vector3d operator+(const vector3d& other) const;
		vector3d operator-(const vector3d& other) const;
		vector3d operator*(double coefficient) const;
		vector3d operator^(const vector3d& other) const;

		double scalar_product(const vector3d other) const;

		angles get_angles() const;

		double x = 0.0, y = 0.0, z = 0.0;
	};

	class timer
	{
	public:
		timer(std::chrono::milliseconds everyXms);
		bool is_elapsed();
		void set_every(std::chrono::milliseconds everyXms);
		void reset();
	private:
		std::chrono::steady_clock::time_point timer_begin;
		std::chrono::milliseconds target_ms;
	};
}