
#pragma once
#include "core/defines.h"
#include "math/math_types.h"

namespace C3D
{
	class C3D_API Camera
	{
	public:
		Camera();
		void Reset();

		[[nodiscard]] vec3 GetPosition() const;
		void SetPosition(const vec3& position);

		[[nodiscard]] vec3 GetEulerRotation() const;
		void SetEulerRotation(const vec3& eulerRotation);

		mat4 GetViewMatrix();

		vec3 GetForward();
		vec3 GetBackward();

		vec3 GetLeft();
		vec3 GetRight();

		vec3 GetUp();
		vec3 GetDown();

		void MoveForward(f32 amount);
		void MoveForward(f64 amount);

		void MoveBackward(f32 amount);
		void MoveBackward(f64 amount);

		void MoveLeft(f32 amount);
		void MoveLeft(f64 amount);

		void MoveRight(f32 amount);
		void MoveRight(f64 amount);

		void MoveUp(f32 amount);
		void MoveUp(f64 amount);

		void MoveDown(f32 amount);
		void MoveDown(f64 amount);

		void AddYaw(f32 amount);
		void AddYaw(f64 amount);

		void AddPitch(f32 amount);
		void AddPitch(f64 amount);
	private:
		/** @brief flag that indicates if camera view needs an update */
		bool m_needsUpdate;

		vec3 m_position;
		vec3 m_eulerRotation;

		mat4 m_viewMatrix;
	};
}
