// Reconstructed from Deponia_Linux.asm, TVector3D and kexVec3 (asm 766335-767040, 1374068), TMatrix3 and idMat3 (asm
// 767045-767742), TMatrix4 (asm 767828-769325): the small 3D maths of the engine, used by the particle emitters, the
// camera of the 3D models and the text matrix. The file names of the original are not known (no x_assert in them); the
// names here are invented.
//
// The conventions, from the code: the elements of a matrix are in row order, a vector is a row that is multiplied from
// the left (`v * M`; the translation of a TMatrix4 is in its last row, elements 12 to 14), and the angles are in
// radians. The operations are done in the order of the asm, in single precision.
#pragma once

struct idVec3 {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

class TMatrix3;

class kexVec3 : public idVec3 {
public:
	kexVec3 operator+(const kexVec3 &other) const;
	kexVec3 &operator+=(const kexVec3 &other);
	kexVec3 operator-(const kexVec3 &other) const;
	kexVec3 &operator-=(const kexVec3 &other);
	float Dot(const kexVec3 &other) const;
	kexVec3 operator*(float scale) const;
	kexVec3 &operator*=(float scale);
	kexVec3 operator/(float divisor) const;
};

class TVector3D : public kexVec3 {
public:
	TVector3D() = default;
	TVector3D(float x_, float y_, float z_) {
		x = x_;
		y = y_;
		z = z_;
	}
	TVector3D(const kexVec3 &other) : kexVec3(other) {
	}

	float Length() const;
	/** Makes the vector one long (no check for a vector of no length). */
	void Unit();
	/** The part of `other` that is along this vector: (this . other) / |this|^2 times this. */
	TVector3D Project(const TVector3D &other) const;
	/** The vector becomes the matrix times it (the matrix on the left of the column). */
	void Rotate(const TMatrix3 &matrix);
	/** Turns the vector round an axis by an angle (the other two coordinates turn). */
	void RotateX(float angle);
	void RotateY(float angle);
	void RotateZ(float angle);
	bool operator==(const TVector3D &other) const;
	/** The vector (as a row) times the matrix. */
	TVector3D operator*(const TMatrix3 &matrix) const;
	TVector3D &operator/=(float divisor);
	/** The cross product. */
	TVector3D operator%(const TVector3D &other) const;
};

/** Four floats (what a TMatrix4 multiplies). */
struct TVector4 {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float w = 0.0f;
};

class idMat3 {
public:
	bool Compare(const idMat3 &other) const;
	/** The row `vector` times the matrix. */
	idVec3 operator*(const idVec3 &vector) const;

	float _m[9];  // the nine elements, in row order
};

class TMatrix3 : public idMat3 {
public:
	TMatrix3 operator+(const TMatrix3 &other) const;
	TMatrix3 &operator+=(const TMatrix3 &other);
	TMatrix3 operator-(const TMatrix3 &other) const;
	TMatrix3 &operator-=(const TMatrix3 &other);
	TMatrix3 operator*(const TMatrix3 &other) const;
	TMatrix3 &operator*=(const TMatrix3 &other);
	TMatrix3 operator*(float scale) const;
	TMatrix3 &operator*=(float scale);

	void Identity();
	void Transpose();
	void RotationX(float angle);
	void RotationY(float angle);
	void RotationZ(float angle);
};

TMatrix3 operator*(float scale, const TMatrix3 &matrix);

class TMatrix4 {
public:
	/** A camera: the matrix turns the way that `up` is made right-angled to `direction` (the position is not used). The columns are
	 *  the cross product of up and direction, up, and direction; the translation row is 0. */
	void SetCamera(const TVector3D &position, const TVector3D &direction, const TVector3D &up);
	/** The same with the direction from two angles: (cos a sin b, sin a sin b, cos b). */
	void SetCamera(float angleA, float angleB, const TVector3D &up);
	/** A perspective projection: m00 = m11 = nearPlane / halfSize, m22 = far / (far - near), m23 = 1, m32 = -near * far / (far - near). */
	void SetProjection(float nearPlane, float farPlane, float halfSize);
	/** The same for a frustum of a width and a height at the near plane. */
	void SetProjection(float nearPlane, float farPlane, float width, float height);
	void Identity();
	void Transpose();
	void RotationX(float angle);
	void RotationY(float angle);
	void RotationZ(float angle);
	void Translate(float x, float y, float z);
	void Scale(float x, float y, float z);

	bool operator==(const TMatrix4 &other) const;
	TMatrix4 operator+(const TMatrix4 &other) const;
	TMatrix4 &operator+=(const TMatrix4 &other);
	TMatrix4 operator-(const TMatrix4 &other) const;
	TMatrix4 &operator-=(const TMatrix4 &other);
	TMatrix4 operator*(const TMatrix4 &other) const;
	TMatrix4 &operator*=(const TMatrix4 &other);
	/** The row `vector` times the matrix. */
	TVector4 operator*(const TVector4 &vector) const;
	TMatrix4 operator*(float scale) const;
	TMatrix4 &operator*=(float scale);

	float _m[16];  // the sixteen elements, in row order
};

TMatrix4 operator*(float scale, const TMatrix4 &matrix);
