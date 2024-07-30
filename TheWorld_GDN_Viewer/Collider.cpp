//#include "pch.h"
#include "Collider.h"
#include "QuadTree.h"
#include "GDN_TheWorld_Viewer.h"
#include "half.h"

#pragma warning (push, 0)
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#pragma warning (pop)

#include <assert.h>


//#define MOCK_COLLIDER true
//#define MOCK_COLLIDER false

namespace godot
{
	Collider::Collider(QuadTree* quadTree)
	{
		m_initialized = false;
		m_quadTree = quadTree;
		m_shapeRID = RID();
		m_bodyRID = RID();
	}
	
	Collider::~Collider(void)
	{
		deinit();
	}

	void Collider::init(Node* attachedNode, uint32_t layer, uint32_t mask)
	{
		if (m_quadTree->Viewer()->Globals()->getParamDisableTerrainCollisions())
			return;

		assert(!m_initialized);
		assert(attachedNode != nullptr);

		godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
		m_shapeRID = ps->heightmap_shape_create();
		m_bodyRID = ps->body_create();
		ps->body_set_mode(m_bodyRID, godot::PhysicsServer3D::BODY_MODE_STATIC);
		ps->body_set_collision_layer(m_bodyRID, layer);
		ps->body_set_collision_mask(m_bodyRID, mask);

		//ps->body_set_ray_pickable(m_bodyRID, false);		// SUPERDEBUGRIC - This is an attempt to workaround https://github.com/godotengine/godot/issues/24390

		// Initial data - This is a workaround to https://github.com/godotengine/godot/issues/25304
#ifdef REAL_T_IS_DOUBLE
		PackedFloat64Array arr;
#else
		PackedFloat32Array arr;
#endif // REAL_T_IS_DOUBLE
		arr.resize(4);
		arr.set(0, 0);
		arr.set(1, 0);
		arr.set(2, 0);
		arr.set(3, 0);
		Dictionary data;
		data["width"] = 2;			// data["map_width"];	
		data["depth"] = 2;			// data["map_depth"];
		data["heights"] = arr;		// data["map_data"];
		data["min_height"] = -1;		// ???
		data["max_height"] = 1;		// ???
		ps->shape_set_data(m_shapeRID, data);

		m_shapeIdx = ps->body_get_shape_count(m_bodyRID);
		ps->body_add_shape(m_bodyRID, m_shapeRID);
		m_colliderEnabled = true;

		// This makes collision hits report the provided object as `collider`
		ps->body_attach_object_instance_id(m_bodyRID, attachedNode->get_instance_id());
			
		m_hasData = false;
		m_hasHiResData = false;
		m_initialized = true;
	}

	void Collider::deinit(void)
	{
		if (m_quadTree->Viewer()->Globals()->getParamDisableTerrainCollisions())
			return;

		exitWorld();
		
		if (m_initialized)
		{
			godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();

			if (m_bodyRID != RID())
				ps->free_rid(m_bodyRID);
			if (m_shapeRID != RID())
			{
				ps->free_rid(m_shapeRID);
				m_shapeIdx = -1;
			}

			m_hasData = false;
			m_hasHiResData = false;
			m_initialized = false;
		}
	}

	void Collider::enterWorld(void)
	{
		if (m_quadTree->Viewer()->Globals()->getParamDisableTerrainCollisions())
			return;

		assert(m_initialized);
		if (!m_initialized)
			return;

		Ref<godot::World3D> world = m_quadTree->Viewer()->get_world_3d();
		if (world != nullptr && world.is_valid())
		{
			godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
			ps->body_set_space(m_bodyRID, world->get_space());
		}

		m_worldEntered = true;
	}

	void Collider::exitWorld(void)
	{
		if (m_quadTree->Viewer()->Globals()->getParamDisableTerrainCollisions())
			return;

		//assert(m_initialized);
		if (!m_initialized)
			return;

		if (!m_worldEntered)
			return;

		godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
		ps->body_set_space(m_bodyRID, RID());

		m_worldEntered = false;
	}

	void Collider::enable(void)
	{
		if (m_initialized)
		{
			godot::PhysicsServer3D::get_singleton()->body_set_shape_disabled(m_bodyRID, m_shapeIdx, false);
			m_colliderEnabled = true;
		}
	}

	void Collider::disable(void)
	{
		if (m_initialized)
		{
			godot::PhysicsServer3D::get_singleton()->body_set_shape_disabled(m_bodyRID, m_shapeIdx, true);
			m_colliderEnabled = false;
		}
	}

	bool Collider::colliderEnabled(void) const
	{
		return m_colliderEnabled;
	}

	bool Collider::setData(bool hiResData)
	{
		bool updated = false;

		GDN_TheWorld_Globals* globals = m_quadTree->Viewer()->Globals();

		if (globals->getParamDisableTerrainCollisions())
			return updated;

		if (!m_initialized)
			return updated;

		TheWorld_Utils::GuardProfiler profiler(std::string("Collider setData 1 ") + __FUNCTION__, "ALL");

		Quadrant* quadrant = m_quadTree->getQuadrant();

		int numVerticesPerSize = 0;

#ifdef REAL_T_IS_DOUBLE
		PackedFloat64Array* heightsForCollider = nullptr;
		PackedFloat64Array resizedHeightsForCollider;
		PackedFloat64Array heightsZeroForCollider;
		size_t elementSize = sizeof(double);
		double zero;
#else
		PackedFloat32Array* heightsForCollider = nullptr;
		PackedFloat32Array resizedHeightsForCollider;
		PackedFloat32Array heightsZeroForCollider;
		size_t elementSize = sizeof(float);
		float zero = 0;
#endif // REAL_T_IS_DOUBLE
		
		float gridStepInWU = 0.0f;
		gridStepInWU = quadrant->getPos().getGridStepInWU();

		if (hiResData)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("Collider setData 1.1 ") + __FUNCTION__, "get heigths opt level = 0");
			numVerticesPerSize = quadrant->getPos().getNumVerticesPerSize();
#ifdef REAL_T_IS_DOUBLE
			PackedFloat64Array& _heightsForCollider = quadrant->getHeightsForCollider();
#else
			PackedFloat32Array& _heightsForCollider = quadrant->getHeightsForCollider();
#endif // REAL_T_IS_DOUBLE
			heightsForCollider = &_heightsForCollider;
			m_hasHiResData = true;
		}
		else
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("Collider setData 1.2 ") + __FUNCTION__, "get heigths opt level > 0");

			//size_t optimizationLevel = quadrant->optimizationLevel();
			size_t optimizationLevel = 4;

			size_t newResolution = globals->resolutionForOptimizationLevel(optimizationLevel);
			numVerticesPerSize = (int)newResolution;
			heightsForCollider = &resizedHeightsForCollider;
			gridStepInWU = gridStepInWU * (float)pow(2, optimizationLevel);

			godot::Ref<godot::Image> image;
			{
				std::string profileHeader("Collider setData 1.2.1 ");
				image = quadrant->getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::heightmap, optimizationLevel, newResolution);
			}

			{
				TheWorld_Utils::GuardProfiler profiler(std::string("Collider setData 1.2.2 ") + __FUNCTION__, "heigths 16 to 32");
				int size = int(numVerticesPerSize * numVerticesPerSize);
				resizedHeightsForCollider.resize(size);
				godot::PackedByteArray array = image->get_data();
				assert(array.size() == size * 2);
				uint16_t* p = (uint16_t*)array.ptr();
				for (int idx = 0; idx < size; idx++)
				{
					TheWorld_Utils::FLOAT_32 f(0);
					f.u32 = half_to_float(*p);
					resizedHeightsForCollider[idx] = (real_t)f.f32;
					p++;
				}
			}

			m_hasHiResData = false;
		}
		
		int areaSize = numVerticesPerSize * numVerticesPerSize;
		//{
		//	TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 5.1.1.1 ") + __FUNCTION__, "Collider::setData - gen empty heights");
		//	PoolRealArray& h = quadrant->getHeightsForCollider();
		//}
		my_assert(areaSize == heightsForCollider->size());
		if (areaSize != heightsForCollider->size())
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Size of Heigths inconsistent, areaSize=" + std::to_string(areaSize) + " Heigths size=" + std::to_string(heightsForCollider->size())).c_str()));

		//size_t size = heightsForCollider.size();	// DEBUG
		Dictionary data;
		data["width"] = numVerticesPerSize;			// data["map_width"];	
		data["depth"] = numVerticesPerSize;			// data["map_depth"];
		if (globals->getParamElevationZero())
		{
			static TheWorld_Utils::MemoryBuffer zeroHeigths;
			m_quadTree->Viewer()->getMainProcessingMutex().lock();
			if (zeroHeigths.size() == 0)
			{
				size_t maxNumVerticesPerSize = quadrant->getPos().getNumVerticesPerSize();
				size_t bufferNeeded = maxNumVerticesPerSize * maxNumVerticesPerSize * elementSize;
				zeroHeigths.reserve(bufferNeeded);
				for (int i = 0; i < maxNumVerticesPerSize * maxNumVerticesPerSize; i++)
					zeroHeigths.append((BYTE*)&zero, sizeof(zero));
				//zeroHeigths.adjustSize(bufferNeeded);
			}
			m_quadTree->Viewer()->getMainProcessingMutex().unlock();

			heightsZeroForCollider.resize(areaSize);
			memcpy((char*)heightsZeroForCollider.ptrw(), zeroHeigths.ptr(), areaSize * elementSize);
			data["heights"] = heightsZeroForCollider;		// data["map_data"];
		}
		else
			data["heights"] = *heightsForCollider;		// data["map_data"];
		Vector3 startPoint = quadrant->getGlobalCoordAABB().position;
		Vector3 endPoint = startPoint + quadrant->getGlobalCoordAABB().size;
		data["min_height"] = startPoint.y;			// ???
		//data["min_height"] = -endPoint.y;			// ???
		data["max_height"] = endPoint.y;			// ???

		godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
		{
			ps->shape_set_data(m_shapeRID, data);
			updated = true;
		}

		heightsForCollider->resize(0);
		heightsZeroForCollider.resize(0);

		{
			updateTransform(gridStepInWU);
		}

		m_hasData = true;

		if (m_quadTree->isVisible())
			enable();
		else
			disable();

		return updated;
	}
	
	void Collider::onGlobalTransformChanged(void)
	{
		if (m_quadTree->Viewer()->Globals()->getParamDisableTerrainCollisions())
			return;

		if (!m_initialized)
			return;

		updateTransform(0.0f);
	}

	void Collider::onCameraTransformChanged(void)
	{
		if (m_quadTree->Viewer()->Globals()->getParamDisableTerrainCollisions())
			return;

		if (!m_initialized)
			return;

		m_cameraGlobalTranform = m_quadTree->Viewer()->getWorldCamera()->get_global_transform();
		float colliderAltitude = getColliderAltitude();
		Transform3D t = getTransform();
		t.origin.y = colliderAltitude;
		setTransform(t);
	}
	
	float Collider::getColliderAltitude(void) const
	{
		//float colliderAltitude = float(-(m_cameraGlobalTranform.origin.y / (float)2.34));
		float colliderAltitude = 0;
		return colliderAltitude;
	}

	void Collider::updateTransform(float gridStepInWU)
	{
		if (m_quadTree->Viewer()->Globals()->getParamDisableTerrainCollisions())
			return;

		assert(m_initialized);
		if (!m_initialized)
			return;

		if (gridStepInWU == 0.0f)
			gridStepInWU = m_lastGridStepInWUUsed;
		else
			m_lastGridStepInWUUsed = gridStepInWU;
		
		//int numVerticesPerSize = m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize();
		float sizeInWU = m_quadTree->getQuadrant()->getPos().getSizeInWU();
		//float gridStepInWU = m_quadTree->getQuadrant()->getPos().getGridStepInWU();
		Transform3D igt = m_quadTree->getInternalGlobalTransform();
		//float axisScaleFactr = sqrtf(powf(gridStepInWU, (float)2.0) / (float)2.0);
		float axisScaleFactr = gridStepInWU;
		Camera3D* cam = m_quadTree->Viewer()->getWorldCamera();
		m_cameraGlobalTranform = cam->get_global_transform();
		float colliderAltitude = getColliderAltitude();
		//Vector3 centerOfQuadrant = igt.origin + 0.5 * Vector3(sizeInWU, 0, sizeInWU);
		//Vector3 centerOfQuadrantDistFromCamera = cam->get_global_transform().origin - centerOfQuadrant;

		// set body to the center of the quadrant
		Transform3D tTranslate(Basis(), Vector3((float)0.5 * sizeInWU, (float)colliderAltitude, (float)0.5 * sizeInWU));
		Transform3D tScale(Basis().scaled(Vector3(axisScaleFactr, (float)1.0, axisScaleFactr)), Vector3(0, 0, 0));
		Transform3D t = tTranslate * tScale;

		m_colliderTransform = igt * t;
		setTransform(m_colliderTransform);
	}

	void Collider::setTransform(Transform3D t) const
	{
		assert(m_initialized);
		if (!m_initialized)
			return;

		godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
		ps->body_set_state(m_bodyRID, godot::PhysicsServer3D::BodyState::BODY_STATE_TRANSFORM, t);
		//ps->body_set_shape_transform(m_bodyRID, 0, t);
	}

	Transform3D Collider::getTransform(void) const
	{
		assert(m_initialized);
		if (!m_initialized)
			return Transform3D();

		godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
		Transform3D t = ps->body_get_state(m_bodyRID, PhysicsServer3D::BodyState::BODY_STATE_TRANSFORM);
		return t;
	}
}

