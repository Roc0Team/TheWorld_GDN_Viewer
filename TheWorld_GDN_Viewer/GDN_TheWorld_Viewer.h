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
		GDN_TheWorld_Viewer* m_viewer;
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
		void setMapData(godot::Ref<godot::Image> mapImage, TheWorld_Utils::QuadrantPos& quadrantPos, std::string& meshId);
		bool updated(void);

		void addUpdatedQuadrant(TheWorld_Utils::QuadrantPos& pos);
		TheWorld_Utils::QuadrantPos fetchUpdatedQuadrant(void);
		void fetchUpdatedQuadrant(TheWorld_Utils::QuadrantPos& pos);
		void fetchUpdatingQuadrant(TheWorld_Utils::QuadrantPos& pos);
		void erase(TheWorld_Utils::QuadrantPos& pos);

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
		GDN_TheWorld_Viewer* m_viewer;
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
#define TIME_INTERVAL_BETWEEN_DUMP 0			// secs, 0 = diasble periodic dump
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
				return m_overrideTerrainType == TheWorld_Utils::TerrainEdit::TerrainType::notset;
			}
			bool overrideTerrainTypeSet(void) const
			{
				return !overrideTerrainTypeNull();
			}
			bool overrideNull(void) const
			{
				if (overrideTerrainTypeNull() and not quadrantActuallyInWorld())
					return true;
				else
					return false;
			}

			void applyColor(void);
			void applyTexture(void);

		private:
			GDN_TheWorld_Map* m_map = nullptr;
			TheWorld_Utils::QuadrantPos m_quadrantPos;
			std::string m_name;

			bool m_quadrantActuallyInWorld = false;
			enum class QuadTree::QuadrantDepth m_quadrantDepth;
			int m_numQuadrantsFromCameraQuadrant = -1;
			bool m_quadrantReady = false;

			bool m_quadrantEdited = false;
			TheWorld_Utils::TerrainEdit m_terrainEdit;

			TheWorld_Utils::TerrainEdit::TerrainType m_overrideTerrainType = TheWorld_Utils::TerrainEdit::TerrainType::notset;
		};

		class MapQuadrantOverrideContainer : public std::map<TheWorld_Utils::QuadrantPos, std::unique_ptr<MapQuadrantOverride>>
		{
		};

		class MapTextureRect
		{
		public:
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

			private:
				MapTextureRect* m_mapTextureRect = nullptr;
				TheWorld_Utils::QuadrantPos m_quadrantPos;
			};

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
			
			MapTextureRect(GDN_TheWorld_Map* map, TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
			~MapTextureRect(void);

			void detachTextureResource(TheWorld_Utils::QuadrantPos& quadrantPos);
			void detachColorResource(TheWorld_Utils::QuadrantPos& quadrantPos);

			TheWorld_Utils::MapTextureRectPos pos(void) const
			{
				return m_mapTextureRectPos;
			}

			bool rectFull(void);
			bool setMapQuadrantData(const godot::Ref<godot::Image> mapImage, const TheWorld_Utils::QuadrantPos& quadrantPos);
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
			Status getStatus(void)
			{
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				long long elapsedFromStatusChange = (now - m_lastStatusChange).count();
				if (m_status == Status::CheckFromServerInProgress and elapsedFromStatusChange > 60000)
					m_status = Status::CheckFromServerRequired;
				return m_status;
			}
			void setStatus(Status status)
			{
				if (m_status != status)
				{
					TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
					m_lastStatusChange = now;
					m_status = status;
				}
			}
			void setInternalImageData(TheWorld_Utils::MemoryBuffer& imageBuffer);

			MapQuadrantColorOverride* getQuadrantColorOverride(TheWorld_Utils::QuadrantPos& quadrantPos, bool createIfMissing = false);
			MapQuadrantTextureOverride* getQuadrantTextureOverride(TheWorld_Utils::QuadrantPos& quadrantPos, bool createIfMissing = false);
			void releaseQuadrantColorOverride(TheWorld_Utils::QuadrantPos& quadrantPos);
			void releaseQuadrantTextureOverride(TheWorld_Utils::QuadrantPos& quadrantPos);

			GDN_TheWorld_Map* getMap(void) const
			{
				return m_map;
			}

		private:
			GDN_TheWorld_Map* m_map = nullptr;
			Status m_status = Status::NotSet;
			TheWorld_Viewer_Utils::MsTimePoint m_lastStatusChange;
			TheWorld_Utils::MapTextureRectPos m_mapTextureRectPos;
			std::vector<std::vector<godot::Ref<godot::Image>>> m_quadrantImages;	// bi-dimensional vector of Image references
			//std::filesystem::file_time_type m_fileTime;
			std::string m_fileTimeStr;		// es: 2023-09-04 19:33:24.7026392
			godot::TextureRect* m_internalTextureRect = nullptr;
			godot::PackedByteArray m_internalImageData;
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

	public:
		static int32_t numImagesForTexture(void);
		void init(GDN_TheWorld_MapViewer* mapViewer, godot::Vector2 containerSize);
		godot::Ref<godot::ShaderMaterial> getTextureRectMaterial(void) const;
		void setMapGridWorldCameraQuadrantPos(godot::Vector2 pos);
		void setMapGridNumVisibleQuadrantsOnNorthSide(int num);
		void setMapGridNumVisibleQuadrantsOnSouthSide(int num);
		void setMapGridNumVisibleQuadrantsOnWestSide(int num);
		void setMapGridNumVisibleQuadrantsOnEastSide(int num);
		void setMapGridNumCacheQuadrants(int num);
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

		godot::Vector2 globalPosToWorldPos(godot::Vector2 global);
		godot::Vector2 worldPosToGlobalPos(godot::Vector2 worldPos);

		MapTextureRect* getMapTextureRect(const TheWorld_Utils::QuadrantPos& quadrantPos, bool add = true);
		MapTextureRect* getMapTextureRect(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos, bool add = true);
		bool eraseMapTextureRect(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos);
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
		bool setMapQuadrantData(const godot::Ref<godot::Image> mapImage, const TheWorld_Utils::QuadrantPos& quadrantPos);
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
		int32_t numImagesForTextureRectSide(void) const
		{
			return s_numImagesForTexture;
		}

		int32_t getNumTextureRects(void);

		void addMapTextureRectRefreshScheduled(TheWorld_Utils::MapTextureRectPos& pos);

		void worldQuadrantMapReset(void);

		void quadrantReady(TheWorld_Utils::QuadrantPos& pos);
		void quadrantWorkInProgress(TheWorld_Utils::QuadrantPos& pos);

		MapTextureRect::MapQuadrantColorOverride* getQuadrantColorOverride(TheWorld_Utils::QuadrantPos& quadrantPos, bool createIfMissing = false);
		MapTextureRect::MapQuadrantTextureOverride* getQuadrantTextureOverride(TheWorld_Utils::QuadrantPos& quadrantPos, bool createIfMissing = false);
		void releaseQuadrantColorOverride(TheWorld_Utils::QuadrantPos& quadrantPos);
		void releaseQuadrantTextureOverride(TheWorld_Utils::QuadrantPos& quadrantPos);
		MapQuadrantOverride* getMapQuadrantOverride(TheWorld_Utils::QuadrantPos& quadrantPos);

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

		godot::Ref<godot::ShaderMaterial> m_textureRectMaterial;
		//godot::Ref<godot::ShaderMaterial> m_quadrantRectMaterial;

		bool m_mapGridOn = false;
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
			,initializedForNormals = 1
			,initializedForSplatmap = 2
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

		void initForNormals(QuadTree* quadtree, TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, size_t tileSize, size_t viewportBorder = 0, bool signalRequired = false);
		void initForSplatmap(QuadTree* quadtree, TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* normalsBuffer,
			float slopeVerticalFactor,
			float slopeFlatFactor,
			float dirtOnRocksFactor,
			float highElevationFactor,
			float lowElevationFactor,
			float minHeigth,
			float maxHeigth,
			size_t tileSize, size_t viewportBorder = 0, bool signalRequired = false);
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
		godot::Vector2 eastQuadrantheights(void) const
		{
			return m_eastQuadrantheights;
		}
		godot::Vector2 westQuadrantheights(void) const
		{
			return m_westQuadrantheights;
		}
		godot::Vector2 southQuadrantheights(void) const
		{
			return m_southQuadrantheights;
		}
		godot::Vector2 northQuadrantheights(void) const
		{
			return m_northQuadrantheights;
		}
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
		TheWorld_Utils::MemoryBuffer* m_float32HeigthsBuffer = nullptr;
		size_t m_numHeights = 0;
		TheWorld_Utils::MemoryBuffer* m_normalsBuffer = nullptr;
		size_t m_numNormals = 0;
		float m_slopeVerticalFactor = 0;
		float m_slopeFlatFactor = 0;
		float m_dirtOnRocksFactor = 0;
		float m_highElevationFactor = 0;
		float m_lowElevationFactor = 0;
		float m_minHeigth = 0;
		float m_maxHeigth = 0;
		godot::Vector2 m_eastQuadrantheights;
		godot::Vector2 m_westQuadrantheights;
		godot::Vector2 m_southQuadrantheights;
		godot::Vector2 m_northQuadrantheights;
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

		enum class TrackedInputEvent
		{
			MOUSE_BUTTON_LEFT = 0
			, MOUSE_BUTTON_RIGHT = 1
			, MOUSE_DOUBLE_BUTTON_LEFT = 2
			, MOUSE_DOUBLE_BUTTON_RIGHT = 3
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
			};
			ProcessStep(enum class ProcessStepIdx idx, size_t elapsed, int num = 1)
			{
				m_idx = idx;
				m_num = num;
				m_elapsed = elapsed;
			}
			void setElapsed(size_t elapsed)
			{
				m_elapsed = elapsed;
				m_num++;
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
				default:
					return "not_set";
				}
			}
			std::string name(void) const
			{
				return idxToString(m_idx);
			}
			std::string msg(void) const
			{
				return name() + (m_num > 1 ? "(" + std::to_string(m_num)  + ")" : "") + " = " + std::to_string(m_elapsed / 1000);
			}
			
		private:
			enum class ProcessStepIdx m_idx = ProcessStepIdx::not_set;
			int m_num = 0;
			size_t m_elapsed = 0;
		};

		class ProcessSteps
		{
		public:

			ProcessSteps() {}

			void init()
			{
				m_processClock.tick();
				m_steps.clear();
			}

			size_t addStep(enum class ProcessStep::ProcessStepIdx idx)
			{
				size_t elapsed = (size_t)m_processClock.partialDuration().count();
				if (m_steps.contains(idx))
					m_steps[idx].setElapsed(elapsed);
				else
					m_steps[idx] = ProcessStep(idx, elapsed);
				m_lastStepIdx = idx;
				return elapsed;
			}

			size_t elapsed()
			{
				return (size_t)m_processClock.partialDuration().count();
			}

			std::map<enum class ProcessStep::ProcessStepIdx, ProcessStep>& steps(void)
			{
				return m_steps;
			}

			std::string lastAddedStepName(void) const
			{
				return ProcessStep::idxToString(m_lastStepIdx);
			}

		private:
			std::map<enum class ProcessStep::ProcessStepIdx, ProcessStep> m_steps;
			TimerMcs m_processClock;
			enum class ProcessStep::ProcessStepIdx m_lastStepIdx = ProcessStep::ProcessStepIdx::not_set;
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

		void setDesideredLookDev(enum class ShaderTerrainData::LookDev desideredLookDev)
		{
			if (m_desiderdLookDev != desideredLookDev)
			{
				m_desiderdLookDev = desideredLookDev;
				m_desideredLookDevChanged = true;
			}
		}

		enum class ShaderTerrainData::LookDev getDesideredLookDev(void) const
		{
			return m_desiderdLookDev;
		}

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
		bool worldMode(void) const;
		void toggleMapView(void);
		void adjustWorldCamera(godot::Vector2 desideredCameraGlobalPos);
		void toggleMapGrid(void);
		void toggleTrackMouse(void);
		void toggleEditMode(void);
		void setEditMode(bool editMode);
		bool isEditMode(void) const;
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
		void toggleQuadrantSelected(void);
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
		GDN_TheWorld_Map* Map(bool useCache = true);
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
		bool recalcQuadrantsInView(void);
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
		int getNumQuadrants(void)
		{
			return getNumQuadrants2(false);
		}
		int getNumQuadrants2(bool recalcNow);
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
		int getNumEmptyQuadrants(void) const;
		int getNumQuadrantsReloadingFromCache(void) const;
		int getNumFlushedQuadrants(void) const;
		int getQuadrantNumWithCollider(void) const;
		int getQuadrantNumWithEnabledCollider(void) const;
		int getQuadrantNumWithHiResCollider(void) const;
		int getQuadrantNumWithLowResCollider(void) const;
		bool allVisibleQuadrantsInitialized(void);
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

		bool canProcessDataUpdate(void) const
		{
			return editMode();
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
		void clearQuadTreeMap(void);
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
		bool positionVisible(void) const
		{
			return m_positionVisible;
		}
		void positionVisible(bool b)
		{
			m_positionVisible = b;
			if (!m_positionVisible)
				trackingDistance(false);
		}
		bool gridOn(void) const
		{
			return m_gridOn;
		}
		void gridOn(bool b)
		{
			if (m_gridOn != b)
			{
				if (isQuadrantSelectedForEdit())
				{
					QuadTree* quadTreeSelectedForEdit = nullptr;
					TheWorld_Utils::QuadrantPos pos = getQuadrantSelForEdit(&quadTreeSelectedForEdit);
					quadTreeSelectedForEdit->materialParamsNeedUpdate(true);
				}

				if (!b)
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
		bool isInputEventTracked(TrackedInputEvent event, std::map<enum class TrackedInputEvent, bool>* inputEvents = nullptr)
		{
			if (inputEvents == nullptr)
				inputEvents = &m_trackedInputEventsForProcess;

			if (inputEvents->contains(event))
				return (*inputEvents)[event];
			else
				return false;
		}
		void compressTrackedInputEvents(std::map<enum class TrackedInputEvent, bool>* inputEvents = nullptr)
		{
			if (inputEvents == nullptr)
				inputEvents = &m_trackedInputEventsForProcess;

			if (!m_trackedInputEvents.empty())
			{
				for (auto& trackedInputEvent : m_trackedInputEvents)
				{
					(*inputEvents)[trackedInputEvent] = true;
				}
			}
		}
		void resetTrackedInputEvents(std::map<enum class TrackedInputEvent, bool>* inputEvents = nullptr)
		{
			if (inputEvents == nullptr)
				inputEvents = &m_trackedInputEventsForProcess;

			if (!inputEvents->empty())
				inputEvents->clear();
			if (!m_trackedInputEvents.empty())
				m_trackedInputEvents.clear();
		}

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
			return m_processSteps.addStep(idx);
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
			if (m_processCanBeInterrupted)
			{
				size_t elapsed = addProcessStep(idx);
				if (elapsed > m_maxProcessDuration)
					return true;
				else
					return false;
			}
			else
				return false;
		}

		bool recalcQuadrantsInViewJustExecuted(void) const
		{
			return m_recalcQuadrantsInViewJustExecuted;
		}
		void recalcQuadrantsInViewJustExecuted(bool b)
		{
			if (b)
				m_recalcQuadrantsInViewJustExecuted = b;
			else
				m_recalcQuadrantsInViewJustExecuted = b;
		}

		bool recalcQuadrantsInViewNeeded(void) const
		{
			return m_recalcQuadrantsInViewNeeded;
		}
		void recalcQuadrantsInViewNeeded(bool b)
		{
			if (b)
				m_recalcQuadrantsInViewNeeded = b;
			else
				m_recalcQuadrantsInViewNeeded = b;
		}

		bool loadQuadrantData(float posX, float posZ, int level);
		bool loadQuadrantDataIndex(int posX, int posZ, int level);
		void clearQuadrantData(void);
		godot::Ref<godot::Image> getQuadrantHeightmapImage(void);
		godot::Ref<godot::Image> getQuadrantNormalmapImage(void);
		godot::Ref<godot::Image> getQuadrantSplatmapImage(void);
		godot::Ref<godot::Image> getQuadrantGlobalmapImage(void);
		godot::Ref<godot::Image> getQuadrantColormapImage(void);
		godot::Vector2 getQuadrantPos(void);
		godot::Vector2 getQuadrantPosIndex(void);
		godot::String getGroundTextureName(int type);
		godot::Ref<godot::ImageTexture> getGroundTextureAlbedoBump(godot::String texName);
		godot::Ref<godot::ImageTexture> getGroundTextureNormalRoughness(godot::String texName);
		int getGroundTextureLayer(godot::String texName);
		godot::Vector4 getGroundTextureLayers(void);

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

		void startInitializingStreamOfQuadrants(void);
		void endInitializingStreamOfQuadrants(void);

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
				godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_CAM_UNDER_WATER, f);
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
			return m_mapQuadrantsMap.getImageNumPixelsPerSize();
		}

		void eraseMapQuadrant(TheWorld_Utils::QuadrantPos& quadrantPos)
		{
			std::lock_guard<std::recursive_mutex> lock(getMainProcessingMutex());
			m_mapQuadrantsMap.erase(quadrantPos);
		}

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
			m_updatedMapQuadrantsNum++;
		}
		size_t fetchUpdatedMapQuadrants(void)
		{
			size_t s = m_updatedMapQuadrantsNum;
			m_updatedMapQuadrantsNum = 0;
			return s;
		}

		size_t getMapQuadrantsNum(void) const
		{
			return m_mapQuadrantsNum;
		}
		size_t getUpdatingMapQuadrantsNum(void) const
		{
			return m_updatingMapQuadrantsNum;
		}
		size_t getToShowMapQuadrantsNum(void) const
		{
			return m_toShowMapQuadrantsNum;
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

	private:
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
		void process_lookDev(void);
		void process_materialParam(godot::Vector3& worldCameraWorldPos, bool processCameraPosForQuadrants, bool processCameraPosForChunks);
		void process_neighbourQuadrantData(QuadTree* quadTree);
		void process_mapQuadrants(godot::Vector2 cameraWorldPos);
		void process_dump(GDN_TheWorld_Globals* globals);
		void process_statistics(GDN_TheWorld_Globals* globals);

		void writeStatisticsNewQuadrants(void);
		TheWorld_Utils::QuadrantPos getQuadrantSelForEdit(QuadTree** quadTreeSel);
		bool isQuadrantSelectedForEdit(void);
		bool initNewQuadsAtPriority(size_t numQuadrantsFromCameraQuadrant, bool& allQuadrantInitialized, bool& allVisibleQuadrantInitialized);
		bool setWorldCameraWorldPos(godot::Vector2 worldPos, float altitudeFromTerrain, bool altitudeIsAbsolute, godot::Transform3D worldCameraWorldTransform, bool overSea);
		bool gridDimensionChanged(void) const
		{
			return m_gridDimensionChanged;
		}
		void gridDimensionChanged(bool b)
		{
			m_gridDimensionChanged = b;
		}

	private:
		bool m_initialized;
		godot::Camera3D* m_editorCamera;
		godot::Node3D* m_anchorForOriginShift = nullptr;
		bool m_worldEntered = false;
		bool m_quitting;
		bool m_visibleInTree;
		bool m_initRequired;
		bool m_isInEditor;
		bool m_isInEditorInitialized;
		bool m_ready;
		bool m_useVisualServer;
		bool m_altPressed;
		bool m_ctrlPressed;
		bool m_shiftPressed;
		bool m_firstProcess;
		int64_t m_timeElapsedFromLastDump;
		bool m_initialWorldViewerPosSet;
		bool m_dumpRequired;
		Chunk* lastTrackedChunk;
		//Vector3 m_mapScaleVector;
		enum class ShaderTerrainData::LookDev m_desiderdLookDev;
		bool m_desideredLookDevChanged;
		bool m_stopDeploy;

		//GDN_TheWorld_MapModder* m_mapModder = nullptr;

		// Statistics data
		//TheWorld_Viewer_Utils::TimerMcs m_processClock;
		size_t m_maxProcessDuration = 0;
		bool m_processCanBeInterrupted = false;
		bool m_lastProcessExecutionInterrupted = false;
		bool m_allCollidersReady = false;
		ProcessSteps m_processSteps;
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
		int m_numQuadrant;
		int m_numInitializedQuadrant;
		int m_numToEraseQuadrant;
		int m_numVisibleQuadrant;
		int m_numEmptyQuadrant;
		int m_numQuadrantReloadingFromCache = 0;
		int m_numFlushedQuadrant;
		int m_numQuadrantWithCollider = 0;
		int m_numQuadrantWithEnabledCollider = 0;
		int m_numQuadrantWithHiResCollider = 0;
		int m_numQuadrantWithLowResCollider = 0;
		int m_numInitializedVisibleQuadrant;
		int m_numQuadrantNeedResetMaterialParam = 0;
		
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

		size_t m_mapQuadrantsNum = 0;
		size_t m_updatingMapQuadrantsNum = 0;
		size_t m_toShowMapQuadrantsNum = 0;
		size_t m_mapQuadrantRects = 0;
		size_t m_updatedMapQuadrantsNum = 0;
		// Statistics data

		std::map<enum class TrackedInputEvent, bool> m_trackedInputEventsForProcess;
		std::map<enum class TrackedInputEvent, bool> m_trackedInputEventsForTrackMouse;
		std::list<enum class TrackedInputEvent> m_trackedInputEvents;

		//bool m_mapGridOn = false;
		bool m_trackMouse = false;
		bool m_editMode = false;
		int64_t m_timeElapsedFromLastQuadrandtInit = 0;
		int64_t m_timeElapsedFromLastUpdateGridDimension = 0;
		int64_t m_timeElapsedFromLastMouseTrack = 0;
		godot::Vector3 m_trackedMouseHitGlobalPos;
		godot::Vector3 m_trackedMouseHitWorldPos;
		godot::Vector3 m_previousTrackedMouseHitInMap;
		bool m_mouseTrackedOnSomething = false;
		bool m_mouseTrackedOnTerrain = false;
		std::string m_mouseQuadrantHitName;
		std::string m_mouseQuadrantHitTag;
		float m_mouseQuadrantHitSize;
		godot::Vector3 m_mouseQuadrantHitPos;
		TheWorld_Utils::QuadrantPos m_quadrantHitPos;
		QuadTree* m_quadrantHit = nullptr;
		std::list<QuadTree*> m_adjacentQuadrantsHit;
		TheWorld_Utils::QuadrantPos m_quadrantSelPos;
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
		bool m_positionVisible = false;
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
		bool m_cameraVerticalOnSea = false;
		//QuadTree* m_cameraQuadTree;
		
		// World Data
		bool m_worldQuadrantsMapValid = false;
		WorldQuadrantsMap m_worldQuadrantsMap;
		std::map <enum class QuadTree::QuadrantDepth, std::map<TheWorld_Utils::QuadrantPos, QuadTree*>> m_worldQuadrantsMapByDepth;
		WorldQuadrantsMap m_extraQuadsMap;
		MapQuadrantImagesMap m_mapQuadrantsMap;
		std::recursive_mutex m_mtxQuadrantsAndMainProcessing;
		std::map<TheWorld_Utils::QuadrantPos, bool> m_toErase;
		TheWorld_Utils::QuadrantPos m_computedWorldCameraQuadrantPos;
		TheWorld_Utils::QuadrantPos m_originShiftedQuadrantPos;
		godot::Vector3 m_lastOriginShiftCameraPosInMap;
		bool m_recalcQuadrantsInViewJustExecuted = false;
		bool m_recalcQuadrantsInViewNeeded = false;
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
		float m_shaderParam_quad_border_line_thickness = 0.005f;
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
		uint64_t m_savedTimeToFadeMessage = 0;
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

		godot::CanvasItem* m_postProcessingUnderWaterEffectNode = nullptr;

		// Loaded quadrant images (mostly for testing)
		TheWorld_Utils::QuadrantPos m_tempPos;
		TheWorld_Utils::TerrainEdit m_tempTerrainEdit;
		godot::Ref<godot::Image> m_tempHeightmapImage;
		godot::Ref<godot::Image> m_tempNormalmapImage;
		godot::Ref<godot::Image> m_tempSplatmapImage;
		godot::Ref<godot::Image> m_tempGlobalmapImage;
		godot::Ref<godot::Image> m_tempColormapImage;
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
