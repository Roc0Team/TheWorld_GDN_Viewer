#pragma once

#include <map>
#include <vector>
#include <array>
#include <memory>
#include <ctime>

#include "Chunk.h"
#include "Collider.h"
//#include <Utils.h>
#include "Tools.h"

#pragma warning(push, 0)
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#pragma warning(pop)

#define _DEBUG_AAB	1

namespace godot
{
	class GDN_TheWorld_Viewer;
	class GDN_TheWorld_Quadrant;
	class GDN_TheWorld_MapModder;

	class MapQuadrantImage
	{
	public:
		MapQuadrantImage(TheWorld_Utils::QuadrantPos& quadrantPos, GDN_TheWorld_Viewer* viewer, size_t optimizationLevel);
		~MapQuadrantImage();

		godot::Ref<godot::Image> getMapImage(void);
		
		void setMapData(godot::Ref<godot::Image> mapImage);
		
		void refreshDataFromServer(std::string& bufferFromServer, std::string& meshIdFromServer, std::string outMeshIdFromBuffer, bool updateCache);
		
		bool updated(void) const
		{
			return m_updated;
		}
		void updated(bool b)
		{
			m_updated = b;
		}
		
		size_t getOptimizationLevel(void) const
		{
			return m_optimizationLevel;
		}
		int32_t getImageResolution(void) const
		{
			return m_imageResolution;
		}

		TheWorld_Utils::MeshCacheBuffer& getMeshCacheBuffer(void);

		void setMeshId(std::string& meshId)
		{
			m_meshId = meshId;
		}
		std::string getMeshId(void) const
		{
			return m_meshId;
		}

		TheWorld_Utils::QuadrantPos getPos(void)
		{
			return m_quadrantPos;
		}

		std::time_t getRefreshTime(void);

	private:
		GDN_TheWorld_Viewer* m_viewer = nullptr;
		TheWorld_Utils::QuadrantPos m_quadrantPos;
		TheWorld_Utils::MeshCacheBuffer m_cache;
		std::string m_meshId;
		bool m_updated = false;
		int64_t m_lastUpdateTime = 0;
		size_t m_optimizationLevel = 0;		// 0 = 2049, 2 = 513, 4 = 129, 6 = 33
		int32_t m_imageResolution = 0;

		// Datas
		godot::Ref<godot::Image> m_mapImage;
		// Datas
	};
	
	enum class QuadType
	{
		NotSet = -1
		, Terrain = 0
		, Sea = 1
		, NotUsed = 2
	};

	
	class Quadrant
	{
		friend class ShaderTerrainData;
		friend class GDN_TheWorld_Viewer;
		friend class GDN_TheWorld_Edit;
		friend class QuadTree;

		//enum class MeshResolutionLevel
		//{
		//	not_set = -1
		//	, low = 0
		//	, high = 1
		//};

		//enum class TexResolutionLevel
		//{
		//	not_set = -1
		//	, low = 0
		//	, med = 1
		//	, high = 2
		//};

		class InternalQuadrantDataLockHolder
		{
		public:
			InternalQuadrantDataLockHolder(TheWorld_Utils::QuadrantPos pos, GDN_TheWorld_Viewer* viewer, bool lock = true, bool lockOnlyInitializedQuadrants = true);
			InternalQuadrantDataLockHolder(QuadTree* quadTree, bool lock = true, bool lockOnlyInitializedQuadrants = true);
			InternalQuadrantDataLockHolder(Quadrant* quadrant, bool lock = true, bool lockOnlyInitializedQuadrants = true);
			~InternalQuadrantDataLockHolder();
			bool lock(void);
			void unlock(void);
			bool lockAcquired(void) const
			{
				return m_lockAcquired;
			}
			Quadrant* quadrant(void) const
			{
				return m_quadrant;
			}

		private:
			void init(Quadrant* quadrant, bool lock, bool lockOnlyInitializedQuadrants);

		private:
			Quadrant* m_quadrant = nullptr;
			bool m_lockAcquired = false;
			int32_t m_numLocks = 0;
		};

		class NeighbourQuadrantData
		{
		public:
			bool initialized(void) const
			{
				return m_initialized;
			}
			void initialized(bool b)
			{
				m_initialized = b;
			}
			bool shaderUpdated(void) const
			{
				return m_shaderUpdated;
			}
			void shaderUpdated(bool b)
			{
				m_shaderUpdated = b;
			}
			std::string textureLayout(void) const
			{
				return m_textureLayout;
			}
			godot::Vector4 groundTextureLayers(void) const
			{
				return m_groundTextureLayers;
			}
			godot::Color getUpperLeftCornerSplatmapTexel(void)
			{
				return m_upperLeftCornerSplatmapTexel;
			}
			void init(Quadrant* owningQuadrant, Quadrant* neighbourQuadrant, TheWorld_Utils::QuadrantPos::DirectionSlot neighbourQuadrantDir, TheWorld_Utils::QuadrantPos& owningQuadrantPos);

			godot::Ref<godot::Texture2D> getSplatmapBorderRowTexture(void);

		private:
			Quadrant* m_owningQuadrant = nullptr;
			bool m_initialized = false;
			std::string m_textureLayout;
			godot::Vector4 m_groundTextureLayers;
			godot::Color m_upperLeftCornerSplatmapTexel;
			bool m_shaderUpdated = false;
			TheWorld_Utils::MemoryBuffer m_splatmapBorderRow;
			std::chrono::system_clock::time_point m_lastBorderRefreshTs;
			//// #####
			//TheWorld_Utils::QuadrantPos m_owningQuadrantPos;
			//TheWorld_Utils::QuadrantPos m_neighbourQuadrantPos;
			//// #####
		};

	public:
		Quadrant(TheWorld_Utils::QuadrantPos& quadrantPos, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree);

		~Quadrant();

		_declspec(dllexport) void populateGridVertices(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrainForced);

#ifdef REAL_T_IS_DOUBLE
		PackedFloat64Array& getHeightsForCollider(bool reload = true)
#else
		PackedFloat32Array& getHeightsForCollider(bool reload = true)
#endif // REAL_T_IS_DOUBLE
		{
			if (m_heightsForCollider.size() == 0 && reload)
			{
				int numVertices = getPos().getNumVerticesPerSize() * getPos().getNumVerticesPerSize();
				m_heightsForCollider.resize((int)numVertices);
				my_assert(getFloat32HeightsBuffer().size() == numVertices * sizeof(float));
#ifdef REAL_T_IS_DOUBLE
				float* f = (float*)getFloat32HeightsBuffer().ptr();
				for (int idx = 0; idx < numVertices; idx++)
				{
					m_heightsForCollider.set(idx, (real_t)*f);
					f++;
				}
#else
				memcpy((char*)m_heightsForCollider.ptrw(), getFloat32HeightsBuffer().ptr(), getFloat32HeightsBuffer().size());
#endif // REAL_T_IS_DOUBLE
			}
			return m_heightsForCollider;
		}

		TheWorld_Utils::QuadrantPos getPos(void) const
		{
			return m_quadrantPos;
		}
		TheWorld_Utils::QuadrantPos& getInternalPos(void)
		{
			return m_quadrantPos;
		}

		bool quadrantAtMinLod(enum class QuadType type);

		//enum class MeshResolutionLevel getMeshResolutionLevel(void);
		//enum class TexResolutionLevel getTexResolutionLevel(void);

		//void clearCachedTexture(void);

		bool isFlushed(void);

		bool hasHeigthsForChunkSplitsAndShaderRendering(void);
		bool needReloadTerrainDataFromCache(void);

		void lockInternalDataUntilCompleteProcessExecution(bool b = true);
		bool internalDataLockedUntilCompleteProcessExecution(void) const;

		void setMeshId(std::string meshId);

		void refreshGridVerticesFromServer(std::string& bufferFromServer, std::string meshIdFromServer, std::string& outMeshIdFromBuffer, bool updateCache);

		void refreshSplatmapBorderRows(void);

		AABB& getGlobalCoordAABB(void)
		{
			return m_globalCoordAABB;
		}

		ShaderTerrainData* getShaderTerrainData(void)
		{
			return m_shaderTerrainData.get();
		}

		Collider* getCollider(void)
		{
			return m_collider.get();
		}

		std::string getMeshId() const
		{
			return m_meshId;
		}

		size_t getIndexFromMap(float posX, float posZ);
		float getAltitudeFromHeigthmap(size_t index);
		float getPosXFromHeigthmap(size_t index);
		float getPosZFromHeigthmap(size_t index);
		godot::Vector3 getNormalFromNormalmap(size_t index, bool& ok);

		TheWorld_Utils::MemoryBuffer& getFloat16HeightsBuffer(bool reloadFromCache = true)
		{
			if (m_float16HeigthsBuffer.empty() && reloadFromCache)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("Quadrant getFloat16HeightsBuffer 1 ") + __FUNCTION__, "Heightmap16 reload from disk");
				TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
				float minAltitude, maxAltitude;
				bool ok = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
			}
			return m_float16HeigthsBuffer;
		}

		TheWorld_Utils::MemoryBuffer& getFloat32HeightsBuffer(bool reloadFromCache = true)
		{
			if (m_float32HeigthsBuffer.empty() && reloadFromCache)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("Quadrant getFloat32HeightsBuffer 1 ") + __FUNCTION__, "Heightmap32 reload from disk");
				TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
				float minAltitude, maxAltitude;
				bool ok = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
			}
			return m_float32HeigthsBuffer;
		}

		TheWorld_Utils::MemoryBuffer& getNormalsBuffer(bool reloadFromCache = true, bool force = false)
		{
			if (m_normalsBuffer.empty() && reloadFromCache)
			{
				if (!getTerrainEdit()->normalsNeedRegen || force)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("Quadrant getNormalsBuffer 1 ") + __FUNCTION__, "Normalmap reload from disk");
					TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
					float minAltitude, maxAltitude;
					bool ok = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
				}
			}
			return m_normalsBuffer;
		}

		TheWorld_Utils::MemoryBuffer& getSplatmapBuffer(bool reloadFromCache = true)
		{
			if (m_splatmapBuffer.empty() && reloadFromCache)
			{
				if (!getTerrainEdit()->splatmapNeedRegen())
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("Quadrant getSplatmapBuffer 1 ") + __FUNCTION__, "Splatmap reload from disk");
					TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
					float minAltitude, maxAltitude;
					bool ok = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
				}
			}
			return m_splatmapBuffer;
		}

		TheWorld_Utils::MemoryBuffer& getColormapBuffer(bool reloadFromCache = true)
		{
			if (m_colormapBuffer.empty() && reloadFromCache)
			{
				if (!getTerrainEdit()->colormapNeedRegen)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("Quadrant getColormapBuffer 1 ") + __FUNCTION__, "Colormap reload from disk");
					TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
					float minAltitude, maxAltitude;
					bool ok = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
				}
			}
			return m_colormapBuffer;
		}

		TheWorld_Utils::MemoryBuffer& getGlobalmapBuffer(bool reloadFromCache = true)
		{
			if (m_globalmapBuffer.empty() && reloadFromCache)
			{
				if (!getTerrainEdit()->globalmapNeedRegen)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("Quadrant getGlobalmapBuffer 1 ") + __FUNCTION__, "Globalmap reload from disk");
					TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
					float minAltitude, maxAltitude;
					bool ok = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
				}
			}
			return m_globalmapBuffer;
		}

		//void fillQuadrantData(TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData& quadrantData)
		//{
		//	quadrantData.meshId = getMeshCacheBuffer().getMeshId();
		//	quadrantData.terrainEdit = getTerrainEdit();
		//	quadrantData.minHeight = getTerrainEdit()->minHeight;
		//	quadrantData.maxHeight = getTerrainEdit()->maxHeight;
		//	TheWorld_Utils::MemoryBuffer& heights16Buffer = getFloat16HeightsBuffer();
		//	quadrantData.heights16Buffer = &heights16Buffer;
		//	TheWorld_Utils::MemoryBuffer& heights32Buffer = getFloat32HeightsBuffer();
		//	quadrantData.heights32Buffer = &heights32Buffer;
		//	TheWorld_Utils::MemoryBuffer& normalsBuffer = getNormalsBuffer();
		//	quadrantData.normalsBuffer = &normalsBuffer;
		//	TheWorld_Utils::MemoryBuffer& splatmapBuffer = getSplatmapBuffer();
		//	quadrantData.splatmapBuffer = &splatmapBuffer;
		//	TheWorld_Utils::MemoryBuffer& colormapBuffer = getColormapBuffer();
		//	quadrantData.colormapBuffer = &colormapBuffer;
		//	TheWorld_Utils::MemoryBuffer& globalmapBuffer = getGlobalmapBuffer();
		//	quadrantData.globalmapBuffer = &globalmapBuffer;
		//	quadrantData.heightsUpdated = heightsUpdated();
		//	quadrantData.normalsUpdated = normalsUpdated();
		//	quadrantData.texturesUpdated = textureUpdated();
		//}

		//void resetHeightsBuffer(std::string actor)
		//{
		//	getFloat16HeightsBuffer(false).clear();
		//	getFloat32HeightsBuffer(false).clear();
		//	setEmpty(true);
		//	setHeightsUpdated(true);

		//	resetNormalsBuffer();
		//	resetColormapBuffer();
		//}

		void resetNormalsBuffer(std::string actor)
		{
			getNormalsBuffer(false).clear(actor);
			getTerrainEdit()->normalsNeedRegen = true;
			setNormalsUpdated(true);

			resetSplatmapBuffer(actor);
		}

		void resetSplatmapBuffer(std::string actor)
		{
			getSplatmapBuffer(false).clear(actor);
			getTerrainEdit()->splatmapNeedRegen(true);
			setSplatmapUpdated(true);
			
			resetGlobalmapBuffer(actor);
		}

		void resetColormapBuffer(std::string actor)
		{
			getColormapBuffer(false).clear(actor);
			getTerrainEdit()->colormapNeedRegen = true;
			setColorsUpdated(true);
		}

		void resetGlobalmapBuffer(std::string actor)
		{
			getGlobalmapBuffer(false).clear(actor);
			getTerrainEdit()->globalmapNeedRegen = true;
			setGlobalmapUpdated(true);
		}

		TheWorld_Utils::TerrainEdit* getTerrainEdit(bool create = true)
		{
			if (m_terrainEdit == nullptr && create)
			{
				m_terrainEdit = make_unique<TheWorld_Utils::TerrainEdit>();
			}
			return m_terrainEdit.get();
		}

		godot::AABB getGlobalAABB(void);
		
		void setNeedUploadToServer(bool b)
		{
			m_needUploadToServer = b;
		}

		bool needUploadToServer(void) const
		{
			return m_needUploadToServer;
		}

		bool needSaveToCache(bool& manual);

		bool lockInternalData(void);
		void unlockInternalData(void);

		bool internalDataLocked() const
		{
			return m_internalDataLocked;
		}

		void setHeightsUpdated(bool b);
		bool heightsUpdated(void) const
		{
			return m_heightsUpdated;
		}
		void setNormalsUpdated(bool b);
		bool normalsUpdated(void) const
		{
			return m_normalsUpdated;
		}
		void setSplatmapUpdated(bool b);
		bool splatmapUpdated(void) const
		{
			return m_splatmapUpdated;
		}
		void setColorsUpdated(bool b);
		bool colorsUpdated(void) const
		{
			return m_colorsUpdated;
		}
		void setGlobalmapUpdated(bool b);
		bool globalmapUpdated(void) const
		{
			return m_globalmapUpdated;
		}
		void setTextureUpdated(bool b)
		{
			m_texturesUpdated = b;
		}
		bool textureUpdated(void) const
		{
			return m_texturesUpdated;
		}
		//void setEmpty(bool b)
		//{
		//	m_empty = b;
		//}
		bool empty(void);

		void releaseTerrainValuesMemory(bool visible);

		size_t calcMapMemoryOccupation(void);
		size_t calcHeightmap16MemoryOccupation(void);
		size_t calcHeightmap32MemoryOccupation(void);
		size_t calcNormalmapMemoryOccupation(void);
		size_t calcSplatmapMemoryOccupation(void);
		size_t calcGlobalmapMemoryOccupation(void);
		size_t calcColormapMemoryOccupation(void);
		size_t calcTerrainEditMemoryOccupation(void);
		size_t calcColliderHeightsMemoryOccupation(void);

		size_t calcTexMemoryOccupation(void);
		size_t calcResizedTexMemoryOccupation(void);

		std::string getCacheFilePath(void)
		{
			return m_cache.getCacheFilePath();
		}

		void checkInternalData(double _delta);

		void process_checkBlendNeeded(double _delta);
		void process_mapModder(double _delta, bool worldQuadrant);
		bool process_blend(void);
		bool mapModdingRequired(void);
		
		GDN_TheWorld_MapModder* getMapModder(void)
		{
			return m_mapModder;
		}
		bool mapModderWorkDone(void) const
		{
			return m_mapModderWorkDone;
		}
		void mapModderWorkDone(bool b)
		{
			m_mapModderWorkDone = b;
		}

		void getQuadrantBufferForCache(TheWorld_Utils::MemoryBuffer& buffer);
		void getQuadrantBufferForCache(std::string& buffer);
		bool saveQuadrantToDiskCache2(std::string& buffer, bool needUploadToserver = true, bool newMeshId = false, bool lock = false);
		bool saveQuadrantToDiskCache2(TheWorld_Utils::MemoryBuffer& buffer, bool needUploadToserver = true, bool newMeshId = false, bool lock = false);
		bool saveQuadrantToDiskCache2(bool newMeshId = false, bool lock = false);

		void renderGlobalMap(bool b)
		{
			if (m_renderGlobalMap != b)
			{
				m_renderGlobalMap = b;
				renderGlobalMapChanged(true);
			}
		}
		bool renderGlobalMap(void) const
		{
			return m_renderGlobalMap;
		}
		bool canCacheHeigthBuffers(bool b)
		{
			if (m_canCacheHeigthBuffers != b)
			{
				m_canCacheHeigthBuffers = b;
				return true;
			}
			else
				return false;
		}
		bool canCacheHeigthBuffers(void) const
		{
			return m_canCacheHeigthBuffers;
		}
		void renderGlobalMapChanged(bool b);
		bool renderGlobalMapChanged(void) const
		{
			return m_renderGlobalMapChanged;
		}

		void optimizationLevel(size_t level);
		size_t optimizationLevel(void) const
		{
			return m_optimizationLevel;
		}
		void optimizationLevelChanged(bool b);
		bool optimizationLevelChanged(void) const
		{
			return m_optimizationLevelChanged;
		}

		godot::Vector3 getCenter(void) const
		{
			return m_quadrantCenter;
		}

		bool canHaveHiResCollisionInfo(void);

		bool executingProcess(void) const
		{
			return m_executingProcess;
		}
		void executingProcess(bool b)
		{
			m_executingProcess = b;
		}

		bool terrainDataJustLoaded(void) const
		{
			return m_terrainDataJustLoaded;
		}
		void terrainDataJustLoaded(bool b)
		{
			m_terrainDataJustLoaded = b;
		}
		
		godot::Vector4 northQuadrantTextureLayersFromTerrainData(void);
		godot::Vector4 westQuadrantTextureLayersFromTerrainData(void);
		godot::Vector4 northwestQuadrantTextureLayersFromTerrainData(void);
		void getSplatmapBorderRow(std::chrono::system_clock::time_point& lastBorderRefreshTs, TheWorld_Utils::MemoryBuffer& splatmapBorderRow, TheWorld_Utils::QuadrantPos::DirectionSlot requiredBorderDir);
		godot::Color getLowerRightCornerSplatmapTexel(void);
		godot::Vector4 quadrantTextureLayers(void) const;
		NeighbourQuadrantData& northQuadrantData(void)
		{
			return m_northQuadrantData;
		}
		NeighbourQuadrantData& southQuadrantData(void)
		{
			return m_southQuadrantData;
		}
		NeighbourQuadrantData& westQuadrantData(void)
		{
			return m_westQuadrantData;
		}
		NeighbourQuadrantData& eastQuadrantData(void)
		{
			return m_eastQuadrantData;
		}
		NeighbourQuadrantData& northWestQuadrantData(void)
		{
			return m_northWestQuadrantData;
		}
		NeighbourQuadrantData& southEastQuadrantData(void)
		{
			return m_southEastQuadrantData;
		}

		godot::Ref<godot::Image> getMapImage(void) const
		{
			return m_mapImage;
		}

		bool checkBlendNeeded(void);
		bool checkBlendNeeded(bool& northSideBlendNeeded, bool& southSideBlendNeeded, bool& eastSideBlendNeeded, bool& westSideBlendNeeded, QuadTree* northQuadrantTree = nullptr, QuadTree* southQuadrantTree = nullptr, QuadTree* eastQuadrantTree = nullptr, QuadTree* westQuadrantTree = nullptr);
		bool checkNorthSideBlendNeeded(QuadTree* northQuadrantTree = nullptr);
		bool checkSouthSideBlendNeeded(QuadTree* southQuadrantTree = nullptr);
		bool checkEastSideBlendNeeded(QuadTree* eastQuadrantTree = nullptr);
		bool checkWestSideBlendNeeded(QuadTree* westQuadrantTree = nullptr);

		void setBlendNeeded(bool b, bool northSideBlendNeeded, bool southSideBlendNeeded, bool eastSideBlendNeeded, bool westSideBlendNeeded);
		bool blendNeeded(void) const
		{
			return m_blendNeeded;
		}
		bool northSideBlendNeeded(void) const
		{
			return m_northSideBlendNeeded;
		}
		bool southSideBlendNeeded(void) const
		{
			return m_southSideBlendNeeded;
		}
		bool eastSideBlendNeeded(void) const
		{
			return m_eastSideBlendNeeded;
		}
		bool westSideBlendNeeded(void) const
		{
			return m_westSideBlendNeeded;
		}
		bool northQuadrantValid(void) const
		{
			return m_northQuadrantValid;
		}
		bool southQuadrantValid(void) const
		{
			return m_southQuadrantValid;
		}
		bool eastQuadrantValid(void) const
		{
			return m_eastQuadrantValid;
		}
		bool westQuadrantValid(void) const
		{
			return m_westQuadrantValid;
		}

		QuadTree* getQuadTree(void) const
		{
			return m_quadTree;
		}

		std::string textureLayout(void) const
		{
			return m_textureLayout;
		}
		std::string continentName(void) const
		{
			return m_continentName;
		}

		void updateTerrainEditTextureLayers(void);

	private:
		TheWorld_Utils::MeshCacheBuffer& getInternalMeshCacheBuffer(void);
		void loadTerrainDataFromCache(void);

	private:
		TheWorld_Utils::QuadrantPos m_quadrantPos;
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;
		static const std::string c_defaultTextureLayout;
		static const std::string c_defaultContinentName;
		std::string m_textureLayout = c_defaultTextureLayout;
		std::string m_continentName = c_defaultContinentName;
		bool m_lockInternalDataUntilCompleteProcessExecution = false;
		TheWorld_Utils::MeshCacheBuffer m_cache;
		godot::AABB m_globalCoordAABB;
		std::unique_ptr<Collider> m_collider;
		std::unique_ptr<ShaderTerrainData> m_shaderTerrainData;
		bool m_needUploadToServer;
		bool m_internalDataLocked;
		DWORD m_internalDataLockingTid = 0;
		std::recursive_mutex m_mtxInternalData;
		//bool m_empty = true;

		GDN_TheWorld_MapModder* m_mapModder = nullptr;
		bool m_mapModderWorkDone = false;

		godot::Vector3 m_quadrantCenter;

		// Terrain Values
		std::string m_meshId;
		TheWorld_Utils::MemoryBuffer m_float16HeigthsBuffer;	// each height is expressed as a 16-bit float (image with FORMAT_RH) and are serialized line by line (each line from x=0 to x=numVertexPerQuadrant, first line ==> z=0, last line z=numVertexPerQuadrant)
		TheWorld_Utils::MemoryBuffer m_float32HeigthsBuffer;	// each height is expressed as a 32-bit and are serialized as above
		TheWorld_Utils::MemoryBuffer m_normalsBuffer;			// each normal is expressed as a three bytes color (r=normal x, g=normal z, b=normal y) and are serialized in the same order as heights
		TheWorld_Utils::MemoryBuffer m_splatmapBuffer;			// for each vertex there is a four bytes color (r=lowElevationTex %, g=dirtTex %, b=highElevationTex %, a=slope %) and are serialized in the same order as heights
		TheWorld_Utils::MemoryBuffer m_colormapBuffer;			// Vertices color
		TheWorld_Utils::MemoryBuffer m_globalmapBuffer;
		std::unique_ptr<TheWorld_Utils::TerrainEdit> m_terrainEdit;

		TheWorld_Utils::MemoryBuffer m_splatmapNorthBorderRow;		// north border row of the splatmap
		TheWorld_Utils::MemoryBuffer m_splatmapSouthBorderRow;		// south border row of the splatmap
		TheWorld_Utils::MemoryBuffer m_splatmapWestBorderRow;		// west border row of the splatmap
		TheWorld_Utils::MemoryBuffer m_splatmapEastBorderRow;		// east border row of the splatmap
		std::chrono::system_clock::time_point m_lastBorderRefreshTs;

#ifdef REAL_T_IS_DOUBLE
		godot::PackedFloat64Array m_heightsForCollider;
#else
		godot::PackedFloat32Array m_heightsForCollider;
#endif // REAL_T_IS_DOUBLE
		godot::Ref<godot::Image> m_mapImage;
		// Terrain Values

		NeighbourQuadrantData m_northQuadrantData;
		NeighbourQuadrantData m_southQuadrantData;
		NeighbourQuadrantData m_westQuadrantData;
		NeighbourQuadrantData m_eastQuadrantData;

		NeighbourQuadrantData m_northWestQuadrantData;
		NeighbourQuadrantData m_southEastQuadrantData;

		bool m_heightsUpdated;
		bool m_normalsUpdated;
		bool m_splatmapUpdated;
		bool m_colorsUpdated;
		bool m_globalmapUpdated;
		bool m_texturesUpdated;
		//bool m_terrainValuesCanBeCleared;

		bool m_renderGlobalMap = false;
		bool m_renderGlobalMapChanged = false;
		bool m_canCacheHeigthBuffers = false;
		size_t m_optimizationLevel = 0;		// optimization level 0 = no optimization: we use full texture and we can split root chunk in child chunks according to distance from camera, for optimization level > 0 we use compressed textures and we don't split the root chunk
		bool m_optimizationLevelChanged = false;

		bool m_executingProcess = false;
		bool m_terrainDataJustLoaded = false;

		bool m_blendNeeded = false;
		bool m_northSideBlendNeeded = false;
		bool m_southSideBlendNeeded = false;
		bool m_eastSideBlendNeeded = false;
		bool m_westSideBlendNeeded = false;
		bool m_northQuadrantValid = false;
		bool m_southQuadrantValid = false;
		bool m_eastQuadrantValid = false;
		bool m_westQuadrantValid = false;
	};

	class ShaderTerrainData
	{
	public:
#define water_globalmap_gdshader_u_sea_color		godot::Color(43.0F / 255.0F, 57.0F / 255.0F, 73.0F / 255.0F)	// 0,16863, 0,22353, 0,28627

		// Shader Params
#define SHADER_PARAM_TERRAIN_HEIGHTMAP				"u_terrain_heightmap"
#define SHADER_PARAM_TERRAIN_NORMALMAP				"u_terrain_normalmap"
#define SHADER_PARAM_TERRAIN_SPLATMAP				"u_terrain_splatmap"
#define SHADER_PARAM_TERRAIN_COLORMAP				"u_terrain_colormap"
//#define SHADER_PARAM_TERRAIN_GLOBALMAP				"u_terrain_globalmap"

#define SHADER_PARAM_INVERSE_TRANSFORM				"u_terrain_inverse_transform"
#define SHADER_PARAM_NORMAL_BASIS					"u_terrain_normal_basis"
#define SHADER_PARAM_QUADRANT_SIZE					"u_quadrant_size"
#define SHADER_PARAM_GRID_STEP						"u_grid_step"
#define SHADER_PARAM_FORCE_ELEVATION_0				"u_force_height_0"
//#define SHADER_PARAM_USE_GLOBALMAP					"u_use_globalmap"
#define SHADER_PARAM_OPTIMIZATION_LEVEL				"u_optimization_level"


#define SHADER_PARAM_NEED_BLENDING					"u_need_blending"
#define SHADER_PARAM_NEED_BLENDING_NORTH			"u_need_blending_north"
#define SHADER_PARAM_NEED_BLENDING_SOUTH			"u_need_blending_south"
#define SHADER_PARAM_NEED_BLENDING_EAST				"u_need_blending_east"
#define SHADER_PARAM_NEED_BLENDING_WEST				"u_need_blending_west"
#define SHADER_PARAM_WIRE_ALBEDO					"u_wireframe_albedo"
#define SHADER_PARAM_NEED_BLENDING_ALBEDO			"u_need_blending_albedo"
#define SHADER_PARAM_SELECTED_ALBEDO				"u_selected_albedo"

#define GLOBAL_SHADER_PARAM_TIME					"gu_time"
#define GLOBAL_SHADER_PARAM_GRID_STEP				"gu_grid_step"

#define GLOBAL_SHADER_PARAM_ALBEDO_DISPL_ARRAY		"gu_ground_albedo_displ_array"
#define GLOBAL_SHADER_PARAM_NORMAL_ROUGHNESS_ARRAY	"gu_ground_normal_roughness_array"
#define GLOBAL_SHADER_PARAM_BASE_VALUES_ARRAY		"gu_ground_base_values_array"
//#define GLOBAL_SHADER_PARAM_GROUND_ALBEDO_ARRAY		"gu_ground_albedo_array"
//#define GLOBAL_SHADER_PARAM_GROUND_NORMAL_ARRAY		"gu_ground_normal_array"
//#define GLOBAL_SHADER_PARAM_GROUND_ROUGHNESS_ARRAY	"gu_ground_roughness_array"
//#define GLOBAL_SHADER_PARAM_GROUND_DISPL_ARRAY		"gu_ground_displ_array"
//#define GLOBAL_SHADER_PARAM_GROUND_AO_ARRAY			"gu_ground_ao_array"

#define GLOBAL_SHADER_PARAM_CAM_ALTITUDE			"gu_camera_altitude_from_terrain"
#define SHADER_PARAM_OVERRIDE_CAM_ALTITUDE			"u_override_camera_altitude_from_terrain"
#define GLOBAL_SHADER_PARAM_CAM_POS					"gu_camera_pos"
#define GLOBAL_SHADER_PARAM_CAM_ON_SEA				"gu_camera_on_sea"
#define GLOBAL_SHADER_PARAM_CAM_UNDER_WATER			"gu_camera_under_water"
//#define GLOBAL_SHADER_PARAM_SHOW_SEA				"gu_show_sea"
//#define GLOBAL_SHADER_PARAM_SHOW_TERRAIN			"gu_show_terrain"
#define GLOBAL_SHADER_PARAM_SHOW_CHUNKS				"gu_show_chunks"
#define GLOBAL_SHADER_PARAM_SHOW_WIREFRAME			"gu_show_wireframe"
#define GLOBAL_SHADER_PARAM_SHOW_NEED_BLENDING		"gu_show_need_blending"
//#define GLOBAL_SHADER_PARAM_INVERSE_TRANSFORM		"gu_terrain_inverse_transform"
//#define GLOBAL_SHADER_PARAM_TEXTURE_OVERFLOW_SIZE	"gu_texture_overflow_size"				// actual value must be set in internal member variable GDN_TheWorld_Globals::m_textureBlendingSize to avoid reading it from RenderingServer (slow operation)
//#define GLOBAL_SHADER_PARAM_TEXTURE_BLENDING_SIZE	"gu_splatmap_blending_size"				// actual value must be set in internal member variable GDN_TheWorld_Globals::m_splatmapBlendingSize to avoid reading it from RenderingServer (slow operation)
#define GLOBAL_SHADER_PARAM_FORCE_ELEVATION_0		"gu_force_height_0"
#define GLOBAL_SHADER_PARAM_HIGHLIGHT_HIT_QUAD		"gu_highlight_hit_quadrant"
#define GLOBAL_SHADER_PARAM_HOLD_BLENDING			"gu_hold_blending"

#define GLOBAL_SHADER_PARAM_SEA_FLAT_SEA_HEIGHT							"gu_sea_flat_sea_height"
#define	DEFAULT_SHADER_PARAM_SEA_FLAT_SEA_HEIGHT							THEWORLD_SEA_LEVEL
#define GLOBAL_SHADER_PARAM_WATER_FOAM									"gu_water_foam"
#define GLOBAL_SHADER_PARAM_SEA_NOISE_SCALE								"gu_sea_noise_scale"
#define	DEFAULT_SHADER_PARAM_SEA_NOISE_SCALE								1000.0f
#define GLOBAL_SHADER_PARAM_SEA_TIME_SCALE								"gu_sea_time_scale"
#define	DEFAULT_SHADER_PARAM_SEA_TIME_SCALE									0.0005f
#define GLOBAL_SHADER_PARAM_SEA_ROUGH_FACTOR							"gu_sea_rough_factor"
#define	DEFAULT_SHADER_PARAM_SEA_ROUGH_FACTOR								10.0f
#define GLOBAL_SHADER_PARAM_SEA_HEIGHT_SCALE							"gu_sea_height_scale"
#define	DEFAULT_SHADER_PARAM_SEA_HEIGHT_SCALE								4.0f
#define GLOBAL_SHADER_PARAM_SEA_END_CAM_ALT_FOR_WAVES					"gu_sea_end_camera_altitude_for_waves"
#define	DEFAULT_SHADER_PARAM_SEA_END_CAM_ALT_FOR_WAVES						1000.0f
#define GLOBAL_SHADER_PARAM_SEA_START_CAM_ALT_FOR_WAVES_ATTENUATION		"gu_sea_start_camera_altitude_for_waves_attenuation"
#define	DEFAULT_SHADER_PARAM_SEA_START_CAM_ALT_FOR_WAVES_ATTENUATION		200.0f
#define GLOBAL_SHADER_PARAM_SEA_START_TERR_DEPTH_FOR_WAVES_ATTENUATION	"gu_sea_start_terrain_depth_for_waves_attenuation"
#define	DEFAULT_SHADER_PARAM_SEA_START_TERR_DEPTH_FOR_WAVES_ATTENUATION		50.0f
#define GLOBAL_SHADER_PARAM_SEA_SCALE_OFFSET_THRESHOLD					"gu_sea_scale_offset_threshold"
#define	DEFAULT_SHADER_PARAM_SEA_SCALE_OFFSET_THRESHOLD						20.0f

//#define SHADER_PARAM_HIGH_MOUNTAINS_TEX_INDEX			"u_high_mountains_tex_index"
//#define SHADER_PARAM_HIGH_MOUNTAINS_ALTITUDE_THRESHOLD	"u_high_mountains_alt_threshold"
//#define SHADER_PARAM_MOUNTAINS_TEX_INDEX				"u_mountains_tex_index"
//#define SHADER_PARAM_MOUNTAINS_ALTITUDE_THRESHOLD		"u_mountains_alt_threshold"
//#define SHADER_PARAM_LOW_MOUNTAINS_TEX_INDEX			"u_low_mountains_tex_index"
//#define SHADER_PARAM_LOW_MOUNTAINS_ALTITUDE_THRESHOLD	"u_low_mountains_alt_threshold"
//#define SHADER_PARAM_HILLS_TEX_INDEX					"u_hills_tex_index"
//#define SHADER_PARAM_HILLS_ALTITUDE_THRESHOLD			"u_hills_alt_threshold"
//#define SHADER_PARAM_PLAINS_TEX_INDEX					"u_plains_tex_index"
//#define SHADER_PARAM_PLAINS_ALTITUDE_THRESHOLD			"u_plains_alt_threshold"
//#define SHADER_PARAM_ABYSS_TEX_INDEX					"u_abyss_tex_index"
//#define SHADER_PARAM_DIRT_TEX_INDEX						"u_dirt_tex_index"
//#define SHADER_PARAM_ROCKS_TEX_INDEX					"u_rocks_tex_index"

#define SHADER_PARAM_GROUND_LAYERS					"u_ground_layers"	// godot::Plane (vec4) ==> normal_x (vec4 x) = low, normal_y (vec4 y) = high, normal_z (vec4 z) = dirt, d (vec4 w) = rock
#define SHADER_PARAM_NORTH_GROUND_LAYERS			"u_north_ground_layers"
//#define SHADER_PARAM_SOUTH_GROUND_LAYERS			"u_south_ground_layers"
#define SHADER_PARAM_WEST_GROUND_LAYERS				"u_west_ground_layers"
//#define SHADER_PARAM_EAST_GROUND_LAYERS			"u_east_ground_layers"
#define SHADER_PARAM_NORTH_WEST_GROUND_LAYERS		"u_north_west_ground_layers"
//#define SHADER_PARAM_SOUTH_EAST_GROUND_LAYERS		"u_south_east_ground_layers"

#define SHADER_PARAM_NORTH_WEST_CORNER_SPLATMAP_TEXEL	"u_north_west_corner_splatmap_texel"
#define SHADER_PARAM_NORTH_BORDER_SPLATMAP			"u_north_border_splatmap"
#define SHADER_PARAM_WEST_BORDER_SPLATMAP			"u_west_border_splatmap"

#define SHADER_PARAM_EDITMODE_SELECTED				"u_editmode_selected"
#define SHADER_PARAM_MOUSE_HIT_UV_QUADRANT			"u_mouse_hit_uv_quadrant"
#define SHADER_PARAM_MOUSE_HIT_RADIUS				"u_mouse_hit_radius"
#define SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS		"u_quad_border_line_thickness"

#define SHADER_PARAM_MOUSE_GRID_ON					"u_grid_on"
#define SHADER_PARAM_GENERATE_GLOBAL_MAP			"u_generate_global_map"

#define SHADER_PARAM_VERTICES_PER_CHUNK				"u_num_vertices_per_chunk"
#define SHADER_PARAM_LOOKDEV_LOD					"u_lod"
#define	SHADER_PARAM_LOD_GRID_STEP					"u_lod_grid_step"
#define	SHADER_PARAM_LOD_CHUNK_SIZE					"u_lod_chunk_size"

//#define SHADER_PARAM_GROUND_ALBEDO_BUMP_0			"u_ground_albedo_bump_0"
//#define SHADER_PARAM_GROUND_ALBEDO_BUMP_1			"u_ground_albedo_bump_1"
//#define SHADER_PARAM_GROUND_ALBEDO_BUMP_2			"u_ground_albedo_bump_2"
//#define SHADER_PARAM_GROUND_ALBEDO_BUMP_3			"u_ground_albedo_bump_3"

//#define SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_0		"u_ground_normal_roughness_0"
//#define SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_1		"u_ground_normal_roughness_1"
//#define SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_2		"u_ground_normal_roughness_2"
//#define SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_3		"u_ground_normal_roughness_3"

#define SHADER_PARAM_GROUND_UV_SCALE_PER_TEXTURE	"u_ground_uv_scale_per_texture"
#define SHADER_PARAM_DEPTH_BLENDING					"u_depth_blending"
#define SHADER_PARAM_TRIPLANAR						"u_triplanar"
#define SHADER_PARAM_TILE_REDUCTION_PER_TEXTURE		"u_tile_reduction_per_texture"
#define SHADER_PARAM_GLOBALMAP_BLEND_START			"u_globalmap_blend_start"
#define SHADER_PARAM_GLOBALMAP_BLEND_DISTANCE		"u_globalmap_blend_distance"
#define SHADER_PARAM_COLORMAP_OPACITY_PER_TEXTURE	"u_colormap_opacity_per_texture"

#define SHADER_PARAM_LOOKDEV_MAP					"u_map"

#define SHADER_PARAM_SLOPE_VERTICAL_FACTOR			"u_slope_vertical_factor"
#define SHADER_PARAM_SLOPE_FLAT_FACTOR				"u_slope_flat_factor"
#define SHADER_PARAM_DIRT_ON_ROCKS_FACTOR			"u_dirt_on_rocks_factor"
#define SHADER_PARAM_HIGH_ELEVATION_FACTOR			"u_high_elevation_factor"
#define SHADER_PARAM_LOW_ELEVATION_FACTOR			"u_low_elevation_factor"
#define SHADER_PARAM_MAX_HEIGHT						"u_max_heigth"
#define SHADER_PARAM_MIN_HEIGHT						"u_min_heigth"
#define SHADER_PARAM_NORTH_HEIGHTS					"u_north_heights"
#define SHADER_PARAM_SOUTH_HEIGHTS					"u_south_heights"
#define SHADER_PARAM_WEST_HEIGHTS					"u_west_heights"
#define SHADER_PARAM_EAST_HEIGHTS					"u_east_heights"

	public:

		enum class LookDev {
			NotSet = 0
			, Heights = 1
			, Normals = 2
			, Splat = 3
			, Color = 4
			, Global = 5
			, ChunkLod = 6
			, Universe = 7
			, ShaderArt = 8
			, Lightning = 9
			, StarNest = 10
			, FlaringStar = 11
			, AnimatedDiamond = 12
			, PlasmaWaves = 13
		};

		ShaderTerrainData(GDN_TheWorld_Viewer* viewer, QuadTree* quadTree);
		~ShaderTerrainData();
		void init(void);
		//static godot::Ref<godot::Image> readGroundTexture(godot::String fileName, bool& ok, GDN_TheWorld_Viewer* viewer = nullptr);
		//static void getGroundTexture(godot::String fileName, GroundTexture* groundTexture, GDN_TheWorld_Viewer* viewer = nullptr);
		bool materialParamsNeedReset(void) const
		{
			return m_materialParamsNeedReset;
		}
		void materialParamsNeedReset(bool b);
		void resetMaterialParams(enum class QuadType type, bool& lastProcessExecutionInterrupted, LookDev lookdev);
		bool materialParamsNeedUpdateTransformChanged(void) const
		{
			return m_materialParamsNeedUpdateTransformChanged;
		}
		bool materialParamsNeedUpdate(void) const
		{
			return m_materialParamsNeedUpdate;
		}
		void materialParamsNeedUpdate(bool b, bool transformChanged = false);
		void updateMaterialParams(enum class QuadType type, LookDev lookdev, bool freeMaps = true, bool updateTerrainData = true);
		enum class ImageType
		{
			heightmap = 0
			,normalmap = 1
			,splatmap = 2
			,globalmap = 3
			,colormap = 4
		};
		godot::Ref<godot::Image> getImageAtResolution(std::string& profileHeader, godot::Ref<godot::Image> hiResImage, std::map<size_t, godot::Ref<godot::Image>>& resizedImageMap, size_t optimizationLevel, size_t resolution);
		godot::Ref<godot::Image> getImageAtResolution(std::string& profileHeader, enum class ImageType imageType, size_t optimizationLevel, size_t resolution);
		godot::Ref<godot::Image> resizeImage(std::string& profileHeader, godot::Ref<godot::Image> hiResImage, size_t newResolution, std::map<size_t, godot::Ref<godot::Image>>& resizedImageMap, size_t optimizationLevel);
		godot::Ref<godot::Material> getRegularSeaMaterial(bool reload = false);
		godot::Ref<godot::Material> getGlobalmapSeaMaterial(bool reload = false);
		godot::Ref<godot::Material> getRegularTerrainMaterial(bool reload = false);
		void releaseRegularTerrainMaterial(void);
		godot::Ref<godot::Material> getLookDevMaterial(enum class ShaderTerrainData::LookDev lookDev, bool reload = false);
		void releaseLookDevMaterial(void);
		std::map<int, godot::Ref<godot::ShaderMaterial>>& getLookDevChunkLodMaterials(void);
		void releaseLookDevChunkLodMaterials(void);
		godot::Ref<godot::Material> getLookDevChunkLodMaterial(int lod, bool reload = false);
		static void freeLookDevSubViewport(void);
		static void releaseGlobals(void);
		static std::string getGroundTexturesDir(void);
		static godot::Ref<godot::Image> loadGroundImage(std::string filePath, std::string mapType, std::string groundImageName, int desiredImageSize, godot::Image::Format desiredImageFormat, bool returnEmpty, GDN_TheWorld_Viewer* viewer = nullptr);
		static godot::Ref<godot::Texture2D> loadGroundTexture(std::string filePath, std::string mapType, std::string groundImageName, int desiredImageSize, godot::Image::Format desiredImageFormat, bool returnEmpty, GDN_TheWorld_Viewer* viewer = nullptr);
		static void loadAllGroundTextures(std::string groundTexturesDir, GDN_TheWorld_Viewer* viewer);
		static bool loadGroundImagesByName(std::string imagesDirPath, std::string imagesDirName, std::string textureName, GDN_TheWorld_Viewer* viewer, godot::Ref<godot::Image>& albedoDisplImage, godot::Ref<godot::Image>& normalRoughnessImage, godot::Ref<godot::Texture2D>& texturePreview);
		class GroundTexture
		{
		public:
			GroundTexture(int32_t idx, std::string layout, std::string textureName, godot::Ref<godot::Texture2D> texturePreview)
			{
				m_idx = idx;
				m_layout = layout;
				m_textureName = textureName;
				m_texturePreview = texturePreview;
			}
			int32_t idx(void) const
			{
				return m_idx;
			}
			godot::Ref<godot::Texture2D> texturePreview(void) const
			{
				return m_texturePreview;
			}
			std::string layout(void) const
			{
				return m_layout;
			}
			std::string textureName(void) const
			{
				return m_textureName;
			}
			bool isDefaultLayout(void)
			{
				if (m_layout.length() == 0 or m_layout == Quadrant::c_defaultTextureLayout)
					return true;
				else
					return false;
			}

		private:
			int32_t m_idx;
			std::string m_layout;
			std::string m_textureName;
			godot::Ref<godot::Texture2D> m_texturePreview;
		};
		class GroundTextures : public std::map<std::string, std::unique_ptr<GroundTexture>>
		{
		public:
			godot::Ref<godot::Texture2D> getTexturePreview(std::string layout, std::string textureName, std::string& textureLayout)
			{
				GroundTexture* g = getGroundTexture(layout, textureName, textureLayout);
				if (g == nullptr)
					return nullptr;
				return g->texturePreview();
			}
			int32_t getTextureIdx(std::string layout, std::string textureName, std::string& layoutForFoundTexture)
			{
				GroundTexture* g = getGroundTexture(layout, textureName, layoutForFoundTexture);
				if (g == nullptr)
					return -1;
				return g->idx();
			}
			std::string getTextureName(int32_t idx)
			{
				std::string layout;
				return getTextureName(idx, layout);
			}
			std::string getTextureName(int32_t idx, std::string& layout)
			{
				layout.clear();
				std::string textureName;
				for (auto& groundTexture : *this)
				{
					if (groundTexture.second->idx() == idx)
					{
						textureName = groundTexture.second->textureName();
						layout = groundTexture.second->layout();
						break;
					}
				}
				return textureName;
			}
			bool containTexture(std::string layout, std::string textureName)
			{
				std::string textureLayout;
				GroundTexture* g = getGroundTexture(layout, textureName, textureLayout);
				if (g == nullptr)
					return false;
				else
					return true;;
			}

		private:
			GroundTexture* getGroundTexture(std::string layout, std::string textureName, std::string& layoutForFoundTexture)
			{
				GroundTexture* g = nullptr;
				layoutForFoundTexture = "";
				if (contains(layout + "_" + textureName))
				{
					g = (*this)[layout + "_" + textureName].get();
					layoutForFoundTexture = layout;
				}
				else if (contains(Quadrant::c_defaultTextureLayout + "_" + textureName))
				{
					g = (*this)[Quadrant::c_defaultTextureLayout + "_" + textureName].get();
					layoutForFoundTexture = Quadrant::c_defaultTextureLayout;
				}
				return g;
			}
		};
		static GroundTextures& getGroundTextures(void)
		{
			return s_groundTextures;
		}

		void mouseHitChanged(godot::Vector2 mouseHit);

		godot::Ref<godot::Image> getOrCreateHeightmapImage();
		static godot::Ref<godot::Image> createHeightmapImage(TheWorld_Utils::MemoryBuffer& heightmapBuffer, int _resolution);
		godot::Ref<godot::Image> getOrCreateNormalmapImage();
		static godot::Ref<godot::Image> createNormalmapImage(TheWorld_Utils::MemoryBuffer& normalmapBuffer, int _resolution);
		godot::Ref<godot::Image> getOrCreateSplatmapImage();
		static godot::Ref<godot::Image> createSplatlmapImage(TheWorld_Utils::MemoryBuffer& splatmapBuffer, int _resolution);
		godot::Ref<godot::Image> getOrCreateGlobalmapImage();
		static godot::Ref<godot::Image> createGlobalmapImage(TheWorld_Utils::MemoryBuffer& globalmapBuffer, int _resolution);
		godot::Ref<godot::Image> getOrCreateColormapImage();
		static godot::Ref<godot::Image> createColormapImage(TheWorld_Utils::MemoryBuffer& colormapBuffer, int _resolution);

		godot::Ref<godot::Image> getHeightmapImage(void)
		{
			return m_heightmapImage;
		}

		godot::Ref<godot::Image>& getHeightmapImageRef(void)
		{
			return m_heightmapImage;
		}
		std::map<size_t, godot::Ref<godot::Image>>& getResizedHeightmapImageMap(void)
		{
			return m_resizedHeightmapImageMap;
		}
		godot::Ref<godot::Image>& getNormalmapImageRef(void)
		{
			return m_normalmapImage;
		}
		std::map<size_t, godot::Ref<godot::Image>>& getResizedNormalmapImageMap(void)
		{
			return m_resizedNormalmapImageMap;
		}
		godot::Ref<godot::Image>& getSplatmapImageRef(void)
		{
			return m_splatmapImage;
		}
		std::map<size_t, godot::Ref<godot::Image>>& getResizedSplatmapImageMap(void)
		{
			return m_resizedSplatmapImageMap;
		}
		godot::Ref<godot::Image>& getColormapImageRef(void)
		{
			return m_colormapImage;
		}
		std::map<size_t, godot::Ref<godot::Image>>& getResizedColormapImageMap(void)
		{
			return m_resizedColormapImageMap;
		}
		godot::Ref<godot::Image>& getGlobalmapImageRef(void)
		{
			return m_globalmapImage;
		}
		std::map<size_t, godot::Ref<godot::Image>>& getResizedGlobalmapImageMap(void)
		{
			return m_resizedGlobalmapImageMap;
		}

		size_t calcTexMemoryOccupation(void)
		{
			size_t occupation = m_heightmapImage == nullptr ? 0 : m_heightmapImage->get_data().size();
			occupation += m_normalmapImage == nullptr ? 0 : m_normalmapImage->get_data().size();
			occupation += m_splatmapImage == nullptr ? 0 : m_splatmapImage->get_data().size();
			occupation += m_globalmapImage == nullptr ? 0 : m_globalmapImage->get_data().size();
			occupation += m_colormapImage == nullptr ? 0 : m_colormapImage->get_data().size();
			occupation += calcResizedTexMemoryOccupation();
			return occupation;
		}

		size_t calcResizedTexMemoryOccupation(void)
		{
			size_t occupation = 0;
			for (auto& item : m_resizedHeightmapImageMap)
				if (item.second != nullptr)
					occupation += item.second->get_data().size();
			for (auto& item : m_resizedNormalmapImageMap)
				if (item.second != nullptr)
					occupation += item.second->get_data().size();
			for (auto& item : m_resizedSplatmapImageMap)
				if (item.second != nullptr)
					occupation += item.second->get_data().size();
			for (auto& item : m_resizedGlobalmapImageMap)
				if (item.second != nullptr)
					occupation += item.second->get_data().size();
			for (auto& item : m_resizedColormapImageMap)
				if (item.second != nullptr)
					occupation += item.second->get_data().size();
			return occupation;
		}

		//void clearCachedTexture(void);

		//static std::string layoutFromContinentId(int32_t id);
		static std::string layoutFromQuadrantPos(TheWorld_Utils::QuadrantPos pos, std::string& continentName);
		static std::string layoutFromQuadrantPos(TheWorld_Utils::QuadrantPos pos);

	private:
		//void debugPrintTexture(std::string tex_name, Ref<Texture> tex);
		//Color encodeNormal(Vector3 normal);

	private:
		static std::string s_groundTexturesDir;
		static std::recursive_mutex s_internalMutex;

		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;

		godot::Ref<godot::ShaderMaterial> m_regularSeaMaterial;
		godot::Ref<godot::ShaderMaterial> m_globalmapSeaMaterial;

		godot::Ref<godot::ShaderMaterial> m_regularTerrainMaterial;
		godot::Ref<godot::ShaderMaterial> m_lookDevMaterial;
		//static godot::SubViewport* g_lookDevSubviewport;
		static GDN_TheWorld_ShaderTexture* g_lookDevShaderTexture;
		std::map<int, godot::Ref<godot::ShaderMaterial>> m_lookDevChunkLodMaterials;
		bool m_materialParamsNeedUpdate;
		bool m_materialParamsNeedUpdateTransformChanged = false;
		bool m_materialParamsNeedReset;

		godot::Ref<godot::Texture> m_heightmapTexture;
		bool m_heightMapTexModified;
		godot::Ref<godot::Image> m_heightmapImage;
		std::map<size_t, godot::Ref<godot::Image>> m_resizedHeightmapImageMap;

		godot::Ref<godot::Texture> m_normalmapTexture;
		bool m_normalMapTexModified;
		godot::Ref<godot::Image> m_normalmapImage;
		std::map<size_t, godot::Ref<godot::Image>> m_resizedNormalmapImageMap;

		godot::Ref<godot::Texture> m_splatmapTexture;
		bool m_splatMapTexModified;
		godot::Ref<godot::Image> m_splatmapImage;
		std::map<size_t, godot::Ref<godot::Image>> m_resizedSplatmapImageMap;

		godot::Ref<godot::Texture> m_colormapTexture;
		bool m_colorMapTexModified;
		godot::Ref<godot::Image> m_colormapImage;
		std::map<size_t, godot::Ref<godot::Image>> m_resizedColormapImageMap;

		godot::Ref<godot::Texture> m_globalmapTexture;
		bool m_globalMapTexModified;
		godot::Ref<godot::Image> m_globalmapImage;
		std::map<size_t, godot::Ref<godot::Image>> m_resizedGlobalmapImageMap;

		static std::recursive_mutex s_mtxStaticData;
		static GroundTextures s_groundTextures;
		
		class Continent
		{
		public:
			class Margin
			{
			public:
				class Delimiter
				{
				public:
					enum class DelimiterType
					{
						NotSet = -1,
						NW = 0,
						SE = 1
					};

				public:
					void setDelimiter(enum class DelimiterType delimiterType, int32_t quadrantPosX, int32_t quadrantPosZ)
					{
						m_delimiterType = delimiterType;
						m_quadrantPosX = quadrantPosX;
						m_quadrantPosZ = quadrantPosZ;
					}
					enum class DelimiterType delimiterType(void) const
					{
						return m_delimiterType;
					}
					int32_t quadrantPosX(void) const
					{
						return m_quadrantPosX;
					}
					int32_t quadrantPosZ(void) const
					{
						return m_quadrantPosZ;
					}

				private:
					enum class DelimiterType m_delimiterType = DelimiterType::NotSet;
					int32_t m_quadrantPosX = 0;
					int32_t m_quadrantPosZ = 0;
				};

			public:
				Margin(void)
				{
					m_delimiters[Delimiter::DelimiterType::NW] = std::make_unique<Delimiter>();
					m_delimiters[Delimiter::DelimiterType::SE] = std::make_unique<Delimiter>();
				}
				void setDelimiter(enum class Delimiter::DelimiterType delimiterType, int32_t quadrantPosX, int32_t quadrantPosZ)
				{
					m_delimiters[delimiterType]->setDelimiter(delimiterType, quadrantPosX, quadrantPosZ);
				}
				Delimiter* getDelimiter(Delimiter::DelimiterType delimiterType)
				{
					if (not m_delimiters.contains(delimiterType) or m_delimiters[delimiterType]->delimiterType() == Delimiter::DelimiterType::NotSet)
						return nullptr;
					return m_delimiters[delimiterType].get();
				}

			private:
				std::map<Delimiter::DelimiterType, std::unique_ptr<Delimiter>> m_delimiters;
			};

		public:
			Continent(int32_t id, std::string name, std::string layout)
			{
				m_id = id;
				m_name = name;
				m_layout = layout;
			}
			std::string name()
			{
				return m_name;
			}
			std::string layout()
			{
				if (m_layout.length() > 0)
					return m_layout;
				else
					return Quadrant::c_defaultTextureLayout;
			}
			bool addMargin(std::string marginAsString)
			{
				int32_t NW_quadrantPosX = 0, NW_quadrantPosZ = 0;
				int32_t SE_quadrantPosX = 0, SE_quadrantPosZ = 0;

				std::vector<std::string> v = TheWorld_Utils::Utils::split(marginAsString, ';');
				if (v.size() != 2)
					return false;
				for (int i = 0; i < v.size(); i++)
				{
					std::vector<std::string> v1 = TheWorld_Utils::Utils::split(v[i], ':');
					if (v1.size() != 2)
						return false;
					std::vector<std::string> v2 = TheWorld_Utils::Utils::split(v1[1], ',');
					if (v2.size() != 2)
						return false;
					if (v1[0] == "NW")
					{
						NW_quadrantPosX = std::stoi(v2[0]);
						NW_quadrantPosZ = std::stoi(v2[1]);
					}
					else if (v1[0] == "SE")
					{
						SE_quadrantPosX = std::stoi(v2[0]);
						SE_quadrantPosZ = std::stoi(v2[1]);
					}
					else
						return false;
				}
				
				m_margins.push_back(std::make_unique<Margin>());
				m_margins.back()->setDelimiter(Margin::Delimiter::DelimiterType::NW, NW_quadrantPosX, NW_quadrantPosZ);
				m_margins.back()->setDelimiter(Margin::Delimiter::DelimiterType::SE, SE_quadrantPosX, SE_quadrantPosZ);

				return true;
			}

			Margin* getMargin(size_t idx)
			{
				if (idx < m_margins.size())
					return m_margins[idx].get();
				else
					return nullptr;
			}

		private:
			int32_t m_id;
			std::string m_name;
			std::string m_layout;
			std::vector<std::unique_ptr<Margin>> m_margins;
		};
		
		static std::map<int32_t, std::unique_ptr<Continent>> s_continents;
	};
	
	class Quad
	{
	public:

		Quad(enum class QuadType chunkType, int slotPosX, int slotPosZ, int lod, enum PosInQuad posInQuad, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree);
		~Quad();

		int slotPosX() const
		{
			return m_slotPosX;
		}
		int slotPosZ() const
		{
			return m_slotPosZ;
		}
		int Lod() const
		{
			return m_lod;
		}
		enum PosInQuad getPosInQuad(void) const
		{
			return m_posInQuad;
		}

		bool isLeaf(void);
		Quad *getChild(int idx);
		void recycleChunk(void);
		void split(void);
		void clearChildren(void);
		void assignChunk(void);
		bool gotJustSplitted(void) const
		{
			return m_justSplitted;
		}
		void setJustSplitted(bool b)
		{
			m_justSplitted = b;
		}
		bool gotJustJoined(void) const
		{
			return m_justJoined;
		}
		void setJustJoined(bool b)
		{
			m_justJoined = b;
		}
		Chunk* getChunk(void)
		{
			return m_chunk; 
		}
		AABB getChunkAABB(void)
		{
			AABB aabb;
			if (m_chunkAABB == aabb)
			{
				if (m_chunk != nullptr)
					m_chunkAABB = m_chunk->getAABB();
				else
					return aabb;
			}
			return m_chunkAABB;
		}
		void resetChachedChunkAABB()
		{
			m_chunkAABB = AABB();
		}
		float getChunkSizeInWUs(void) const
		{
			return m_chunkSizeInWUs; 
		}
		void setCameraPosInMap(godot::Vector3 cameraPosInMap);
		void setSplitRequired(bool b)
		{
			if (b)
				m_splitRequired = b;
			else
				m_splitRequired = b;
		}
		bool splitRequired(void) const
		{
			return m_splitRequired;
		}


	private:
		enum class QuadType m_quadType = QuadType::NotSet;
		enum PosInQuad m_posInQuad;
		std::array<std::unique_ptr<Quad>, 4> m_children;
		Chunk* m_chunk;
		int m_slotPosX;	// expresses the orizzontal (X) and vertical (Z) position of the quad (and of the corrispondig chunk) in the grid of chunks
		int m_slotPosZ;	// at the current (of the quad) lod : 0 the first chunk, 1 the following to the max number of chunks on a size for the specific lod
		int m_lod;
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;
		AABB m_chunkAABB;
		float m_chunkSizeInWUs;
		bool m_splitRequired;
		bool m_justSplitted = false;
		bool m_justJoined = false;
	};

	enum class QuadrantStatus
	{
		loadTerrainDataFromCacheNeeded = -4
		,loadTerrainDataFromCacheInProgress = -3
		,refreshTerrainDataNeeded = -2
		,refreshTerrainDataInProgress = -1
		,uninitialized = 0
		,getTerrainDataInProgress = 1
		,initialized = 2
		,toErase = 3
		,initializationStopped = 4
	};

	class QuadTree
	{
	private:
		class Quads
		{
		public:
			Quads(void)
			{
			}
			~Quads(void)
			{
			}

		public:
			std::unique_ptr<Quad> m_quadRoot;
			Chunk::MapChunk m_chunks;
			std::vector<Chunk*> m_updatedChunks;
			bool m_colliderNeedReset = false;
			bool m_hitByMouse = false;
			bool m_updateForAllChunksForced = false;
		};

	public:
		enum class QuadrantDepth
		{
			NotSet = -1
			, Initial = 0		// Quadrant included in the minimum depth of quadrants
			, Growing = 1		// Quadrant acquired after minimum deploy of world 
			, Cache = 2		// Invisible quadrant which are part of the cache pool of quadrants
		};

		static std::string statusToString(enum class QuadrantStatus status);
		static std::string quadrantDepthToString(enum class QuadrantDepth depth);

		QuadTree(GDN_TheWorld_Viewer* viewer, TheWorld_Utils::QuadrantPos quadrantId);
		~QuadTree();

		static std::string quadTypeToString(QuadType type);
		void onGlobalTransformChanged(void);

		void init(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrainForced);
		void checkAndInitSeaQuadrant(void);
		void refreshTerrainData(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrain);
		enum class UpdateStage
		{
			Stage1 = 1,
			Stage2 = 2,
			Stage3 = 3
		};
		void checkMouseHitOnTerrain(void);
		void checkMouseHit(enum class QuadType type);
		void internalCheckMouseHit(Quad* quad);
		void update(Vector3 cameraPosInMap, enum class UpdateStage updateStage, int& numSplitRequired, enum class QuadType type);
		Chunk* getChunkAt(Chunk::ChunkPos pos, enum class Chunk::DirectionSlot dir, enum class QuadType type);
		Chunk* getChunkAt(Chunk::ChunkPos pos, enum class QuadType type);
		bool isChunkOnBorderOfQuadrant(Chunk::ChunkPos pos, TheWorld_Utils::QuadrantPos& XMinusQuadrantPos, TheWorld_Utils::QuadrantPos& XPlusQuadrantPos, TheWorld_Utils::QuadrantPos& ZMinusQuadrantPos, TheWorld_Utils::QuadrantPos& ZPlusQuadrantPos);
		void addChunk(Chunk* chunk, enum class QuadType type);
		void addChunkUpdate(Chunk* chunk, enum class QuadType type);
		void clearChunkUpdate(enum class QuadType type);
		std::vector<Chunk*>& getUpdatedChunks(enum class QuadType type)
		{
			return m_quads[type]->m_updatedChunks;
		}
		void ForAllChunk(Chunk::ChunkAction& chunkAction, enum class QuadType type);
		void dump(bool minimal);
		int getNumSplits(void) 
		{
			if (!isValid())
				return 0;
			if (!isVisible())
				return 0;
			return m_numSplits;
		}
		int getNumJoins(void)
		{ 
			if (!isValid())
				return 0;
			if (!isVisible())
				return 0;
			return m_numJoins;
		}
		int getNumChunks(bool onlyVisible, enum class QuadType type)
		{
			if (!isValid())
				return 0;

			if (onlyVisible && !isVisible())
				return 0;

			switch (type)
			{
			case QuadType::NotSet:
			case QuadType::NotUsed:
			{
				int num = 0;
				for (int i = 0; i < (int)QuadType::NotUsed; i++)
				{
					if (containsQuadOfType((enum class QuadType)i))
					{
						num += getNumChunks(onlyVisible, (enum class QuadType)i);
					}
				}
				return num;
			}
			break;
			default:
			{
				int num = 0;
				if (containsQuadOfType(type))
				{
					Chunk::MapChunk::iterator itMapChunk;
					for (Chunk::MapChunk::iterator itMapChunk = m_quads[type]->m_chunks.begin(); itMapChunk != m_quads[type]->m_chunks.end(); itMapChunk++)
						num += (int)itMapChunk->second.size();
				}
				return num;
			}
			break;
			}
			
		}
		int getNumActiveChunks(map<int, size_t>& mapNumActiveChunksPerLod, enum class QuadType type)
		{
			//mapNumActiveChunksPerLod.clear();

			if (!isValid())
				return 0;

			if (!isVisible())
				return 0;

			switch (type)
			{
			case QuadType::NotSet:
			case QuadType::NotUsed:
			{
				int num = 0;
				for (int i = 0; i < (int)QuadType::NotUsed; i++)
				{
					if (containsQuadOfType((enum class QuadType)i))
					{
						num += getNumActiveChunks(mapNumActiveChunksPerLod, (enum class QuadType)i);
					}
				}
				return num;
			}
			break;
			default:
			{
				int num = 0;
				if (containsQuadOfType(type))
				{
					Chunk::MapChunk::iterator itMapChunk;
					for (Chunk::MapChunk::iterator itMapChunk = m_quads[type]->m_chunks.begin(); itMapChunk != m_quads[type]->m_chunks.end(); itMapChunk++)
					{
						int numPerLod = 0;
						for (Chunk::MapChunkPerLod::iterator itMapChunkPerLod = itMapChunk->second.begin(); itMapChunkPerLod != itMapChunk->second.end(); itMapChunkPerLod++)
						{
							if (itMapChunkPerLod->second->isActive())
							{
								num++;
								numPerLod++;
							}
						}
						if (mapNumActiveChunksPerLod.contains(itMapChunk->first))
							mapNumActiveChunksPerLod[itMapChunk->first] += numPerLod;
						else
							mapNumActiveChunksPerLod[itMapChunk->first] = numPerLod;
					}
				}
				return num;
			}
			break;
			}
		}
		Quadrant* getQuadrant(void)
		{
			return m_worldQuadrant; 
		}
		Ref<Material> getCurrentMaterial(enum class QuadType type, int lod);
		void setLookDevMaterial(enum class ShaderTerrainData::LookDev lookDev, enum class QuadType type, bool reload = false);
		void setRegularMaterial(enum class QuadType type, bool reload = false);
		void reloadCurrentMaterial(enum class QuadType type);
		void setQuadrantOptimizations(godot::Vector3& worldCameraWorldPos);
		bool resetMaterialParams(bool& lastProcessExecutionInterrupted, godot::Vector3& worldCameraWorldPos, bool force = false);
		bool materialParamsNeedReset(void);
		void materialParamsNeedReset(bool b);
		bool updateCollider(enum class QuadType type);
		bool colliderNeedReset(enum class QuadType type);
		void colliderNeedReset(bool b, enum class QuadType type);
		bool updateMaterialParams(bool updateTerrainData = true);
		bool materialParamsNeedUpdate(void);
		void materialParamsNeedUpdate(bool b, bool transformChanged = false);
		bool statusInitialized(void)
		{
			return status() == QuadrantStatus::initialized;
		}
		bool statusLoadTerrainDataFromCacheInProgress(void)
		{
			return status() == QuadrantStatus::loadTerrainDataFromCacheInProgress;
		}
		bool statusLoadTerrainDataFromCacheNeeded(void)
		{
			return status() == QuadrantStatus::loadTerrainDataFromCacheNeeded;
		}
		bool statusGetTerrainDataInProgress(void)
		{
			return status() == QuadrantStatus::getTerrainDataInProgress;
		}
		bool statusRefreshTerrainDataNeeded(void)
		{
			bool b = status() == QuadrantStatus::refreshTerrainDataNeeded;
			//if (b)
			//{
			//	b = status() == QuadrantStatus::refreshTerrainDataNeeded;
			//}
			return b;
		}
		bool statusRefreshTerrainDataInProgress(void)
		{
			return status() == QuadrantStatus::refreshTerrainDataInProgress;
		}
		bool statusUninitialized(void)
		{
			return status() == QuadrantStatus::uninitialized;
		}
		bool statusToErase(void)
		{
			return status() == QuadrantStatus::toErase;
		}
		enum class QuadrantStatus currentStatus(void) const
		{
			return m_status;
		}
		void checkSeaChunksToDelete(void);
		bool statusInProgressOnServer(void) const;
		enum class QuadrantStatus status(void);
		void setStatus(enum class QuadrantStatus status);
		void setPrevStatus(void);
		void refreshSeaInfo(void);
		bool isValid(void)
		{
			//return status() == QuadrantStatus::initialized;
			return statusInitialized();
		}
		bool isRefreshingData(void)
		{
			if (status() == QuadrantStatus::loadTerrainDataFromCacheNeeded || status() == QuadrantStatus::loadTerrainDataFromCacheInProgress || status() == QuadrantStatus::refreshTerrainDataNeeded || status() == QuadrantStatus::refreshTerrainDataInProgress)
				return true;
			else
				return false;
		}
		bool isVisible(void)
		{
			if (statusToErase())
				return false;

			return m_isVisible; 
		}
		void setVisible(bool b)
		{
			if (m_isVisible != b)
			{
				m_isVisible = b;
				Collider* collider = m_worldQuadrant->getCollider();
				if (collider != nullptr)
					if (b)
						collider->enable();
					else
						collider->disable();
			}
		}
		void refreshTime(std::timespec& time)
		{
			m_refreshTime = time; 
		}
		std::timespec getRefreshTime(void)  const
		{
			return m_refreshTime; 
		}
		void setTag(string tag)
		{
			m_tag = tag;
			m_worldQuadrant->m_quadrantPos.setTag(tag);
		}
		std::string getTag(void)
		{
			return m_tag;
		}
		Transform3D getInternalGlobalTransform(void);
		GDN_TheWorld_Viewer* Viewer(void)
		{
			return m_viewer; 
		}

		GDN_TheWorld_Quadrant* getGDNQuadrant(void)
		{
			return m_GDN_Quadrant;
		}

		Chunk::MapChunk& getChunks(enum class QuadType type)
		{
			return m_quads[type]->m_chunks;
		}

		void setEditModeQuadrantSelected(bool b)
		{
			m_editModeQuadrantSelected = b;
		}

		bool editModeQuadrantSelected(void) const
		{
			return m_editModeQuadrantSelected;
		}

		enum class ShaderTerrainData::LookDev getLookDev(void) const
		{
			return m_lookDev;
		}

		void mouseHitOnTerrainChanged(godot::Vector3 mouseHit, bool hit);
		void mouseHitChanged(godot::Vector3 mouseHit, bool hit, enum class QuadType type);
		void getAdjacentQuadrants(std::list<QuadTree*>& adjacentQuadrantsHit);

		Quad* getQuadRoot(enum class QuadType type)
		{
			if (containsQuadOfType(type))
				return m_quads[type]->m_quadRoot.get();
			else
				return nullptr;
		}
		bool forceRecalcOptimizationLevel(void) const
		{
			return m_forceRecalcOptimizationLevel;
		}

		void requestInitDelayed(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrainForced)
		{
			my_assert(status() == QuadrantStatus::uninitialized && !m_initDelayedRequested);
			if (status() != QuadrantStatus::uninitialized || m_initDelayedRequested)
				throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Quadrant status not compliant: " + std::to_string((int)status())).c_str()));

			m_initDelayedRequested = true;
			m_cameraX = cameraX;
			m_cameraY = cameraY;
			m_cameraZ = cameraZ;
			m_cameraYaw = cameraYaw;
			m_cameraPitch = cameraPitch;
			m_cameraRoll = cameraRoll;
			m_setCamera = setCamera;
			m_cameraDistanceFromTerrainForced = cameraDistanceFromTerrainForced;
		}
			
		bool initDelayedRequested(void) const
		{
			return m_initDelayedRequested;
		}
			
		void initDelayed(bool force = false)
		{
			if (m_initDelayedRequested || force)
			{
				init(m_cameraX, m_cameraY, m_cameraZ, m_cameraYaw, m_cameraPitch, m_cameraRoll, m_setCamera, m_cameraDistanceFromTerrainForced);
				m_initDelayedRequested = false;
			}
		}

		float getLastComputedDistanceFromCamera(void) const
		{
			return m_lastComputedQuadrantDistanceFromCamera;
		}

		bool containsQuadOfType(enum class QuadType type) const
		{
			if (m_quads.contains(type))
				return true;
			else
				return false;
		}

		bool containsSeaChunks() const
		{
			return containsQuadOfType(QuadType::Sea);
		}

		enum class QuadrantDepth quadrantDepth(void) const
		{
			return m_quadrantDepth;
		}
		void quadrantDepth(enum class QuadrantDepth p)
		{
			m_quadrantDepth = p;
		}
		size_t numQuadrantsFromCameraQuadrant(void) const
		{
			return m_numQuadrantsFromCameraQuadrant;
		}
		void numQuadrantsFromCameraQuadrant(size_t n)
		{
			m_numQuadrantsFromCameraQuadrant = (int)n;
		}

		void autoSaveRequired(bool b)
		{
			m_autoSaveRequired = b;
		}
		bool autoSaveRequired(void) const
		{
			return m_autoSaveRequired;
		}
		int canContainSea(void) const
		{
			return m_canContainSea;
		}
		void canContainSea(int i)
		{
			m_canContainSea = i;
		}

		bool updateForAllChunksForced(enum class QuadType type)
		{
			if (containsQuadOfType(type))
			{
				return m_quads[type]->m_updateForAllChunksForced;
			}
			else
				return false;
		}
		void updateForAllChunksForced(enum class QuadType type, bool b)
		{
			if (containsQuadOfType(type))
			{
				m_quads[type]->m_updateForAllChunksForced = b;
			}
		}
		void forceUpdateForAllChunks(enum class QuadType type);

		bool quadrantReady(void) const
		{
			return m_quadrantReady;
		}
		void quadrantReady(bool b)
		{
			m_quadrantReady = b;
		}

	private:
		void internalUpdate(Vector3 cameraPosInMap, Quad* quadTreeNode, enum class UpdateStage updateStage, int& numSplitRequired, enum class QuadType type);

	private:
		enum class QuadrantStatus m_status;
		bool m_quadrantReady = false;
		enum class QuadrantStatus m_prevStatus = QuadrantStatus::uninitialized;
		TheWorld_Viewer_Utils::MsTimePoint m_lastStatusChange;
		bool m_isVisible;
		std::string m_tag;
		GDN_TheWorld_Viewer* m_viewer;
		Quadrant* m_worldQuadrant;
		std::timespec m_refreshTime;
		GDN_TheWorld_Quadrant* m_GDN_Quadrant;
		bool m_editModeQuadrantSelected = false;
		enum class ShaderTerrainData::LookDev m_lookDev;
		int m_canContainSea = QUADRANT_SEA_INFO_NOTSET;
		QuadrantDepth m_quadrantDepth = QuadrantDepth::NotSet;
		int m_numQuadrantsFromCameraQuadrant = -1;

		std::map<enum class QuadType, std::unique_ptr<Quads>> m_quads;

		float m_lastComputedQuadrantDistanceFromCamera = 0;
		bool m_forceRecalcOptimizationLevel = false;

		// Terrain Quads/Chunks
		//std::unique_ptr<Quad> m_terrainQuadRoot;
		//Chunk::MapChunk m_mapTerrainChunk;
		//std::vector<Chunk*> m_vectTerrainChunkUpdate;
		//bool m_colliderNeedReset = false;
		//bool m_hitByMouse = false;

		// Sea
		//std::unique_ptr<Quad> m_seaQuadRoot;
		//Chunk::MapChunk m_seaMapChunk;
		//std::vector<Chunk*> m_vectSeaChunkUpdate;

		// Statistics
		int m_numSplits;
		int m_numJoins;
		int m_numLeaf;

		// init delayed
		bool m_initDelayedRequested = false;
		float m_cameraX = 0.0f;
		float m_cameraY = 0.0f;
		float m_cameraZ = 0.0f;
		float m_cameraYaw = 0.0f;
		float m_cameraPitch = 0.0f;
		float m_cameraRoll = 0.0f;
		bool m_setCamera = false;
		float m_cameraDistanceFromTerrainForced = 0.0f;

		bool m_autoSaveRequired = true;
	};
}
