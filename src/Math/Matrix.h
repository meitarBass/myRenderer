#ifndef RENDERER_MATRIX_H
#define RENDERER_MATRIX_H

#include "Vec.h"

template <typename T, int M, int N>
struct Matrix {
private:
    Vec<T, M> columns[N] = {};

public:
    Matrix() = default;

    Matrix(T diagVal) {
        for (int col = 0 ; col < N ; col++) {
            columns[col] = Vec<T, M>{};
            if(col < M)
                columns[col][col] = diagVal;
        }
    }

    template <typename... Args>
    requires (sizeof...(Args) == N) && (... && std::is_same_v<Args, Vec<T, M>>)
    Matrix(Args... args) : columns { args... } {}

    ~Matrix() = default;
    Matrix(const Matrix &other) = default;
    Matrix(Matrix &&other) noexcept = default;
    Matrix& operator=(const Matrix &other) = default;
    Matrix& operator=(Matrix &&other) noexcept = default;

    Vec<T, M>& operator[](const int i) {
        return columns[i];
    }

    const Vec<T , M>& operator[](const int i) const {
        return columns[i];
    }

    template <int K>
    Matrix<T, M, K> operator*(const Matrix<T, N, K>& other) const {
        Matrix<T, M, K> res = Matrix<T, M, K>(0);
        for (int col = 0 ; col < K; col++) {
            res[col] = (*this) * other[col];
        }
        return res;
    }

    Vec<T, M> operator*(const Vec<T, N>& v) const {
        Vec<T, M> res = {};
        for (int col = 0 ; col < N; col++) {
            res = res + (columns[col] * v[col]);
        }
        return res;
    }

    Matrix<T, N, M> transpose() const {
        Matrix<T, N, M> res(0);
        for (int col = 0; col < N; col++) {
            for (int row = 0; row < M; row++) {
                res[row][col] = columns[col][row];
            }
        }
        return res;
    }

    static Matrix<T, M, N> identity() {
        return Matrix<T, M, N>{1};
    }

    static Matrix<float, 4, 4> translation(Vec3f v) {
        Matrix<float, 4, 4> mat{1};
        mat[3].x() = v.x();
        mat[3].y() = v.y();
        mat[3].z() = v.z();

        return mat;
    }

    static Matrix<float, 4, 4> scale(float sx, float sy, float sz) {
        auto mat = Matrix<float, 4, 4>::identity();

        mat[0][0] = sx;
        mat[1][1] = sy;
        mat[2][2] = sz;

        return mat;
    }

    static Matrix<float, 4, 4> lookat(Vec3f eye, Vec3f center, Vec3f up) {
        Vec3f z = (eye - center).normalize();
        Vec3f x = cross(up, z).normalize();
        Vec3f y = cross(z, x).normalize();

        auto matrixInverse = Matrix<float, 4, 4>::identity();
        auto translation = Matrix<float, 4, 4>::identity();

        for (int i = 0; i < 3; i++) {
            matrixInverse[i][0] = x[i];
            matrixInverse[i][1] = y[i];
            matrixInverse[i][2] = z[i];
            translation[3][i] = -eye[i];
        }
        return matrixInverse * translation;
    }

    static Matrix<float, 4, 4> projection(float cameraDist) {
        // Perspective Projection
        auto res = Matrix<float, 4, 4>::identity();
        res[2][3] = -1.f / cameraDist;
        return res;
    }

    static Matrix<float, 4, 4> rotationX(float angle) {
        static_assert(N == M && N == 4);
        angle = GraphicsUtils::angleToRadians(angle);

        auto res = Matrix<float, 4, 4>::identity();
        float s = std::sin(angle);
        float c = std::cos(angle);
        res[1][1] = c;  res[2][1] = -s;
        res[1][2] = s; res[2][2] = c;
        return res;
    }

    static Matrix<float, 4, 4> rotationY(float angle) {
        static_assert(N == M && N == 4);
        angle = GraphicsUtils::angleToRadians(angle);

        auto res = Matrix<float, 4, 4>::identity();
        float s = std::sin(angle);
        float c = std::cos(angle);
        res[0][0] = c;  res[2][0] = s;
        res[0][2] = -s; res[2][2] = c;
        return res;
    }


    static Matrix<float, 4, 4> rotationZ(float angle) {
        static_assert(N == M && N == 4);
        angle = GraphicsUtils::angleToRadians(angle);

        auto res = Matrix<float, 4, 4>::identity();
        float s = std::sin(angle);
        float c = std::cos(angle);
        res[0][0] = c; res[1][0] = -s;
        res[0][1] = s; res[1][1] = c;
        return res;
    }

    static Matrix<float, 4, 4> shear(float xy, float xz,
                                     float yx, float yz,
                                     float zx, float zy) {
        auto res = Matrix<float, 4, 4>::identity();
        res[1][0] = xy;
        res[2][0] = xz;
        res[0][1] = yx;
        res[2][1] = yz;
        res[0][2] = zx;
        res[1][2] = zy;

        return res;
    }

    static Matrix<float, 4, 4> viewport(float x, float y, float w, float h) {
        auto m = Matrix<float, 4, 4>::identity();

        m[0][0] = w / 2.f;
        m[1][1] = h / 2.f;
        m[2][2] = 0.5;

        m[3][0] = x + w / 2.f;
        m[3][1] = y + h / 2.f;
        m[3][2] = 0.5;

        return m;
    }

    Matrix<float, 3, 3> inverseTranspose3x3() const {
        Matrix<float, 3, 3> res;

        const float m00 = columns[0][0], m01 = columns[1][0], m02 = columns[2][0];
        const float m10 = columns[0][1], m11 = columns[1][1], m12 = columns[2][1];
        const float m20 = columns[0][2], m21 = columns[1][2], m22 = columns[2][2];

        float det = m00 * (m11 * m22 - m12 * m21) -
                    m01 * (m10 * m22 - m12 * m20) +
                    m02 * (m10 * m21 - m11 * m20);

        if (std::abs(det) < GraphicsUtils::EPSILON) return Matrix<float, 3, 3>(1.0f);

        const float invDet = 1.0f / det;

        res[0][0] =  (m11 * m22 - m12 * m21) * invDet;
        res[0][1] = -(m10 * m22 - m12 * m20) * invDet;
        res[0][2] =  (m10 * m21 - m11 * m20) * invDet;

        res[1][0] = -(m01 * m22 - m02 * m21) * invDet;
        res[1][1] =  (m00 * m22 - m02 * m20) * invDet;
        res[1][2] = -(m00 * m21 - m01 * m20) * invDet;

        res[2][0] =  (m01 * m12 - m02 * m11) * invDet;
        res[2][1] = -(m00 * m12 - m02 * m10) * invDet;
        res[2][2] =  (m00 * m11 - m01 * m10) * invDet;

        return res;
    }
};

using Matrix4f4 = Matrix<float, 4, 4>;
using Matrix3f3 = Matrix<float, 3, 3>;

#endif //RENDERER_MATRIX_H
