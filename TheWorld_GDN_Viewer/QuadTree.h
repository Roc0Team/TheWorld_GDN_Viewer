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
	//class QuadTree;
	//class TerrainEdit;

	/*
	class QuadrantPos
	{
	public:
		enum class DirectionSlot
		{
			Center = -1

			//
			// - o
			//
			, XMinus = 0
			, WestXMinus = 0

			//
			//   o -
			//
			, XPlus = 1
			, EastXPlus = 1

			//   -
			//   o
			//
			, ZMinus = 2
			, NorthZMinus = 2

			//
			//   o
			//   -
			, ZPlus = 3
			, SouthZPlus = 3

			// - 
			//   o
			//   
			, ZMinusXMinus = 4
			, NorthwestZMinusXMinus = 4

			//     - 
			//   o
			//   
			, ZMinusXPlus = 5
			, NortheastZMinusXPlus = 5

			//     
			//   o
			// -    
			, ZPlusXMinus = 6
			, SouthwestZPlusXMinus = 6

			//      
			//   o
			//     -
			, ZPlusXPlus = 7
			, SoutheastZPlusXPlus = 7
		};

		QuadrantPos()
		{
			reset();
			//m_lowerXGridVertex = m_lowerZGridVertex = m_gridStepInWU = 0;
			//m_numVerticesPerSize = m_level = 0;
			//m_sizeInWU = 0;
			//m_initialized = false;
		}

		void reset(void)
		{
			m_lowerXGridVertex = m_lowerZGridVertex = m_gridStepInWU = 0;
			m_numVerticesPerSize = m_level = 0;
			m_sizeInWU = 0;
			m_tag.clear();
			m_name.clear();
			m_initialized = false;
		}

		bool empty(void) const
		{
			return m_initialized == false;
		}

		QuadrantPos(const QuadrantPos& quadrantPos)
		{
			*this = quadrantPos;
			//m_lowerXGridVertex = quadrantPos.m_lowerXGridVertex;
			//m_lowerZGridVertex = quadrantPos.m_lowerZGridVertex;
			//m_numVerticesPerSize = quadrantPos.m_numVerticesPerSize;
			//m_level = quadrantPos.m_level;
			//m_gridStepInWU = quadrantPos.m_gridStepInWU;
			//m_sizeInWU = quadrantPos.m_sizeInWU;
			//m_tag = quadrantPos.m_tag;
			//m_name = quadrantPos.m_name;
			//m_initialized = true;
		}

		// x, z pos a point contained in the quadrant in global coord: calculating starting point of the quadrant in global coord
		QuadrantPos(float x, float z, int level, int numVerticesPerSize, float gridStepInWU)
		{
			float gridSizeInWU = (numVerticesPerSize - 1) * gridStepInWU;
			//float f = x / gridSizeInWU;
			//f = floor(f);
			//f = f * gridSizeInWU;
			m_lowerXGridVertex = floor(x / gridSizeInWU) * gridSizeInWU;
			m_lowerZGridVertex = floor(z / gridSizeInWU) * gridSizeInWU;
			m_numVerticesPerSize = numVerticesPerSize;
			m_level = level;
			m_gridStepInWU = gridStepInWU;
			m_sizeInWU = (m_numVerticesPerSize - 1) * m_gridStepInWU;
			m_initialized = true;
			resetName();
		}

		QuadrantPos(int x, int z, int level, int numVerticesPerSize, float gridStepInWU)
		{
			float gridSizeInWU = (numVerticesPerSize - 1) * gridStepInWU;
			m_lowerXGridVertex = x * gridSizeInWU;
			m_lowerZGridVertex = z * gridSizeInWU;
			m_numVerticesPerSize = numVerticesPerSize;
			m_level = level;
			m_gridStepInWU = gridStepInWU;
			m_sizeInWU = (m_numVerticesPerSize - 1) * m_gridStepInWU;
			m_initialized = true;
			resetName();
		}

		bool operator<(const QuadrantPos& quadrantPos) const
		{
			assert(m_initialized == true);
			assert(quadrantPos.m_initialized == true);
			assert(m_level == quadrantPos.m_level);
			if (m_level < quadrantPos.m_level)
				return true;
			if (m_level > quadrantPos.m_level)
				return false;
			// m_level == quadrantPos.m_level

			assert(m_sizeInWU == quadrantPos.m_sizeInWU);
			if (m_sizeInWU < quadrantPos.m_sizeInWU)
				return true;
			if (m_sizeInWU > quadrantPos.m_sizeInWU)
				return false;
			// m_sizeInWU == quadrantPos.m_sizeInWU

			if (m_lowerZGridVertex < quadrantPos.m_lowerZGridVertex)
				return true;
			if (m_lowerZGridVertex > quadrantPos.m_lowerZGridVertex)
				return false;
			// m_lowerZGridVertex == quadrantPos.m_lowerZGridVertex

			return m_lowerXGridVertex < quadrantPos.m_lowerXGridVertex;
		}

		bool operator==(const QuadrantPos& quadrantPos) const
		{
			if (m_lowerXGridVertex == quadrantPos.m_lowerXGridVertex
				&& m_lowerZGridVertex == quadrantPos.m_lowerZGridVertex
				&& m_numVerticesPerSize == quadrantPos.m_numVerticesPerSize
				&& m_level == quadrantPos.m_level
				&& m_gridStepInWU == quadrantPos.m_gridStepInWU)
				return true;
			else
				return false;
		}

		void operator=(const QuadrantPos& quadrantPos)
		{
			m_lowerXGridVertex = quadrantPos.m_lowerXGridVertex;
			m_lowerZGridVertex = quadrantPos.m_lowerZGridVertex;
			m_numVerticesPerSize = quadrantPos.m_numVerticesPerSize;
			m_level = quadrantPos.m_level;
			m_gridStepInWU = quadrantPos.m_gridStepInWU;
			m_sizeInWU = quadrantPos.m_sizeInWU;
			m_tag = quadrantPos.m_tag;
			m_name = quadrantPos.m_name;
			//m_initialized = true;
			m_initialized = quadrantPos.m_initialized;
			//return *this;
		}

		std::string getIdStr(void)
		{
			return "ST" + to_string(m_gridStepInWU) + "_SZ" + to_string(m_numVerticesPerSize) + "_L" + to_string(m_level) + "_X" + to_string(m_lowerXGridVertex) + "_Z" + to_string(m_lowerZGridVertex);
		}

		float getLowerXGridVertex() const
		{
			return m_lowerXGridVertex; 
		};
		float getLowerZGridVertex() const
		{
			return m_lowerZGridVertex; 
		};
		int getNumVerticesPerSize() const
		{
			return m_numVerticesPerSize;
		};
		int getLevel() const
		{
			return m_level;
		};
		float getGridStepInWU()  const
		{
			return m_gridStepInWU; 
		};
		float getSizeInWU() const
		{
			return m_sizeInWU; 
		};
		int getIndexPosX(void) const
		{
			float gridSizeInWU = (m_numVerticesPerSize - 1) * m_gridStepInWU;
			return (int)(m_lowerXGridVertex / gridSizeInWU);
		}
		int getIndexPosZ(void) const
		{
			float gridSizeInWU = (m_numVerticesPerSize - 1) * m_gridStepInWU;
			return (int)(m_lowerZGridVertex / gridSizeInWU);
		}
		_declspec(dllexport) QuadrantPos getQuadrantPos(enum class DirectionSlot dir, int numSlot = 1);
		void setTag(std::string tag)
		{
			m_tag = tag;
		}
		std::string getTag(void)
		{
			return m_tag;
		}
		_declspec(dllexport) size_t distanceInPerimeter(QuadrantPos& q) const;
		bool isInitialized(void) const
		{
			return m_initialized;
		}

		std::string getName(void)
		{
			return m_name;
		}

	private:
		void resetName(void)
		{
			float gridSizeInWU = (m_numVerticesPerSize - 1) * m_gridStepInWU;
			//m_name = "Quadrant-" + std::to_string((int)(m_lowerXGridVertex / gridSizeInWU)) + "-" + std::to_string((int)(m_lowerZGridVertex / gridSizeInWU));
			m_name = "Quadrant " + std::to_string((int)(m_lowerXGridVertex / gridSizeInWU)) + "," + std::to_string((int)(m_lowerZGridVertex / gridSizeInWU));
		}

	private:
		// ID
		float m_lowerXGridVertex;	// starting point in the X axis of the quadrant in global coord
		float m_lowerZGridVertex;	// starting point in the Z axis of the quadrant in global coord
		int m_numVerticesPerSize;
		int m_level;
		float m_gridStepInWU;
		// ID

		bool m_initialized;

		float m_sizeInWU;
		std::string m_tag;
		std::string m_name;
	};*/

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
			godot::Vector4 groundTextureLayers(void) const
			{
				return m_groundTextureLayers;
			}
			void groundTextureLayers(godot::Vector4 layers)
			{
				m_groundTextureLayers = layers;
				initialized(true);
				shaderUpdated(false);
			}
		private:
			bool m_initialized = false;
			godot::Vector4 m_groundTextureLayers;
			bool m_shaderUpdated = false;
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

		TheWorld_Utils::QuadrantPos getPos(void)
		{
			return m_quadrantPos;
		}

		bool quadrantAtMinimumLod(void);

		//enum class MeshResolutionLevel getMeshResolutionLevel(void);
		//enum class TexResolutionLevel getTexResolutionLevel(void);

		//void clearCachedTexture(void);

		bool isFlushed(void);

		bool hasHeigthsForChunkSplitsAndShaderRendering(void);
		bool needReloadTerrainDataFromCache(void);

		void lockInternalDataUntilCompleteProcessExecution(bool b = true);
		bool internalDataLockedUntilCompleteProcessExecution(void) const;

		size_t getLastDistanceInPerimeterFromCameraQuadrant(void) const
		{
			return m_lastDistanceInPerimeterFromCameraQuadrant;
		}
		void setLastDistanceInPerimeterFromCameraQuadrant(size_t value)
		{
			m_lastDistanceInPerimeterFromCameraQuadrant = value;
		}

		void setMeshId(std::string meshId);

		void refreshGridVerticesFromServer(std::string& bufferFromServer, std::string meshIdFromServer, std::string& outMeshIdFromBuffer, bool updateCache);

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
				if (!getTerrainEdit()->extraValues.splatmapNeedRegen)
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
				if (!getTerrainEdit()->extraValues.colormapNeedRegen)
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
				if (!getTerrainEdit()->extraValues.globalmapNeedRegen)
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
			getTerrainEdit()->extraValues.splatmapNeedRegen = true;
			setSplatmapUpdated(true);
			
			resetGlobalmapBuffer(actor);
		}

		void resetColormapBuffer(std::string actor)
		{
			getColormapBuffer(false).clear(actor);
			getTerrainEdit()->extraValues.colormapNeedRegen = true;
			setColorsUpdated(true);
		}

		void resetGlobalmapBuffer(std::string actor)
		{
			getGlobalmapBuffer(false).clear(actor);
			getTerrainEdit()->extraValues.globalmapNeedRegen = true;
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

		bool lockInternalData(void)
		{
			bool lockAcquired = false;
			{
				std::lock_guard<std::recursive_mutex> lock(m_mtxInternalData);
				if (!m_internalDataLocked)
				{
					m_internalDataLocked = true;
					lockAcquired = true;
				}
			}
			return lockAcquired;
		}

		void unlockInternalData(void)
		{
			std::lock_guard<std::recursive_mutex> lock(m_mtxInternalData);
			if (m_internalDataLocked)
				m_internalDataLocked = false;
			else
				m_internalDataLocked = false;
		}

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
		void setEmpty(bool b)
		{
			m_empty = b;
		}
		bool empty(void) const
		{
			return m_empty;
		}

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

		void process_mapModder(double _delta);

		GDN_TheWorld_MapModder* getMapModder(void)
		{
			return m_mapModder;
		}

		void getQuadrantBufferForCache(TheWorld_Utils::MemoryBuffer& buffer);
		void getQuadrantBufferForCache(std::string& buffer);
		void saveQuadrantToDiskCache(TheWorld_Utils::MemoryBuffer& buffer, bool needUploadToserver = true, bool newMeshId = false);
		void saveQuadrantToDiskCache(bool newMeshId = false);

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

	private:
		TheWorld_Utils::MeshCacheBuffer& getMeshCacheBuffer(void);
		void loadTerrainDataFromCache(void);

	private:
		TheWorld_Utils::QuadrantPos m_quadrantPos;
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;
		bool m_lockInternalDataUntilCompleteProcessExecution = false;
		TheWorld_Utils::MeshCacheBuffer m_cache;
		godot::AABB m_globalCoordAABB;
		std::unique_ptr<Collider> m_collider;
		std::unique_ptr<ShaderTerrainData> m_shaderTerrainData;
		bool m_needUploadToServer;
		bool m_internalDataLocked;
		std::recursive_mutex m_mtxInternalData;
		bool m_empty;

		GDN_TheWorld_MapModder* m_mapModder = nullptr;

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
#ifdef REAL_T_IS_DOUBLE
		godot::PackedFloat64Array m_heightsForCollider;
#else
		godot::PackedFloat32Array m_heightsForCollider;
#endif // REAL_T_IS_DOUBLE
		// Terrain Values

		NeighbourQuadrantData m_northQuadrantData;
		NeighbourQuadrantData m_southQuadrantData;
		NeighbourQuadrantData m_westQuadrantData;
		NeighbourQuadrantData m_eastQuadrantData;

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
		size_t m_optimizationLevel = 0;
		bool m_optimizationLevelChanged = false;

		size_t m_lastDistanceInPerimeterFromCameraQuadrant = -1;

		bool m_executingProcess = false;
		bool m_terrainDataJustLoaded = false;
	};

	class ShaderTerrainData
	{
	public:
		class GroundTexture
		{
		public:
			GroundTexture()
			{
				//m_albedo_bump_tex = nullptr;
				//m_normal_roughness_tex = nullptr;
				initialized = false;
			}

		public:
			bool initialized;
			godot::Ref<godot::ImageTexture> m_albedo_bump_tex;
			godot::Ref<godot::ImageTexture> m_normal_roughness_tex;
			godot::Ref<godot::Texture2D> m_tex;
		};

		// Shader Params
#define SHADER_PARAM_TERRAIN_HEIGHTMAP				"u_terrain_heightmap"
#define SHADER_PARAM_TERRAIN_NORMALMAP				"u_terrain_normalmap"
#define SHADER_PARAM_TERRAIN_SPLATMAP				"u_terrain_splatmap"
#define SHADER_PARAM_TERRAIN_COLORMAP				"u_terrain_colormap"
#define SHADER_PARAM_TERRAIN_GLOBALMAP				"u_terrain_globalmap"

#define SHADER_PARAM_INVERSE_TRANSFORM				"u_terrain_inverse_transform"
#define SHADER_PARAM_NORMAL_BASIS					"u_terrain_normal_basis"
#define SHADER_PARAM_GRID_STEP						"u_grid_step"
#define SHADER_PARAM_FORCE_ELEVATION_0				"u_force_height_0"
#define SHADER_PARAM_USE_GLOBALMAP					"u_use_globalmap"
#define SHADER_PARAM_OPTIMIZATION_LEVEL				"u_optimization_level"

#define GLOBAL_SHADER_PARAM_GRID_STEP				"gu_grid_step"
#define GLOBAL_SHADER_PARAM_ALBEDO_BUMP_ARRAY		"gu_ground_albedo_bump_array"
#define GLOBAL_SHADER_PARAM_NORMAL_ROUGHNESS_ARRAY	"gu_ground_normal_roughness_array"
#define GLOBAL_SHADER_PARAM_CAM_ALTITUDE			"gu_camera_altitude_from_terrain"
#define GLOBAL_SHADER_PARAM_CAM_POS					"gu_camera_pos"
//#define GLOBAL_SHADER_PARAM_INVERSE_TRANSFORM		"gu_terrain_inverse_transform"
//#define GLOBAL_SHADER_PARAM_TEXTURE_BLENDING_SIZE	"gu_texture_blending_size"				// actual value must be set in internal member variable GDN_TheWorld_Globals::m_textureBlendingSize to avoid reading it from RenderingServer (slow operation)
//#define GLOBAL_SHADER_PARAM_TEXTURE_BLENDING_SIZE	"gu_splatmap_blending_size"				// actual value must be set in internal member variable GDN_TheWorld_Globals::m_splatmapBlendingSize to avoid reading it from RenderingServer (slow operation)


#define SHADER_PARAM_GROUND_LAYERS					"u_ground_layers"	// godot::Plane (vec4) ==> normal_x (vec4 x) = low, normal_y (vec4 y) = high, normal_z (vec4 z) = dirt, d (vec4 w) = rock
#define SHADER_PARAM_NORTH_GROUND_LAYERS			"u_north_ground_layers"
#define SHADER_PARAM_SOUTH_GROUND_LAYERS			"u_south_ground_layers"
#define SHADER_PARAM_EAST_GROUND_LAYERS				"u_east_ground_layers"
#define SHADER_PARAM_WEST_GROUND_LAYERS				"u_west_ground_layers"

#define SHADER_PARAM_EDITMODE_SELECTED				"u_editmode_selected"
#define SHADER_PARAM_MOUSE_HIT						"u_mouse_hit"
#define SHADER_PARAM_MOUSE_HIT_RADIUS				"u_mouse_hit_radius"
#define SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS		"u_quad_border_line_thickness"

#define SHADER_PARAM_MOUSE_GRID_ON					"u_grid_on"

#define SHADER_PARAM_VERTICES_PER_CHUNK				"u_num_vertices_per_chunk"
#define SHADER_PARAM_LOOKDEV_LOD					"u_lod"
#define	SHADER_PARAM_LOD_GRID_STEP					"u_lod_grid_step"
#define	SHADER_PARAM_LOD_CHUNK_SIZE					"u_lod_chunk_size"

#define SHADER_PARAM_GROUND_ALBEDO_BUMP_0			"u_ground_albedo_bump_0"
#define SHADER_PARAM_GROUND_ALBEDO_BUMP_1			"u_ground_albedo_bump_1"
#define SHADER_PARAM_GROUND_ALBEDO_BUMP_2			"u_ground_albedo_bump_2"
#define SHADER_PARAM_GROUND_ALBEDO_BUMP_3			"u_ground_albedo_bump_3"

#define SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_0		"u_ground_normal_roughness_0"
#define SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_1		"u_ground_normal_roughness_1"
#define SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_2		"u_ground_normal_roughness_2"
#define SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_3		"u_ground_normal_roughness_3"

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
		static godot::Ref<godot::Image> readGroundTexture(godot::String fileName, bool& ok, GDN_TheWorld_Viewer* viewer = nullptr);
		static void getGroundTexture(godot::String fileName, GroundTexture* groundTexture, GDN_TheWorld_Viewer* viewer = nullptr);
		bool materialParamsNeedReset(void) const
		{
			return m_materialParamsNeedReset;
		}
		void materialParamsNeedReset(bool b)
		{
			m_materialParamsNeedReset = b;
		}
		void resetMaterialParams(bool& lastProcessExecutionInterrupted, LookDev lookdev);
		bool materialParamsNeedUpdate(void) const
		{
			return m_materialParamsNeedUpdate;
		}
		void materialParamsNeedUpdate(bool b)
		{
			m_materialParamsNeedUpdate = b;
		}
		void updateMaterialParams(LookDev lookdev);
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
		godot::Ref<godot::Material> getRegularMaterial(bool reload = false);
		void releaseRegularMaterial(void);
		godot::Ref<godot::Material> getLookDevMaterial(enum class ShaderTerrainData::LookDev lookDev, bool reload = false);
		void releaseLookDevMaterial(void);
		std::map<int, godot::Ref<godot::ShaderMaterial>>& getLookDevChunkLodMaterials(void);
		void releaseLookDevChunkLodMaterials(void);
		godot::Ref<godot::Material> getLookDevChunkLodMaterial(int lod, bool reload = false);
		static void freeLookDevSubViewport(void);
		static void releaseGlobals(void);
		static void getGroundTextures(godot::String dirName, GDN_TheWorld_Viewer* viewer = nullptr);
		static std::map<std::string, std::unique_ptr<GroundTexture>>& getGroundTextures(void)
		{
			return s_groundTextures;
		}
		static bool groundTexturesNeedProcessing(void)
		{
			return s_groundTexturesNeedProcessing;
		}
		static void groundTexturesNeedProcessing(bool b)
		{
			s_groundTexturesNeedProcessing = b;
		}
		static std::map<std::string, int32_t>& getGroundTextureLayers(void)
		{
			return s_groundTextureLayers;
		}
		godot::Ref<godot::ImageTexture> getAlbedoBumpTexture(std::string textureName);
		godot::Ref<godot::ImageTexture> getNormalRoughnessTexture(std::string textureName);

		void mouseHitChanged(godot::Vector2 mouseHit);

		godot::Ref<godot::Image> getOrCreateHeightmapImage();
		static godot::Ref<godot::Image> createHeightmapImage(TheWorld_Utils::MemoryBuffer& heightmapBuffer, int _resolution);
		godot::Ref<godot::Image> getOrCreateNormalmapImage();
		static godot::Ref<godot::Image> createNormalmapImage(TheWorld_Utils::MemoryBuffer& normalmapBuffer, int _resolution);
		godot::Ref<godot::Image> getOrCreateSplatmapImage();
		static godot::Ref<godot::Image> createSplatlmapImage(TheWorld_Utils::MemoryBuffer& splatmapBuffer, int _resolution);
		godot::Ref<godot::Image> getOrCreateGlobalmapImage();
		static godot::Ref<godot::Image> createGloablmapImage(TheWorld_Utils::MemoryBuffer& globalmapBuffer, int _resolution);
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

	private:
		//void debugPrintTexture(std::string tex_name, Ref<Texture> tex);
		//Color encodeNormal(Vector3 normal);

	private:
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;

		godot::Ref<godot::ShaderMaterial> m_regularMaterial;
		godot::Ref<godot::ShaderMaterial> m_lookDevMaterial;
		//static godot::SubViewport* g_lookDevSubviewport;
		static GDN_TheWorld_ShaderTexture* g_lookDevShaderTexture;
		std::map<int, godot::Ref<godot::ShaderMaterial>> m_lookDevChunkLodMaterials;
		bool m_materialParamsNeedUpdate;
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

		static std::map<std::string, std::unique_ptr<GroundTexture>> s_groundTextures;
		static bool s_groundTexturesNeedProcessing;
		//static std::recursive_mutex s_groundTexturesMutex;
		static std::map<std::string, int32_t> s_groundTextureLayers;

		bool m_sendTextureToShader = false;
	};
	
	class Quad
	{
	public:

		Quad(int slotPosX, int slotPosZ, int lod, enum PosInQuad posInQuad, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree);
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
		void setCameraPos(Vector3 globalCoordCameraLastPos);
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
	public:
		QuadTree(GDN_TheWorld_Viewer* viewer, TheWorld_Utils::QuadrantPos quadrantId);
		~QuadTree();

		void onGlobalTransformChanged(void);

		void init(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrainForced);
		void refreshTerrainData(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrain);
		enum class UpdateStage
		{
			Stage1 = 1,
			Stage2 = 2,
			Stage3 = 3
		};
		void checkMouseHit();
		void internalCheckMouseHit(Quad* quad);
		void update(Vector3 cameraPosGlobalCoord, enum class UpdateStage updateStage, int& numSplitRequired);
		Chunk* getChunkAt(Chunk::ChunkPos pos, enum class Chunk::DirectionSlot dir);
		Chunk* getChunkAt(Chunk::ChunkPos pos);
		bool isChunkOnBorderOfQuadrant(Chunk::ChunkPos pos, TheWorld_Utils::QuadrantPos& XMinusQuadrantPos, TheWorld_Utils::QuadrantPos& XPlusQuadrantPos, TheWorld_Utils::QuadrantPos& ZMinusQuadrantPos, TheWorld_Utils::QuadrantPos& ZPlusQuadrantPos);
		void addChunk(Chunk* chunk);
		void addChunkUpdate(Chunk* chunk);
		void clearChunkUpdate(void);
		std::vector<Chunk*>& getChunkUpdate(void)
		{
			return m_vectChunkUpdate; 
		}
		void ForAllChunk(Chunk::ChunkAction& chunkAction);
		void dump(void);
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
		int getNumChunks(bool onlyVisible)
		{
			if (!isValid())
				return 0;

			if (onlyVisible && !isVisible())
				return 0;

			int num = 0;
			//for (int i = 0; i < (int)m_mapChunk.size(); i++)
			//	num += (int)m_mapChunk[i].size();
			Chunk::MapChunk::iterator itMapChunk;
			for (Chunk::MapChunk::iterator itMapChunk = m_mapChunk.begin(); itMapChunk != m_mapChunk.end(); itMapChunk++)
				num += (int)itMapChunk->second.size();
			return num;
		}
		//int getNumChunks(bool onlyVisible, map<int, size_t>& mapNumVisibleChunksPerLod)
		//{
		//	mapNumVisibleChunksPerLod.clear();

		//	if (!isValid())
		//		return 0;

		//	if (onlyVisible && !isVisible())
		//		return 0;

		//	int num = 0;
		//	//for (int i = 0; i < (int)m_mapChunk.size(); i++)
		//	//	num += (int)m_mapChunk[i].size();
		//	Chunk::MapChunk::iterator itMapChunk;
		//	for (Chunk::MapChunk::iterator itMapChunk = m_mapChunk.begin(); itMapChunk != m_mapChunk.end(); itMapChunk++)
		//	{
		//		mapNumVisibleChunksPerLod[itMapChunk->first] = itMapChunk->second.size();
		//		num += (int)itMapChunk->second.size();
		//	}
		//	return num;
		//}
		int getNumActiveChunks(map<int, size_t>& mapNumActiveChunksPerLod)
		{
			mapNumActiveChunksPerLod.clear();

			if (!isValid())
				return 0;

			if (!isVisible())
				return 0;

			int num = 0;
			Chunk::MapChunk::iterator itMapChunk;
			for (Chunk::MapChunk::iterator itMapChunk = m_mapChunk.begin(); itMapChunk != m_mapChunk.end(); itMapChunk++)
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
				mapNumActiveChunksPerLod[itMapChunk->first] = numPerLod;
			}
			return num;
		}
		Quadrant* getQuadrant(void)
		{
			return m_worldQuadrant; 
		}
		void setLookDevMaterial(enum class ShaderTerrainData::LookDev lookDev, bool reload = false);
		void setRegularMaterial(bool reload = false);
		Ref<Material> getCurrentMaterial(int lod);
		void reloadCurrentMaterial(void);
		void setOptimizations(godot::Vector3& cameraPosGlobalCoord);
		bool resetMaterialParams(bool& lastProcessExecutionInterrupted, bool force = false);
		bool materialParamsNeedReset(void);
		void materialParamsNeedReset(bool b);
		bool updateCollider(void);
		bool colliderNeedReset(void) const;
		void colliderNeedReset(bool b);
		bool updateMaterialParams(void);
		bool materialParamsNeedUpdate(void);
		void materialParamsNeedUpdate(bool b);
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
			if (b)
			{
				b = status() == QuadrantStatus::refreshTerrainDataNeeded;
			}
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
		enum class QuadrantStatus status(void)
		{
			if (m_status == QuadrantStatus::getTerrainDataInProgress)
			{
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				long long elapsedFromStatusChange = (now - m_lastStatusChange).count();
				if (elapsedFromStatusChange > (THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE * 2))
					m_status = QuadrantStatus::uninitialized;
			}
			return m_status;
		}
		void setStatus(enum class QuadrantStatus status)
		{
			if (status == QuadrantStatus::loadTerrainDataFromCacheNeeded)
				m_status = status;

			m_lastStatusChange = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
			m_status = status;
			Quadrant* quadrant = getQuadrant();
			if (quadrant != nullptr)
				quadrant->lockInternalDataUntilCompleteProcessExecution();
		}
		bool isValid(void)
		{
			return status() == QuadrantStatus::initialized;
		}
		bool isVisible(void)
		{
			if (statusToErase())
				return false;

			return m_isVisible; 
		}
		void setVisible(bool b)
		{
			m_isVisible = b; 
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

		Chunk::MapChunk& getMapChunk(void)
		{
			return m_mapChunk;
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

		void mouseHitChanged(godot::Vector3 mouseHit, bool hit);
		void getAdjacentQuadrants(std::list<QuadTree*>& adjacentQuadrantsHit);

		Quad* getRoot(void)
		{
			return m_root.get();
		}
		bool forceRecalcOptimizationLevel(void) const
		{
			return m_forceRecalcOptimizationLevel;
		}

		void requestInitDelayed(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrainForced)
		{
			my_assert(status() == QuadrantStatus::uninitialized && !m_initDelayed);
			if (status() != QuadrantStatus::uninitialized || m_initDelayed)
				throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Quadrant status not compliant: " + std::to_string((int)status())).c_str()));

			m_initDelayed = true;
			m_cameraX = cameraX;
			m_cameraY = cameraY;
			m_cameraZ = cameraZ;
			m_cameraYaw = cameraYaw;
			m_cameraPitch = cameraPitch;
			m_cameraRoll = cameraRoll;
			m_setCamera = setCamera;
			m_cameraDistanceFromTerrainForced = cameraDistanceFromTerrainForced;
		}
			
		void initDelayed(void)
		{
			if (m_initDelayed)
			{
				init(m_cameraX, m_cameraY, m_cameraZ, m_cameraYaw, m_cameraPitch, m_cameraRoll, m_setCamera, m_cameraDistanceFromTerrainForced);
				m_initDelayed = false;
			}
		}


	private:
		void internalUpdate(Vector3 cameraPosGlobalCoord, Quad* quadTreeNode, enum class UpdateStage updateStage, int& numSplitRequired);

	private:
		enum class QuadrantStatus m_status;
		TheWorld_Viewer_Utils::MsTimePoint m_lastStatusChange;
		bool m_isVisible;
		std::string m_tag;
		std::unique_ptr<Quad> m_root;
		GDN_TheWorld_Viewer* m_viewer;
		Chunk::MapChunk m_mapChunk;
		std::vector<Chunk*> m_vectChunkUpdate;
		Quadrant* m_worldQuadrant;
		std::timespec m_refreshTime;
		GDN_TheWorld_Quadrant* m_GDN_Quadrant;
		bool m_editModeQuadrantSelected = false;
		bool m_hitByMouse = false;
		enum class ShaderTerrainData::LookDev m_lookDev;
		bool m_colliderNeedReset = false;
		//godot::Vector3 m_quadCenterGlobal;
		//float m_splitDistance = 0;
		float m_lastComputedDistanceFromCamera = 0;
		bool m_forceRecalcOptimizationLevel = false;

		// Statistics
		int m_numSplits;
		int m_numJoins;
		int m_numLeaf;

		// init delayed
		bool m_initDelayed = false;
		float m_cameraX = 0.0f;
		float m_cameraY = 0.0f;
		float m_cameraZ = 0.0f;
		float m_cameraYaw = 0.0f;
		float m_cameraPitch = 0.0f;
		float m_cameraRoll = 0.0f;
		bool m_setCamera = false;
		float m_cameraDistanceFromTerrainForced = 0.0f;
	};
}
