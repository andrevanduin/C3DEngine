
#pragma once
#include "core/defines.h"
#include "math/math_types.h"

namespace C3D
{
	class C3D_API Transform
	{
	public:
		Transform();
		explicit Transform(const vec3& position);
		explicit Transform(const quat& rotation);
		Transform(const vec3& position, const quat& rotation);
		Transform(const vec3& position, const quat& rotation, const vec3& scale);

		[[nodiscard]] Transform* GetParent() const;
		void SetParent(Transform* parent);

		[[nodiscard]] vec3 GetPosition() const;
		void SetPosition(const vec3& position);
		void Translate(const vec3& translation);

		[[nodiscard]] quat GetRotation() const;
		void SetRotation(const quat& rotation);
		void Rotate(const quat& rotation);

		[[nodiscard]] vec3 GetScale() const;
		void SetScale(const vec3& scale);
		void Scale(const vec3& scale);

		void SetPositionRotation(const vec3& position, const quat& rotation);
		void SetPositionRotationScale(const vec3& position, const quat& rotation, const vec3& scale);

		void TranslateRotate(const vec3& translation, const quat& rotation);

		mat4 GetLocal();
		mat4 GetWorld();

	private:
		Transform* m_parent{};

		vec3 m_position{};
		vec3 m_scale{};
		quat m_rotation{};
		mat4 m_local{};

		bool m_needsUpdate{};
	};
}