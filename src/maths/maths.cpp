#include "maths.hpp"
#include <cmath>
#include <numbers>

double maths::vector3d::length() const
{
	return std::sqrt(x * x + y * y + z * z);
}

double maths::vector3d::squared_length() const
{
	return x * x + y * y + z * z;
}

maths::vector3d maths::vector3d::normalized() const
{
	double length = this->length();
	return vector3d(x / length, y / length, z / length);
}

maths::vector3d maths::vector3d::operator+(const vector3d& other) const
{
	return {x + other.x, y + other.y, z + other.z};
}

maths::vector3d maths::vector3d::operator-(const vector3d& other) const
{
	return { x - other.x, y - other.y, z - other.z };
}

maths::vector3d maths::vector3d::operator*(double coefficient) const
{
	return { x * coefficient, y * coefficient, z * coefficient};
}

maths::vector3d maths::vector3d::operator^(const vector3d& other) const
{
	return
	{
		y * other.z - other.y * z,
		z * other.x - other.z * x,
		x * other.y - other.x * y
	};
}

double maths::vector3d::scalar_product(const vector3d other) const
{
	return x * other.x + y * other.y + z * other.z;
}

maths::angles maths::vector3d::get_angles() const
{
	double hypxz = std::sqrt(x * x + z * z);

	double pitchRad = std::atan(-y / hypxz);

	double yawDeg = 0.0;
	double pitchDeg = to_degrees(pitchRad);

	if (x != 0.0)
	{
		double yawRad = std::atan2(z, x) - (std::numbers::pi / 2.0);
		yawDeg = to_degrees(yawRad);
	}

	return angles((float)yawDeg, (float)pitchDeg);
}



maths::vector3d maths::angles::get_vector() const
{
	double yaw_rad = to_radians(yaw);
	double pitch_rad = to_radians(pitch);

	double cos_pitch = std::cos(pitch_rad);
	return { -std::sin(yaw_rad) * cos_pitch, -std::sin(pitch_rad), std::cos(yaw_rad) * cos_pitch };
}

maths::angles maths::angles::mod360() const
{
	return angles(maths::mod360(yaw), maths::mod360(pitch));
}

maths::angles maths::angles::operator-(const angles& other) const
{
	return angles(yaw - other.yaw, pitch - other.pitch);
}

maths::angles maths::angles::operator+(const angles& other) const
{
	return angles(yaw + other.yaw, pitch + other.pitch);
}

maths::angles maths::angles::operator*(float coefficient) const
{
	return angles(yaw * coefficient, pitch * coefficient);
}

float maths::mod360(float angle)
{
	while (angle <= -180.0f)
		angle += 360.0f;
	while (angle > 180.0f)
		angle -= 360.0f;
	return angle;
}

maths::timer::timer(std::chrono::milliseconds everyXms)
{
	set_every(everyXms);
}

bool maths::timer::is_elapsed()
{
	std::chrono::steady_clock::time_point timer_now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(timer_now - timer_begin) >= target_ms)
	{
		timer_begin = timer_now;
		return true;
	}
	return false;
}

void maths::timer::set_every(std::chrono::milliseconds everyXms)
{
	reset();
	target_ms = everyXms;
}

void maths::timer::reset()
{
	timer_begin = std::chrono::steady_clock::now();
}
