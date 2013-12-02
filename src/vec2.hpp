#pragma once

template <class T>
class Vec2
{
public:
	Vec2() = default;
	Vec2(const Vec2&) = default;
	Vec2(T x_val, T y_val):
		x(x_val), y(y_val)
	{}

	Vec2& operator=(const Vec2&) = default;

	Vec2 operator+(const Vec2& other) const
	{
		return Vec2(x + other.x, y + other.y);
	}

	Vec2& operator+=(const Vec2& other)
	{
		*this = *this + other;
		return *this;
	}

	Vec2 operator-(const Vec2& other) const
	{
		return Vec2(x - other.x, y - other.y);
	}

	Vec2& operator-=(const Vec2& other)
	{
		*this = *this - other;
		return *this;
	}

	Vec2 operator*(const T scalar) const
	{
		return Vec2(x * scalar, y * scalar);
	}

	Vec2 operator*=(const T scalar)
	{
		*this = *this * scalar;
		return *this;
	}

	bool operator==(const Vec2& other)
	{
		return x == other.x && y == other.y;
	}

	bool operator!=(const Vec2& other)
	{
		return x != other.x || y != other.y;
	}

	T x, y;
};

extern template class Vec2<int>;
typedef Vec2<int> IVec2;
