/*
 * Rock Fracturing - Emscripten/WebAssembly Port
 * Based on: https://github.com/aparis69/Rock-fracturing
 * Paper: "Modeling Rocky Scenery using Implicit Blocks" (TVC 2020)
 * 
 * Port changes:
 * - Removed OpenMP (sequential for browser)
 * - Replaced stb_image with procedural warping texture
 * - Exported C API for JavaScript integration
 * - Removed file I/O, output via memory buffers
 */

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <queue>
#include <algorithm>
#include <iostream>

// ============================================================
// vec.h - Vector math
// ============================================================

struct Vector2;
struct Vector3;

namespace Math
{
    inline double Clamp(double x, double a = 0.0, double b = 1.0)
    {
        return x < a ? a : x > b ? b : x;
    }

    inline double Step(double x, double a, double b)
    {
        if (x < a) return 0.0;
        else if (x > b) return 1.0;
        else return (x - a) / (b - a);
    }

    template<typename T>
    inline T Min(T a, T b) { return a < b ? a : b; }

    template<typename T>
    inline T Max(T a, T b) { return a > b ? a : b; }

    template<typename T>
    inline T Lerp(T a, T b, double t) { return (a * (1.0 - t)) + (b * t); }

    inline double Abs(double a) { return a < 0 ? -a : a; }

    inline double QuinticSmooth(double t)
    {
        return pow(t, 3.0) * (t * (t * 6.0 - 15.0) + 10.0);
    }
}

struct Vector3
{
    double x, y, z;

    Vector3() : x(0.0), y(0.0), z(0.0) {}
    Vector3(double n) : x(n), y(n), z(n) {}
    Vector3(double x, double y, double z) : x(x), y(y), z(z) {}

    Vector3& operator+= (const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vector3 operator-= (const Vector3& v) { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator*= (double f) { return Vector3(x * f, y * f, z * f); }
    Vector3 operator/= (double f) { return Vector3(x / f, y / f, z / f); }
    Vector3 operator*(const Vector3& u) const { return Vector3(x * u.x, y * u.y, z * u.z); }
    Vector3 operator*(double k) const { return Vector3(x * k, y * k, z * k); }
    Vector3 operator/(double k) const { return Vector3(x / k, y / k, z / k); }
    bool operator==(const Vector3& u) const { return (x == u.x && y == u.y && z == u.z); }
    bool operator!=(const Vector3& u) const { return (x != u.x || y != u.y || z != u.z); }
    Vector3 operator-(const Vector3& u) const { return Vector3(x - u.x, y - u.y, z - u.z); }
    Vector3 operator+(const Vector3& u) const { return Vector3(x + u.x, y + u.y, z + u.z); }
    Vector3 operator+(double k) const { return Vector3(x + k, y + k, z + k); }
    double operator[](int i) const { if (i == 0) return x; else if (i == 1) return y; return z; }
    double& operator[](int i) { if (i == 0) return x; else if (i == 1) return y; return z; }

    double Max() const { return Math::Max(Math::Max(x, y), z); }
    double Min() const { return Math::Min(Math::Min(x, y), z); }
    int MaxIndex() const
    {
        if (x >= y) { if (x >= z) return 0; else return 2; }
        else { if (y >= z) return 1; else return 2; }
    }
    static Vector3 Min(const Vector3& a, const Vector3& b) { return Vector3(Math::Min(a.x, b.x), Math::Min(a.y, b.y), Math::Min(a.z, b.z)); }
    static Vector3 Max(const Vector3& a, const Vector3& b) { return Vector3(Math::Max(a.x, b.x), Math::Max(a.y, b.y), Math::Max(a.z, b.z)); }
};

inline Vector3 Cross(const Vector3& u, const Vector3& v)
{
    return Vector3((u.y * v.z) - (u.z * v.y), (u.z * v.x) - (u.x * v.z), (u.x * v.y) - (u.y * v.x));
}
inline double Dot(const Vector3& u, const Vector3& v) { return u.x * v.x + u.y * v.y + u.z * v.z; }
inline double Magnitude(const Vector3& u) { return sqrt(u.x * u.x + u.y * u.y + u.z * u.z); }
inline double SquaredMagnitude(const Vector3& u) { return u.x * u.x + u.y * u.y + u.z * u.z; }
inline Vector3 Normalize(const Vector3& v) { double kk = 1.0 / Magnitude(v); return v * kk; }
inline Vector3 operator-(const Vector3& v) { return Vector3(-v.x, -v.y, -v.z); }
inline bool operator>(const Vector3& u, const Vector3& v) { return (u.x > v.x) && (u.y > v.y) && (u.z > v.z); }
inline bool operator<(const Vector3& u, const Vector3& v) { return (u.x < v.x) && (u.y < v.y) && (u.z < v.z); }
inline bool operator>=(const Vector3& u, const Vector3& v) { return (u.x >= v.x) && (u.y >= v.y) && (u.z >= v.z); }
inline bool operator<=(const Vector3& u, const Vector3& v) { return (u.x <= v.x) && (u.y <= v.y) && (u.z <= v.z); }
inline Vector3 operator*(double a, const Vector3& v) { return v * a; }
inline Vector3 Abs(const Vector3& u) { return Vector3(u[0] > 0.0 ? u[0] : -u[0], u[1] > 0.0 ? u[1] : -u[1], u[2] > 0.0 ? u[2] : -u[2]); }

struct Vector2
{
    double x, y;

    Vector2() : x(0.0), y(0.0) {}
    Vector2(double n) : x(n), y(n) {}
    Vector2(double x, double y) : x(x), y(y) {}
    Vector2(const Vector3& v) : x(v.x), y(v.z) {}

    Vector2 operator*(double k) const { return Vector2(x * k, y * k); }
    Vector2 operator/(double k) const { return Vector2(x / k, y / k); }
    Vector2 operator-(const Vector2& u) const { return Vector2(x - u.x, y - u.y); }
    Vector2 operator+(const Vector2& u) const { return Vector2(x + u.x, y + u.y); }
    double operator[](int i) const { if (i == 0) return x; return y; }
    double& operator[](int i) { if (i == 0) return x; return y; }
    Vector3 ToVector3(double yy) const { return Vector3(x, yy, y); }
    double Max() const { return Math::Max(x, y); }
    double Min() const { return Math::Min(x, y); }
};

inline Vector2 Abs(const Vector2& u) { return Vector2(u[0] > 0.0 ? u[0] : -u[0], u[1] > 0.0 ? u[1] : -u[1]); }
inline bool operator>(const Vector2& u, const Vector2& v) { return (u.x > v.x) && (u.y > v.y); }
inline bool operator<(const Vector2& u, const Vector2& v) { return (u.x < v.x) && (u.y < v.y); }
inline bool operator>=(const Vector2& u, const Vector2& v) { return (u.x >= v.x) && (u.y >= v.y); }
inline bool operator<=(const Vector2& u, const Vector2& v) { return (u.x <= v.x) && (u.y <= v.y); }

// ============================================================
// Matrix3 / Matrix4
// ============================================================

struct Matrix3
{
    double r[9];
    Matrix3() { for (int i = 0; i < 9; i++) r[i] = 0.0; }
    Matrix3(const Vector3& a, const Vector3& b, const Vector3& c)
    {
        r[0] = a[0]; r[1] = a[1]; r[2] = a[2];
        r[3] = b[0]; r[4] = b[1]; r[5] = b[2];
        r[6] = c[0]; r[7] = c[1]; r[8] = c[2];
    }
    Matrix3(double a00, double a01, double a02, double a10, double a11, double a12, double a20, double a21, double a22)
    {
        r[0] = a00; r[1] = a01; r[2] = a02;
        r[3] = a10; r[4] = a11; r[5] = a12;
        r[6] = a20; r[7] = a21; r[8] = a22;
    }
    double Determinant() const
    {
        return r[0] * r[4] * r[8] + r[1] * r[5] * r[6] + r[2] * r[3] * r[7]
             - r[2] * r[4] * r[6] - r[1] * r[3] * r[8] - r[0] * r[5] * r[7];
    }
    double& operator()(int i, int j) { return r[i + j + j + j]; }
    double operator()(int i, int j) const { return r[i + j + j + j]; }
};

struct Matrix4
{
    double r[16];
    Matrix4() { for (int i = 0; i < 16; i++) r[i] = 0.0; }
    Matrix4(const Matrix3& a)
    {
        r[0] = a.r[0]; r[1] = a.r[1]; r[2] = a.r[2];
        r[4] = a.r[3]; r[5] = a.r[4]; r[6] = a.r[5];
        r[8] = a.r[6]; r[9] = a.r[7]; r[10] = a.r[8];
        r[3] = r[7] = r[11] = 0.0;
        r[12] = r[13] = r[14] = 0.0;
        r[15] = 1.0;
    }
    double& operator()(int i, int j) { return r[i + j + j + j]; }
    double operator()(int i, int j) const { return r[i + j + j + j]; }
    double Determinant() const
    {
        const Matrix4& M = *this;
        return M(0, 0) * Matrix3(M(1, 1), M(1, 2), M(1, 3), M(2, 1), M(2, 2), M(2, 3), M(3, 1), M(3, 2), M(3, 3)).Determinant()
             - M(1, 0) * Matrix3(M(0, 1), M(0, 2), M(0, 3), M(2, 1), M(2, 2), M(2, 3), M(3, 1), M(3, 2), M(3, 3)).Determinant()
             + M(2, 0) * Matrix3(M(0, 1), M(0, 2), M(0, 3), M(1, 1), M(1, 2), M(1, 3), M(3, 1), M(3, 2), M(3, 3)).Determinant()
             - M(3, 0) * Matrix3(M(0, 1), M(0, 2), M(0, 3), M(1, 1), M(1, 2), M(1, 3), M(2, 1), M(2, 2), M(2, 3)).Determinant();
    }
};

// ============================================================
// basics.h - Random, Ray, Plane, Triangle, Box, Sphere, etc.
// ============================================================

class Random
{
public:
    static inline double Uniform(double a, double b) { return a + (b - a) * Uniform(); }
    static inline double Uniform() { return double(rand()) / RAND_MAX; }
    static inline int Integer() { return rand(); }
};

class Ray
{
public:
    Vector3 o, d;
    Ray() {}
    Ray(const Vector3& oo, const Vector3& dd) : o(oo), d(dd) {}
    Vector3 operator()(double t) const { return o + t * d; }
};

class Plane
{
protected:
    Vector3 p;
    Vector3 n;
    double c;
public:
    Plane() : p(Vector3(0)), n(Vector3(0)), c(0.0) {}
    Plane(const Vector3& p, const Vector3& n) : p(p), n(n), c(Dot(p, n)) {}

    Vector3 Normal() const { return n; }
    Vector3 Point() const { return p; }
    double Signed(const Vector3& pp) const { return Dot(n, pp) - c; }
    int Side(const Vector3& pp) const
    {
        double r = Dot(n, pp) - c;
        if (r > 1e-6) return 1;
        else if (r < -1e-6) return -1;
        return 0;
    }
    static bool Intersection(const Plane& a, const Plane& b, const Plane& c, Vector3& p)
    {
        double e = Matrix4(Matrix3(a.Normal(), b.Normal(), c.Normal())).Determinant();
        if (e < 1e-06) return false;
        p = (Dot(a.Point(), a.Normal()) * (Cross(b.Normal(), c.Normal()))) +
            (Dot(b.Point(), b.Normal()) * (Cross(c.Normal(), a.Normal()))) +
            (Dot(c.Point(), c.Normal()) * (Cross(a.Normal(), b.Normal())));
        p = p / (-e);
        return true;
    }
    static std::vector<Vector3> ConvexPoints(const std::vector<Plane>& planes)
    {
        std::vector<Vector3> pts;
        for (int i = 0; i < (int)planes.size(); i++)
        {
            for (int j = i + 1; j < (int)planes.size(); j++)
            {
                for (int k = j + 1; k < (int)planes.size(); k++)
                {
                    Vector3 p;
                    bool intersect = Intersection(planes[i], planes[j], planes[k], p);
                    if (intersect)
                    {
                        bool isInside = true;
                        for (int l = 0; l < (int)planes.size(); l++)
                        {
                            if (l == i || l == j || l == k) continue;
                            int s = planes[l].Side(p);
                            if (s > 0) { isInside = false; break; }
                        }
                        if (isInside) pts.push_back(p);
                    }
                }
            }
        }
        return pts;
    }
};

class Triangle
{
private:
    Vector3 pts[3];
public:
    Triangle() {}
    Triangle(const Vector3& a, const Vector3& b, const Vector3& c) { pts[0] = a; pts[1] = b; pts[2] = c; }
    Vector3 Center() const { return (pts[0] + pts[1] + pts[2]) / 3.0; }
    Vector3 Normal() const { return Normalize(Cross(pts[1] - pts[0], pts[2] - pts[0])); }
};

class Box
{
protected:
    Vector3 a, b;
public:
    Box() {}
    Box(const Vector3& A, const Vector3& B) : a(A), b(B) {}
    Box(const Vector3& C, double R) { Vector3 RR = Vector3(R); a = C - RR; b = C + RR; }
    Box(const Box& b1, const Box& b2) { a = Vector3::Min(b1.a, b2.a); b = Vector3::Max(b1.b, b2.b); }
    Box(const std::vector<Vector3>& pts)
    {
        for (int j = 0; j < 3; j++)
        {
            a[j] = pts.at(0)[j];
            b[j] = pts.at(0)[j];
            for (int i = 1; i < (int)pts.size(); i++)
            {
                if (pts.at(i)[j] < a[j]) a[j] = pts.at(i)[j];
                if (pts.at(i)[j] > b[j]) b[j] = pts.at(i)[j];
            }
        }
    }

    bool Contains(const Vector3& p) const { return (p > a && p < b); }
    Box Extended(const Vector3& r) const { return Box(a - r, b + r); }
    Vector3 Center() const { return (a + b) / 2.0; }
    double Distance(const Vector3& p) const
    {
        double r = 0.0;
        for (int i = 0; i < 3; i++)
        {
            if (p[i] < a[i]) { double s = p[i] - a[i]; r += s * s; }
            else if (p[i] > b[i]) { double s = p[i] - b[i]; r += s * s; }
        }
        return r;
    }
    Vector3 Diagonal() const { return (b - a); }
    Vector3 Size() const { return (b - a); }
    Vector3 RandomInside() const
    {
        Vector3 s = b - a;
        double randw = Random::Uniform(-1.0 * s[0] / 2.0, s[0] / 2.0);
        double randh = Random::Uniform(-1.0 * s[1] / 2.0, s[1] / 2.0);
        double randl = Random::Uniform(-1.0 * s[2] / 2.0, s[2] / 2.0);
        return (a + b) / 2.0 + Vector3(randw, randh, randl);
    }
    Vector3 Vertex(int i) const { if (i == 0) return a; return b; }
    Vector3& operator[](int i) { if (i == 0) return a; return b; }
    Vector3 operator[](int i) const { if (i == 0) return a; return b; }
};

class Box2D
{
protected:
    Vector2 a, b;
public:
    Box2D() : a(Vector2(0)), b(Vector2(0)) {}
    Box2D(const Vector2& A, const Vector2& B) : a(A), b(B) {}
    Box2D(const Vector2& C, double R) { Vector2 RR = Vector2(R); a = C - RR; b = C + RR; }

    bool Contains(const Vector2& p) const { return (p > a && p < b); }
    Vector2 Vertex(int i) const { if (i == 0) return a; return b; }
    Vector2 Center() const { return (a + b) / 2.0; }
    Vector2& operator[](int i) { if (i == 0) return a; return b; }
    Vector2 operator[](int i) const { if (i == 0) return a; return b; }
};

class Circle
{
protected:
    Vector3 center;
    Vector3 normal;
    double radius;
public:
    Circle(const Vector3& c, const Vector3& n, double r) : center(c), normal(n), radius(r) {}
    Vector3 Center() const { return center; }
    Vector3 Normal() const { return normal; }
    double Radius() const { return radius; }
    bool Intersect(const Ray& ray, double& t) const
    {
        double e = Dot(normal, ray.d);
        if (fabs(e) < 1e-6) return false;
        t = Dot(center - ray.o, normal) / e;
        if (t < 0.0) return false;
        Vector3 p = ray(t) - center;
        if (Dot(p, p) > radius * radius) return false;
        return true;
    }
};

class Sphere
{
protected:
    Vector3 center;
    double radius;
public:
    Sphere(const Vector3& c, double r) : center(c), radius(r) {}
    double Distance(const Vector3& p) const
    {
        double a = Dot((p - center), (p - center));
        if (a < radius * radius) return 0.0;
        a = sqrt(a) - radius;
        a *= a;
        return a;
    }
    Vector3 RandomSurface() const { return Vector3(0); }
    Vector3 Center() const { return center; }
    double Radius() const { return radius; }
};

// ScalarField2D
class ScalarField2D
{
protected:
    Box2D box;
    int nx, ny;
    std::vector<double> values;
public:
    ScalarField2D() : nx(0), ny(0) {}
    ScalarField2D(int nx, int ny, const Box2D& bbox) : box(bbox), nx(nx), ny(ny) { values.resize(size_t(nx * ny)); }

    bool Inside(int i, int j) const { return !(i < 0 || i >= nx || j < 0 || j >= ny); }
    int ToIndex1D(int i, int j) const { return i * nx + j; }
    double Get(int row, int column) const { return values[ToIndex1D(row, column)]; }
    void Set(int row, int column, double v) { values[ToIndex1D(row, column)] = v; }

    double GetValueBilinear(const Vector2& p) const
    {
        Vector2 q = p - box.Vertex(0);
        Vector2 d = box.Vertex(1) - box.Vertex(0);
        double texelX = 1.0 / double(nx - 1);
        double texelY = 1.0 / double(ny - 1);
        double u = q[0] / d[0];
        double v = q[1] / d[1];
        int i = int(v * (ny - 1));
        int j = int(u * (nx - 1));
        if (!Inside(i, j) || !Inside(i + 1, j + 1)) return -1.0;
        double anchorU = j * texelX;
        double anchorV = i * texelY;
        double localU = (u - anchorU) / texelX;
        double localV = (v - anchorV) / texelY;
        double v1 = Get(i, j);
        double v2 = Get(i + 1, j);
        double v3 = Get(i + 1, j + 1);
        double v4 = Get(i, j + 1);
        return (1 - localU) * (1 - localV) * v1
             + (1 - localU) * localV * v2
             + localU * (1 - localV) * v4
             + localU * localV * v3;
    }
};

// ============================================================
// noise.h - Perlin noise (procedural warping replacement)
// ============================================================

static int Perm[512] =
{
    151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233,
    7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
    190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219,
    203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174,
    20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27,
    166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230,
    220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25,
    63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
    200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173,
    186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118,
    126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182,
    189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163,
    70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19,
    98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246,
    97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162,
    241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181,
    199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150,
    254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128,
    195, 78, 66, 215, 61, 156, 180, 151, 160, 137, 91, 90, 15, 131,
    13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69,
    142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75,
    0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
    88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74,
    165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229,
    122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
    102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132,
    187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86,
    164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124,
    123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59,
    227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119,
    248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172,
    9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178,
    185, 112, 104, 218, 246, 97, 228, 251, 34, 242, 193, 238, 210,
    144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239,
    107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176,
    115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114,
    67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};

class PerlinNoise
{
public:
    static inline double Gradient(int hash, double x, double y, double z)
    {
        const int h = hash & 15;
        const double u = h < 8 ? x : y, v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    static inline double GetValue(const Vector3& p)
    {
        double x = p.x, y = p.y, z = p.z;
        const int unit_x = int(floor(x)) & 255;
        const int unit_y = int(floor(y)) & 255;
        const int unit_z = int(floor(z)) & 255;
        x = x - floor(x);
        y = y - floor(y);
        z = z - floor(z);
        const double u = Math::QuinticSmooth(x);
        const double v = Math::QuinticSmooth(y);
        const double w = Math::QuinticSmooth(z);
        const int a = Perm[unit_x] + unit_y;
        const int aa = Perm[a] + unit_z;
        const int ab = Perm[a + 1] + unit_z;
        const int b = Perm[unit_x + 1] + unit_y;
        const int ba = Perm[b] + unit_z;
        const int bb = Perm[b + 1] + unit_z;
        const double l1 = Math::Lerp(Gradient(Perm[aa], x, y, z), Gradient(Perm[ba], x - 1, y, z), u);
        const double l2 = Math::Lerp(Gradient(Perm[ab], x, y - 1, z), Gradient(Perm[bb], x - 1, y - 1, z), u);
        const double l3 = Math::Lerp(Gradient(Perm[aa + 1], x, y, z - 1), Gradient(Perm[ba + 1], x - 1, y, z - 1), u);
        const double l4 = Math::Lerp(Gradient(Perm[ab + 1], x, y - 1, z - 1), Gradient(Perm[bb + 1], x - 1, y - 1, z - 1), u);
        const double l5 = Math::Lerp(l1, l2, v);
        const double l6 = Math::Lerp(l3, l4, v);
        return Math::Lerp(l5, l6, w);
    }

    static inline double fBm(const Vector3& p, double a, double f, int o)
    {
        double ret = 0.0;
        double freq = f;
        double amp = a;
        for (int i = 0; i < o; i++)
        {
            ret += (GetValue(p * freq) * 0.5 + 0.5) * amp;
            amp *= 0.5;
            freq *= 2.0;
        }
        return ret;
    }
};

// ============================================================
// blocks.h / blocks.cpp / blocks-sdf.cpp - Core fracture logic
// ============================================================

enum FractureType
{
    Equidimensional = 0,
    Tabular = 1,
    Rhombohedral = 2,
    Polyhedral = 3
};

struct PointSet3
{
    std::vector<Vector3> pts;
    Vector3 At(int i) const { return pts[i]; }
    Vector3& At(int i) { return pts[i]; }
    int Size() const { return int(pts.size()); }
};

struct FractureSet
{
    std::vector<Circle> fractures;
    Circle At(int i) const { return fractures[i]; }
    int Size() const { return int(fractures.size()); }
};

struct BlockCluster
{
    std::vector<Vector3> pts;
};

struct SDFNode
{
    Box box;
    SDFNode() {}
    SDFNode(const Box& box) : box(box) {}
    Vector3 Gradient(const Vector3& p) const
    {
        static const double Epsilon = 0.01;
        double x = Signed(Vector3(p[0] - Epsilon, p[1], p[2])) - Signed(Vector3(p[0] + Epsilon, p[1], p[2]));
        double y = Signed(Vector3(p[0], p[1] - Epsilon, p[2])) - Signed(Vector3(p[0], p[1] + Epsilon, p[2]));
        double z = Signed(Vector3(p[0], p[1], p[2] - Epsilon)) - Signed(Vector3(p[0], p[1], p[2] + Epsilon));
        return Vector3(x, y, z) * (0.5 / Epsilon);
    }
    virtual double Signed(const Vector3& p) const = 0;
    virtual ~SDFNode() {}
};

struct SDFUnionSphereLOD : public SDFNode
{
    Sphere sphere;
    double re;
    SDFNode* e[2];

    SDFUnionSphereLOD(SDFNode* a, SDFNode* b, double re)
        : SDFNode(Box(a->box, b->box).Extended(Vector3(re))), re(re), sphere(Sphere(box.Center(), box.Size().Max()))
    {
        e[0] = a;
        e[1] = b;
    }

    double Signed(const Vector3& p) const
    {
        double sd = sphere.Distance(p);
        if (sd > re) return sd;
        double se = Math::Min(e[0]->Signed(p), e[1]->Signed(p));
        if (sd < sphere.Radius()) return se;
        double a = (sd - sphere.Radius()) / (re - sphere.Radius());
        return (1.0 - a) * se + a * sd;
    }

    static SDFNode* OptimizedBVH(std::vector<SDFNode*>& nodes, double re)
    {
        if (nodes.size() == 0) return nullptr;
        return OptimizedBVHRecursive(nodes, 0, int(nodes.size()), re);
    }

    static SDFNode* OptimizedBVHRecursive(std::vector<SDFNode*>& pts, int begin, int end, double re)
    {
        struct BVHPartitionPredicate
        {
            int axis;
            double cut;
            BVHPartitionPredicate(int a, double c) : axis(a), cut(c) {}
            bool operator()(SDFNode* p) const { return (p->box.Center()[axis] < cut); }
        };

        int nodeCount = end - begin;
        if (nodeCount <= 1) return pts[begin];

        Box bbox = pts[begin]->box;
        for (int i = begin + 1; i < end; i++)
            bbox = Box(bbox, pts[i]->box);

        int stretchedAxis = bbox.Diagonal().MaxIndex();
        double axisMiddleCut = (bbox[0][stretchedAxis] + bbox[1][stretchedAxis]) / 2.0;
        auto pmid = std::partition(pts.begin() + begin, pts.begin() + end, BVHPartitionPredicate(stretchedAxis, axisMiddleCut));
        int midIndex = std::distance(pts.begin(), pmid);
        if (midIndex == begin || midIndex == end)
            midIndex = (begin + end) / 2;

        SDFNode* left = OptimizedBVHRecursive(pts, begin, midIndex, re);
        SDFNode* right = OptimizedBVHRecursive(pts, midIndex, end, re);
        return new SDFUnionSphereLOD(left, right, re);
    }
};

// Global warping field (procedurally generated)
static ScalarField2D warpingField;

struct SDFGradientWarp : public SDFNode
{
    SDFNode* e;

    SDFGradientWarp(SDFNode* e) : e(e) { box = e->box; }

    double WarpingStrength(const Vector3& p, const Vector3& n) const
    {
        const double texScale = 0.1642;
        Vector2 x = Abs(Vector2(p[2], p[1])) * texScale;
        Vector2 y = Abs(Vector2(p[0], p[2])) * texScale;
        Vector2 z = Abs(Vector2(p[1], p[0])) * texScale;

        double tmp;
        x = Vector2(modf(x[0], &tmp), modf(x[1], &tmp));
        y = Vector2(modf(y[0], &tmp), modf(y[1], &tmp));
        z = Vector2(modf(z[0], &tmp), modf(z[1], &tmp));

        Vector3 ai = Abs(n);
        ai = ai / (ai[0] + ai[1] + ai[2]);

        return ai[0] * warpingField.GetValueBilinear(x)
             + ai[1] * warpingField.GetValueBilinear(y)
             + ai[2] * warpingField.GetValueBilinear(z);
    }

    double Signed(const Vector3& p) const
    {
        Vector3 g = e->Gradient(p);
        double s = 0.65 * WarpingStrength(p, -Normalize(g));
        return e->Signed(p + g * s);
    }
};

struct SDFBlock : public SDFNode
{
    std::vector<Plane> planes;
    double smoothRadius;

    SDFBlock(const std::vector<Plane>& pl, double sr)
        : SDFNode(Box(Plane::ConvexPoints(pl)).Extended(Vector3(0.01)))
    {
        planes = pl;
        smoothRadius = sr;
        box = box.Extended(Vector3(sr));
    }

    double SmoothingPolynomial(double d1, double d2, double sr) const
    {
        double h = Math::Max(sr - Math::Abs(d1 - d2), 0.0) / sr;
        return Math::Min(d1, d2) - h * h * sr * 0.25;
    }

    double Signed(const Vector3& p) const
    {
        double d = planes.at(0).Signed(p);
        for (int i = 1; i < (int)planes.size(); i++)
        {
            double dd = planes.at(i).Signed(p);
            d = -SmoothingPolynomial(-d, -dd, smoothRadius);
        }
        return d;
    }
};

// Generate procedural warping texture (replaces stb_image loading)
static void GenerateProceduralWarpingField()
{
    const int size = 128;
    warpingField = ScalarField2D(size, size, Box2D(Vector2(0), Vector2(1)));
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            double u = double(i) / (size - 1);
            double v = double(j) / (size - 1);
            double val = PerlinNoise::fBm(Vector3(u * 8.0, v * 8.0, 0.0), 1.0, 1.0, 6);
            val = Math::Clamp(val, 0.0, 1.0);
            warpingField.Set(i, j, val);
        }
    }
}

static bool BreakFractureConstraint(const Vector3& p, const Vector3& c, const FractureSet& fractures)
{
    double tmax = Magnitude(p - c);
    Ray ray = Ray(p, Normalize(c - p));
    for (int ci = 0; ci < fractures.Size(); ci++)
    {
        double t;
        if (fractures.At(ci).Intersect(ray, t) && t < tmax)
            return true;
    }
    return false;
}

static bool CanBeLinkedToCluster(const Vector3& candidate, const std::vector<Vector3>& cluster, const FractureSet& fractures, double R_Max)
{
    for (int i = 0; i < (int)cluster.size(); i++)
    {
        Vector3 p = cluster[i];
        if (SquaredMagnitude(candidate - p) > R_Max) return false;
        for (int c = 0; c < fractures.Size(); c++)
        {
            double tmax = Magnitude(p - candidate);
            double t;
            if (fractures.At(c).Intersect(Ray(p, Normalize(candidate - p)), t) && t < tmax)
                return false;
        }
    }
    return true;
}

PointSet3 PoissonSamplingBox(const Box& box, double r, int n)
{
    PointSet3 set;
    double c = 4.0 * r * r;
    for (int i = 0; i < n; i++)
    {
        Vector3 t = box.RandomInside();
        bool hit = false;
        for (int j = 0; j < set.Size(); j++)
        {
            if (SquaredMagnitude(t - set.At(j)) < c) { hit = true; break; }
        }
        if (hit == false) set.pts.push_back(t);
    }
    return set;
}

FractureSet GenerateFractures(FractureType type, const Box& box, double r)
{
    FractureSet set;
    if (type == FractureType::Equidimensional)
    {
        Box inflatedRockDomain = box.Extended(Vector3(-3.0));
        PointSet3 samples = PoissonSamplingBox(inflatedRockDomain, 3.0, 1000);
        for (int i = 0; i < samples.Size(); i++)
        {
            int a = Random::Integer() % 3;
            Vector3 axis = a == 0 ? Vector3(1, 0, 0) : a == 1 ? Vector3(0, 1, 0) : Vector3(0, 0, 1);
            double fr = Random::Uniform(10.0, 15.0);
            set.fractures.push_back(Circle(samples.At(i), axis, fr));
        }
    }
    else if (type == FractureType::Rhombohedral)
    {
        Box inflatedRockDomain = box.Extended(Vector3(-3.0));
        PointSet3 samples = PoissonSamplingBox(inflatedRockDomain, 3.0, 1000);
        for (int i = 0; i < samples.Size(); i++)
        {
            int a = Random::Integer() % 3;
            Vector3 axis = a == 0 ? Vector3(0.5, 0.5, 0) : a == 1 ? Vector3(0, 0.5, 0.5) : Vector3(0.5, 0, 0.5);
            double fr = Random::Uniform(10.0, 15.0);
            set.fractures.push_back(Circle(samples.At(i), axis, fr));
        }
    }
    else if (type == FractureType::Polyhedral)
    {
        PointSet3 samples = PoissonSamplingBox(box, 2.0, 1000);
        for (int i = 0; i < samples.Size(); i++)
        {
            double fr = Random::Uniform(3.0, 8.0);
            // Random surface point on unit sphere
            double theta = Random::Uniform(0, 2.0 * M_PI);
            double phi = acos(Random::Uniform(-1.0, 1.0));
            Vector3 axis = Vector3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
            set.fractures.push_back(Circle(samples.At(i), axis, fr));
        }
    }
    else if (type == FractureType::Tabular)
    {
        const int fracturing = 10;
        Vector3 p = box[0];
        p.x += box.Diagonal()[0] / 2.0;
        p.z += box.Diagonal()[1] / 2.0;
        double step = box.Size()[2] / double(fracturing);
        double noiseStep = step / 10.0;
        for (int i = 0; i < fracturing - 1; i++)
        {
            p.y += step + Random::Uniform(-noiseStep, noiseStep);
            Vector3 axis = Vector3(0, -1, 0);
            set.fractures.push_back(Circle(p, axis, 20.0));
        }
    }
    return set;
}

std::vector<BlockCluster> ComputeBlockClusters(PointSet3& set, const FractureSet& frac)
{
    const int allPtsSize = set.Size();
    const float R_Neighborhood = 2.5f * 2.5f;

    std::vector<std::vector<int>> graph;
    graph.resize(allPtsSize);
    for (int i = 0; i < allPtsSize; i++)
    {
        Vector3 p = set.At(i);
        for (int j = 0; j < allPtsSize; j++)
        {
            if (i == j) continue;
            Vector3 q = set.At(j);
            if (SquaredMagnitude(p - q) < R_Neighborhood && !BreakFractureConstraint(p, q, frac))
                graph[i].push_back(j);
        }
    }

    const float R_Max_Block = 10.5f * 10.5f;
    std::vector<bool> visitedFlags(allPtsSize, false);
    std::vector<BlockCluster> clusters;
    for (int j = 0; j < allPtsSize; j++)
    {
        std::queue<int> toVisit;
        toVisit.push(j);
        std::vector<Vector3> cluster;
        while (toVisit.empty() == false)
        {
            int index = toVisit.front();
            toVisit.pop();
            if (visitedFlags[index]) continue;
            Vector3 q = set.At(index);
            if (!CanBeLinkedToCluster(q, cluster, frac, R_Max_Block)) continue;
            cluster.push_back(q);
            visitedFlags[index] = true;
            for (int i = 0; i < (int)graph[index].size(); i++)
            {
                if (visitedFlags[graph[index][i]]) continue;
                toVisit.push(graph[index][i]);
            }
        }
        if (cluster.size() > 8)
            clusters.push_back({ cluster });
    }
    return clusters;
}

// ============================================================
// convhull_3d - Embedded (simplified for Emscripten)
// ============================================================

// We include the full convhull_3d implementation inline
#define CONVHULL_3D_ENABLE
// The convhull_3d implementation is included below

typedef double CH_FLOAT;
typedef struct _ch_vertex {
    union {
        CH_FLOAT v[3];
        struct { CH_FLOAT x, y, z; };
    };
} ch_vertex;
typedef ch_vertex ch_vec3;

// Forward declaration
extern "C" void convhull_3d_build(ch_vertex* const in_vertices, const int nVert, int** out_faces, int* nOut_faces);

// Full convhull_3d implementation embedded
#include "convhull_3d_impl.inc"

// ============================================================
// Marching Cubes - Embedded
// ============================================================

#define MC_IMPLEM_ENABLE
#define MC_CPP_USE_DOUBLE_PRECISION

namespace MC
{
    typedef double MC_FLOAT;
    typedef unsigned int muint;

    typedef struct mcVec3f
    {
        union {
            MC_FLOAT v[3];
            struct { MC_FLOAT x, y, z; };
        };
        inline mcVec3f& operator+=(const mcVec3f& r) { x += r.x; y += r.y; z += r.z; return *this; }
        inline MC_FLOAT& operator[](int i) { return v[i]; }
    } mcVec3f;

    static inline MC_FLOAT mc_internalLength2(const mcVec3f& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
    static inline MC_FLOAT mc_internalLength(const mcVec3f& v) { return std::sqrt(mc_internalLength2(v)); }
    static inline mcVec3f mc_internalNormalize(const mcVec3f& v) { MC_FLOAT vv = mc_internalLength(v); return mcVec3f({ v.x / vv, v.y / vv, v.z / vv }); }
    static inline mcVec3f mc_internalCross(const mcVec3f& v1, const mcVec3f& v2) { return mcVec3f({ v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x }); }
    inline mcVec3f operator-(const mcVec3f& l, const mcVec3f r) { return mcVec3f({ l.x - r.x, l.y - r.y, l.z - r.z }); }

    typedef struct mcVec3i
    {
        union {
            muint v[3];
            struct { muint x, y, z; };
        };
        inline muint& operator[](int i) { return v[i]; }
    } mcVec3i;

    typedef struct mcMesh
    {
        std::vector<mcVec3f> vertices;
        std::vector<mcVec3f> normals;
        std::vector<muint> indices;
    } mcMesh;

    // Marching cubes lookup table
    #include "mc_tables.inc"

    static inline muint mc_internalToIndex1D(muint i, muint j, muint k, const mcVec3i& size) { return (k * size.y + j) * size.x + i; }
    static inline muint mc_internalToIndex1DSlab(muint i, muint j, muint k, const mcVec3i& size) { return size.x * size.y * (k % 2) + j * size.x + i; }

    static void mc_internalComputeEdge(mcVec3i* slab_inds, mcMesh& outputMesh, double va, double vb, int axis, muint x, muint y, muint z, const mcVec3i& size)
    {
        if ((va < 0.0) == (vb < 0.0)) return;
        mcVec3f v = { MC_FLOAT(x), MC_FLOAT(y), MC_FLOAT(z) };
        v[axis] += va / (va - vb);
        slab_inds[mc_internalToIndex1DSlab(x, y, z, size)][axis] = muint(outputMesh.vertices.size());
        outputMesh.vertices.push_back(v);
        outputMesh.normals.push_back(mcVec3f({ 0, 0, 0 }));
    }

    static inline void mc_internalAccumulateNormal(mcMesh& mesh, muint a, muint b, muint c)
    {
        mcVec3f& va = mesh.vertices[a];
        mcVec3f& vb = mesh.vertices[b];
        mcVec3f& vc = mesh.vertices[c];
        mcVec3f ab = va - vb;
        mcVec3f cb = vc - vb;
        mcVec3f n = mc_internalCross(cb, ab);
        mesh.normals[a] += n;
        mesh.normals[b] += n;
        mesh.normals[c] += n;
    }

    void marching_cube(MC_FLOAT* field, muint nx, muint ny, muint nz, mcMesh& outputMesh)
    {
        outputMesh.vertices.reserve(100000);
        outputMesh.normals.reserve(100000);
        outputMesh.indices.reserve(400000);

        const mcVec3i size = { nx, ny, nz };
        mcVec3i* slab_inds = new mcVec3i[nx * ny * 2];
        MC_FLOAT vs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        muint edge_indices[12];
        for (muint z = 0; z < nx - 1; z++)
        {
            for (muint y = 0; y < ny - 1; y++)
            {
                for (muint x = 0; x < nz - 1; x++)
                {
                    vs[0] = field[mc_internalToIndex1D(x, y, z, size)];
                    vs[1] = field[mc_internalToIndex1D(x + 1, y, z, size)];
                    vs[2] = field[mc_internalToIndex1D(x, y + 1, z, size)];
                    vs[3] = field[mc_internalToIndex1D(x + 1, y + 1, z, size)];
                    vs[4] = field[mc_internalToIndex1D(x, y, z + 1, size)];
                    vs[5] = field[mc_internalToIndex1D(x + 1, y, z + 1, size)];
                    vs[6] = field[mc_internalToIndex1D(x, y + 1, z + 1, size)];
                    vs[7] = field[mc_internalToIndex1D(x + 1, y + 1, z + 1, size)];

                    const int config_n =
                        ((vs[0] < 0) << 0) | ((vs[1] < 0) << 1) | ((vs[2] < 0) << 2) | ((vs[3] < 0) << 3) |
                        ((vs[4] < 0) << 4) | ((vs[5] < 0) << 5) | ((vs[6] < 0) << 6) | ((vs[7] < 0) << 7);
                    if (config_n == 0 || config_n == 255) continue;

                    if (y == 0 && z == 0) mc_internalComputeEdge(slab_inds, outputMesh, vs[0], vs[1], 0, x, y, z, size);
                    if (z == 0) mc_internalComputeEdge(slab_inds, outputMesh, vs[2], vs[3], 0, x, y + 1, z, size);
                    if (y == 0) mc_internalComputeEdge(slab_inds, outputMesh, vs[4], vs[5], 0, x, y, z + 1, size);
                    mc_internalComputeEdge(slab_inds, outputMesh, vs[6], vs[7], 0, x, y + 1, z + 1, size);

                    if (x == 0 && z == 0) mc_internalComputeEdge(slab_inds, outputMesh, vs[0], vs[2], 1, x, y, z, size);
                    if (z == 0) mc_internalComputeEdge(slab_inds, outputMesh, vs[1], vs[3], 1, x + 1, y, z, size);
                    if (x == 0) mc_internalComputeEdge(slab_inds, outputMesh, vs[4], vs[6], 1, x, y, z + 1, size);
                    mc_internalComputeEdge(slab_inds, outputMesh, vs[5], vs[7], 1, x + 1, y, z + 1, size);

                    if (x == 0 && y == 0) mc_internalComputeEdge(slab_inds, outputMesh, vs[0], vs[4], 2, x, y, z, size);
                    if (y == 0) mc_internalComputeEdge(slab_inds, outputMesh, vs[1], vs[5], 2, x + 1, y, z, size);
                    if (x == 0) mc_internalComputeEdge(slab_inds, outputMesh, vs[2], vs[6], 2, x, y + 1, z, size);
                    mc_internalComputeEdge(slab_inds, outputMesh, vs[3], vs[7], 2, x + 1, y + 1, z, size);

                    edge_indices[0] = slab_inds[mc_internalToIndex1DSlab(x, y, z, size)].x;
                    edge_indices[1] = slab_inds[mc_internalToIndex1DSlab(x, y + 1, z, size)].x;
                    edge_indices[2] = slab_inds[mc_internalToIndex1DSlab(x, y, z + 1, size)].x;
                    edge_indices[3] = slab_inds[mc_internalToIndex1DSlab(x, y + 1, z + 1, size)].x;
                    edge_indices[4] = slab_inds[mc_internalToIndex1DSlab(x, y, z, size)].y;
                    edge_indices[5] = slab_inds[mc_internalToIndex1DSlab(x + 1, y, z, size)].y;
                    edge_indices[6] = slab_inds[mc_internalToIndex1DSlab(x, y, z + 1, size)].y;
                    edge_indices[7] = slab_inds[mc_internalToIndex1DSlab(x + 1, y, z + 1, size)].y;
                    edge_indices[8] = slab_inds[mc_internalToIndex1DSlab(x, y, z, size)].z;
                    edge_indices[9] = slab_inds[mc_internalToIndex1DSlab(x + 1, y, z, size)].z;
                    edge_indices[10] = slab_inds[mc_internalToIndex1DSlab(x, y + 1, z, size)].z;
                    edge_indices[11] = slab_inds[mc_internalToIndex1DSlab(x + 1, y + 1, z, size)].z;

                    const uint64_t& config = mc_internalMarching_cube_tris[config_n];
                    const size_t n_triangles = config & 0xF;
                    const size_t n_indices = n_triangles * 3;
                    const size_t indexBase = outputMesh.indices.size();
                    int offset = 4;
                    for (size_t i = 0; i < n_indices; i++)
                    {
                        const int edge = (config >> offset) & 0xF;
                        outputMesh.indices.push_back(edge_indices[edge]);
                        offset += 4;
                    }
                    for (size_t i = 0; i < n_triangles; i++)
                    {
                        mc_internalAccumulateNormal(outputMesh,
                            outputMesh.indices[indexBase + i * 3 + 0],
                            outputMesh.indices[indexBase + i * 3 + 1],
                            outputMesh.indices[indexBase + i * 3 + 2]);
                    }
                }
            }
        }
        for (size_t i = 0; i < outputMesh.normals.size(); i++)
            outputMesh.normals[i] = mc_internalNormalize(outputMesh.normals[i]);
        delete[] slab_inds;
    }
}

// ============================================================
// ComputeBlockSDF / PolygonizeSDF
// ============================================================

SDFNode* ComputeBlockSDF(const std::vector<BlockCluster>& clusters)
{
    std::vector<SDFNode*> primitives;
    for (int k = 0; k < (int)clusters.size(); k++)
    {
        std::vector<Vector3> allPts = clusters[k].pts;
        int n = int(allPts.size());
        if (n <= 4) continue;

        ch_vertex* vertices = new ch_vertex[n];
        for (int i = 0; i < n; i++)
            vertices[i] = { allPts[i][0], allPts[i][1], allPts[i][2] };
        int* faceIndices = NULL;
        int nFaces;
        convhull_3d_build(vertices, n, &faceIndices, &nFaces);
        if (nFaces == 0)
        {
            delete[] vertices;
            continue;
        }

        std::vector<Plane> planes;
        for (int i = 0; i < nFaces; i++)
        {
            const int j = i * 3;
            Vector3 v1 = Vector3(float(vertices[faceIndices[j + 0]].x), float(vertices[faceIndices[j + 0]].y), float(vertices[faceIndices[j + 0]].z));
            Vector3 v2 = Vector3(float(vertices[faceIndices[j + 1]].x), float(vertices[faceIndices[j + 1]].y), float(vertices[faceIndices[j + 1]].z));
            Vector3 v3 = Vector3(float(vertices[faceIndices[j + 2]].x), float(vertices[faceIndices[j + 2]].y), float(vertices[faceIndices[j + 2]].z));
            Vector3 pn = Triangle(v1, v2, v3).Normal();
            Vector3 pc = Triangle(v1, v2, v3).Center();
            planes.push_back(Plane(pc, pn));
        }

        auto convex = Plane::ConvexPoints(planes);
        if (convex.size() > 0)
        {
            const double smoothRadius = 0.25;
            primitives.push_back(new SDFGradientWarp(new SDFBlock(planes, smoothRadius)));
        }

        delete[] vertices;
        delete[] faceIndices;
    }
    return SDFUnionSphereLOD::OptimizedBVH(primitives, 0.5);
}

MC::mcMesh PolygonizeSDF(const Box& box, SDFNode* node, int n)
{
    // Compute field function (sequential - no OpenMP in browser)
    MC::MC_FLOAT* field = new MC::MC_FLOAT[n * n * n];
    Vector3 cellDiagonal = (box[1] - box[0]) / (n - 1);
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            for (int k = 0; k < n; k++)
            {
                Vector3 p = Vector3(box[0][0] + i * cellDiagonal[0], box[0][1] + j * cellDiagonal[1], box[0][2] + k * cellDiagonal[2]);
                field[(k * n + j) * n + i] = (float)node->Signed(p);
            }
        }
    }

    MC::mcMesh mesh;
    MC::marching_cube(field, n, n, n, mesh);
    delete[] field;
    return mesh;
}

// ============================================================
// Emscripten Export API
// ============================================================

// Output mesh data structure for JS
static std::vector<float> g_vertices;
static std::vector<float> g_normals;
static std::vector<unsigned int> g_indices;

extern "C" {

// Generate rock fracture mesh
// fractureType: 0=Equidimensional, 1=Tabular, 2=Rhombohedral, 3=Polyhedral
// resolution: marching cubes resolution (e.g., 80-200)
// seed: random seed
// tileSize: size of the cubic tile
int generateRock(int fractureType, int resolution, int seed, double tileSize)
{
    srand(seed);

    // Generate procedural warping field
    GenerateProceduralWarpingField();

    FractureType type = (FractureType)fractureType;
    Box tile = Box(Vector3(0), tileSize / 2.0);

    // (1) Sample a cubic tile
    PointSet3 samples = PoissonSamplingBox(tile, 0.5, 10000);

    // (2) Generate fracture distribution
    auto fractures = GenerateFractures(type, tile, 3);

    // (3) Clustering
    auto clusters = ComputeBlockClusters(samples, fractures);

    // (4) Implicit primitive extraction
    SDFNode* sdf = ComputeBlockSDF(clusters);
    if (!sdf) return 0;

    // (5) Mesh extraction
    MC::mcMesh mesh = PolygonizeSDF(sdf->box, sdf, resolution);

    // Copy to global buffers
    g_vertices.clear();
    g_normals.clear();
    g_indices.clear();

    // Scale and center mesh
    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        g_vertices.push_back((float)mesh.vertices.at(i).x);
        g_vertices.push_back((float)mesh.vertices.at(i).y);
        g_vertices.push_back((float)mesh.vertices.at(i).z);
        g_normals.push_back((float)mesh.normals.at(i).x);
        g_normals.push_back((float)mesh.normals.at(i).y);
        g_normals.push_back((float)mesh.normals.at(i).z);
    }
    for (size_t i = 0; i < mesh.indices.size(); i++)
    {
        g_indices.push_back(mesh.indices.at(i));
    }

    return (int)mesh.indices.size() / 3;
}

// Get mesh data pointers
float* getVertices() { return g_vertices.data(); }
float* getNormals() { return g_normals.data(); }
unsigned int* getIndices() { return g_indices.data(); }
int getVertexCount() { return (int)g_vertices.size() / 3; }
int getTriangleCount() { return (int)g_indices.size() / 3; }
int getVertexDataSize() { return (int)g_vertices.size() * sizeof(float); }
int getIndexDataSize() { return (int)g_indices.size() * sizeof(unsigned int); }

} // extern "C"
