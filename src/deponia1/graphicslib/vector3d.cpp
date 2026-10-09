#include "graphicslib/vector3d.h"

#include <initializer_list>
#include <math.h>

// Confirmed (asm lines 766711-766960)
kexVec3 kexVec3::operator+(const kexVec3 &other) const {
	kexVec3 result;

	result.x = x + other.x;
	result.y = y + other.y;
	result.z = z + other.z;
	return result;
}

kexVec3 &kexVec3::operator+=(const kexVec3 &other) {
	x += other.x;
	y += other.y;
	z += other.z;
	return *this;
}

kexVec3 kexVec3::operator-(const kexVec3 &other) const {
	kexVec3 result;

	result.x = x - other.x;
	result.y = y - other.y;
	result.z = z - other.z;
	return result;
}

kexVec3 &kexVec3::operator-=(const kexVec3 &other) {
	x -= other.x;
	y -= other.y;
	z -= other.z;
	return *this;
}

float kexVec3::Dot(const kexVec3 &other) const {
	return x * other.x + y * other.y + z * other.z;
}

kexVec3 kexVec3::operator*(float scale) const {
	kexVec3 result;

	result.x = x * scale;
	result.y = y * scale;
	result.z = z * scale;
	return result;
}

kexVec3 &kexVec3::operator*=(float scale) {
	x *= scale;
	y *= scale;
	z *= scale;
	return *this;
}

kexVec3 kexVec3::operator/(float divisor) const {
	kexVec3 result;

	result.x = x / divisor;
	result.y = y / divisor;
	result.z = z / divisor;
	return result;
}

// Confirmed (asm lines 766335-766366)
float TVector3D::Length() const {
	return sqrtf(x * x + y * y + z * z);
}

// Confirmed (asm lines 766368-766413)
void TVector3D::Unit() {
	float length = sqrtf(x * x + y * y + z * z);

	x /= length;
	y /= length;
	z /= length;
}

// Confirmed (asm lines 766415-766504)
TVector3D TVector3D::Project(const TVector3D &other) const {
	float dot = other.x * x + other.y * y + other.z * z;
	float length = sqrtf(x * x + y * y + z * z);
	float lengthSquared = length * sqrtf(x * x + y * y + z * z);
	float factor = dot / lengthSquared;

	return TVector3D(x * factor, y * factor, z * factor);
}

// Confirmed (asm lines 766506-766546)
void TVector3D::Rotate(const TMatrix3 &matrix) {
	float ox = x;
	float oy = y;
	float oz = z;

	x = matrix._m[0] * ox + matrix._m[1] * oy + matrix._m[2] * oz;
	y = matrix._m[3] * ox + matrix._m[4] * oy + matrix._m[5] * oz;
	z = ox * matrix._m[6] + oy * matrix._m[7] + oz * matrix._m[8];
}

// Confirmed (asm lines 766548-766670)
void TVector3D::RotateX(float angle) {
	float s = sinf(angle);
	float c = cosf(angle);
	float oy = y;
	float oz = z;

	y = oy * c + oz * s;
	z = c * oz - s * oy;
}

void TVector3D::RotateY(float angle) {
	float s = sinf(angle);
	float c = cosf(angle);
	float ox = x;
	float oz = z;

	x = ox * c - oz * s;
	z = s * ox + c * oz;
}

void TVector3D::RotateZ(float angle) {
	float s = sinf(angle);
	float c = cosf(angle);
	float ox = x;
	float oy = y;

	x = ox * c - oy * s;
	y = s * ox + c * oy;
}

// Confirmed (asm lines 766672-766709)
bool TVector3D::operator==(const TVector3D &other) const {
	return x == other.x && y == other.y && z == other.z;
}

// Confirmed (asm lines 766863-766906)
TVector3D TVector3D::operator*(const TMatrix3 &matrix) const {
	TVector3D result;
	const float *m = matrix._m;

	result.x = x * m[0] + y * m[3] + z * m[6];
	result.y = x * m[1] + y * m[4] + z * m[7];
	result.z = x * m[2] + y * m[5] + z * m[8];
	return result;
}

// Confirmed (asm lines 766984-767006)
TVector3D &TVector3D::operator/=(float divisor) {
	x /= divisor;
	y /= divisor;
	z /= divisor;
	return *this;
}

// Confirmed (asm lines 767008-767043)
TVector3D TVector3D::operator%(const TVector3D &other) const {
	TVector3D result;

	result.x = y * other.z - z * other.y;
	result.y = z * other.x - x * other.z;
	result.z = x * other.y - y * other.x;
	return result;
}

// Confirmed (asm lines 767217-767268)
bool idMat3::Compare(const idMat3 &other) const {
	for (int i = 0; i < 9; i++) {
		if (_m[i] != other._m[i])
			return false;
	}

	return true;
}

// Confirmed (asm lines 767742-767790)
idVec3 idMat3::operator*(const idVec3 &vector) const {
	idVec3 result;

	result.x = vector.x * _m[0] + vector.y * _m[3] + vector.z * _m[6];
	result.y = vector.x * _m[1] + vector.y * _m[4] + vector.z * _m[7];
	result.z = vector.x * _m[2] + vector.y * _m[5] + vector.z * _m[8];
	return result;
}

// Confirmed (asm lines 767269-767810, the element by element ones)
TMatrix3 TMatrix3::operator+(const TMatrix3 &other) const {
	TMatrix3 result;

	for (int i = 0; i < 9; i++)
		result._m[i] = _m[i] + other._m[i];

	return result;
}

TMatrix3 &TMatrix3::operator+=(const TMatrix3 &other) {
	for (int i = 0; i < 9; i++)
		_m[i] += other._m[i];

	return *this;
}

TMatrix3 TMatrix3::operator-(const TMatrix3 &other) const {
	TMatrix3 result;

	for (int i = 0; i < 9; i++)
		result._m[i] = _m[i] - other._m[i];

	return result;
}

TMatrix3 &TMatrix3::operator-=(const TMatrix3 &other) {
	for (int i = 0; i < 9; i++)
		_m[i] -= other._m[i];

	return *this;
}

// Confirmed (asm lines 767301-767420, 767423-767520): the product of this matrix and `other` (this on the left).
TMatrix3 TMatrix3::operator*(const TMatrix3 &other) const {
	TMatrix3 result;

	for (int row = 0; row < 3; row++) {
		for (int column = 0; column < 3; column++) {
			result._m[row * 3 + column] = other._m[column] * _m[row * 3] + other._m[3 + column] * _m[row * 3 + 1] +
			                              other._m[6 + column] * _m[row * 3 + 2];
		}
	}

	return result;
}

TMatrix3 &TMatrix3::operator*=(const TMatrix3 &other) {
	*this = *this * other;
	return *this;
}

TMatrix3 TMatrix3::operator*(float scale) const {
	TMatrix3 result;

	for (int i = 0; i < 9; i++)
		result._m[i] = _m[i] * scale;

	return result;
}

TMatrix3 &TMatrix3::operator*=(float scale) {
	for (int i = 0; i < 9; i++)
		_m[i] *= scale;

	return *this;
}

TMatrix3 operator*(float scale, const TMatrix3 &matrix) {
	return matrix * scale;
}

// Confirmed (asm lines 767045-767105)
void TMatrix3::Identity() {
	for (int i = 0; i < 9; i++)
		_m[i] = 0.0f;

	_m[0] = 1.0f;
	_m[4] = 1.0f;
	_m[8] = 1.0f;
}

void TMatrix3::Transpose() {
	float t;

	t = _m[1];
	_m[1] = _m[3];
	_m[3] = t;
	t = _m[6];
	_m[6] = _m[2];
	_m[2] = t;
	t = _m[5];
	_m[5] = _m[7];
	_m[7] = t;
}

// Confirmed (asm lines 767107-767215): a turn of the row vectors, the sine below the diagonal in the first.
void TMatrix3::RotationX(float angle) {
	float s = sinf(angle);
	float c = cosf(angle);

	_m[0] = 1.0f;
	_m[1] = 0.0f;
	_m[2] = 0.0f;
	_m[3] = 0.0f;
	_m[4] = c;
	_m[5] = s;
	_m[6] = 0.0f;
	_m[7] = -s;
	_m[8] = c;
}

void TMatrix3::RotationY(float angle) {
	float s = sinf(angle);
	float c = cosf(angle);

	_m[0] = c;
	_m[1] = 0.0f;
	_m[2] = -s;
	_m[3] = 0.0f;
	_m[4] = 1.0f;
	_m[5] = 0.0f;
	_m[6] = s;
	_m[7] = 0.0f;
	_m[8] = c;
}

void TMatrix3::RotationZ(float angle) {
	float s = sinf(angle);
	float c = cosf(angle);

	_m[0] = c;
	_m[1] = s;
	_m[2] = 0.0f;
	_m[3] = -s;
	_m[4] = c;
	_m[5] = 0.0f;
	_m[6] = 0.0f;
	_m[7] = 0.0f;
	_m[8] = 1.0f;
}

// Confirmed (asm lines 767828-767926)
void TMatrix4::SetCamera(const TVector3D &/*position*/, const TVector3D &direction, const TVector3D &up) {
	float d = up.x * direction.x + up.y * direction.y + up.z * direction.z;
	float cz = up.z - direction.z * d;
	float cy = up.y - direction.y * d;
	float cx = up.x - direction.x * d;
	float length = sqrtf(cx * cx + cy * cy + cz * cz);
	float ux = cx / length;
	float uy = cy / length;
	float uz = cz / length;

	_m[6] = direction.y;
	_m[2] = direction.x;
	_m[3] = 0.0f;
	_m[7] = 0.0f;
	_m[10] = direction.z;
	_m[11] = 0.0f;
	_m[12] = 0.0f;
	_m[13] = 0.0f;
	_m[14] = 0.0f;
	_m[15] = 1.0f;
	_m[5] = uy;
	_m[9] = uz;
	_m[1] = ux;
	_m[0] = direction.z * uy - direction.y * uz;
	_m[8] = direction.y * ux - uy * direction.x;
	_m[4] = direction.x * uz - ux * direction.z;
}

// Confirmed (asm lines 767927-768060)
void TMatrix4::SetCamera(float angleA, float angleB, const TVector3D &up) {
	float sinA = sinf(angleA);
	float cosA = cosf(angleA);
	float sinB = sinf(angleB);
	float cosB = cosf(angleB);
	TVector3D direction(cosA * sinB, sinA * sinB, cosB);

	SetCamera(TVector3D(), direction, up);
}

// Confirmed (asm lines 768062-768110)
void TMatrix4::SetProjection(float nearPlane, float farPlane, float halfSize) {
	float span = farPlane - nearPlane;
	float k = nearPlane / halfSize;
	float zero = 0.0f * k;

	_m[0] = k;
	_m[5] = k;

	for (int i : {1, 2, 3, 4, 6, 7, 8, 9, 12, 13, 15})
		_m[i] = zero;

	_m[10] = (halfSize * farPlane) / (nearPlane * span) * k;
	_m[11] = (halfSize / nearPlane) * k;
	_m[14] = (-halfSize * farPlane) / span * k;
}

// Confirmed (asm lines 768112-768150)
void TMatrix4::SetProjection(float nearPlane, float farPlane, float width, float height) {
	float span = farPlane - nearPlane;
	float twiceNear = nearPlane + nearPlane;

	_m[11] = 1.0f;
	_m[5] = twiceNear / height;
	_m[10] = farPlane / span;
	_m[0] = twiceNear / width;

	for (int i : {1, 2, 3, 4, 6, 7, 8, 9, 12, 13, 15})
		_m[i] = 0.0f;

	_m[14] = -farPlane / span * nearPlane;
}

void TMatrix4::Identity() {
	for (int i = 0; i < 16; i++)
		_m[i] = 0.0f;

	_m[0] = 1.0f;
	_m[5] = 1.0f;
	_m[10] = 1.0f;
	_m[15] = 1.0f;
}

void TMatrix4::Transpose() {
	for (int row = 0; row < 4; row++) {
		for (int column = row + 1; column < 4; column++) {
			float t = _m[row * 4 + column];

			_m[row * 4 + column] = _m[column * 4 + row];
			_m[column * 4 + row] = t;
		}
	}
}

// Confirmed (asm lines 768235-768400)
void TMatrix4::RotationX(float angle) {
	float s = sinf(angle);
	float c = cosf(angle);

	Identity();
	_m[5] = c;
	_m[6] = s;
	_m[9] = -s;
	_m[10] = c;
}

void TMatrix4::RotationY(float angle) {
	float s = sinf(angle);
	float c = cosf(angle);

	Identity();
	_m[0] = c;
	_m[2] = -s;
	_m[8] = s;
	_m[10] = c;
}

void TMatrix4::RotationZ(float angle) {
	float s = sinf(angle);
	float c = cosf(angle);

	Identity();
	_m[0] = c;
	_m[1] = s;
	_m[4] = -s;
	_m[5] = c;
}

void TMatrix4::Translate(float x, float y, float z) {
	Identity();
	_m[12] = x;
	_m[13] = y;
	_m[14] = z;
}

void TMatrix4::Scale(float x, float y, float z) {
	Identity();
	_m[0] = x;
	_m[5] = y;
	_m[10] = z;
}

// Confirmed (asm lines 768440-768560, 768610-769250)
bool TMatrix4::operator==(const TMatrix4 &other) const {
	for (int i = 0; i < 16; i++) {
		if (_m[i] != other._m[i])
			return false;
	}

	return true;
}

TMatrix4 TMatrix4::operator+(const TMatrix4 &other) const {
	TMatrix4 result;

	for (int i = 0; i < 16; i++)
		result._m[i] = _m[i] + other._m[i];

	return result;
}

TMatrix4 &TMatrix4::operator+=(const TMatrix4 &other) {
	for (int i = 0; i < 16; i++)
		_m[i] += other._m[i];

	return *this;
}

TMatrix4 TMatrix4::operator-(const TMatrix4 &other) const {
	TMatrix4 result;

	for (int i = 0; i < 16; i++)
		result._m[i] = _m[i] - other._m[i];

	return result;
}

TMatrix4 &TMatrix4::operator-=(const TMatrix4 &other) {
	for (int i = 0; i < 16; i++)
		_m[i] -= other._m[i];

	return *this;
}

// Confirmed (asm lines 768800-769000, 769030-769200): the product of this matrix and `other` (this on the left).
TMatrix4 TMatrix4::operator*(const TMatrix4 &other) const {
	TMatrix4 result;

	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++) {
			result._m[row * 4 + column] = other._m[column] * _m[row * 4] + other._m[4 + column] * _m[row * 4 + 1] +
			                              other._m[8 + column] * _m[row * 4 + 2] +
			                              other._m[12 + column] * _m[row * 4 + 3];
		}
	}

	return result;
}

TMatrix4 &TMatrix4::operator*=(const TMatrix4 &other) {
	*this = *this * other;
	return *this;
}

// Confirmed (asm lines 769200-769260)
TVector4 TMatrix4::operator*(const TVector4 &vector) const {
	TVector4 result;

	result.x = vector.x * _m[0] + vector.y * _m[4] + vector.z * _m[8] + vector.w * _m[12];
	result.y = vector.x * _m[1] + vector.y * _m[5] + vector.z * _m[9] + vector.w * _m[13];
	result.z = vector.x * _m[2] + vector.y * _m[6] + vector.z * _m[10] + vector.w * _m[14];
	result.w = vector.x * _m[3] + vector.y * _m[7] + vector.z * _m[11] + vector.w * _m[15];
	return result;
}

TMatrix4 TMatrix4::operator*(float scale) const {
	TMatrix4 result;

	for (int i = 0; i < 16; i++)
		result._m[i] = _m[i] * scale;

	return result;
}

TMatrix4 &TMatrix4::operator*=(float scale) {
	for (int i = 0; i < 16; i++)
		_m[i] *= scale;

	return *this;
}

TMatrix4 operator*(float scale, const TMatrix4 &matrix) {
	return matrix * scale;
}
