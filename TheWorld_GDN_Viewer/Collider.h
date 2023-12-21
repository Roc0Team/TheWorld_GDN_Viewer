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
		void setData(void);
		void updateTransform();
		void onGlobalTransformChanged(void);
		void onCameraTransformChanged(void);
		void setTransform(Transform3D t);
		Transform3D getTransform(void);
		bool hasData(void)
		{
			if (m_initialized && m_hasData)
				return true;
			else
				return false;
		}

	private:
		inline float getColliderAltitude(void);

	private:
		bool m_initialized;
		bool m_enteredWorld = false;
		bool m_hasData = false;
		QuadTree* m_quadTree;
		RID m_shapeRID;
		RID m_bodyRID;
		Transform3D m_colliderTransform;
		Transform3D m_cameraGlobalTranform;
	};
}
