#ifndef RENDERER_VEC_H
#define RENDERER_VEC_H

#include <type_traits>
#include <cmath>

namespace GraphicsUtils {
    constexpr float EPSILON = 1e-5f;
    constexpr float PI = 3.14159265359f;
    constexpr float MAX_COLOR_F = 255.0f;

    inline float angleToRadians(float angle) {
        return PI * angle / 180;
    }
}

template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

template <Numeric T, int n>
struct Vec {

private:
    T raw[n];

public:
    Vec() : raw{0} {}

    explicit Vec(const Vec<T, 3>& other) requires (n == 4)
            : raw { other[0], other[1], other[2], static_cast<T>(1)}
    {}

    ~Vec() = default;
    Vec(const Vec& other) = default;
    Vec(Vec&& other) noexcept = default;
    Vec& operator=(const Vec& other) = default;
    Vec& operator=(Vec&& other) noexcept = default;

    template <typename... Args>
    requires (sizeof...(Args) == n)
    Vec(Args... args) : raw { static_cast<T>(args)...} {}

    T& operator[](int i) { return raw[i]; }
    const T& operator[](int i) const { return raw[i]; }


    T& x() { return raw[0]; }
    T& y() { static_assert(n >= 2); return raw[1]; }
    T& z() { static_assert(n >= 3); return raw[2]; }
    T& w() { static_assert(n >= 4); return raw[3]; }

    T x() const { return raw[0]; }
    T y() const { static_assert(n >= 2); return raw[1]; }
    T z() const { static_assert(n >= 3); return raw[2]; }
    T w() const { static_assert(n >= 4); return raw[3]; }

    Vec& operator+=(const Vec& other) {
        for(int i = 0 ; i < n ; i++) {
            raw[i] += other.raw[i];
        }
        return *this;
    }

    Vec operator+(const Vec& other) const
    {
        return Vec(*this) += other;
    }

    Vec operator-()
    {
        Vec res;
        for(int i = 0; i < n ; i++)  {
            res.raw[i] = -raw[i];
        }
        return res;
    }

    Vec& operator -=(const Vec& other) {
        for (int i = 0 ; i < n; i++) {
            raw[i] -= other.raw[i];
        }
        return *this;
    }

    Vec operator-(const Vec& other) const
    {
        Vec res = *this;
        res -= other;
        return res;
    }

    Vec operator*(T f) const
    {
        Vec res;
        for (int i = 0; i < n; i++) {
            res.raw[i] = raw[i] * f;
        }
        return res;
    }

    Vec operator/(T f) const
    {
        Vec res;
        float inverseF = 1.0f / f;
        for (int i = 0; i < n; i++) {
            res.raw[i] = raw[i] * inverseF;
        }
        return res;
    }

    [[nodiscard]] T lengthSquared() const
    {
        T length = 0;
        for(int i = 0 ; i < n; i++) {
            length += (raw[i] * raw[i]);
        }

        return length;
    }

    [[nodiscard]] T length() const
    {
        return std::sqrt(lengthSquared());
    }

    [[nodiscard]] Vec normalize() const
    {
        T l2 = lengthSquared();
        if (l2 < GraphicsUtils::EPSILON * GraphicsUtils::EPSILON) {
            return *this;
        }
        auto vecLengthInverse = 1.0f / std::sqrt(l2);
        return (*this) * vecLengthInverse;
    }
};

using Vec2f = Vec<float, 2>;
using Vec2i = Vec<int, 2>;
using Vec3f = Vec<float, 3>;
using Vec3i = Vec<int, 3>;
using Vec4f = Vec<float, 4>;
using Vec4i = Vec<int, 4>;
using Point3 = Vec<float, 3>;

template <Numeric T, int n>
[[nodiscard]] T dotProduct(const Vec<T, n> &v1, const Vec<T, n> &v2) {
    T res = 0;
    for (int i = 0; i < n; i++) {
        res += (v1[i] * v2[i]);
    }
    return res;
}

template <Numeric T>
[[nodiscard]] Vec<T, 3> cross(const Vec<T, 3> &v1, const Vec<T, 3> &v2)
{
    return Vec<T, 3>(v1.y() * v2.z() - v1.z() * v2.y(),
                     v1.z() * v2.x() - v1.x() * v2.z(),
                     v1.x() * v2.y() - v1.y() * v2.x());
}

template <Numeric T, int n>
[[nodiscard]] Vec<T, n> operator*(float f, const Vec<T, n> &v)
{
    return v * f;
}


template <Numeric T>
[[nodiscard]] float determinant2D(const Vec<T, 2> &v1, const Vec<T, 2> &v2)
{
    return v1.x() * v2.y() - v1.y() * v2.x();
}

#endif //RENDERER_VEC_H
