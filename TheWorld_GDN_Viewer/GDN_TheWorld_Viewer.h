#pragma once

#pragma warning(push, 0)
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/texture.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/mesh_instance2d.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/sub_viewport_container.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/container.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/progress_bar.hpp>
#include <godot_cpp/classes/color_rect.hpp>
//#include <godot_cpp/classes/dictionary.hpp>
#pragma warning(pop)

#include <map>
#include <memory>
#include <thread>
#include <filesystem>

#include "GDN_TheWorld_Globals.h"
#include "QuadTree.h"
//#include <MapManager.h>

#define MAX_SETTABLE_DEPTH_ON_PERIMETER	25
#define MAX_SETTABLE_CACHE_ON_PERIMETER	5

#define ADD_PROCESS_STEP(qualifier) addProcessStep(ProcessStep::ProcessStepIdx::##qualifier)

#define CHECK_PROCESS_STEP(qualifier) \
	if (m_lastProcessExecutionInterrupted or processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::##qualifier)) \
	{ \
		m_lastProcessExecutionInterrupted = true; \
		return; \
	}

#define PROCESS_STEP(func, profiler_function, profiler_name) \
	{ \
		TheWorld_Utils::GuardProfiler profiler(std::string(profiler_function) + __FUNCTION__, profiler_name); \
		func; \
	}

#define PROCESS_STEP_WITH_CHECK(func, qualifier, profiler_function, profiler_name) \
	{ \
		PROCESS_STEP(func, profiler_function, profiler_name) \
		CHECK_PROCESS_STEP(qualifier) \
	}

namespace godot
{
	class MeshCache;
	class QuadTree;
	class MapQuadrant;
	class Chunk;
	class GDN_TheWorld_Globals;
	class GDN_TheWorld_Camera;
	class GDN_TheWorld_Edit;
	class GDN_TheWorld_Drawer;
	class GDN_TheWorld_Gizmo3d;

	//typedef std::map<TheWorld_Utils::QuadrantPos, std::unique_ptr<QuadTree>> WorldQuadrantsMap;
	//typedef std::map<TheWorld_Utils::QuadrantPos, std::unique_ptr<MapQuadrant>> MapQuadrantsMap;

	class WorldQuadrantsMap : public std::map<TheWorld_Utils::QuadrantPos, std::unique_ptr<QuadTree>>
	{
	public:
		void init(GDN_TheWorld_Viewer* viewer);

	private:
		GDN_TheWorld_Viewer* m_viewer = nullptr;
	};

	class MapQuadrantImagesMap : public std::map<TheWorld_Utils::QuadrantPos, std::unique_ptr<MapQuadrantImage>>
	{
	public:
		enum class UpdateStatus
		{
			UpdateNeeded = 0
			, UpdateInProgress = 1
		};

		class UpdatingInfo
		{
		public:
			UpdatingInfo(void)
			{
				m_status = UpdateStatus::UpdateNeeded;
				m_lastStatusChangeTime = TIME()->get_ticks_msec();
			}
			~UpdatingInfo(void) {}

			void status(enum class UpdateStatus status)
			{
				if (m_status != status)
				{
					m_status = status;
					m_lastStatusChangeTime = TIME()->get_ticks_msec();
				}
			}
			enum class UpdateStatus status(void) const
			{
				return m_status;
			}

			uint64_t elapsedFromLastStatusChange(void) const
			{
				return TIME()->get_ticks_msec() - m_lastStatusChangeTime;
			}

		private:
			enum class UpdateStatus m_status = UpdateStatus::UpdateNeeded;
			uint64_t m_lastStatusChangeTime = 0;
		};

	public:
		void init(GDN_TheWorld_Viewer* viewer);
		//godot::Ref<godot::Image> getMapImage(TheWorld_Utils::QuadrantPos& quadrantPos);
		//void setMapData(godot::Ref<godot::Image> mapImage, TheWorld_Utils::QuadrantPos& quadrantPos, std::string& meshId);
		bool updated(void);

		void clearThrash(void);
		void addUpdatedQuadrant(TheWorld_Utils::QuadrantPos& pos);
		//TheWorld_Utils::QuadrantPos fetchUpdatedQuadrant(void);
		void eraseUpdatedQuadrant(TheWorld_Utils::QuadrantPos& pos);
		void eraseUpdatingQuadrant(TheWorld_Utils::QuadrantPos& pos);
		void eraseByValue(TheWorld_Utils::QuadrantPos pos)
		{
			eraseByRef(pos);
		}
		void eraseByRef(TheWorld_Utils::QuadrantPos& pos);

		bool updateNeeded(TheWorld_Utils::QuadrantPos& pos);
		bool scheduleQuadrantForUpdate(TheWorld_Utils::QuadrantPos& pos);
		bool checkUpdatingQuadrants(void);

		void lock(void);
		void unlock(void);

		//godot::Vector2i getMapImageSize(void) const
		//{
		//	return m_mapImageSize;
		//}

		static int32_t getMapQuadrantOptimizationLevel(void)
		{
			return s_optimizationLevel;
		}
		int32_t getImageNumPixelsPerSize(void) const
		{
			return m_imageNumPixelsPerSize;
		}

		size_t getNumUpdatingQuadrants(void) const
		{
			return m_updatingQuadrants.size();
		}
		size_t getNumQuadrantsToShow(void) const
		{
			return m_updatedQuadrants.size();
		}

	private:
		//std::recursive_mutex m_internalMutex;
		GDN_TheWorld_Viewer* m_viewer = nullptr;
		//godot::Vector2i m_mapImageSize;
		std::map<TheWorld_Utils::QuadrantPos, bool> m_updatedQuadrants;
		std::map<TheWorld_Utils::QuadrantPos, std::unique_ptr<UpdatingInfo>> m_updatingQuadrants;
		static constexpr int32_t s_optimizationLevel = 4;		// 0 = 2049, 2 = 513, 4 = 129, 6 = 33
		int32_t m_imageNumPixelsPerSize = 0;
	};

#define STREAMER_SLEEP_TIME			10
#define STREAMER_SYNC_PERIOD_TIME	1000
#define FAR_HORIZON_MULTIPLIER		50

#define MIN_MAP_SCALE 0.01F
#define TIME_INTERVAL_BETWEEN_PERIDODIC_DUMP 0			// secs, 0 = diasble periodic dump
#define TIME_INTERVAL_BETWEEN_STATISTICS 1000	// ms, 0 = diasble periodic dump
#define TIME_INTERVAL_BETWEEN_MOUSE_TRACK 50	// 250	// ms
#define TIME_INTERVAL_BETWEEN_ELAB_CAMERA -1	// 2000	// ms
#define TIME_INTERVAL_BETWEEN_RECALC_QUADRANT 1000	// 250	// ms
//#define TIME_INTERVAL_BETWEEN_QUADRANT_INIT_AT_LOW_PRIORITY	 200	// ms
#define TIME_INTERVAL_BETWEEN_QUADRANT_INIT_AT_LOW_PRIORITY	 0	// ms

//#define DEFAULT_HIGH_MOUNTAINS_TEX_NAME					"PaintedPlaster017_1k"
//#define DEFAULT_HIGH_MOUNTAINS_ALTITUDE_THRESHOLD			2000
//#define DEFAULT_MOUNTAINS_TEX_NAME						"Ground003_2K"
//#define DEFAULT_MOUNTAINS_ALTITUDE_THRESHOLD				1500
//#define DEFAULT_LOW_MOUNTAINS_TEX_NAME					"Ground003_2K"
//#define DEFAULT_LOW_MOUNTAINS_ALTITUDE_THRESHOLD			700
//#define DEFAULT_HILLS_TEX_NAME							"Ground003_2K"
//#define DEFAULT_HILLS_ALTITUDE_THRESHOLD					300
//#define DEFAULT_PLAINS_TEX_NAME							"Ground003_2K"
//#define DEFAULT_PLAINS_ALTITUDE_THRESHOLD					0
//#define DEFAULT_ABYSS_TEX_NAME							"sand"
//#define DEFAULT_DIRT_TEX_NAME								"Ground049C_1K"
//#define DEFAULT_ROCKS_TEX_NAME							"rocks07"

	class GDN_TheWorld_MapViewportContainer;
	class GDN_TheWorld_MapViewport;
	class GDN_TheWorld_Map;
	class GDN_TheWorld_MapCamera;

	class GDN_TheWorld_MapViewer : public godot::Node2D
	{
		GDCLASS(GDN_TheWorld_MapViewer, Node2D)

	protected:
		static void _bind_methods(void);
		//void _notification(int p_what);

	public:
		GDN_TheWorld_MapViewer(void);
		~GDN_TheWorld_MapViewer(void);

		//
		// Godot Standard Functions
		//
		//virtual void _ready(void) override;
		//virtual void _process(double _delta) override;
		//virtual void _input(const Ref<InputEvent>& event) override;
		//virtual void _unhandled_input(const Ref<InputEvent>& event) override;

	public:
		void setViewer(GDN_TheWorld_Viewer* viewer)
		{
			m_viewer = viewer;
		}
		GDN_TheWorld_Viewer* getViewer(void)
		{
			return m_viewer;
		}

		void setMapViewportContainer(GDN_TheWorld_MapViewportContainer* mapViewportContainer)
		{
			m_mapViewportContainer = mapViewportContainer;
		}
		GDN_TheWorld_MapViewportContainer* getMapViewportContainer(void)
		{
			return m_mapViewportContainer;
		}

		void setMap(GDN_TheWorld_Map* map)
		{
			m_map = map;
		}
		GDN_TheWorld_Map* getMap(void)
		{
			return m_map;
		}

	private:
		GDN_TheWorld_Viewer* m_viewer = nullptr;
		GDN_TheWorld_MapViewportContainer* m_mapViewportContainer = nullptr;
		GDN_TheWorld_Map* m_map = nullptr;
	};

	class GDN_TheWorld_MapViewportContainer : public godot::SubViewportContainer
	{
		GDCLASS(GDN_TheWorld_MapViewportContainer, SubViewportContainer)

	protected:
		static void _bind_methods(void);
		void _notification(int p_what);

	public:
		GDN_TheWorld_MapViewportContainer(void);
		~GDN_TheWorld_MapViewportContainer(void);

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;
		virtual void _unhandled_input(const Ref<InputEvent>& event) override;

	public:
		void sizeChanged(godot::Vector2 newSize);

		void setMapViewer(GDN_TheWorld_MapViewer* mapViewer)
		{
			m_mapViewer = mapViewer;
		}
		GDN_TheWorld_MapViewer* getMapViewer(void)
		{
			return m_mapViewer;
		}
		void setMapViewport(GDN_TheWorld_MapViewport* mapViewport)
		{
			m_mapViewport = mapViewport;
		}
		GDN_TheWorld_MapViewport* getMapViewport(void)
		{
			return m_mapViewport;
		}
		void setMap(GDN_TheWorld_Map* map)
		{
			m_map = map;
		}
		GDN_TheWorld_Map* getMap(void)
		{
			return m_map;
		}

	private:
		GDN_TheWorld_MapViewer* m_mapViewer = nullptr;
		GDN_TheWorld_MapViewport* m_mapViewport = nullptr;
		godot::GDN_TheWorld_Map* m_map = nullptr;
	};

	class GDN_TheWorld_MapViewport : public godot::SubViewport
	{
		GDCLASS(GDN_TheWorld_MapViewport, SubViewport)

	protected:
		static void _bind_methods(void);
		void _notification(int p_what);

	public:
		GDN_TheWorld_MapViewport(void);
		~GDN_TheWorld_MapViewport(void);

		//
		// Godot Standard Functions
		//
		//virtual void _ready(void) override;
		//virtual void _process(double _delta) override;
		//virtual void _input(const Ref<InputEvent>& event) override;
		//virtual void _unhandled_input(const Ref<InputEvent>& event) override;

	public:
		void setMapViewportContainer(GDN_TheWorld_MapViewportContainer* mapViewportContainer)
		{
			m_mapViewportContainer = mapViewportContainer;
		}
		GDN_TheWorld_MapViewportContainer* getMapViewportContainer(void)
		{
			return m_mapViewportContainer;
		}
		void setMap(GDN_TheWorld_Map* map)
		{
			m_map = map;
		}
		GDN_TheWorld_Map* getMap(void)
		{
			return m_map;
		}

	private:
		GDN_TheWorld_MapViewportContainer* m_mapViewportContainer = nullptr;
		godot::GDN_TheWorld_Map* m_map = nullptr;
	};

	class GDN_TheWorld_Map : public godot::Node2D, public TheWorld_ClientServer::ClientCallback, public TheWorld_Utils::ThreadInitDeinit
	{
		GDCLASS(GDN_TheWorld_Map, godot::Node2D)

	protected:
		static void _bind_methods(void);
		void _notification(int p_what);

	public:
		const static int32_t c_map_z_index_quadrant_texture_override = 1;
		const static int32_t c_map_z_index_quadrant_color_override = 2;
		const static int32_t c_map_z_index_rect_color_overlay = 3;
		const static int32_t c_map_z_index_drag_box = 99;
		
		class MapQuadrantOverride
		{
		public:
			enum class Status
			{
				NotSet = -1
			};

			MapQuadrantOverride(GDN_TheWorld_Map* map, TheWorld_Utils::QuadrantPos& quadrantPos);
			~MapQuadrantOverride(void);

			void internalSetName(void);
			std::string getName(void) const
			{
				return  m_name;
			}
			//void render(void);
			void setVisible(bool b);
			enum class QuadTree::QuadrantDepth quadrantDepth(void) const
			{
				return m_quadrantDepth;
			}
			int numQuadrantsFromCameraQuadrant(void) const
			{
				return m_numQuadrantsFromCameraQuadrant;
			}
			void quadrantDepth(enum class QuadTree::QuadrantDepth quadrantDepth, int numQuadrantsFromCameraQuadrant);
			bool isQuadrantReady(void) const
			{
				return m_quadrantReady;
			}
			void quadrantReady(void);
			void quadrantWorkInProgress(void);
			bool quadrantActuallyInWorld(void) const
			{
				return m_quadrantActuallyInWorld;
			}
			void quadrantActuallyInWorld(bool b)
			{
				m_quadrantActuallyInWorld = b;
			}

			bool overrideTerrainTypeNull(void) const
			{
				if (m_terrainEdit.terrainType == TheWorld_Utils::TerrainEdit::TerrainType::notset)
					return true;
				if (m_terrainEdit.terrainType == TheWorld_Utils::TerrainEdit::TerrainType::unknown)
					return true;
				return false;
			}
			bool overrideTerrainTypeUnknown(void) const
			{
				return m_terrainEdit.terrainType == TheWorld_Utils::TerrainEdit::TerrainType::unknown;
			}
			bool overrideTerrainTypeSet(void) const
			{
				return !overrideTerrainTypeNull();
			}
			bool edited(void) const
			{
				return overrideTerrainTypeSet();
			}
			void resetEditedStatus(void)
			{
				if (edited())
				{
					TheWorld_Utils::TerrainEdit nullTerrainEdit(TheWorld_Utils::TerrainEdit::TerrainType::notset);
					setTerrainEdit(nullTerrainEdit);
					applyTexture();
				}
			}
			bool canBeErased(void) const
			{
				if (overrideTerrainTypeNull() and not quadrantActuallyInWorld() and not refreshInProgress())
					return true;
				else
					return false;
			}

			void applyColor(void);
			void applyTexture(void);
			void setTerrainEdit(TheWorld_Utils::TerrainEdit& terrainEdit)
			{
				m_terrainEdit = terrainEdit;
			}
			TheWorld_Utils::TerrainEdit getTerrainEdit(void) const
			{
				return m_terrainEdit;
			}
			TheWorld_Utils::TerrainEdit::TerrainType getTerrainType(void) const
			{
				return m_terrainEdit.terrainType;
			}
			bool editMode(void) const
			{
				return m_editMode;
			}
			void editMode(bool b)
			{
				m_editMode = b;
			}

			bool refreshInProgress(void) const
			{
				return m_refreshInProgress;
			}
			void refreshInProgress(bool b)
			{
				m_refreshInProgress = b;
			}

			bool quadrantOverrideBlendNeeded(void) const
			{
				return m_quadrantOverrideBlendNeeded;
			}
			void quadrantOverrideBlendNeeded(bool b)
			{
				m_quadrantOverrideBlendNeeded = b;
			}

			TheWorld_Utils::QuadrantPos getPos(void) const
			{
				return m_quadrantPos;
			}

		private:
			GDN_TheWorld_Map* m_map = nullptr;
			TheWorld_Utils::QuadrantPos m_quadrantPos;
			std::string m_name;

			bool m_quadrantActuallyInWorld = false;
			enum class QuadTree::QuadrantDepth m_quadrantDepth = QuadTree::QuadrantDepth::NotSet;
			int m_numQuadrantsFromCameraQuadrant = -1;
			bool m_quadrantReady = false;

			TheWorld_Utils::TerrainEdit m_terrainEdit;
			bool m_editMode = false;

			bool m_refreshInProgress = false;

			bool m_quadrantOverrideBlendNeeded = false;
		};

		class MapQuadrantOverrideContainer : public std::map<TheWorld_Utils::QuadrantPos, std::unique_ptr<MapQuadrantOverride>>
		{
		};

		class MapTextureRect
		{
		public:
			enum class Status
			{
				NotSet = -1
				, CheckFromServerNotDone = 0
				, CheckFromServerRequired = 1
				, CheckFromServerInProgress = 2
				, CheckFromServerDone = 3
				, AcqQuadrantsNeeded = 4
				, AcqQuadrantsInProgress = 5
				, AcqQuadrantsDone = 6
			};

			class MapQuadrantTextureOverride : public godot::TextureRect
			{
				GDCLASS(MapQuadrantTextureOverride, godot::TextureRect)

			protected:
				static void _bind_methods(void);
				//void _notification(int p_what);

			public:
				//
				// Godot Standard Functions
				//
				//virtual void _ready(void) override;
				//virtual void _process(double _delta) override;
				//virtual void _input(const Ref<InputEvent>& event) override;
				//virtual void _unhandled_input(const Ref<InputEvent>& event) override;

				MapQuadrantTextureOverride(void);
				~MapQuadrantTextureOverride(void);

				void init(MapTextureRect* mapTextureRect, TheWorld_Utils::QuadrantPos& quadrantPos);
				void freeResource(void);
				void apply(GDN_TheWorld_Map::MapQuadrantOverride* mapQuadrantOverride);
				void applyOverride(void);
				bool canBeErased(GDN_TheWorld_Map::MapQuadrantOverride* mapQuadrantOverride);

			private:
				MapTextureRect* m_mapTextureRect = nullptr;
				TheWorld_Utils::QuadrantPos m_quadrantPos;
			};

			class MapQuadrantColorOverride : public godot::ColorRect
			{
				GDCLASS(MapQuadrantColorOverride, godot::ColorRect)

			protected:
				static void _bind_methods(void);
				//void _notification(int p_what);

			public:
				//
				// Godot Standard Functions
				//
				//virtual void _ready(void) override;
				//virtual void _process(double _delta) override;
				//virtual void _input(const Ref<InputEvent>& event) override;
				//virtual void _unhandled_input(const Ref<InputEvent>& event) override;

				MapQuadrantColorOverride(void);
				~MapQuadrantColorOverride(void);

				void init(MapTextureRect* mapTextureRect, TheWorld_Utils::QuadrantPos& quadrantPos);
				void freeResource(void);
				void apply(GDN_TheWorld_Map::MapQuadrantOverride* mapQuadrantOverride);
				void applyOverride(void);
				bool canBeErased(GDN_TheWorld_Map::MapQuadrantOverride* mapQuadrantOverride);

			private:
				MapTextureRect* m_mapTextureRect = nullptr;
				TheWorld_Utils::QuadrantPos m_quadrantPos;
			};

			class MapTextureColorOverlay : public godot::ColorRect
			{
				GDCLASS(MapTextureColorOverlay, godot::ColorRect)

			protected:
				static void _bind_methods(void);
				//void _notification(int p_what);

			public:
				//
				// Godot Standard Functions
				//
				//virtual void _ready(void) override;
				//virtual void _process(double _delta) override;
				//virtual void _input(const Ref<InputEvent>& event) override;
				//virtual void _unhandled_input(const Ref<InputEvent>& event) override;

				MapTextureColorOverlay(void);
				~MapTextureColorOverlay(void);

				void init(MapTextureRect* mapTextureRect);
				void rectStatusChanged(void);

			private:
				MapTextureRect* m_mapTextureRect = nullptr;
			};

		public:
			MapTextureRect(GDN_TheWorld_Map* map, TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
			~MapTextureRect(void);

			static godot::Color colorFromStatus(enum class Status status, bool& highlighted);

			void init(GDN_TheWorld_Map* map = nullptr);
			void detachTextureResource(TheWorld_Utils::QuadrantPos& quadrantPos);
			void detachColorResource(TheWorld_Utils::QuadrantPos& quadrantPos);

			TheWorld_Utils::MapTextureRectPos pos(void) const
			{
				return m_mapTextureRectPos;
			}

			bool rectFull(std::chrono::system_clock::time_point& moreRecentQuadrantTime);
			void saveRectImage(godot::Ref<godot::Image> rectImage, std::chrono::system_clock::time_point moreRecentQuadrantTime);
			bool setMapQuadrantData(const godot::Ref<godot::Image> mapImage, const TheWorld_Utils::QuadrantPos& quadrantPos, std::chrono::system_clock::time_point quadrantTime);
			bool destroyInternalTextureRect(void);
			bool isTextureRectRendered(void);
			godot::TextureRect* getInternalTextureRect(void);
			bool eraseCachedTexture(void);
			bool renderCachedTexture(void);
			bool renderInternalTexture(void);
			void renderImage(godot::Ref<godot::Image> image);
			godot::String getImageCachePath(void);
			std::string fileTimeStr(void) const
			{
				return m_fileTimeStr;
			}
			//void fileTimeStr(std::string& _fileTimeStr)
			//{
			//	m_fileTimeStr = _fileTimeStr;
			//}
			void fileTimeStrFromFile(std::string& _pathFile)
			{
				m_fileTimeStr = TheWorld_Utils::last_write_time_str_utc(_pathFile);
			}
			bool canCheckForRefresh(void)
			{
				return m_fileTimeStr.size() > 0;
			}
			void checkForRefreshDisabled(void)
			{
				m_fileTimeStr.clear();
			}
			bool internalImageDataValid(void)
			{
				return m_internalImageData.size() > 0;
			}
			bool CheckFromServerNotDone()
			{
				//return false;
				return getStatus() == Status::CheckFromServerNotDone;
			}
			bool CheckFromServerRequired()
			{
				//return false;
				return getStatus() == Status::CheckFromServerRequired;
			}
			bool CheckFromServerInProgress()
			{
				return getStatus() == Status::CheckFromServerInProgress;
			}
			bool CheckFromServerDone()
			{
				return getStatus() == Status::CheckFromServerDone;
			}
			bool AcqQuadrantsNeeded()
			{
				return getStatus() == Status::AcqQuadrantsNeeded;
			}
			bool AcqQuadrantsInProgress()
			{
				return getStatus() == Status::AcqQuadrantsInProgress;
			}
			bool AcqQuadrantsDone()
			{
				return getStatus() == Status::AcqQuadrantsDone;
			}
			Status getStatus(void);
			void setStatus(Status status);
			void setInternalImageData(TheWorld_Utils::MemoryBuffer& imageBuffer, std::chrono::system_clock::time_point imageTime);

			MapQuadrantColorOverride* getQuadrantColorOverride(TheWorld_Utils::QuadrantPos& quadrantPos, bool createIfMissing = false);
			MapQuadrantTextureOverride* getQuadrantTextureOverride(TheWorld_Utils::QuadrantPos& quadrantPos, bool createIfMissing = false);
			void releaseQuadrantColorOverride(TheWorld_Utils::QuadrantPos& quadrantPos);
			void releaseQuadrantTextureOverride(TheWorld_Utils::QuadrantPos& quadrantPos);

			GDN_TheWorld_Map* getMap(void) const
			{
				return m_map;
			}

			class QuadrantImage
			{
			public:
				QuadrantImage(godot::Ref<godot::Image> image, std::chrono::system_clock::time_point quadrantTime)
				{
					m_image = image;
					m_quadrantTime = quadrantTime;
				}
				~QuadrantImage()
				{
				}
				godot::Ref<godot::Image> image(void) const
				{
					return m_image;
				}
				std::chrono::system_clock::time_point quadrantTime(void) const
				{
					return m_quadrantTime;
				}

			private:
				godot::Ref<godot::Image> m_image;
				std::chrono::system_clock::time_point m_quadrantTime;
			};

		private:
			GDN_TheWorld_Map* m_map = nullptr;
			Status m_status = Status::NotSet;
			TheWorld_Viewer_Utils::MsTimePoint m_lastStatusChange;
			TheWorld_Utils::MapTextureRectPos m_mapTextureRectPos;
			std::vector<std::vector<QuadrantImage>> m_quadrantImages;	// bi-dimensional vector of Image references
			//std::filesystem::file_time_type m_fileTime;
			std::string m_fileTimeStr;		// es: 2023-09-04 19:33:24.7026392
			godot::TextureRect* m_internalTextureRect = nullptr;
			MapTextureColorOverlay* m_internalColorRect = nullptr;
			godot::PackedByteArray m_internalImageData;
			std::chrono::system_clock::time_point m_imageTime;
			std::map<TheWorld_Utils::QuadrantPos, std::unique_ptr<MapQuadrantColorOverride>> m_quadrantColorOverrideContainer;
			std::map<TheWorld_Utils::QuadrantPos, std::unique_ptr<MapQuadrantTextureOverride>> m_quadrantTextureOverrideContainer;
		};

		class MapTextureRectContainer : public std::map<TheWorld_Utils::MapTextureRectPos, std::unique_ptr<MapTextureRect>>
		{
		};

		class UploadingMapTextureRect
		{
		public:
			enum class Status
			{
				NotSet = -1
				, UploadRequired = 0
				, UploadInProgress = 1
				, Uploaded = 2

			};
			UploadingMapTextureRect(GDN_TheWorld_Map* map, TheWorld_Utils::MapTextureRectPos& mapTextureRectPos, std::string& utcFileTimeStr, godot::Ref<godot::Image> rectImage)
			{
				m_status = Status::UploadRequired;
				m_map = map;
				m_mapTextureRectPos = mapTextureRectPos;
				m_utcFileTimeStr = utcFileTimeStr;
				m_rectImage = rectImage;
			}
			~UploadingMapTextureRect(void)
			{
			}

			TheWorld_Utils::MapTextureRectPos pos(void) const
			{
				return m_mapTextureRectPos;
			}
			std::string utcFileTimeStr(void) const
			{
				return m_utcFileTimeStr;
			}
			godot::Ref<godot::Image> rectImage(void)
			{
				return m_rectImage;
			}

			bool uploadRequired(void)
			{
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				long long elapsedFromStatusChange = (now - m_lastStatusChange).count();
				if (m_status == Status::UploadInProgress and elapsedFromStatusChange > 60000)
					m_status = Status::UploadRequired;
				return m_status == Status::UploadRequired;
			}
			bool uploadInProgress(void) const
			{
				return m_status == Status::UploadInProgress;
			}
			bool uploaded(void) const
			{
				return m_status == Status::Uploaded;
			}
			void setStatus(enum class Status status)
			{
				if (m_status != status)
				{
					TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
					m_lastStatusChange = now;
					m_status = status;
				}
			}

		private:
			enum class Status m_status = Status::NotSet;
			TheWorld_Viewer_Utils::MsTimePoint m_lastStatusChange;
			GDN_TheWorld_Map* m_map;
			TheWorld_Utils::MapTextureRectPos m_mapTextureRectPos;
			std::string m_utcFileTimeStr;		// es: 2023-09-04 19:33:24.7026392
			godot::Ref<godot::Image> m_rectImage;
		};

		class UploadingMapTextureRectContainer : public std::map<TheWorld_Utils::MapTextureRectPos, std::unique_ptr<UploadingMapTextureRect>>
		{
		};

	public:
		GDN_TheWorld_Map(void);
		~GDN_TheWorld_Map(void);

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;
		virtual void _unhandled_input(const Ref<InputEvent>& event) override;
		virtual void _physics_process(double _delta) override;

	public:
		bool dragBoxOn(void) const
		{
			return m_dragBoxOn;
		}
		static int32_t numImagesForTexture(void);
		void init(GDN_TheWorld_MapViewer* mapViewer, godot::Vector2 containerSize);
		void enterMapMode(void);
		void exitMapMode(void);
		void setMapGridLineThickness(float thickness);
		bool mapGridOn(void) const
		{
			return m_mapGridOn;
		}
		void mapGridOn(bool b);
		void replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply);
		virtual void threadInit(void) {}
		virtual void threadDeinit(void) {}
		GDN_TheWorld_MapViewer* getMapViewer(void) const
		{
			return m_mapViewer;
		}
		void viewportSizeChanged(void);
		void viewportResized(godot::Vector2 newSize);
		void resizeRuler(void);
		void resizeRuler(godot::Vector2 newSize);

		//void setScale(float scaleFactor);
		float getQuadrantWorldSize(void);
		float getQuadrantGlobalSize(void) const;
		int32_t getNumQuadrantsPerSize(void) const;
		godot::Vector2i getImageSize(void) const;
		godot::Vector2i getTextureRectSize(void) const;
		//godot::Vector2i getTextureRectIdxFromWorldPos(float x, float z, int level, int numVerticesPerSize, float gridStepInWU) const;
		void getScreenTextureRects(std::vector<TheWorld_Utils::MapTextureRectPos>& rectsPos);
		bool eraseNotInScreenTextureRects(void);
		godot::Rect2 getScreenRectInGlobalCoord(void) const;
		godot::Size2 getScreenSizeInGlobalCoord(void) const;
		godot::Vector2 getMousePosInGlobalCoord(void) const;
		godot::Vector2 getMousePosInGlobalCoord(godot::Vector2 mousePosInViewport) const;

		godot::Vector2 globalPosToWorldPos(godot::Vector2 globalPos);
		godot::Vector2 worldPosToGlobalPos(godot::Vector2 worldPos);
		float globalSizeToWorldSize(float globalSize);
		float worldSizeToGlobalSize(float worldSize);

		MapTextureRect* getMapTextureRect(const TheWorld_Utils::QuadrantPos& quadrantPos, bool add = true);
		MapTextureRect* getMapTextureRect(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos, bool add = true);
		//bool eraseMapTextureRect(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
		bool destroyTextureRect(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
		bool isTextureRectRendered(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
		bool eraseCachedTexture(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
		bool renderCachedTexture(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
		bool renderInternalTexture(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
		bool CheckFromServerNotDone(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
		bool CheckFromServerRequired(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
		bool CheckFromServerInProgress(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
		bool CheckFromServerDone(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
		bool AcqQuadrantsNeeded(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
		bool AcqQuadrantsInProgress(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
		void setStatus(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos, MapTextureRect::Status status);
		bool setMapQuadrantData(const godot::Ref<godot::Image> mapImage, const TheWorld_Utils::QuadrantPos& quadrantPos, std::chrono::system_clock::time_point quadrantTime);
		void setWorldFlag(godot::String name, godot::Color c, bool setCameraDir = false, bool visible = true);
		godot::String getCacheDir(void);

		void selectTextureRectForUpload(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos, std::string& fileTimeStr, godot::Ref<godot::Image> rectImage);
		bool uploadSelectedTextureRect(void);
		bool checkRequiredTextureRect(void);

		//void zoom(int zoomRequired)
		//{
		//	m_zoomRequired = zoomRequired;
		//}

		//float getScaleFactor(void) const
		//{
		//	return m_scaleFactor;
		//}
		static int32_t numImagesForTextureRectSide(void)
		{
			return s_numImagesForTexture;
		}

		int32_t getNumTextureRects(void);
		int32_t getMapQuadrantColorOverrideNum(void);
		int32_t getMapQuadrantTextureOverrideNum(void);

		void addMapTextureRectRefreshScheduled(TheWorld_Utils::MapTextureRectPos& pos);

		void worldQuadrantsMapReset(void);

		void quadrantReady(TheWorld_Utils::QuadrantPos& pos);
		void quadrantWorkInProgress(TheWorld_Utils::QuadrantPos& pos);

		MapTextureRect::MapQuadrantColorOverride* getQuadrantColorOverride(TheWorld_Utils::QuadrantPos& quadrantPos, bool createIfMissing = false);
		MapTextureRect::MapQuadrantTextureOverride* getQuadrantTextureOverride(TheWorld_Utils::QuadrantPos& quadrantPos, bool createIfMissing = false);
		void releaseQuadrantColorOverride(TheWorld_Utils::QuadrantPos& quadrantPos);
		void releaseQuadrantTextureOverride(TheWorld_Utils::QuadrantPos& quadrantPos);
		void getAllMapQuadrantOverridePos(std::list<TheWorld_Utils::QuadrantPos>& list, bool onlyEdited = false);
		size_t getAllMapQuadrantOverrideNum(bool onlyEdited = false);
		MapQuadrantOverride* getMapQuadrantOverride(TheWorld_Utils::QuadrantPos& quadrantPos);
		MapQuadrantOverride* addEditedMapQuadrantOverride(TheWorld_Utils::QuadrantPos& quadrantPos, TheWorld_Utils::TerrainEdit& terrainEdit);
		void removeEditedMapQuadrantOverride(TheWorld_Utils::QuadrantPos& quadrantPos);
		MapQuadrantOverride* addRefreshingMapQuadrantOverride(TheWorld_Utils::QuadrantPos& quadrantPos);
		void removeRefreshingMapQuadrantOverride(TheWorld_Utils::QuadrantPos& quadrantPos);
		void editMode(TheWorld_Utils::QuadrantPos& quadrantPos, bool b);

		void incMapTextureRectNum(void)
		{
			m_mapTextureRectNum++;
		}
		void decMapTextureRectNum(void)
		{
			m_mapTextureRectNum--;
		}
		void incMapQuadrantColorOverrideNum(void)
		{
			m_mapQuadrantColorOverrideNum++;
		}
		void decMapQuadrantColorOverrideNum(void)
		{
			m_mapQuadrantColorOverrideNum--;
		}
		void incMapQuadrantTextureOverrideNum(void)
		{
			m_mapQuadrantTextureOverrideNum++;
		}
		void decMapQuadrantTextureOverrideNum(void)
		{
			m_mapQuadrantTextureOverrideNum--;
		}

		void resetDragInputEvents(void);

	private:
		GDN_TheWorld_Globals* Globals(bool useCache = true);

	private:
		GDN_TheWorld_MapViewer* m_mapViewer = nullptr;
		GDN_TheWorld_Globals* m_globals = nullptr;
		GDN_TheWorld_MapViewport* m_mapViewport = nullptr;
		MapTextureRectContainer m_mapTextureRectContainer;
		MapQuadrantOverrideContainer m_mapQuadrantOverrideContainer;
		UploadingMapTextureRectContainer m_uploadingMapTextureRectContainer;
		std::map<TheWorld_Utils::MapTextureRectPos, bool> m_mapTextureRectRefreshScheduled;
		godot::Dictionary m_worldFlagDict;
		static constexpr int32_t s_numImagesForTexture = 5;

		godot::Label* m_northLabel = nullptr;
		godot::Label* m_southLabel = nullptr;
		godot::Label* m_eastLabel = nullptr;
		godot::Label* m_westLabel = nullptr;

		godot::Vector2 m_rulerGroupCustomMinimumSize;
		godot::VBoxContainer* m_rulerGroup = nullptr;

		godot::TextureRect* m_compassImage = nullptr;

		//godot::Ref<godot::ShaderMaterial> m_textureRectMaterial;
		//godot::Ref<godot::ShaderMaterial> m_quadrantRectMaterial;

		bool m_mapGridOn = false;

		int32_t m_mapTextureRectNum = 0;
		int32_t m_mapQuadrantColorOverrideNum = 0;
		int32_t m_mapQuadrantTextureOverrideNum = 0;

		bool m_destroying = false;

		//bool m_startDragging = false;
		//bool m_stopDragging = false;
		enum class DragBoxType
		{
			Unknown = -1,
			Select,
			Unselect
		};
		bool m_dragBoxOn = false;
		DragBoxType m_dragBoxType = DragBoxType::Unknown;
		godot::ColorRect* m_dragBox = nullptr;
		TheWorld_Utils::QuadrantPos m_startDragBoxQuadrantPos;
		TheWorld_Utils::QuadrantPos m_endDragBoxQuadrantPos;

		bool m_shiftPressed = false;
		bool m_ctrlPressed = false;
		bool m_altPressed = false;
	};

	class GDN_TheWorld_HudControls : public godot::Container
	{
		GDCLASS(GDN_TheWorld_HudControls, Container)

	protected:
		static void _bind_methods(void) {};
		void _notification(int p_what) {};

	public:
		GDN_TheWorld_HudControls() {}
		~GDN_TheWorld_HudControls() {}

		void init(GDN_TheWorld_Viewer* viewer);
		void setPos(godot::Size2& windowSize);
		void setLoadingProgress(float v);
		void resetLoadingProgress(void);
		void showLoadingBar(bool reset = true);
		void hideLoadingBar(void);
		bool isLoadingBarVisibile(void);

		//
		// Godot Standard Functions
		//
		//virtual void _ready(void) override {};
		//virtual void _process(double _delta) override {};
		//virtual void _input(const Ref<InputEvent>& event) override;
		//virtual void _unhandled_input(const Ref<InputEvent>& event) override;

	private:
		GDN_TheWorld_Viewer* m_viewer = nullptr;
		godot::VBoxContainer* m_loadingVBox = nullptr;
		godot::Label* m_loadingLabel = nullptr;
		godot::ProgressBar* m_loadingProgressBar = nullptr;
		float m_loadingProgress = 0;
	};

	class GDN_TheWorld_MapModder : public godot::Node
	{
		GDCLASS(GDN_TheWorld_MapModder, Node)

	public:
		enum class MapType
		{

			uninitialized = 0
			//,initializedForNormals = 1
			//,initializedForSplatmap = 2
			,initializedForGlobalmap = 3
		};

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		GDN_TheWorld_MapModder();
		~GDN_TheWorld_MapModder();

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _physics_process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;

		void custom_ready(GDN_TheWorld_Viewer* viewer, Node* parent, bool debug = false);
		void debug_process(double _delta);

		void framePostDraw(void);

		//void initForNormals(QuadTree* quadtree, TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, size_t tileSize, size_t viewportBorder = 0, bool signalRequired = false);
		//void initForSplatmap(QuadTree* quadtree, TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* normalsBuffer,
		//	float slopeVerticalFactor,
		//	float slopeFlatFactor,
		//	float dirtOnRocksFactor,
		//	float highElevationFactor,
		//	float lowElevationFactor,
		//	float minHeigth,
		//	float maxHeigth,
		//	size_t tileSize, size_t viewportBorder = 0, bool signalRequired = false);
		void initForGlobalMap(QuadTree* quadtree, size_t tileSize = 512, bool signalRequired = false);
		void deinit(void);
		void startProcessing(void);
		bool working(void);
		bool ready(void);
		TheWorld_Utils::MemoryBuffer* getOutBuffer(void);
		void startProcessRequired(bool b);
		bool startProcessRequired(void) const;
		void deinitRequired(bool b);
		bool deinitRequired(void) const;
		//godot::Vector2 eastQuadrantheights(void) const
		//{
		//	return m_eastQuadrantheights;
		//}
		//godot::Vector2 westQuadrantheights(void) const
		//{
		//	return m_westQuadrantheights;
		//}
		//godot::Vector2 southQuadrantheights(void) const
		//{
		//	return m_southQuadrantheights;
		//}
		//godot::Vector2 northQuadrantheights(void) const
		//{
		//	return m_northQuadrantheights;
		//}
		static void incNumWorking()
		{
			s_numWorking++;
		}
		static void decNumWorking()
		{
			std::lock_guard<std::recursive_mutex> lock(s_mtxInternal);
			if (s_numWorking > 0)
				s_numWorking--;
		}
		static size_t getNumWorking()
		{
			return s_numWorking;
		}

	private:
		bool workToDo(void);

	private:
		//const int32_t c_uninitialized = 0;
		//const int32_t c_initializedForNormals = 1;
		//const int32_t c_initializedForSplatmap = 2;
		//const int32_t c_initializedForGlobalmap = 3;
		enum class MapType m_initialized = MapType::uninitialized;

		bool m_startProcessingRequired = false;
		bool m_deinitRequired = false;

		static int32_t s_numWorking;
		static std::recursive_mutex s_mtxInternal;

		bool m_signalRequired = false;

		GDN_TheWorld_Viewer* m_viewer = nullptr;
		Node* m_parent = nullptr;
		int32_t m_numProcessExecutions = 0;
		size_t m_tileSize = 0;
		size_t m_viewportBorder = 0;
		size_t m_imageSize = 0;
		//TheWorld_Utils::MemoryBuffer* m_float32HeigthsBuffer = nullptr;
		//size_t m_numHeights = 0;
		//TheWorld_Utils::MemoryBuffer* m_normalsBuffer = nullptr;
		//size_t m_numNormals = 0;
		float m_slopeVerticalFactor = 0;
		float m_slopeFlatFactor = 0;
		float m_dirtOnRocksFactor = 0;
		float m_highElevationFactor = 0;
		float m_lowElevationFactor = 0;
		float m_minHeigth = 0;
		float m_maxHeigth = 0;
		//godot::Vector2 m_eastQuadrantheights;
		//godot::Vector2 m_westQuadrantheights;
		//godot::Vector2 m_southQuadrantheights;
		//godot::Vector2 m_northQuadrantheights;
		QuadTree* m_quadTree = nullptr;
		godot::SubViewport* m_mapModderViewport = nullptr;
		godot::Sprite2D* m_map = nullptr;
		godot::MeshInstance3D* m_terrainPlane = nullptr;
		godot::MeshInstance3D* m_seaPlane = nullptr;
		godot::Camera3D* m_camera = nullptr;
		godot::Vector3 m_displacementPlane;
		float m_cameraAltitude = 0.0f;

		const int16_t c_tileStatusWaiting = 0;
		const int16_t c_tileStatusProcessing = 1;
		std::map<godot::Vector2i, int16_t> m_tiles;
		std::queue<godot::Vector2i> m_tilesQueue;
		godot::Vector2i m_currentTile;
		bool m_processingTile = false;

		godot::Ref<godot::Image> m_outImage;
		bool m_outImageValid = false;
		std::unique_ptr<TheWorld_Utils::MemoryBuffer> m_outBuffer;
		bool m_frameAvailable = false;

		godot::Sprite2D* m_debugSprite = nullptr;
	};

	// World Node Local Coordinate System is the same as MapManager coordinate system
	// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
	// Chunk and QuadTree coordinates are in Viewer Node local coordinate System
	// X growing on East direction
	// Z growing on South direction
	class GDN_TheWorld_Viewer : public godot::Node3D, public TheWorld_ClientServer::ClientCallback, public TheWorld_Utils::ThreadInitDeinit
	{
		GDCLASS(GDN_TheWorld_Viewer, Node3D)

		class TrackedInputEvent
		{
		public:
			enum class TrackedInputEventType
			{
				NOT_SET = -1
				, MOUSE_BUTTON_LEFT = 0
				, MOUSE_BUTTON_RIGHT = 1
				, MOUSE_DOUBLE_BUTTON_LEFT = 2
				, MOUSE_DOUBLE_BUTTON_RIGHT = 3
			};

			TrackedInputEvent(GDN_TheWorld_Viewer* viewer, enum class TrackedInputEventType type = TrackedInputEventType::NOT_SET, int32_t delayToBeAvailable = -1)
			{
				m_type = type;
				m_time = std::chrono::system_clock::time_point();
				if (type != TrackedInputEventType::NOT_SET)
					m_time = TheWorld_Utils::current_utc_time_point();
				if (type == TrackedInputEventType::MOUSE_DOUBLE_BUTTON_LEFT or type == TrackedInputEventType::MOUSE_DOUBLE_BUTTON_RIGHT)
					m_delayToBeAvailable = 0;
				if (delayToBeAvailable >= 0)
					m_delayToBeAvailable = delayToBeAvailable;
				m_viewer = viewer;
				if (m_viewer != nullptr)
					m_mousePosInViewport = m_viewer->getMousePosInViewport();
			}

			bool operator<(const TrackedInputEvent& event) const
			{
				if (m_type < event.m_type)
					return true;
				else
					return false;
			}
			bool operator==(const TrackedInputEvent& event) const
			{
				return m_type == event.m_type;
			}
			void operator=(const TrackedInputEvent& event)
			{
				m_type = event.m_type;
				m_time = event.m_time;
				m_delayToBeAvailable = event.m_delayToBeAvailable;
				m_viewer = event.m_viewer;
				m_mousePosInViewport = event.m_mousePosInViewport;
			}
			enum class TrackedInputEventType type(void) const
			{
				return m_type;
			}
			std::chrono::system_clock::time_point time(void) const
			{
				return m_time;
			}
			std::string strTime(void) const
			{
				return TheWorld_Utils::time_point_to_str_time(m_time);
			}
			bool isAvailable(void) const
			{
				if (m_delayToBeAvailable == 0)
					return true;
				if (m_type == TrackedInputEventType::NOT_SET)
					return false;
				auto delay_in_queue = std::chrono::duration_cast<std::chrono::milliseconds>(TheWorld_Utils::current_utc_time_point() - m_time);
				return delay_in_queue.count() > m_delayToBeAvailable;
			}
			void reset(void)
			{
				m_type = TrackedInputEventType::NOT_SET;
			}
			bool isReset(void) const
			{
				return m_type == TrackedInputEventType::NOT_SET;
			}
			GDN_TheWorld_Viewer* getViewer() const
			{
				return m_viewer;
			}
			godot::Vector2 mousePosInViewport(void) const
			{
				return m_mousePosInViewport;
			}

		private:
			enum class TrackedInputEventType m_type = TrackedInputEventType::NOT_SET;
			std::chrono::system_clock::time_point m_time;
			int32_t m_delayToBeAvailable = 300;	// default value in ms
			GDN_TheWorld_Viewer* m_viewer = nullptr;
			godot::Vector2 m_mousePosInViewport;
		};
			
		class TrackedInputEventContainer : public std::map<TrackedInputEvent, bool>
		{
		};

		// Its purpose is to transform 2 single-click event enough close together in 1 double-click: the second is transformed and the previous is deleted (only if thay are enough far together)
		class TrackedInputEventQueue : public std::list<TrackedInputEvent>
		{
		public:
			void push_back(const TrackedInputEvent& trackedInputEvent)
			{
				switch (trackedInputEvent.type())
				{
				case TrackedInputEvent::TrackedInputEventType::MOUSE_BUTTON_LEFT:
				{
					// invalidating last single button left still not available
					size_t _size = size();
					int32_t num = 0;
					for (reverse_iterator it = rbegin(); it != rend(); it++)
					{
						if (it->type() == TrackedInputEvent::TrackedInputEventType::MOUSE_BUTTON_LEFT and !it->isAvailable() and !it->isReset())
						{
							it->reset();
							num++;
						}
						if (num == 1)
							break;
					}
					if (num == 0)
					{
						// Not found any single button left inside the availability delay (still not available): we can add single click
						std::list<TrackedInputEvent>::push_back(trackedInputEvent);
						//PLOG_DEBUG << "*** added SINGLE BUTTON_LEFT ***" << " " << trackedInputEvent.strTime() << " (" << size() << ")";
					}
					else
					{
						// Found a single button left inside the availability delay (still not available): we removed single click and transform this click in double click
						TrackedInputEvent doubleClickEvent(trackedInputEvent.getViewer(), TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_LEFT);
						std::list<TrackedInputEvent>::push_back(doubleClickEvent);
						//PLOG_DEBUG << "*** removed last single BUTTON LEFT and added DOUBLE BUTTON_LEFT ***" << " " << doubleClickEvent.strTime() << " (" << size() << ")";
					}
				}
				break;
				case TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_LEFT:
				{
					// invalidating last 2 single button left still not available
					size_t _size = size();
					int32_t num = 0;
					for (reverse_iterator it = rbegin(); it != rend(); it++)
					{
						if (it->type() == TrackedInputEvent::TrackedInputEventType::MOUSE_BUTTON_LEFT and !it->isAvailable() and !it->isReset())
						{
							it->reset();
							num++;
						}
						if (num == 2)
							break;
					}
					// now can add double click
					std::list<TrackedInputEvent>::push_back(trackedInputEvent);
					//PLOG_DEBUG << "*** added DOUBLE BUTTON_LEFT ***" << " " << trackedInputEvent.strTime() << " (" << size() << ")";
				}
				break;
				case TrackedInputEvent::TrackedInputEventType::MOUSE_BUTTON_RIGHT:
				{
					// invalidating last single button right still not available
					size_t _size = size();
					int32_t num = 0;
					for (reverse_iterator it = rbegin(); it != rend(); it++)
					{
						if (it->type() == TrackedInputEvent::TrackedInputEventType::MOUSE_BUTTON_RIGHT and !it->isAvailable() and !it->isReset())
						{
							it->reset();
							num++;
						}
						if (num == 1)
							break;
					}
					if (num == 0)
					{
						// Not found any single button right inside the availability delay (still not available): we can add single click
						std::list<TrackedInputEvent>::push_back(trackedInputEvent);
						//PLOG_DEBUG << "*** added SINGLE BUTTON_RIGHT ***" << " " << trackedInputEvent.strTime() << " (" << size() << ")";
					}
					else
					{
						// Found a single button right inside the availability delay (still not available): we removed single click and transform this click in double click
						TrackedInputEvent doubleClickEvent(trackedInputEvent.getViewer(), TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_RIGHT);
						std::list<TrackedInputEvent>::push_back(doubleClickEvent);
						//PLOG_DEBUG << "*** removed last single BUTTON BUTTON_RIGHT and added DOUBLE BUTTON_RIGHT ***" << " " << doubleClickEvent.strTime() << " (" << size() << ")";
					}
				}
				break;
				case TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_RIGHT:
				{
					// invalidating last 2 single button right still not available
					size_t _size = size();
					int32_t num = 0;
					for (reverse_iterator it = rbegin(); it != rend(); it++)
					{
						if (it->type() == TrackedInputEvent::TrackedInputEventType::MOUSE_BUTTON_RIGHT and !it->isAvailable() and !it->isReset())
						{
							it->reset();
							num++;
						}
						if (num == 2)
							break;
					}
					// now can add double click
					std::list<TrackedInputEvent>::push_back(trackedInputEvent);
					//PLOG_DEBUG << "*** added DOUBLE BUTTON_RIGHT ***" << " " << trackedInputEvent.strTime() <<  " (" << size() << ")";
				}
				break;
				default:
					std::list<TrackedInputEvent>::push_back(trackedInputEvent);
					break;
				}
			}
		};

		enum class Margin {
			MARGIN_LEFT,
			MARGIN_TOP,
			MARGIN_RIGHT,
			MARGIN_BOTTOM
		};

		friend class GDN_TheWorld_Edit;
		//friend class Chunk;
		//friend class TerrainChunk;
		//friend class QuadTree;
		//friend class ShaderTerrainData;

	protected:
		static void _bind_methods();
		void _notification(int p_what);
		void _init(void);

	public:
		class ProcessStep
		{
		public:

			ProcessStep() {}

			enum class ProcessStepIdx
			{
				not_set = -1
				, process_updateCameraQuadrant = 0
				, process_initNewQuads = 1
				, process_trackMouse = 2
				, process_updateLodInQuads = 3
				, process_updateMeshForUpdatedChunks = 4
				, UpdateTerrainVisibility = 5
				, UpdateDebugMode = 6
				, process_lookDev = 7
				, ReloadSelectedQuadrantShader = 8
				, ReloadAllShaders = 9
				, ShaderParamChanged = 10
				, Quadrant_resetMaterialParams_heightmap = 11
				, Quadrant_resetMaterialParams_normalmap = 12
				, Quadrant_resetMaterialParams_splatmap = 13
				, Quadrant_resetMaterialParams_globalmap = 14
				, Quadrant_resetMaterialParams_colormap = 15
				, Quadrant_updateMaterialParams = 16
				, Quadrant_updateCollider = 17
				, EndOfProcessExecution = 18
				, process_updateCameraInQuadrants = 19
				, process_updateOriginShift = 20
				, process_castRayFromCamera = 21
				, process_updateGridDimension = 22
				, process_mapQuadrants = 23
				, process_updateHud = 24
				, process_initialStuff = 25
				, process_adjustMapQuadrants = 26
				, process_mapQuadrantsUI = 27
				, process_delayedWorldMode = 28
				, process_eraseOutOfViewQuadrants = 29
				, process_checkCameraMovement = 30
				, process_beforeProcessImpl = 31
				, process_initialStuff_0 = 32
				, process_initialStuff_1 = 33
				, process_dummy_0 = 34
				, process_dummy_1 = 35
				, process_dummy_2 = 36
				, process_dummy_3 = 37
				, process_dummy_4 = 38
				, process_dummy_5 = 39
				, process_dummy_6 = 40
				, process_dummy_7 = 41
				, process_dummy_8 = 42
				, process_dummy_9 = 43
				, process_mapModder = 44
				, process_updateTerrainMaterial
				, process_materialParams
				// if you add a new index here, update maxProcessStepIdx below
			};
			static const int maxProcessStepIdx = int(ProcessStepIdx::process_materialParams);
			static const size_t numProcessStepIndices = maxProcessStepIdx + 1;

			ProcessStep(enum class ProcessStepIdx idx)
			{
				m_idx = idx;
			}
			void setElapsedFromProcessStart(size_t elapsed, size_t deltaFromPreviousStep)
			{
				m_numInCurrentExecution++;
				m_currentElapsedFromProcessStart = elapsed;
				m_currentDiffFromPreviousStep += deltaFromPreviousStep;
			}
			void clearForCurrentExecution(void)
			{
				m_numInCurrentExecution = 0;
				m_currentElapsedFromProcessStart = 0;
				m_currentDiffFromPreviousStep = 0;
			}
			void finalizeCurrentExecution(size_t& previousStepElapsedFromProcessStart)
			{
				if (m_numInCurrentExecution > 0)
				{
					//if (m_idx == ProcessStepIdx::EndOfProcessExecution)			// DEBUG
					//	assert(m_idx == ProcessStepIdx::EndOfProcessExecution);		// DEBUG

					m_numTotExecutions++;
					m_totElapsedFromProcessStart += m_currentElapsedFromProcessStart;
					m_totDiffFromPreviousStep += (m_currentElapsedFromProcessStart - previousStepElapsedFromProcessStart);
					m_totDiffFromPreviousStep_1 += m_currentDiffFromPreviousStep;
					previousStepElapsedFromProcessStart = m_currentElapsedFromProcessStart;
				}
			}
			size_t getNumInCurrentExecution(void) const
			{
				return m_numInCurrentExecution;
			}
			bool operator<(const ProcessStep& processStep) const
			{
				if (m_currentElapsedFromProcessStart < processStep.m_currentElapsedFromProcessStart)
					return true;
				else
					return false;
			}
			static std::string idxToString(enum class ProcessStepIdx idx)
			{
				switch (idx)
				{
				case ProcessStepIdx::process_updateCameraQuadrant:
					return "process_updateCameraQuadrant";
				case ProcessStepIdx::process_initNewQuads:
					return "process_initNewQuads";
				case ProcessStepIdx::process_trackMouse:
					return "process_trackMouse";
				case ProcessStepIdx::process_updateLodInQuads:
					return "process_updateLodInQuads";
				case ProcessStepIdx::process_updateMeshForUpdatedChunks:
					return "process_updateMeshForUpdatedChunks";
				case ProcessStepIdx::UpdateTerrainVisibility:
					return "UpdateTerrainVisibility";
				case ProcessStepIdx::UpdateDebugMode:
					return "UpdateDebugMode";
				case ProcessStepIdx::process_lookDev:
					return "process_lookDev";
				case ProcessStepIdx::ReloadSelectedQuadrantShader:
					return "ReloadSelectedQuadrantShader";
				case ProcessStepIdx::ReloadAllShaders:
					return "ReloadAllShaders";
				case ProcessStepIdx::ShaderParamChanged:
					return "ShaderParamChanged";
				case ProcessStepIdx::Quadrant_resetMaterialParams_heightmap:
					return "Quadrant_resetMaterialParams_heightmap";
				case ProcessStepIdx::Quadrant_resetMaterialParams_normalmap:
					return "Quadrant_resetMaterialParams_normalmap";
				case ProcessStepIdx::Quadrant_resetMaterialParams_splatmap:
					return "Quadrant_resetMaterialParams_splatmap";
				case ProcessStepIdx::Quadrant_resetMaterialParams_globalmap:
					return "Quadrant_resetMaterialParams_globalmap";
				case ProcessStepIdx::Quadrant_resetMaterialParams_colormap:
					return "Quadrant_resetMaterialParams_colormap";
				case ProcessStepIdx::Quadrant_updateMaterialParams:
					return "Quadrant_updateMaterialParams";
				case ProcessStepIdx::Quadrant_updateCollider:
					return "Quadrant_updateCollider";
				case ProcessStepIdx::EndOfProcessExecution:
					return "EndOfProcessExecution";
				case ProcessStepIdx::process_updateCameraInQuadrants:
					return "process_updateCameraInQuadrants";
				case ProcessStepIdx::process_updateOriginShift:
					return "process_updateOriginShift";
				case ProcessStepIdx::process_castRayFromCamera:
					return "process_castRayFromCamera";
				case ProcessStepIdx::process_updateGridDimension:
					return "process_updateGridDimension";
				case ProcessStepIdx::process_mapQuadrants:
					return "process_mapQuadrants";
				case ProcessStepIdx::process_updateHud:
					return "process_updateHud";
				case ProcessStepIdx::process_initialStuff:
					return "process_initialStuff";
				case ProcessStepIdx::process_adjustMapQuadrants:
					return "process_adjustMapQuadrants";
				case ProcessStepIdx::process_mapQuadrantsUI:
					return "process_mapQuadrantsUI";
				case ProcessStepIdx::process_delayedWorldMode:
					return "process_delayedWorldMode";
				case ProcessStepIdx::process_eraseOutOfViewQuadrants:
					return "process_eraseOutOfViewQuadrants";
				case ProcessStepIdx::process_checkCameraMovement:
					return "process_checkCameraMovement";
				case ProcessStepIdx::process_beforeProcessImpl:
					return "process_beforeProcessImpl";
				case ProcessStepIdx::process_initialStuff_0:
					return "process_initialStuff_0";
				case ProcessStepIdx::process_initialStuff_1:
					return "process_initialStuff_1";
				case ProcessStepIdx::process_dummy_0:
					return "process_dummy_0";
				case ProcessStepIdx::process_dummy_1:
					return "process_dummy_1";
				case ProcessStepIdx::process_dummy_2:
					return "process_dummy_2";
				case ProcessStepIdx::process_dummy_3:
					return "process_dummy_3";
				case ProcessStepIdx::process_dummy_4:
					return "process_dummy_4";
				case ProcessStepIdx::process_dummy_5:
					return "process_dummy_5";
				case ProcessStepIdx::process_dummy_6:
					return "process_dummy_6";
				case ProcessStepIdx::process_dummy_7:
					return "process_dummy_7";
				case ProcessStepIdx::process_dummy_8:
					return "process_dummy_8";
				case ProcessStepIdx::process_dummy_9:
					return "process_dummy_9";
				case ProcessStepIdx::process_mapModder:
					return "process_mapModder";
				default:
					return "not_set";
				}
			}
			std::string name(void) const
			{
				return idxToString(m_idx);
			}
			std::string msg(void)
			{
				size_t num = 0;
				size_t averageElapsedFromProcessStart = getAverageElapsedFromProcessStart(num);
				return name() + " - Avg execution (num " + std::to_string(num) + ") = " + TheWorld_Utils::quickAddThousandSeparators(std::to_string(averageElapsedFromProcessStart / 1000)) + " mcs" + (m_numInCurrentExecution > 0 ? std::string(" - Last execution ") + "(" + std::to_string(m_numInCurrentExecution) + " times)" + " = " + TheWorld_Utils::quickAddThousandSeparators(std::to_string(m_currentElapsedFromProcessStart / 1000)) + " mcs" : "");
			}
			enum class ProcessStepIdx getIdx(void) const
			{
				return m_idx;
			}
			void resetAverageValues(void)
			{
				m_totDiffFromPreviousStep = m_totDiffFromPreviousStep_1 = m_totElapsedFromProcessStart = m_numTotExecutions = 0;
			}
			void computeAverageValues(bool reset = true)
			{
				m_averageElapsedFromProcessStart = 0;
				m_averageDiffFromPreviousStep = 0;
				m_averageDiffFromPreviousStep_1 = 0;
				if (m_numTotExecutions != 0)
				{
					m_averageElapsedFromProcessStart = m_totElapsedFromProcessStart / m_numTotExecutions;
					m_averageDiffFromPreviousStep = m_totDiffFromPreviousStep / m_numTotExecutions;
					m_averageDiffFromPreviousStep_1 = m_totDiffFromPreviousStep_1 / m_numTotExecutions;
				}
				m_totNumAverage = m_numTotExecutions;
				if (reset)
					resetAverageValues();
				//if (m_idx == ProcessStepIdx::EndOfProcessExecution)				// DEBUG
				//	assert(m_idx == ProcessStepIdx::EndOfProcessExecution);		// DEBUG
			}
			bool setInOrdering(void) const
			{
				return m_setInOrdering;
			}
			void setInOrdering(bool b)
			{
				m_setInOrdering = b;
			}
			size_t getTotDiffFromPreviousStep(void) const
			{
				//return m_totDiffFromPreviousStep;
				return m_totDiffFromPreviousStep_1;
			}
			size_t getAverageElapsedFromProcessStart(size_t& num) const
			{
				num = m_totNumAverage;
				return m_averageElapsedFromProcessStart;
			}
			size_t getAverageDiffFromPreviousStep(size_t& num) const
			{
				num = m_totNumAverage;
				//return m_averageDiffFromPreviousStep;
				return m_averageDiffFromPreviousStep_1;
			}

		private:
			enum class ProcessStepIdx m_idx = ProcessStepIdx::not_set;
			
			size_t m_numInCurrentExecution = 0;
			size_t m_currentElapsedFromProcessStart = 0;
			size_t m_currentDiffFromPreviousStep = 0;

			size_t m_numTotExecutions = 0;
			size_t m_totElapsedFromProcessStart = 0;
			size_t m_totDiffFromPreviousStep = 0;
			size_t m_totDiffFromPreviousStep_1 = 0;

			size_t m_totNumAverage = 0;
			size_t m_averageElapsedFromProcessStart = 0;
			size_t m_averageDiffFromPreviousStep = 0;
			size_t m_averageDiffFromPreviousStep_1 = 0;		// average weight from previous step during an execution of _process method considering all single contributes if inside a loop
			bool m_setInOrdering = false;
		};

		class ProcessSteps
		{
		public:

			ProcessSteps() {}

			void initForCurrentExecution()
			{
				if (not m_initialized)
				{
					m_steps.resize(ProcessStep::numProcessStepIndices);
					for (int i = 0; i < ProcessStep::numProcessStepIndices; i++)
						m_steps[i] = ProcessStep(ProcessStep::ProcessStepIdx(i));
					m_initialized = true;
				}

				for (auto& processStep : m_steps)
				{
					processStep.clearForCurrentExecution();
				}

				m_lastStepIdx = ProcessStep::ProcessStepIdx::not_set;
				m_lastStepElapsedFromProcessStart = 0;

				m_processClock.tick();
			}

			void finalizeCurrentExecution(void);

			size_t addElapsedFromProcessStart(enum class ProcessStep::ProcessStepIdx idx)
			{
				size_t elapsed = (size_t)m_processClock.partialDuration().count();
				size_t deltaFromPreviousStep = elapsed;
				if (m_lastStepIdx != ProcessStep::ProcessStepIdx::not_set)
					deltaFromPreviousStep = elapsed - m_lastStepElapsedFromProcessStart;
				m_steps[int(idx)].setElapsedFromProcessStart(elapsed, deltaFromPreviousStep);
				m_lastStepIdx = idx;
				m_lastStepElapsedFromProcessStart = elapsed;
				return elapsed;
			}

			size_t elapsed()
			{
				return (size_t)m_processClock.partialDuration().count();
			}

			std::vector<ProcessStep>& steps(void)
			{
				return m_steps;
			}

			std::list<ProcessStep::ProcessStepIdx>& stepsOrder(void)
			{
				return m_stepsOrder;
			}

			std::string lastAddedStepName(void) const
			{
				return ProcessStep::idxToString(m_lastStepIdx);
			}

		private:
			bool m_initialized = false;
			std::vector<ProcessStep> m_steps;
			std::list<ProcessStep::ProcessStepIdx> m_stepsOrder;
			TimerNans m_processClock;
			enum class ProcessStep::ProcessStepIdx m_lastStepIdx = ProcessStep::ProcessStepIdx::not_set;
			size_t m_lastStepElapsedFromProcessStart = 0;
		};

		GDN_TheWorld_Viewer();
		~GDN_TheWorld_Viewer();
		bool initRequired(void) const
		{
			return m_initRequired;
		};
		void initRequired(bool b)
		{
			m_initRequired = b;
		};
		bool init(void);
		bool canDeinit(void);
		void preDeinit(void);
		void deinit(void);
		bool quitting() const
		{
			return m_quitting;
		}
		
		virtual void setPosWorldViewer(godot::Size2& windowSize);

		virtual void createMapViewer(void);
		virtual void setPosMapViewer(godot::Size2& windowSize);

		virtual void createHud(void);
		virtual void setPosHud(godot::Size2& windowSize);

		virtual void threadInit(void) {}
		virtual void threadDeinit(void) {}

		void replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply);

		//void setDesideredLookDev(enum class ShaderTerrainData::LookDev desideredLookDev)
		//{
		//	if (m_desiderdLookDev != desideredLookDev)
		//	{
		//		m_desiderdLookDev = desideredLookDev;
		//		m_desideredLookDevChanged = true;
		//	}
		//}

		//enum class ShaderTerrainData::LookDev getDesideredLookDev(void) const
		//{
		//	return m_desiderdLookDev;
		//}

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _physics_process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;
		
		void process_impl(double _delta, godot::Camera2D* activeMapCamera, godot::Camera3D* activeWorldCamera);

		//Ref<Material> getRegularMaterial(void);
		//Ref<Material> getLookDevMaterial(void);
		godot::Plane getShaderParamGroundUVScale() const
		{
			return m_shaderParam_groundUVScale;
		}
		bool getShaderParamDepthBlending() const
		{
			return m_shaderParam_depthBlending;
		}
		bool getShaderParamTriplanar() const
		{
			return m_shaderParam_triplanar;
		}
		godot::Plane getShaderParamTileReduction() const
		{
			return m_shaderParam_tileReduction;
		}
		float getShaderParamGlobalmapBlendStart() const
		{
			return m_shaderParam_globalmapBlendStart;
		}
		float getShaderParamGlobalmapBlendDistance() const
		{
			return m_shaderParam_globalmapBlendDistance;
		}
		godot::Plane getShaderParamColormapOpacity() const
		{
			return m_shaderParam_colormapOpacity;
		}

		float getShaderParamQuadBorderLineThickness(void) const
		{
			return m_shaderParam_quad_border_line_thickness;
		}
		float getShaderParamMouseHitRadius(void) const
		{
			//if (positionVisible())
				return m_shaderParam_mouse_hit_radius;
			//else
			//	return 0;
		}
		void setShaderParam(godot::String name, godot::Variant value);
		void debugPrint(String message)
		{
			Globals()->debugPrint(message);
		}
		void errorPrint(String message)
		{
			Globals()->errorPrint(message);
		}
		void warningPrint(String message)
		{
			Globals()->warningPrint(message);
		}
		void infoPrint(String message)
		{
			Globals()->infoPrint(message);
		}
		void print(String message)
		{
			Globals()->print(message);
		}
		void setEditorInterface(godot::Object* editorInterface)
		{
			m_editorInterface = (godot::EditorInterface*)editorInterface;
		}
		void setEditor3dOverlay(godot::Object* editor3dOverlay);
		bool mapGridOn(void);
		void mapGridOn(bool b);
		bool trackMouse(void) const;
		void trackMouse(bool b);
		bool _mapMode(void) const
		{
			return mapMode();
		}
		bool mapMode(void) const;
		void mapMode(bool b);
		bool _worldMode(void) const
		{
			return worldMode();
		}
		bool adjustWorldCameraRequired(void) const;
		void adjustWorldCameraRequired(bool b, godot::Vector2 desideredCameraWorldPos = godot::Vector2());
		bool delayedWorldMode(void) const;
		void delayedWorldMode(bool b);
		bool adjustEnvironmentRequired(void) const
		{
			return m_adjustEnvironmentRequired;
		}
		void adjustEnvironmentRequired(bool b)
		{
			m_adjustEnvironmentRequired = b;
		}
		bool worldMode(void) const;
		void toggleMapView(void);
		void adjustWorldCamera(godot::Vector2 desideredCameraGlobalPos);
		void toggleMapGrid(void);
		void toggleTrackMouse(void);
		void toggleEditMode(void);
		void setEditMode(bool editMode);
		bool isEditMode(void) const;
		bool infoPanelVisible(void) const
		{
			return m_infoPanelVisible;
		}
		void setInfoPanelVisible(bool b);
		void toggleDebugVisibility(void);
		enum class GDN_TheWorld_Globals::ChunkDebugMode getChunkDebugMode(void) const;
		void rotateChunkDebugMode(void);
		void toggleWireframe(void) const;
		void rotateDrawingMode(void);
		bool getTrackMouseState(void) const
		{
			return trackMouse();
		}
		//void toggleFullScreen(void);
		void toggleWorldQuadrantSelected(void);
		void selectWorldQuadrant(TheWorld_Utils::QuadrantPos pos);
		void selectWorldQuadrantRef(TheWorld_Utils::QuadrantPos& pos);
		void deselectWorldQuadrant(void);
		void selectMapQuadrant(TheWorld_Utils::QuadrantPos pos);
		void selectMapQuadrantRef(TheWorld_Utils::QuadrantPos& pos);
		void deselectMapQuadrant(void);
		void clearOverrideMapQuadrant(TheWorld_Utils::QuadrantPos pos);
		void clearOverrideMapQuadrantRef(TheWorld_Utils::QuadrantPos& pos);
		void addClearOverrideMapQuadrant(TheWorld_Utils::QuadrantPos pos);
		void addClearOverrideMapQuadrantRef(TheWorld_Utils::QuadrantPos& pos);
		godot::Camera3D* getCustomCamera(void);
		godot::Camera3D* getWorldCamera(void);
		godot::Camera3D* getWorldCamera2(bool force = false);
		godot::Camera2D* getMapCamera(void);
		godot::Camera3D* getWorldCameraInEditor(void);
		void setWorldEditorCamera(godot::Camera3D* editorCamera);
		void setMinDepthQuadOnPerimeter(int depth);
		int getMinDepthQuadOnPerimeter(void) const;
		void setCacheQuadOnPerimeter(int cache);
		int getCacheQuadOnPerimeter(void) const;
		void setMaxDepthQuadOnPerimeter(int depth);
		size_t numQuadrantsPerGridSize(void) const;
		godot::Vector2 getGridPosInWUs(void);
		float getMaxGridSizeInWUs(void);

		int getMaxDepthQuadOnPerimeter(void) const;
		int maxActualDepthQuadInAllDirections(void) const;
		void checkNewVisibleQuadrantsRectSize(godot::Transform3D newCameraT, godot::Transform3D oldCameraT);
		godot::Rect2 getVisibleQuadrantsRect(TheWorld_Utils::QuadrantPos& cameraQuadrantPos, bool& valid);
		void setForceShaderElevation0(bool elevation0);
		bool getForceShaderElevation0(void) const;
		String getInfoCamera(void);
		int getCameraProjectionMode(void);
		GDN_TheWorld_Globals* Globals(bool useCache = true);
		//GDN_TheWorld_WorldViewportContainer* WorldViewportContainer(bool useCache = true);
		//GDN_TheWorld_WorldViewport* WorldViewport(bool useCache = true);
		GDN_TheWorld_Camera* WorldCamera(bool useCache = true);
		GDN_TheWorld_HudControls* HudControls(bool useCache = true);
		GDN_TheWorld_MapViewer* getMapViewer(void)
		{
			return MapViewer();
		}
		//float getMapScaleFactor(void);
		GDN_TheWorld_MapViewer* MapViewer(bool useCache = true);
		GDN_TheWorld_MapViewportContainer* MapViewportContainer(bool useCache = true);
		GDN_TheWorld_MapViewport* MapViewport(bool useCache = true);
		GDN_TheWorld_Map* Map(bool canUseCache = true, bool mustUseCache = false);
		GDN_TheWorld_MapCamera* MapCamera(bool useCache = true);
		godot::GDN_TheWorld_Edit* EditModeUIControl(bool useCache = true);
		godot::Control* getOrCreateEditModeUIControl(void);
		void createEditModeUI(void);
		void deployWorld(float cameraX, float cameraY, float cameraZ, float cameraDistanceFromTerrainForced, float cameraYaw, float cameraPitch, float cameraRoll, int level, int chunkSizeShift, int heightmapResolutionShift);
		void undeployWorld(void);
		void destroyModeUIControl(void);
		bool initialWorldViewerPosSet(void) const
		{
			return m_initialWorldViewerPosSet;
		}
		bool recalcGridOfQuadrantsInView(void);
		Node3D* getWorldNode(void) const;
		MeshCache* getMeshCache(void)
		{
			return m_meshCache.get(); 
		}
		Transform3D getInternalGlobalTransform(void) const;
		void setDumpRequired(void)
		{
			m_dumpRequired = true; 
		}
		void adjustEnvironment(void);
		template <typename T> void getChildrenOfType(godot::Array& outNodes, godot::Node* parent = nullptr, int level = 0, bool recurse = true);
		void dump(void);
		void dumpRecurseIntoChildrenNodes(Array nodes, int level);
		void setWorldCameraTerrainChunk(Chunk* chunk, QuadTree* quadTree);
		AABB getWorldCameraTerrainChunkLocalAABB(void);
		AABB getWorldCameraTerrainChunkLocalDebugAABB(void);
		Transform3D getWorldCameraTerrainChunkGlobalTransformApplied(void);
		Transform3D getWorldCameraTerrainChunkDebugGlobalTransformApplied(void);
		String getWorldCameraTerrainChunkId(void);
		String getWorldCameraTerrainQuadrantName(void);
		int getNumSplits(void) const;
		int getNumJoins(void) const;
		int getNumChunks(void) const;
		int getNumActiveChunks(void) const;
		int getNumTerrainChunks(void) const;
		int getNumActiveTerrainChunks(void) const;
		int getNumSeaChunks(void) const;
		int getNumActiveSeaChunks(void) const;
		int getProcessDuration(void) const;
		//int getRefreshMapQuadDuration(void) const;
		int getUpdateQuads1Duration(void) const;
		int getUpdateQuads2Duration(void) const;
		int getUpdateQuads3Duration(void) const;
		int getUpdateChunksDuration(void) const;
		int getUpdateMaterialParamsDuration(void) const;
		int getMouseTrackHitDuration(void) const;
		int getProcessNotOwnsLock(void) const;
		float getLoadingProgress(void);
		int getMinNumQuadrants(void) const
		{
			return (int)pow((getMinDepthQuadOnPerimeter() + getCacheQuadOnPerimeter()) * 2 + 1, 2);
		}
		int getMinNumVisibleQuadrants(void) const
		{
			return (int)pow((getMinDepthQuadOnPerimeter()) * 2 + 1, 2);
		}
		int getMaxNumQuadrants(void) const
		{
			return (int)pow((getMaxDepthQuadOnPerimeter() + getCacheQuadOnPerimeter()) * 2 + 1, 2);
		}
		int getMaxNumVisibleQuadrants(void) const
		{
			return (int)pow(getMaxDepthQuadOnPerimeter() * 2 + 1, 2);
		}
		int getNumQuadrantsInView(void)
		{
			return getNumQuadrantsInView2(false);
		}
		int getNumQuadrantsInView2(bool recalcNow);
		int getNumQuadrantsInMemory(void)
		{
			return getNumQuadrantsInMemory2(false);
		}
		int getNumQuadrantsInMemory2(bool recalcNow);
		int getNumInitializedQuadrants(void)
		{
			return getNumInitializedQuadrants2(false);
		}
		int getNumReadyQuadrantsAtDepth(int& numEvaluated, float& numReady, QuadTree::QuadrantDepth depth1);
		int getNumReadyQuadrantsAtDepth(int& numEvaluated, float& numReady, QuadTree::QuadrantDepth depth1, QuadTree::QuadrantDepth depth2);
		int getNumReadyQuadrantsAtDepth(int& numEvaluated, float& numReady, QuadTree::QuadrantDepth depth1, QuadTree::QuadrantDepth depth2, QuadTree::QuadrantDepth depth3);
		int getNumReadyQuadrants(void);
		int getNumInitializedQuadrants2(bool recalcNow);
		int getNumToEraseQuadrants(void) const;
		int getNumVisibleQuadrants(void) const;
		int getNumInitializedVisibleQuadrants(void) const;
		int getNumNeedShaderResetQuadrants(void) const;
		int getNumNeedShaderUpdateQuadrants(void) const;
		int getNumEmptyQuadrants(void) const;
		int getNumQuadrantsLoadingFromServer(void) const;
		int getNumQuadrantsLoadingFromLocalCache(void) const;
		int getNumFlushedQuadrants(void) const;
		int getQuadrantNumWithCollider(void) const;
		int getQuadrantNumWithEnabledCollider(void) const;
		int getQuadrantNumWithHiResCollider(void) const;
		int getQuadrantNumWithLowResCollider(void) const;
		float getDelayDegree(void);
		void calcDelayDegree(void);
		bool allQuadrantsInitialized(void) const
		{
			return m_allQuadrantsInitialized;
		}
		void allQuadrantsInitialized(bool b)
		{
			m_allQuadrantsInitialized = b;
		}
		bool allVisibleQuadrantsInitialized(void) const
		{
			return m_allVisibleQuadrantsInitialized;
		}
		void allVisibleQuadrantsInitialized(bool b)
		{
			m_allVisibleQuadrantsInitialized = b;
		}
		bool recalcAllVisibleQuadrantsInitialized(void);	// ha senso usarlo o conviene usare allVisibleQuadrantInitialized() per evitare di ricalcolarlo più di una volta nel corso di una esecuzione di _process?
		void refreshQuadrantsStatistics(void);
		void resetMaterialParamsForEveryQuadrant(void);
		void updateMaterialParamsForEveryQuadrant(void);
		void setColliderNeedUpdateForEveryQuadrant(void);
		GDN_TheWorld_Globals::ChunkDebugMode getRequiredChunkDebugMode(void) const
		{
			return m_requiredChunkDebugMode;
		}
		String getChunkDebugModeStr(void) const;
		bool getDebugContentVisibility(void) const
		{
			return m_debugContentVisibility; 
		}
		String getDebugDrawMode(void) const;
		bool useVisualServer(void) const
		{
			return m_useVisualServer; 
		}
		void forceRefreshQuadTreeMap(void) 
		{
			m_refreshQuadTreeMap = true; 
		}
		size_t getExtraQuadsMapSize(void);
		void getAllQuadrantPos(std::vector<TheWorld_Utils::QuadrantPos>& allQuandrantPos, bool withExtraQuads = false);
		QuadTree* getQuadTree(TheWorld_Utils::QuadrantPos pos, bool excludeToErase = true, bool lockQuadrant = false, bool extraQuadAllowed = false);
		Chunk* getActiveChunkAt(TheWorld_Utils::QuadrantPos& pos, Chunk::ChunkPos chunkPos, enum class Chunk::DirectionSlot dir, Chunk::LookForChunk filter, enum class QuadType type);
		Chunk* getActiveChunkAt(Chunk* chunk, enum class Chunk::DirectionSlot dir, Chunk::LookForChunk filter, enum class QuadType type);
		bool terrainShiftPermitted(void);
		godot::Vector3 getMouseHitGlobalPos(void) const
		{
			if (mapMode())
				return m_trackedMouseHitGlobalPos;
			else
			{
				if (m_mouseTrackedOnSomething)
					return m_trackedMouseHitGlobalPos;
				else
					return godot::Vector3();
			}
		}
		godot::Vector3 getMouseHitWorldPos(void) const
		{
			if (mapMode())
				return m_trackedMouseHitWorldPos;
			else
			{
				if (m_mouseTrackedOnSomething)
					return m_trackedMouseHitWorldPos;
				else
					return godot::Vector3();
			}
		}
		float getMouseHitDistanceFromCamera(void)
		{
			if (m_mouseTrackedOnSomething)
			{
				godot::Camera3D* camera = getWorldCamera();
				if (camera == nullptr)
					return 0.0f;
				else
					return (float)camera->get_global_transform().origin.distance_to(m_trackedMouseHitGlobalPos);
			}
			else
				return 0.0f;
		}
		godot::String _getMouseQuadrantHitName(void)
		{
			return getMouseQuadrantHitName().c_str();
		}
		std::string getMouseQuadrantHitName(void)
		{
			return m_mouseQuadrantHitName;
		}
		godot::String _getMouseQuadrantHitTag(void)
		{
			return getMouseQuadrantHitTag().c_str();
		}
		std::string getMouseQuadrantHitTag(void)
		{
			return m_mouseQuadrantHitTag;
		}
		godot::Vector3 getMouseQuadrantHitPos(void) const
		{
			return m_mouseQuadrantHitPos;
		}
		float getMouseQuadrantHitSize(void) const
		{
			return m_mouseQuadrantHitSize;
		}
		godot::String getMouseChunkHitId(void)
		{
			if (m_mouseTrackedOnSomething && m_mouseHitTerrainChunk != nullptr && m_mouseHitTerrainChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return std::string(m_mouseHitTerrainChunk->getQuadTree()->getQuadrant()->getPos().getName() + " " + m_mouseHitTerrainChunk->getIdStr()).c_str();
				//return m_mouseHitChunk->getIdStr().c_str();
			}
			else
				return "";
		}
		godot::Vector3 getMouseChunkHitPos(void)
		{
			if (m_mouseTrackedOnSomething && m_mouseHitTerrainChunk != nullptr && m_mouseHitTerrainChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return Vector3(m_mouseHitTerrainChunk->getLowerXInWUsGlobal(), 0, m_mouseHitTerrainChunk->getLowerZInWUsGlobal());
			}
			else
				return Vector3();
		}
		float getMouseChunkHitSize(void)
		{
			if (m_mouseTrackedOnSomething && m_mouseHitTerrainChunk != nullptr && m_mouseHitTerrainChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return m_mouseHitTerrainChunk->getChunkSizeInWUs();
			}
			else
				return 0.0;
		}

		float getMouseChunkHitDistFromCam(void)
		{
			if (m_mouseTrackedOnSomething && m_mouseHitTerrainChunk != nullptr && m_mouseHitTerrainChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return m_mouseHitTerrainChunk->getDistanceFromCamera();
			}
			else
				return 0.0;
		}

		void setMouseHitTerrainChunk(Chunk* chunk)
		{
			m_mouseHitTerrainChunk = chunk;
		}
		//void setMouseHitQuadTree(QuadTree* quadTree)
		//{
		//	m_mouseHitQuadTree = quadTree;
		//}

		bool editMode(void) const
		{
			return m_editMode;
		}

		bool canProcessWorldDataUpdate(void) const
		{
			static bool canProcessDataUpdate = false;
			return (canProcessDataUpdate or editMode());
		}

		//void GenerateHeigths(void);

		std::recursive_mutex& getMainProcessingMutex(void)
		{
			return m_mtxQuadrantsAndMainProcessing;
		}

		std::string to_string(godot::String s)
		{
			//godot::PackedByteArray byteStream = s.to_utf8_buffer();
			const char* str = s.utf8().get_data();
			std::string ret = s.utf8().get_data();
			//char* str = s.alloc_c_string();
			//std::string ret = str;
			//godot::api->godot_free(str);
			return ret;
		}

		bool isInEditor(void)
		{
			if (m_isInEditorInitialized)
				return m_isInEditor;

			GDN_TheWorld_Globals* g = Globals();
			if (g == nullptr)
				return false;

			m_isInEditor = IS_EDITOR_HINT();
			m_isInEditorInitialized = true;
			
			return m_isInEditor;
		}

		godot::Vector2 getMousePosInViewport(void)
		{
			if (isInEditor())
			{
				if (m_editor3dOverlay != nullptr)
				{
					return m_editor3dOverlay->get_local_mouse_position();
				}
				else
					return godot::Vector2(0.0f, 0.0f);
			}
			else
				return get_viewport()->get_mouse_position();
		}
		
		godot::Size2 getViewportSize(void)
		{
			if (isInEditor())
			{
				if (m_editor3dOverlay != nullptr)
				{
					godot::Rect2 rect = m_editor3dOverlay->get_rect();
					return godot::Size2(rect.size);
				}
				else
					return godot::Size2(0.0f, 0.0f);
			}
			else
			{
				godot::Viewport* v = get_viewport();
				if (v != nullptr)
				{
					godot::Rect2 rect = v->get_visible_rect();
					return godot::Size2(rect.size);
				}
				else
					return godot::Vector2(0.0f, 0.0f);
			}
		}
		
		godot::Size2 getSavedViewportSize(void) const
		{
			return m_savedViewportSize;
		}

		void viewportSizeChanged(void);
		bool ReloadAllShadersRequired(void) const
		{
			return m_reloadAllShadersRequired;
		}
		void ReloadAllShadersRequired(bool b)
		{
			m_reloadAllShadersRequired = b;
		}
		bool ReloadSelectedQuadrantShaderRequired(void) const
		{
			return m_reloadSelectedQuadrantShaderRequired;
		}
		void ReloadSelectedQuadrantShaderRequired(bool b)
		{
			m_reloadSelectedQuadrantShaderRequired = b;
		}
		void trackedMouseHitChanged(godot::Camera3D* activeWorldCamera, QuadTree* quadrantHit, std::list<QuadTree*>& adjacentQuadrantsHit, bool hit, godot::Vector3 mouseHit, godot::Vector3 normalHit = Vector3Zero);
		void clearQuadrantsMaps(void);
		bool gizmoVisible(void) const
		{
			return m_gizmoVisible;
		}
		void gizmoVisible(bool b)
		{
			m_gizmoVisible = b;
		}
		bool normalVisible(void) const
		{
			return m_normalVisible;
		}
		void normalVisible(bool b)
		{
			m_normalVisible = b;
		}
		bool showPositionOnCursorHit(void) const
		{
			return m_showPositionOnCursorHit;
		}
		void showPositionOnCursorHit(bool b)
		{
			m_showPositionOnCursorHit = b;
			if (!m_showPositionOnCursorHit)
				trackingDistance(false);
		}
		void highlightHitQuadrant(bool b, bool force = false);
		bool highlightHitQuadrant(void) const
		{
			return m_highlightingHitQuadrant;
		}
		bool gridOn(void) const
		{
			return m_gridOn;
		}
		void gridOn(bool b)
		{
			if (m_gridOn != b)
			{
				if (isWorldQuadrantSelectedForEdit())
				{
					QuadTree* quadTreeSelectedForEdit = nullptr;
					TheWorld_Utils::QuadrantPos pos = getWorldQuadrantSelForEdit(&quadTreeSelectedForEdit);
					quadTreeSelectedForEdit->materialParamsNeedUpdate(true);
				}

				if (!b)
					updateMaterialParamsForEveryQuadrant();
				else
					updateMaterialParamsForEveryQuadrant();
			}

			m_gridOn = b;
		}
		bool castRayFromCamera(void) const
		{
			return m_castRayFromCamera;
		}
		void castRayFromCamera(bool b)
		{
			m_castRayFromCamera = b;
		}
		bool trackingDistance(void) const
		{
			return m_trackingDistance;
		}
		void trackingDistance(bool b, godot::Vector3 savedPos = godot::Vector3())
		{
			m_trackingDistance = b;
			if (m_trackingDistance)
				m_savedPosForTrackingDistance = savedPos;
			else
				if (m_positionDrawer != nullptr)
				{
					if (m_linePositionIdx != -1)
						m_positionDrawer->updateLine(m_linePositionIdx, godot::Vector3(), godot::Vector3());
					if (m_spherePositionStartIdx != -1)
						m_positionDrawer->updateSphere(m_spherePositionStartIdx, godot::Vector3(), 0.0f);
					if (m_spherePositionEndIdx != -1)
						m_positionDrawer->updateSphere(m_spherePositionEndIdx, godot::Vector3(), 0.0f);
				}
		}
		
		bool isInputEventTracked(TrackedInputEvent::TrackedInputEventType eventType, TrackedInputEvent* trackedInputEvent, TrackedInputEventContainer* inputEvents = nullptr);
		void compressTrackedInputEvents(TrackedInputEventContainer* inputEvents = nullptr, TrackedInputEventQueue* trackedInputEventQueue = nullptr);
		void resetTrackedInputEvents(TrackedInputEventContainer* inputEvents = nullptr, TrackedInputEventQueue* trackedInputEventQueue = nullptr);

		bool mouseInsideMainEditPanel(void);

		void quadrantModderWorkDone(int32_t map_type, float x, float z, int32_t num_vertices_per_side, int32_t level, float grid_step);

		void incNumVisibleQuadrantsStreamed(void)
		{
			m_numVisibleQuadrantsStreamed++;
		}
		void incNumInvisibleQuadrantsStreamed(void)
		{
			m_numInvisibleQuadrantsStreamed++;
		}
		void incNumTextureResize(void)
		{
			m_numTextureResizing++;
		}
		void addToTextureResizingElapsed(size_t elapsed)
		{
			m_elapsedTextureResizing += elapsed;
		}
		void incNumOptimizationLevelChange(void)
		{
			m_numOptimizationLevelChange++;
		}
		void incNumLoadTerrainData(size_t elapsedLoadTerrainData)
		{
			m_numLoadTerrainData++;
			m_elapsedLoadTerrainData += elapsedLoadTerrainData;
		}

		void incNumResetShaderParam(size_t elapsedResetShaderParam)
		{
			m_numResetShaderParam++;
			m_elapsedResetShaderParam += elapsedResetShaderParam;
		}
		void incNumUpdateShaderParam(size_t elapsedUpdateShaderParam)
		{
			m_numUpdateShaderParam++;
			m_elapsedUpdateShaderParam += elapsedUpdateShaderParam;
		}
		void incNumSetHiResCollisionData(size_t elapsedSetHiResCollisionData)
		{
			m_numSetHiResCollisionData++;
			m_elapsedSetHiResCollisionData += elapsedSetHiResCollisionData;
		}
		void incNumSetLowResCollisionData(size_t elapsedSetLowResCollisionData)
		{
			m_numSetLowResCollisionData++;
			m_elapsedSetLowResCollisionData += elapsedSetLowResCollisionData;
		}

		size_t addProcessStep(enum class ProcessStep::ProcessStepIdx idx)
		{
			return m_processSteps.addElapsedFromProcessStart(idx);
		}
		//void forceProcessCameraPosForQuadrants(void)
		//{
		//	m_lastProcessedCameraPosForQuadrants = godot::Vector3();
		//}
		void forceProcessWorldCameraPosForChunks(void)
		{
			m_lastProcessedWorldCameraPosForChunks = godot::Vector3();
		}
		bool processCanBeInterrupted(enum class ProcessStep::ProcessStepIdx idx)
		{
			size_t elapsed = addProcessStep(idx);
			if (m_processCanBeInterrupted)
			{
				if (elapsed > m_maxProcessDuration)
					return true;
				else
					return false;
			}
			else
				return false;
		}

		bool recalcGridOfQuadrantsInViewJustExecuted(void) const
		{
			return m_recalcGridOfQuadrantsInViewJustExecuted;
		}
		void recalcGridOfQuadrantsInViewJustExecuted(bool b)
		{
			if (b)
				m_recalcGridOfQuadrantsInViewJustExecuted = b;
			else
				m_recalcGridOfQuadrantsInViewJustExecuted = b;
		}

		bool recalcGridOfQuadrantsInViewNeeded(void) const
		{
			return m_recalcGridOfQuadrantsInViewNeeded;
		}
		void recalcGridOfQuadrantsInViewNeeded(bool b)
		{
			if (b)
				m_recalcGridOfQuadrantsInViewNeeded = b;
			else
				m_recalcGridOfQuadrantsInViewNeeded = b;
		}

		bool loadQuadrantData(float posX, float posZ, int level);
		bool loadQuadrantDataIndex(int posX, int posZ, int level);
		void clearTestQuadrantData(void);
		godot::Ref<godot::Image> getTestQuadrantHeightmapImage(void);
		godot::Ref<godot::Image> getTestQuadrantNormalmapImage(void);
		godot::Ref<godot::Image> getTestQuadrantSplatmapImage(void);
		godot::Ref<godot::Image> getTestQuadrantGlobalmapImage(void);
		godot::Ref<godot::Image> getTestQuadrantColormapImage(void);
		godot::Vector2 getQuadrantPos(void);
		godot::Vector2i getQuadrantPosIndex(void);
		godot::String getGroundTextureName(int type);
		int getGroundTextureIdx(godot::String& layout, godot::String texName);
		godot::Vector4 getGroundTextureIdxs(godot::String layput);

		godot::Transform3D globalToWorld3d(godot::Transform3D global) const;
		godot::Transform3D globalToWorld(godot::Transform3D global) const;
		godot::Transform2D globalToWorld2d(godot::Transform2D global);
		godot::Transform2D globalToWorld(godot::Transform2D global);
		godot::Transform3D globalPosToWorldTransform(godot::Vector3 global) const;
		godot::Vector3 globalPosToWorldPos3d(godot::Vector3 global) const;
		godot::Vector3 globalPosToWorldPos(godot::Vector3 global) const;
		godot::Vector2 globalPosToWorldPos2d(godot::Vector2 global);
		godot::Vector2 globalPosToWorldPos(godot::Vector2 global);
		godot::Transform3D worldToGlobal(godot::Transform3D world) const;
		godot::Transform3D worldPosToGlobalTransform(godot::Vector3 worldPos) const;
		godot::Vector3 worldPosToGlobalPos3d(godot::Vector3 world) const;
		godot::Vector3 worldPosToGlobalPos(godot::Vector3 world) const;
		godot::Vector2 worldPosToGlobalPos2d(godot::Vector2 worldPos);
		godot::Vector2 worldPosToGlobalPos(godot::Vector2 worldPos);

		bool streamingQuadrantsInProgress(void) const;
		void startNewStreamOfQuadrants(void);
		void endLastStreamOfQuadrants(void);

		void worldCameraTerrainChunk(Chunk* c);
		Chunk* worldCameraTerrainChunk(void);
		void mouseHitTerrainChunk(Chunk* c);
		Chunk* mouseHitTerrainChunk(void);

		bool allVisibleCollidersReady(void);
		bool allCollidersReady(void) const;
		void allCollidersReady(bool b);
		bool worldQuadrantsMapValid(void) const
		{
			return m_worldQuadrantsMapValid;
		}
		WorldQuadrantsMap& worldQuadrantsMapReference(void)
		{
			return m_worldQuadrantsMap;
		}

		bool finishedResizingGrid(void) const
		{
			return m_finishedResizingGrid;
		}
		void finishedResizingGrid(bool b)
		{
			if (b)
				m_finishedResizingGrid = b;
			else
				m_finishedResizingGrid = b;
		}

		void terrainHeightsChangedEvent(void);

		void prepareToCheckBlendNeeded(void);

		float shaderParamSeaFlatSeaHeightApplied(void) const
		{
			return m_shaderParamSeaFlatSeaHeightApplied;
		}
		void shaderParamSeaFlatSeaHeightApplied(float f)
		{
			m_shaderParamSeaFlatSeaHeightApplied = f;
			godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SEA_FLAT_SEA_HEIGHT, m_shaderParamSeaFlatSeaHeightApplied);
		}

		float shaderParamSeaNoiseScaleApplied(void) const
		{
			return m_shaderParamSeaNoiseScaleApplied;
		}
		void shaderParamSeaNoiseScaleApplied(float f)
		{
			m_shaderParamSeaNoiseScaleApplied = f;
			godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SEA_NOISE_SCALE, m_shaderParamSeaNoiseScaleApplied);
		}

		float shaderParamSeaTimeScaleApplied(void) const
		{
			return m_shaderParamSeaTimeScaleApplied;
		}
		void shaderParamSeaTimeScaleApplied(float f)
		{
			m_shaderParamSeaTimeScaleApplied = f;
			godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SEA_TIME_SCALE, m_shaderParamSeaTimeScaleApplied);
		}

		float shaderParamSeaHeightScaleApplied(void) const
		{
			return m_shaderParamSeaHeightScaleApplied;
		}
		void shaderParamSeaHeightScaleApplied(float f)
		{
			m_shaderParamSeaHeightScaleApplied = f;
			godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SEA_HEIGHT_SCALE, m_shaderParamSeaHeightScaleApplied);
		}

		float shaderParamSeaRoughFactorApplied(void) const
		{
			return m_shaderParamSeaRoughFactorApplied;
		}
		void shaderParamSeaRoughFactorApplied(float f)
		{
			m_shaderParamSeaRoughFactorApplied = f;
			godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SEA_ROUGH_FACTOR, m_shaderParamSeaRoughFactorApplied);
		}

		float shaderParamSeaEndCamAltForWavesApplied(void) const
		{
			return m_shaderParamSeaEndCamAltForWavesApplied;
		}
		void shaderParamSeaEndCamAltForWavesApplied(float f)
		{
			m_shaderParamSeaEndCamAltForWavesApplied = f;
			godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SEA_END_CAM_ALT_FOR_WAVES, m_shaderParamSeaEndCamAltForWavesApplied);
		}

		float shaderParamSeaStartCamAltForWavesAttenuationApplied(void) const
		{
			return m_shaderParamSeaStartCamAltForWavesAttenuationApplied;
		}
		void shaderParamSeaStartCamAltForWavesAttenuationApplied(float f)
		{
			m_shaderParamSeaStartCamAltForWavesAttenuationApplied = f;
			godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SEA_START_CAM_ALT_FOR_WAVES_ATTENUATION, m_shaderParamSeaStartCamAltForWavesAttenuationApplied);
		}

		float shaderParamSeaStartTerrDepthForWavesAttenuationApplied(void) const
		{
			return m_shaderParamSeaStartTerrDepthForWavesAttenuationApplied;
		}
		void shaderParamSeaStartTerrDepthForWavesAttenuationApplied(float f)
		{
			m_shaderParamSeaStartTerrDepthForWavesAttenuationApplied = f;
			godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SEA_START_TERR_DEPTH_FOR_WAVES_ATTENUATION, m_shaderParamSeaStartTerrDepthForWavesAttenuationApplied);
		}

		float shaderParamSeaScaleOffsetThresholdApplied(void) const
		{
			return m_shaderParamSeaScaleOffsetThresholdApplied;
		}
		void shaderParamSeaScaleOffsetThresholdApplied(float f)
		{
			m_shaderParamSeaScaleOffsetThresholdApplied = f;
			godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SEA_SCALE_OFFSET_THRESHOLD, m_shaderParamSeaScaleOffsetThresholdApplied);
		}

		bool shaderParamCameraUnderWaterApplied(void) const
		{
			return m_shaderParamCameraUnderWaterApplied;
		}
		void shaderParamCameraUnderWaterApplied(bool b)
		{
			//if (b)
			//	assert(b);
			//else
			//	assert(!b);

			if (!m_shaderParamCameraUnderWaterSet or b != m_shaderParamCameraUnderWaterApplied)
			{
				m_shaderParamCameraUnderWaterApplied = b;
				m_shaderParamCameraUnderWaterSet = true;
				float f = b ? 1.0f : 0.0f;
				//godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_CAM_UNDER_WATER, f);
				godot::Ref<godot::ShaderMaterial> mat = m_postProcessingUnderWaterEffectMesh->get_surface_override_material(0);
				if (mat.is_valid())
				{
					mat->set_shader_parameter(SHADER_PARAM_CAM_UNDER_WATER, f);
				}
			}
		}

		bool initialized(void) const
		{
			return m_initialized;
		}

		void setHudMessage(godot::String hudMessage, int savedTimeToFadeMessage);
		void hudMessage(std::string hudMessage, int64_t savedTimeToFadeMessage);
		//GDN_TheWorld_Map* getMap(void)
		//{
		//	//GDN_TheWorld_Map* map = nullptr;
		//	//GDN_TheWorld_MapViewer* mapViewer = MapViewer();
		//	//if (mapViewer != nullptr)
		//	//	map = mapViewer->getMap();
		//	//return map;
		//	return Map();
		//}

		int32_t getImageNumPixelsPerSize(void)
		{
			return m_mapQuadrantImagesMap.getImageNumPixelsPerSize();
		}

		//void eraseMapQuadrant(TheWorld_Utils::QuadrantPos& quadrantPos)
		//{
		//	std::lock_guard<std::recursive_mutex> lock(getMainProcessingMutex());
		//	m_mapQuadrantImagesMap.erase(quadrantPos);
		//}

		void forceProcessNewMapQuadrants(bool b)
		{
			m_forceProcessNewMapQuadrants = b;
		}
		bool forceProcessNewMapQuadrants(void) const
		{
			return m_forceProcessNewMapQuadrants;
		}

		void incUpdatedMapQuadrants(void)
		{
			m_updatedMapQuadrantNum++;
		}
		size_t fetchUpdatedMapQuadrants(void)
		{
			size_t s = m_updatedMapQuadrantNum;
			m_updatedMapQuadrantNum = 0;
			return s;
		}

		size_t getMapQuadrantNum(void) const
		{
			return m_mapQuadrantNum;
		}
		size_t getUpdatingMapQuadrantNum(void) const
		{
			return m_updatingMapQuadrantNum;
		}
		size_t getToShowMapQuadrantNum(void) const
		{
			return m_toShowMapQuadrantNum;
		}
		size_t getMapQuadrantOverrideNum(void) const
		{
			return m_mapQuadrantOverrideNum;
		}
		size_t getMapQuadrantEditedOverrideNum(void) const
		{
			return m_mapQuadrantEditedOverrideNum;
		}
		size_t getMapQuadrantColorOverrideNum(void) const
		{
			return m_mapQuadrantColorOverrideNum;
		}
		size_t getMapQuadrantTextureOverrideNum(void) const
		{
			return m_mapQuadrantTextureOverrideNum;
		}
		size_t getMapQuadrantRects(void) const
		{
			return m_mapQuadrantRects;
		}

		void pauseMouseMovement(void);
		void resumeMouseMovement(void);

		int getLevel() const
		{
			return m_worldViewerLevel;
		}
		
		float getQuadrantWorldSize(void)
		{
			float ret = 0;

			GDN_TheWorld_Map* map = Map();
			if (map != nullptr)
			{
				ret = map->getQuadrantWorldSize();
			}

			return ret;
		}
		float getQuadrantGlobalSize(void)
		{
			float ret = 0;

			GDN_TheWorld_Map* map = Map();
			if (map != nullptr)
			{
				ret = map->getQuadrantGlobalSize();
			}

			return ret;
		}
		int32_t getNumQuadrantsPerSize(void)
		{
			int32_t ret = 0;

			GDN_TheWorld_Map* map = Map();
			if (map != nullptr)
			{
				ret = map->getNumQuadrantsPerSize();
			}

			return ret;
		}

		bool worldStreamingInProgress(void) const
		{
			return m_streamingInProgress;
		}
		void worldStreamingInProgress(bool b)
		{
			m_streamingInProgress = b;
		}

		QuadTree* getCameraQuadrant(godot::Camera3D* camera = nullptr);
		bool isCameraQuadrantColliderSet(void);
		bool worldStreamingTerminated(void);
		bool worldStreamingTerminated(float& loadingProgress);
		float getloadingProgressScript(void);

		void uploadTextureRectInProgress(bool b)
		{
			m_uploadTextureRectInProgress = b;
		}
		void checkServerTextureRectInProgress(bool b)
		{
			m_checkServerTextureRectInProgress = b;
		}
		godot::Node3D* getAnchorForOriginShift(void);

		void worldQuadrantReady(TheWorld_Utils::QuadrantPos& pos);
		void worldQuadrantWorkInProgress(TheWorld_Utils::QuadrantPos& pos);

		bool serverGenerationInProgress(void) const
		{
			return m_serverGenerationInProgress;
		}
		void serverGenerationInProgress(bool b)
		{
			m_serverGenerationInProgress = b;
		}

		bool debugTrace(void) const
		{
			return m_debugTrace;
		}

		void ctrlPressed(bool b);

		bool ctrlPressed(void) const
		{
			return m_ctrlPressed;
		}

		//void resetDataRefreshToShaderFlags(void);
		//void requestDataRefreshToShader(QuadTree* quadTree, bool request = true);
		//size_t getNumQuadrantsToRefreshToShader(void) const;
		//void getQuadrantsToRefreshToShader(std::vector<QuadTree*>& quadrantsToRefresh) const;

		void incNumQuadrantNeedResetMaterialParam(void)
		{
			m_numQuadrantNeedResetMaterialParam++;
		}
		void decNumQuadrantNeedResetMaterialParam(void)
		{
			m_numQuadrantNeedResetMaterialParam--;
		}
		void incNumQuadrantNeedUpdateMaterialParam(void)
		{
			m_numQuadrantNeedUpdateMaterialParam++;
		}
		void decNumQuadrantNeedUpdateMaterialParam(void)
		{
			m_numQuadrantNeedUpdateMaterialParam--;
		}
		void incNumQuadrantLoadingFromLocalCache(void)
		{
			m_numQuadrantLoadingFromLocalCache++;
		}
		void decNumQuadrantLoadingFromLocalCache(void)
		{
			m_numQuadrantLoadingFromLocalCache--;
		}
		void incNumQuadrantLoadingFromServer(void)
		{
			m_numQuadrantLoadingFromServer++;
		}
		void decNumQuadrantLoadingFromServer(void)
		{
			m_numQuadrantLoadingFromServer--;
		}

		bool refreshRequired(void) const
		{
			return m_refreshRequired;
		}
		void refreshRequired(bool b)
		{
			m_refreshRequired = b;
		}
		bool isWorldQuadrantSelectedForEdit(void);
		bool isMapQuadrantSelectedForEdit(void);

	private:
		void doSelectWorldQuadrant(TheWorld_Utils::QuadrantPos& pos);
		void _doSelectWorldQuadrant(TheWorld_Utils::QuadrantPos& pos, QuadTree* quadTreeSel);
		void doDeselectWorldQuadrant(void);
		void doSelectMapQuadrant(TheWorld_Utils::QuadrantPos& pos);
		void doDeselectMapQuadrant(void);
		void _findChildNodes(godot::Array& foundNodes, godot::Array& searchNodes, String searchClass);
		void onTransformChanged(void);
		GDN_TheWorld_Camera* getCachedCustomWorldCamera(bool useCache = true)
		{ 
			return m_customWorldCamera;
		};
		void setCachedCustomWorldCamera(GDN_TheWorld_Camera* camera)
		{
			m_customWorldCamera = camera;
		};
		void printKeyboardMapping(void);
		void streamer(void);
		void process_mapModeInputEvents(void);
		void process_eraseOutOfViewQuads(GDN_TheWorld_Edit* editModeUIControl);
		void process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution(double _delta);
		bool process_updateGridDimension(GDN_TheWorld_Globals* globals, godot::Camera3D* activeWorldCamera);
		void process_updateWorldCameraQuadrant(GDN_TheWorld_Globals* globals, godot::Vector3& cameraWorldPos);
		void process_initNewQuads(bool processCameraPosForQuadrants);
		void process_updateCameraPosInShader(godot::Camera3D* activeWorldCamera);
		void process_updateHud(godot::Camera3D* activeWorldCamera);
		void process_updateOriginShift(godot::Camera2D* activeMapCamera, godot::Camera3D* activeWorldCamera);
		void setAnchorForOriginShift(godot::Node3D* anchorForOriginShift);
		void process_trackMouse(godot::Camera3D* activeWorldCamera);
		void process_castRayFromCamera(godot::Camera3D* activeWorldCamera);
		void process_updateLodInQuads(godot::Vector3& cameraPosInMap);
		void process_updateLodInQuads(godot::Vector3& cameraPosInMap, enum class QuadType type);
		void process_updateMeshForUpdatedChunks(void);
		void process_updateMeshForUpdatedChunks(enum class QuadType type);
		//void process_lookDev(void);
		void process_materialParams(godot::Vector3& worldCameraWorldPos, bool processCameraPosForQuadrants, bool processCameraPosForChunks);
		void process_neighbourQuadrantData(QuadTree* quadTree);
		void process_mapQuadrants(godot::Vector2 cameraWorldPos);
		void process_dump(GDN_TheWorld_Globals* globals);
		void process_statistics(GDN_TheWorld_Globals* globals);

		//void updateTerrainMaterial(godot::Vector3& worldCameraWorldPos, bool processCameraPosForQuadrants, bool processCameraPosForChunks);
		void writeStreamingNewQuadrantsStatistics(void);
		TheWorld_Utils::QuadrantPos getWorldQuadrantSelForEdit(QuadTree** quadTreeSel);
		bool initNewQuadsAtPriority(size_t numQuadrantsFromCameraQuadrant, size_t& numInitialized);
		bool setWorldCameraWorldPos(godot::Vector2 worldPos, float altitudeFromTerrain, bool altitudeIsAbsolute, godot::Transform3D worldCameraWorldTransform, bool overSea, bool adjustPositionOnTerrain = true);
		bool worldGridDimensionChanged(void) const
		{
			return m_gridDimensionChanged;
		}
		void worldGridDimensionChanged(bool b)
		{
			m_gridDimensionChanged = b;
		}

	private:
		bool m_initialized;
		godot::Camera3D* m_editorCamera;
		godot::Node3D* m_anchorForOriginShift = nullptr;
		bool m_allQuadrantsInitialized = false;
		bool m_allVisibleQuadrantsInitialized = false;
		bool m_forceUpdateTerrainMaterial = false;
		bool m_worldEntered = false;
		bool m_quitting;
		bool m_visibleInTree;
		bool m_initRequired;
		bool m_isInEditor;
		bool m_isInEditorInitialized;
		bool m_ready;
		bool m_useVisualServer;
		bool m_altPressed = false;
		bool m_ctrlPressed = false;
		bool m_shiftPressed = false;
		bool m_firstProcess;
		int64_t m_timeElapsedFromLastDump;
		bool m_initialWorldViewerPosSet;
		bool m_dumpRequired = false;
		bool m_dumpScheduled = false;
		Chunk* lastTrackedChunk;
		//Vector3 m_mapScaleVector;
		//enum class ShaderTerrainData::LookDev m_desiderdLookDev = ShaderTerrainData::LookDev::NotSet;
		//bool m_desideredLookDevChanged = false;
		bool m_stopDeploy;

		//std::map<TheWorld_Utils::QuadrantPos, bool> m_mapQuadrantToShader;	// Remarked function GDN_TheWorld_Viewer::updateTerrainMaterial
		//std::vector<QuadTree*> m_vecQuadrantToShader;
		//ShaderTerrainData::GridControlData m_gridControlData;					// Remarked function GDN_TheWorld_Viewer::updateTerrainMaterial

		//GDN_TheWorld_MapModder* m_mapModder = nullptr;

		// Statistics data
		//TheWorld_Viewer_Utils::TimerMcs m_processClock;
		size_t m_maxProcessDuration = 0;
		bool m_processCanBeInterrupted = false;
		bool m_lastProcessExecutionInterrupted = false;
		bool m_allCollidersReady = false;
		ProcessSteps m_processSteps;
		std::vector<ProcessStep> m_processStepsForDump;
		//std::vector<ProcessStep> m_processSteps;
		//std::string m_lastProcessedStep;
		size_t m_numProcessExecution = 0;				long long m_sumProcessDuration = 0;					size_t m_averageProcessDuration = 0;				size_t m_lastProcessDuration = 0;
		size_t m_numProcessImplExecution = 0;			long long m_sumProcessImplDuration = 0;				size_t m_averageProcessImplDuration = 0;			size_t m_lastProcessImplDuration = 0;
		//size_t m_numRefreshMapQuad = 0;				long long m_sumRefreshMapQuadDuration = 0;			size_t m_averageRefreshMapQuadDuration = 0;			size_t m_lastRefreshMapQuadDuration = 0;
		size_t m_numUpdateQuads1 = 0;					long long m_sumUpdateQuads1Duration = 0;			size_t m_averageUpdateQuads1Duration = 0;			size_t m_lastUpdateQuads1Duration = 0;
		size_t m_numUpdateQuads2 = 0;					long long m_sumUpdateQuads2Duration = 0;			size_t m_averageUpdateQuads2Duration = 0;			size_t m_lastUpdateQuads2Duration = 0;
		size_t m_numUpdateQuads3 = 0;					long long m_sumUpdateQuads3Duration = 0;			size_t m_averageUpdateQuads3Duration = 0;			size_t m_lastUpdateQuads3Duration = 0;
		size_t m_numUpdateChunks = 0;					long long m_sumUpdateChunksDuration = 0;			size_t m_averageUpdateChunksDuration = 0;			size_t m_lastUpdateChunksDuration = 0;
		size_t m_numUpdateMaterialParams = 0;			long long m_sumUpdateMaterialParamsDuration = 0;	size_t m_averageUpdateMaterialParamsDuration = 0;	size_t m_lastUpdateMaterialParamsDuration = 0;
		size_t m_numMouseTrackHit = 0;					long long m_sumMouseTrackHitDuration = 0;			size_t m_averageMouseTrackHitDuration = 0;			size_t m_lastMouseTrackHitDuration = 0;
	
		size_t m_numEraseQuads = 0;						long long m_sumEraseQuads = 0;						size_t m_averageEraseQuads = 0;						size_t m_lastEraseQuads = 0;
		size_t m_numProcessMapModder = 0;				long long m_sumProcessMapModder = 0;				size_t m_averageProcessMapModder = 0;				size_t m_lastProcessMapModder = 0;
		size_t m_numUpdateCameraQuad = 0;				long long m_sumUpdateCameraQuad = 0;				size_t m_averageUpdateCameraQuad = 0;				size_t m_lastUpdateCameraQuad = 0;
		size_t m_numStreamingQuadrantStuff = 0;			long long m_sumStreamingQuadrantStuff = 0;			size_t m_averageStreamingQuadrantStuff = 0;			size_t m_lastStreamingQuadrantStuff = 0;

		int64_t m_timeElapsedFromLastStatistic;
		int m_numSplits;
		int m_numJoins;
		int m_numChunks = 0;
		map<int, size_t> m_mapNumActiveChunksPerLod;
		int m_numActiveChunks;
		int m_numTerrainChunks = 0;
		map<int, size_t> m_mapNumActiveTerrainChunksPerLod;
		int m_numActiveTerrainChunks;
		int m_numSeaChunks = 0;
		map<int, size_t> m_mapNumActiveSeaChunksPerLod;
		int m_numActiveSeaChunks;
		int m_numProcessNotOwnsLock;
		int m_numQuadrantInView = 0;
		int m_numQuadrantInMemory = 0;
		int m_numInitializedQuadrant;
		int m_numToEraseQuadrant = 0;
		int m_numVisibleQuadrant;
		int m_numEmptyQuadrant;
		int m_numQuadrantLoadingFromServer = 0;
		int m_numQuadrantLoadingFromLocalCache = 0;
		int m_numFlushedQuadrant;
		int m_numQuadrantWithCollider = 0;
		int m_numQuadrantWithEnabledCollider = 0;
		int m_numQuadrantWithHiResCollider = 0;
		int m_numQuadrantWithLowResCollider = 0;
		int m_numInitializedVisibleQuadrant;
		int m_numQuadrantNeedResetMaterialParam = 0;
		int m_numQuadrantNeedUpdateMaterialParam = 0;

		float m_delayDegree = 0.0f;

		TimerMs m_streamingTime;
		TimerMs m_notStreamingTime;
		bool m_streamingInProgress = false;
		size_t m_numVisibleQuadrantsStreamed = 0;
		size_t m_numInvisibleQuadrantsStreamed = 0;
		size_t m_numTextureResizing = 0;
		size_t m_elapsedTextureResizing = 0;
		size_t m_numOptimizationLevelChange = 0;
		size_t m_numLoadTerrainData = 0;
		size_t m_elapsedLoadTerrainData = 0;
		size_t m_numResetShaderParam = 0;
		size_t m_elapsedResetShaderParam = 0;
		size_t m_numUpdateShaderParam = 0;
		size_t m_elapsedUpdateShaderParam = 0;
		size_t m_numSetHiResCollisionData = 0;
		size_t m_elapsedSetHiResCollisionData = 0;
		size_t m_numSetLowResCollisionData = 0;
		size_t m_elapsedSetLowResCollisionData = 0;

		size_t m_mapQuadrantNum = 0;
		size_t m_updatingMapQuadrantNum = 0;
		size_t m_toShowMapQuadrantNum = 0;
		size_t m_mapQuadrantOverrideNum = 0;
		size_t m_mapQuadrantEditedOverrideNum = 0;
		size_t m_mapQuadrantColorOverrideNum = 0;
		size_t m_mapQuadrantTextureOverrideNum = 0;
		size_t m_mapQuadrantRects = 0;
		size_t m_updatedMapQuadrantNum = 0;

		std::map<TheWorld_Utils::QuadrantPos, size_t> m_debugProcessedNeighbourQuadrant;
		// Statistics data

		TrackedInputEventQueue m_trackedInputEventsQueueForProcess;
		TrackedInputEventContainer m_trackedInputEventsForProcess;

		TrackedInputEventQueue m_trackedInputEventsQueueForTrackMouse;
		TrackedInputEventContainer m_trackedInputEventsForTrackMouse;

		bool m_adjustEnvironmentRequired = false;

		//bool m_mapGridOn = false;
		bool m_trackMouse = false;
		bool m_editMode = false;
		bool m_infoPanelVisible = false;
		int64_t m_timeElapsedFromLastQuadrandtInit = 0;
		int64_t m_timeElapsedFromLastUpdateGridDimension = 0;
		int64_t m_timeElapsedFromLastMouseTrack = 0;
		godot::Vector3 m_trackedMouseHitGlobalPos;
		godot::Vector3 m_trackedMouseHitWorldPos;
		godot::Vector3 m_previousTrackedMouseHitWorldPos;
		bool m_mouseTrackedOnSomething = false;
		bool m_mouseTrackedOnTerrain = false;
		std::string m_mouseQuadrantHitName;
		std::string m_mouseQuadrantHitTag;
		float m_mouseQuadrantHitSize;
		godot::Vector3 m_mouseQuadrantHitPos;
		TheWorld_Utils::QuadrantPos m_quadrantHitPos;
		QuadTree* m_quadrantHit = nullptr;
		std::list<QuadTree*> m_adjacentQuadrantsHit;
		TheWorld_Utils::QuadrantPos m_selectedMapQuadrantPos;
		TheWorld_Utils::QuadrantPos m_selectedWorldQuadrantPos;
		Chunk* m_mouseHitTerrainChunk = nullptr;
		GDN_TheWorld_Drawer* m_normalDrawer = nullptr;
		GDN_TheWorld_Drawer* m_positionDrawer = nullptr;
		GDN_TheWorld_Drawer* m_rayFromCameraDrawer = nullptr;
		class DebugRayToUpdateGridDimensionDrawer
		{
		public:
			enum class Side
			{
				NotSet = -1
				, North = 0
				, South = 1
				, West = 2
				, East = 3
			};
		public:
			DebugRayToUpdateGridDimensionDrawer(GDN_TheWorld_Viewer* _viewer, godot::Camera3D* _camera, enum class Side _side, float _angle, godot::Vector3 _rayStart, godot::Vector3 _rayEnd, bool _hit, godot::Vector3 _hitPosition, bool _maxDistanceFromCameraForSide = false);
			~DebugRayToUpdateGridDimensionDrawer(void);
			void setup(godot::Vector3 _rayStart, godot::Vector3 _rayEnd, bool _hit, godot::Vector3 _hitPosition, bool _maxDistanceFromCameraForSide = false);
			static std::string sideToString(enum class Side side);
		public:
			GDN_TheWorld_Viewer* viewer = nullptr;
			godot::Camera3D* camera = nullptr;
			enum class Side side = Side::NotSet;
			float angle = 0;
			godot::Vector3 rayStart;
			godot::Vector3 rayEnd;
			bool hit = false;
			godot::Vector3 hitPosition;
			GDN_TheWorld_Drawer* m_rayDrawer = nullptr;
			//int32_t rayIdx = -1;
			int32_t sphere = -1;
			int32_t label = -1;
			bool maxDistanceFromCameraForSide = false;
		};
		std::map<enum class DebugRayToUpdateGridDimensionDrawer::Side, std::map<float, std::unique_ptr<DebugRayToUpdateGridDimensionDrawer>>> m_debugRayToUpdateGridDimension;
		int32_t m_normalLineIdx = -1;
		int32_t m_normalSphereIdx = -1;
		int32_t m_labelPositionXIdx = -1;
		int32_t m_labelPositionYIdx = -1;
		int32_t m_labelPositionZIdx = -1;
		int32_t m_labelPositionDistIdx = -1;
		int32_t m_labelPositionDirIdx = -1;
		int32_t m_linePositionIdx = -1;
		int32_t m_spherePositionStartIdx = -1;
		int32_t m_spherePositionEndIdx = -1;
		int32_t m_lineRayFromCameraIdx = -1;
		int32_t m_sphereRayFromCameraStartIdx = -1;
		int32_t m_sphereRayFromCameraEndIdx = -1;
		bool m_normalVisible = false;
		GDN_TheWorld_Gizmo3d* m_gizmo = nullptr;
		bool m_gizmoVisible = false;
		bool m_showPositionOnCursorHit = false;
		bool m_highlightingHitQuadrant = false;
		bool m_trackingDistance = false;
		bool m_gridOn = false;
		bool m_castRayFromCamera = false;
		godot::Vector3 m_savedPosForTrackingDistance;

		bool m_debugContentVisibility;
		bool m_updateTerrainVisibilityRequired;
		
		enum class GDN_TheWorld_Globals::ChunkDebugMode m_currentChunkDebugMode;
		enum class GDN_TheWorld_Globals::ChunkDebugMode m_requiredChunkDebugMode;
		bool m_updateDebugModeRequired;
		
		Viewport::DebugDraw m_debugDraw;

		bool m_refreshRequired;

		godot::Size2 m_savedViewportSize;
		
		//RID m_viewFrustumMeshInstance;
		//RID m_viewFrustumMeshRID;
		//Ref<Mesh> m_viewFrustumMesh;

		// MeshCache
		std::unique_ptr<MeshCache> m_meshCache;

		// Viewer (Camera)
		GDN_TheWorld_Camera* m_customWorldCamera = nullptr;
		Chunk* m_worldCameraTerrainChunk = nullptr;
		bool m_cameraVerticalOnSeaSet = false;
		bool m_cameraVerticalOnSea = false;
		//QuadTree* m_cameraQuadTree;
		
		// World Data
		bool m_worldQuadrantsMapValid = false;
		WorldQuadrantsMap m_worldQuadrantsMap;
		//std::map <enum class QuadTree::QuadrantDepth, std::map<TheWorld_Utils::QuadrantPos, QuadTree*>> m_worldQuadrantsMapByDepth;
		std::map <size_t, std::map<TheWorld_Utils::QuadrantPos, QuadTree*>> m_worldQuadrantsMapByDistanceFromCameraQuadrant;
		TheWorld_Utils::QuadrantPos m_computedWorldCameraQuadrantPos;
		TheWorld_Utils::QuadrantPos m_gridWorldCameraQuadrantPos;
		WorldQuadrantsMap m_extraQuadsMap;
		MapQuadrantImagesMap m_mapQuadrantImagesMap;
		std::map < TheWorld_Utils::QuadrantPos, bool > m_mapQuadrantImagesToDeleteMap;
		std::recursive_mutex m_mtxQuadrantsAndMainProcessing;
		std::map<TheWorld_Utils::QuadrantPos, bool> m_toErase;
		TheWorld_Utils::QuadrantPos m_originShiftedQuadrantPos;
		godot::Vector3 m_lastOriginShiftCameraPosInMap;
		bool m_recalcGridOfQuadrantsInViewJustExecuted = false;
		bool m_recalcGridOfQuadrantsInViewNeeded = false;
		bool m_finishedResizingGrid = true;
		bool m_refreshQuadTreeMap = false;
		int m_numWorldVerticesPerSize;
		int m_worldViewerLevel;		// actually world viewer manage one level at the time, otherwise we should have multiple quadtrees
		bool m_gridDimensionChanged = true;
		int m_minVisibleQuadrantOnPerimeter = -1;
		int m_numVisibleQuadrantOnPerimeterOnNorthSide = -1;
		int m_numVisibleQuadrantOnPerimeterOnNorthSideApplied = -1;
		int m_numVisibleQuadrantOnPerimeterOnSouthSide = -1;
		int m_numVisibleQuadrantOnPerimeterOnSouthSideApplied = -1;
		int m_numVisibleQuadrantOnPerimeterOnWestSide = -1;
		int m_numVisibleQuadrantOnPerimeterOnWestSideApplied = -1;
		int m_numVisibleQuadrantOnPerimeterOnEastSide = -1;
		int m_numVisibleQuadrantOnPerimeterOnEastSideApplied = -1;
		int m_numCacheQuadrantOnPerimeter = -1;
		// Node cache
		GDN_TheWorld_Globals* m_globals = nullptr;
		GDN_TheWorld_Edit* m_editModeUIControl;
		//Ref<ShaderMaterial> m_regularMaterial;
		//Ref<ShaderMaterial> m_lookDevMaterial;
		
		// Shader parameters
		godot::Plane m_shaderParam_groundUVScale;
		bool m_shaderParam_depthBlending;
		bool m_shaderParam_triplanar;
		godot::Plane m_shaderParam_tileReduction;
		float m_shaderParam_globalmapBlendStart;
		float m_shaderParam_globalmapBlendDistance;
		float m_shaderParam_quad_border_line_thickness = 0.05f;	// 0.005f;
		float m_shaderParam_mouse_hit_radius = 0.001f;
		godot::Plane  m_shaderParam_colormapOpacity;
		bool m_shaderParamChanged = false;
		bool m_reloadAllShadersRequired = false;
		bool m_reloadSelectedQuadrantShaderRequired = false;

		// streamer thread
		std::thread m_streamerThread;
		bool m_streamerThreadRequiredExit;
		bool m_streamerThreadRunning;

		// Editor stuff
		godot::EditorInterface* m_editorInterface;
		godot::Control* m_editor3dOverlay;
		int m_minDepthQuadOnPerimeter = -1;
		int m_maxDepthQuadOnPerimeter = -1;
		int m_cacheQuadOnPerimeter = -1;
		bool m_forceShaderElevation0 = false;

		godot::Vector3 m_lastProcessedWorldCameraPosForQuadrants;
		Chunk* m_lastWorldCameraTerrainChunk = nullptr;
		int64_t m_timeElapsedFromLastProcessWorldCameraPosForQuadrants = 0;
		godot::Vector3 m_lastProcessedWorldCameraPosForChunks;

		godot::Vector2 m_lastProcessedCameraForMapQuadrants;
		bool m_forceProcessNewMapQuadrants = false;
		int64_t m_timeElapsedFromLastUpdatingQuadrantsCheck = 0;
		int32_t m_savedNumMapRectsInScreen = 0;

		float m_lastCameraAltitudeSetToShader = -1.0f;
		godot::Vector3 m_lastCameraPosSetToShader;
		bool m_lastCameraPosSetToShaderInitialized = false;

		TheWorld_Utils::ThreadPool m_tpClientWorker;

		bool m_quadrantSyncInProgress = false;
		bool m_uploadTextureRectInProgress = false;
		bool m_checkServerTextureRectInProgress = false;
		bool m_serverGenerationInProgress = false;

		float m_shaderParamSeaFlatSeaHeightApplied = 0;
		float m_shaderParamSeaNoiseScaleApplied = 0;
		float m_shaderParamSeaTimeScaleApplied = 0;
		float m_shaderParamSeaHeightScaleApplied = 0;
		float m_shaderParamSeaRoughFactorApplied = 0;
		float m_shaderParamSeaEndCamAltForWavesApplied = 0;
		float m_shaderParamSeaStartCamAltForWavesAttenuationApplied = 0;
		float m_shaderParamSeaStartTerrDepthForWavesAttenuationApplied = 0;
		float m_shaderParamSeaScaleOffsetThresholdApplied = 0;
		bool m_shaderParamCameraUnderWaterApplied = false;
		bool m_shaderParamCameraUnderWaterSet = false;

		GDN_TheWorld_HudControls* m_hudControls = nullptr;
		godot::HBoxContainer* m_hudMessageContainer = nullptr;
		godot::Label* m_hudMessageLabel = nullptr;
		uint64_t m_fadingTime = 0;
		uint64_t m_savedTimeToFadeHudMessage = 0;
		uint64_t m_savedTimeToFadeStreamingMessage = 0;
		bool m_streamingMessageOn = false;
		std::string m_hudMessage;
		godot::HBoxContainer* m_hudStreamingInfoContainer = nullptr;
		godot::Label* m_hudStreamingInfoLabel = nullptr;
		godot::SubViewportContainer* m_satelliteSubViewportContainer = nullptr;
		godot::Label* m_northLabel = nullptr;
		godot::Label* m_southLabel = nullptr;
		godot::Label* m_eastLabel = nullptr;
		godot::Label* m_westLabel = nullptr;
		godot::SubViewport* m_satelliteSubViewport = nullptr;
		godot::Camera3D* m_satelliteCamera = nullptr;
		godot::SubViewportContainer* m_backMirrorSubViewportContainer = nullptr;
		godot::SubViewport* m_backMirrorSubViewport = nullptr;
		godot::Camera3D* m_backMirrorCamera = nullptr;

		//GDN_TheWorld_WorldViewportContainer* m_worldViewportContainer = nullptr;
		//GDN_TheWorld_WorldViewport* m_worldViewport = nullptr;

		bool m_mapMode = false;
		bool m_adjustWorldCameraRequired = false;
		godot::Vector2 m_desideredCameraWorldPos;
		bool m_delayedWorldMode = false;
		GDN_TheWorld_MapViewer* m_mapViewer = nullptr;
		GDN_TheWorld_MapViewportContainer* m_mapViewportContainer = nullptr;
		GDN_TheWorld_MapViewport* m_mapViewport = nullptr;
		GDN_TheWorld_Map* m_map = nullptr;
		GDN_TheWorld_MapCamera* m_mapCamera = nullptr;
		float m_altitudeForToggleToWorldMode = 0;
		bool m_altitudeForToggleToWorldModeIsAbsolute = true;
		godot::Transform3D m_worldCameraWorldTransformForToggleToWorldMode;
		bool mapCameraGlobalPositionSet = false;
		godot::Vector2 m_mapCameraGlobalPos;

		//godot::CanvasItem* m_postProcessingUnderWaterEffectNode = nullptr;
		godot::MeshInstance3D* m_postProcessingUnderWaterEffectMesh = nullptr;

		// Loaded quadrant images (mostly for testing)
		TheWorld_Utils::QuadrantPos m_tempPos;
		TheWorld_Utils::TerrainEdit m_tempTerrainEdit;
		godot::Ref<godot::Image> m_testQuadrantHeightmapImage;
		godot::Ref<godot::Image> m_testQuadrantNormalmapImage;
		godot::Ref<godot::Image> m_testQuadrantSplatmapImage;
		godot::Ref<godot::Image> m_testQuadrantGlobalmapImage;
		godot::Ref<godot::Image> m_testQuadrantColormapImage;
		bool m_debugTrace = false;
		size_t m_num = 0;
		size_t m_loadingWorldRefresh = 0;
	};
}

//	/root
//		...											created in Godot editor
//			/TheWorld_Main							created in Godot editor
//				/GDN_TheWorld_MainNode				Only to startup the viewer and globals
//				/GDN_TheWorld_Globals
//				/GDN_TheWorld_Viewer
//				/GDN_TheWorld_Camera
//				/EditModeUI
