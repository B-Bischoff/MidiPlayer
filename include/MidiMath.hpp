#pragma once

struct Vec2 {
	float x;
	float y;

	bool operator==(const Vec2& v) const {
		return x == v.x && y == v.y;
	}
	bool operator!=(const Vec2& v) const {
		return !(*this == v);
	}
};

static Vec2 lerp(const Vec2& p0, const Vec2& p1, double t)
{
	Vec2 v;
	v.x = (1.0 - t) * p0.x + t * p1.x;
	v.y = (1.0 - t) * p0.y + t * p1.y;
	return v;
}

static double inverseLerp(double start, double end, double value)
{
	return (value - start) / (end - start);
}

static Vec2 bezierQuadratic(const Vec2& p0, const Vec2& p1, const Vec2& p2, double t)
{
	Vec2 p0p1 = lerp(p0, p1, t);
	Vec2 p1p2 = lerp(p1, p2, t);
	return lerp(p0p1, p1p2, t);
}
