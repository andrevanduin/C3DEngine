
#include "transform.h"

namespace C3D
{
	Transform::Transform()
		:  m_position(0), m_scale(1), m_rotation(1.0f, 0, 0, 0), m_local(1.0f)
	{}

	Transform::Transform(const vec3& position)
		: m_position(position), m_scale(1), m_rotation(1.0f, 0, 0, 0), m_local(1.0f), m_needsUpdate(true)
	{}

	Transform::Transform(const quat& rotation)
		: m_position(0), m_scale(1), m_rotation(rotation), m_local(1.0f), m_needsUpdate(true)
	{
	}

	Transform::Transform(const vec3& position, const quat& rotation)
		: m_position(position), m_scale(1), m_rotation(rotation), m_local(1.0f), m_needsUpdate(true)
	{

	}

	Transform::Transform(const vec3& position, const quat& rotation, const vec3& scale)
		: m_position(position), m_scale(scale), m_rotation(rotation), m_local(1.0f), m_needsUpdate(true)
	{
	}

	Transform* Transform::GetParent() const
	{
		return m_parent;
	}

	void Transform::SetParent(Transform* parent)
	{
		m_parent = parent;
		m_needsUpdate = true;
	}

	vec3 Transform::GetPosition() const
	{
		return m_position;
	}

	void Transform::SetPosition(const vec3& position)
	{
		m_position = position;
		m_needsUpdate = true;
	}

	void Transform::Translate(const vec3& translation)
	{
		m_position += translation;
		m_needsUpdate = true;
	}

	quat Transform::GetRotation() const
	{
		return m_rotation;
	}

	void Transform::SetRotation(const quat& rotation)
	{
		m_rotation = rotation;
		m_needsUpdate = true;
	}

	void Transform::Rotate(const quat& rotation)
	{
		m_rotation *= rotation;
		m_needsUpdate = true;
	}

	vec3 Transform::GetScale() const
	{
		return m_scale;
	}

	void Transform::SetScale(const vec3& scale)
	{
		m_scale = scale;
		m_needsUpdate = true;
	}

	void Transform::Scale(const vec3& scale)
	{
		m_scale *= scale;
		m_needsUpdate = true;
	}

	void Transform::SetPositionRotation(const vec3& position, const quat& rotation)
	{
		m_position = position;
		m_rotation = rotation;
		m_needsUpdate = true;
	}

	void Transform::SetPositionRotationScale(const vec3& position, const quat& rotation, const vec3& scale)
	{
		m_position = position;
		m_rotation = rotation;
		m_scale = scale;
		m_needsUpdate = true;
	}

	void Transform::TranslateRotate(const vec3& translation, const quat& rotation)
	{
		m_position += translation;
		m_rotation *= rotation;
		m_needsUpdate = true;
	}

	mat4 Transform::GetLocal()
	{
		if (m_needsUpdate)
		{
			mat4 tr = translate(m_position) * mat4(m_rotation);
			tr *= scale(m_scale);
			m_local = tr;
			m_needsUpdate = false;
		}

		return m_local;
	}

	mat4 Transform::GetWorld()
	{
		const mat4 local = GetLocal();
		if (m_parent)
		{
			const mat4 parent = m_parent->GetWorld();
			return parent * local;
		}
		return local;
	}
}
