#pragma once

#pragma warning (push, 0)
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/basis.hpp>
#pragma warning (pop)

namespace godot
{
	class QuadTree;

	class Collider
	{
	public:
		Collider(QuadTree* quadTree);
		~Collider(void);

		void init(Node* attachedNode, uint32_t initialLayer, uint32_t initialMask);
		void deinit(void);

		void enterWorld(void);
		void exitWorld(void);
		bool setData(bool hiResData);
		void updateTransform(float gridStepInWU);
		void onGlobalTransformChanged(void);
		void onCameraTransformChanged(void);
		void setTransform(Transform3D t) const;
		Transform3D getTransform(void) const;
		bool hasData(void) const
		{
			if (m_initialized && m_hasData)
				return true;
			else
				return false;
		}
		bool hasHiResData(void) const
		{
			if (m_initialized && m_hasData && m_hasHiResData)
				return true;
			else
				return false;
		}
		bool initialized(void) const
		{
			return m_initialized;
		}
		bool worldEntered(void) const
		{
			return m_worldEntered;
		}

	private:
		inline float getColliderAltitude(void) const;

	private:
		bool m_initialized;
		bool m_worldEntered = false;
		bool m_hasData = false;
		bool m_hasHiResData = false;
		QuadTree* m_quadTree;
		RID m_shapeRID;
		RID m_bodyRID;
		Transform3D m_colliderTransform;
		Transform3D m_cameraGlobalTranform;
		float m_lastGridStepInWUUsed = 0.0f;
	};
}
