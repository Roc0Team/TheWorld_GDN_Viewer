#pragma once
#include "Viewer_Utils.h"

#pragma warning (push, 0)
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_Edit.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/check_button.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/style_box_empty.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#pragma warning (pop)

#include <WorldModifier.h>

namespace godot
{
	//class GDN_TheWorld_Viewer;

	class GDN_TheWorld_Edit : public godot::Container, public TheWorld_Utils::ThreadInitDeinit, public TheWorld_ClientServer::ClientCallback
	{
		GDCLASS(GDN_TheWorld_Edit, Container)

	public:

		class InnerData
		{
		public:
			class SelTexturePanel
			{
			public:
				class TextureItemPanel
				{
				public:
					TextureItemPanel()
					{
						m_panel = nullptr;
						m_idx = -1;
						m_selected = false;
					}
					~TextureItemPanel()
					{
					}

				public:
					godot::PanelContainer* m_panel;
					int32_t m_idx;
					bool m_selected;
				};

			public:
				SelTexturePanel()
				{
					m_openSelTexturesRequired = false;
					m_closeSelTexturesRequired = false;
					m_changeTextureRequired = false;
					m_wnd = nullptr;
				}
				~SelTexturePanel()
				{
				}

			public:
				bool m_openSelTexturesRequired;
				bool m_closeSelTexturesRequired;
				bool m_changeTextureRequired;
				std::string m_selectedTexSlotName;
				std::string m_selectedTexName;
				std::string m_newTexSlotName;
				std::string m_newTexName;
				godot::Window* m_wnd;
				std::map<std::string, std::unique_ptr<TextureItemPanel>> m_textureItems;
			};

		public:
			InnerData()
			{
				m_selTexturePanel = std::make_unique<SelTexturePanel>();
				m_mainPanelContainer = nullptr;
				m_mainTabContainer = nullptr;
				m_mainTerrainScrollContainer = nullptr;
				m_terrGenerationVBoxContainer = nullptr;
				m_terrGenerationButton = nullptr;
				m_quadrantTextureEditVBoxContainer = nullptr;
				m_quadrantTextureEditButton = nullptr;
				m_seed = nullptr;
				m_frequency = nullptr;
				m_fractalOctaves = nullptr;
				m_fractalLacunarity = nullptr;
				m_fractalGain = nullptr;
				m_fractalWeightedStrength = nullptr;
				m_fractalPingPongStrength = nullptr;
				m_amplitudeLabel = nullptr;
				m_scaleFactorLabel = nullptr;
				m_elapsedLabel = nullptr;
				m_elapsed1Label = nullptr;
				m_message = nullptr;
				m_terrTypeOptionButton = nullptr;
				m_lowElevationTexPanel = nullptr;
				m_highElevationTexPanel = nullptr;
				m_dirtTexPanel = nullptr;
				m_rocksTexPanel = nullptr;
				m_lowElevationTex = nullptr;
				m_highElevationTex = nullptr;
				m_dirtTex = nullptr;
				m_rocksTex = nullptr;
				m_lowElevationTexSelected = false;
				m_highElevationTexSelected = false;
				m_dirtTexSelected = false;
				m_rocksTexSelected = false;
				m_onGoing = false;
				m_mouseInsideEditPanel = false;
				m_slopeVerticalFactor = nullptr;
				m_slopeFlatFactor = nullptr;
				m_dirtOnRocksFactor = nullptr;
				//m_highElevationFactor = nullptr;
				//m_lowElevationFactor = nullptr;
				m_splatMapMode = nullptr;
			}
			~InnerData()
			{
			}

		public:
			godot::Ref<godot::StyleBoxFlat> m_styleBoxSelectedFrame;
			godot::Ref<godot::StyleBoxFlat> m_styleBoxHighlightedFrame;

			godot::Control* m_mainPanelContainer;
			godot::Control* m_mainTabContainer;
			godot::ScrollContainer* m_mainTerrainScrollContainer;
			godot::Control* m_mainScrolledVBoxContainer = nullptr;
			godot::Control* m_mainFixedVBoxContainer = nullptr;
			godot::Label* m_dummyLabel = nullptr;

			godot::Label* m_elapsedLabel;
			godot::Label* m_elapsed1Label;

			godot::Label* m_numQuadrantToSaveLabel = nullptr;
			godot::Label* m_numQuadrantToUploadLabel = nullptr;
			godot::Label* m_numQuadrantToBlendLabel = nullptr;
			godot::Label* m_numQuadrantLocked = nullptr;

			godot::Label* m_message;
			godot::String m_lastMessage;

			godot::Label* m_textureOverflowSizeLabel_1 = nullptr;
			godot::HSlider* m_textureOverflowSizeHSlider = nullptr;
			godot::Label* m_textureOverflowSizeLabel_2 = nullptr;

			//godot::CheckBox* m_allCheckBox = nullptr;
			//godot::CheckBox* m_collisonHiResGridCheckBox = nullptr;
			//godot::CheckBox* m_collisonLowResGridCheckBox = nullptr;
			godot::OptionButton* m_terrTypeOptionButton;
			godot::Button* m_generateTerrButton = nullptr;
			godot::CheckButton* m_terrTypeButton = nullptr;
			//godot::OptionButton* m_lookDevOptionButton = nullptr;

			godot::Button* m_serverInfoButton = nullptr;
			godot::Control* m_serverInfoVBoxContainer = nullptr;
			godot::Label* m_procgenHeader = nullptr;
			godot::Label* m_procgenStatus = nullptr;
			godot::Label* m_procgenClient = nullptr;
			godot::Label* m_procgenLevel = nullptr;
			godot::Label* m_procgenHeightsProgress = nullptr;
			godot::Label* m_procgenBlendProgress = nullptr;
			godot::Label* m_procgenMinMaxProgress = nullptr;
			godot::Label* m_procgenClientElabProgress = nullptr;
			TimerMs m_procgenClock;

			godot::Button* m_terrainInfoButton = nullptr;
			godot::Control* m_terrainInfoVBoxContainer = nullptr;
			godot::CheckBox* m_showGizmo = nullptr;
			godot::CheckBox* m_showNormal = nullptr;
			godot::CheckBox* m_showPosition = nullptr;
			godot::CheckBox* m_showTerrain = nullptr;
			godot::CheckBox* m_showSea = nullptr;
			godot::CheckBox* m_showFog = nullptr;
			godot::CheckBox* m_showChunks = nullptr;
			godot::CheckBox* m_showWireframe = nullptr;
			godot::CheckBox* m_highlightHitQuadrant = nullptr;
			godot::CheckBox* m_gridON = nullptr;
			godot::CheckBox* m_castRay = nullptr;
			godot::Label* m_mouseHitInfoLabel = nullptr;
			godot::Label* m_hitQuadInfoLabel = nullptr;
			godot::Label* m_selQuadInfoLabel = nullptr;
			godot::CheckBox* m_selQuadSeaCheckBox = nullptr;
			bool m_pressedViaCode = false;

			godot::Button* m_terrGenerationButton = nullptr;
			godot::Control* m_terrGenerationVBoxContainer = nullptr;
			godot::LineEdit* m_seed = nullptr;
			godot::LineEdit* m_frequency = nullptr;
			godot::LineEdit* m_fractalOctaves = nullptr;
			godot::LineEdit* m_fractalLacunarity = nullptr;
			godot::LineEdit* m_fractalGain = nullptr;
			godot::LineEdit* m_fractalWeightedStrength = nullptr;
			godot::LineEdit* m_fractalPingPongStrength = nullptr;
			godot::LineEdit* m_amplitudeBlendFactor = nullptr;
			godot::LineEdit* m_blendCycles = nullptr;
			godot::LineEdit* m_blendingBendingFactor = nullptr;
			godot::LineEdit* m_minRandomBendingExtension = nullptr;
			godot::LineEdit* m_maxRandomBendingExtension = nullptr;
			godot::LineEdit* m_amplitudeLabel = nullptr;
			godot::LineEdit* m_scaleFactorLabel = nullptr;
			godot::CheckBox* m_ignoreDesideredMinHeightLabel = nullptr;
			godot::LineEdit* m_desideredMinHeightLabel = nullptr;
			godot::LineEdit* m_postGenerateElevationLabel = nullptr;

			godot::Button* m_terrBlendingButton = nullptr;
			godot::Control* m_terrBlendingVBoxContainer = nullptr;
			//godot::CheckBox* m_blendAllCheckBox = nullptr;
			godot::CheckBox* m_checkBlend = nullptr;
			godot::CheckBox* m_fullCheckBlend = nullptr;
			godot::CheckBox* m_autoBlendCheckBox = nullptr;

			godot::Control* m_quadrantTextureEditVBoxContainer;
			godot::Button* m_quadrantTextureEditButton;

			godot::LineEdit* m_slopeVerticalFactor;
			godot::LineEdit* m_slopeFlatFactor;
			godot::LineEdit* m_dirtOnRocksFactor;
			//godot::LineEdit* m_highElevationFactor;
			//godot::LineEdit* m_lowElevationFactor;
			godot::LineEdit* m_splatMapMode;
			godot::LineEdit* m_groundLayout = nullptr;
			godot::PanelContainer* m_lowElevationTexPanel;
			godot::PanelContainer* m_highElevationTexPanel;
			godot::PanelContainer* m_dirtTexPanel;
			godot::PanelContainer* m_rocksTexPanel;
			godot::TextureRect* m_lowElevationTex;
			godot::TextureRect* m_highElevationTex;
			godot::TextureRect* m_dirtTex;
			godot::TextureRect* m_rocksTex;
			godot::Label* m_lowElevationTexName = nullptr;
			godot::Label* m_highElevationTexName = nullptr;
			godot::Label* m_dirtTexName = nullptr;
			godot::Label* m_rocksTexName = nullptr;
			godot::Label* m_lowElevationLayoutName = nullptr;
			godot::Label* m_highElevationLayoutName = nullptr;
			godot::Label* m_dirtLayoutName = nullptr;
			godot::Label* m_rocksLayoutName = nullptr;
			bool m_lowElevationTexSelected;
			bool m_highElevationTexSelected;
			bool m_dirtTexSelected;
			bool m_rocksTexSelected;

			QuadTree* m_quadTreeHit = nullptr;
			int m_lastOptimizationLevelDisplayed = -1;
			int m_lastHiResCollisionGridDisplayed = -1;
			std::string m_elapsed;
			bool m_onGoing;
			std::string m_minHeightStr;
			std::string m_maxHeightStr;
			//std::string m_canContainSeaStr;
			std::string m_counterStr;
			std::string m_note1Str;
			std::string m_note2Str;
			godot::Vector3 m_mouseHitPos;
			std::string m_mouseQuadHitStr;
			std::string m_mouseQuadHitPosStr;
			std::string m_mouseQuadSelStr;
			std::string m_mouseQuadSelPosStr;
			std::map<TheWorld_Utils::QuadrantPos, bool> m_mapQuadrantsToBlend;
			std::recursive_mutex m_mtxMapQuadrantsToBlend;
			std::map<TheWorld_Utils::QuadrantPos, bool> m_mapQuadToSave;
			std::recursive_mutex m_mtxMapQuadToSave;
			std::map<TheWorld_Utils::QuadrantPos, bool> m_mapQuadToDelayedBlend;
			std::recursive_mutex m_mtxMapQuadToDelayedBlend;

			std::unique_ptr<SelTexturePanel> m_selTexturePanel;

			bool m_mouseInsideEditPanel;

			std::map<size_t, std::map<int, TheWorld_Utils::MapManagerProceduralGenerationStatus>> m_serverProceGenStatusMap;
			bool m_waitingForServerGen = false;
			bool m_serverProceGenStatusMapChanged = false;

			enum class TheWorld_Utils::TerrainEdit::TerrainType m_selectedTerrainType = TheWorld_Utils::TerrainEdit::TerrainType::notset;

			std::map<TheWorld_Utils::TerrainEdit::TerrainType, godot::Ref<godot::Texture2D>> m_terrainTypeIcons;

			godot::Ref<godot::ImageTexture> m_dragCursorShape;
			godot::Vector2 m_dragCursorShapeSize;
		};

		enum class Margin {
			MARGIN_LEFT,
			MARGIN_TOP,
			MARGIN_RIGHT,
			MARGIN_BOTTOM
		};

	protected:
		static void _bind_methods();
		void _notification(int p_what);
		void _init(void);

	public:
		GDN_TheWorld_Edit();
		~GDN_TheWorld_Edit();
		void init(GDN_TheWorld_Viewer* viewer);
		void deinit(void);

		template <class T> T* createControl(godot::Node* parent, std::string name = "", godot::String text = "", std::string signal = "", godot::Object* callableObject = nullptr, std::string callableMethod = "", godot::Variant custom1 = "", godot::Variant custom2 = "", godot::Variant custom3 = "", Color selfModulateColor = Color(0.0f, 0.0f, 0.0f, 0.0f));
			
		virtual void threadInit(void) {}
		virtual void threadDeinit(void) {}

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _physics_process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;
		virtual void _unhandled_input(const Ref<InputEvent>& event) override;

		virtual void replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply);

		//
		// Test
		//
		//String hello(String target1, String target2, int target3)
		//{
		//	return String("Test, {0} {1} {2}!").format(Array::make(target1, target2, target3)); 
		//};

		bool initialized(void) const
		{
			return m_initialized;
		}
		godot::Window* initSelTexturePanel(void);
		void expandServerInfoPanel(void);
		void compressServerInfoPanel(void);
		void expandTerrainGenerationPanel(void);
		void compressTerrainGenerationPanel(void);
		void voidSignalManager(godot::Object* obj, godot::String signal, godot::String className, int instanceId, godot::String objectName, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3);
		void guiInputEvent(const godot::Ref<godot::InputEvent>& event, godot::Object* obj, godot::String signal, godot::String className, int instanceId, godot::String objectName, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3);
		void toggledSignalManager(const bool pressed, godot::Object* obj, godot::String signal, godot::String className, int instanceId, godot::String objectName, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3);
		void m_textureOverflowSizeHSliderValueChanged(double d);
		void controlNeedResize(void);
		void setSizeUI(void);
		void changeTextureAction(void);
		void editModeGenerateAction(void);
		void editModeSetTerrType(void);
		void editModeLockQuadrantAndNeighbours(TheWorld_Utils::QuadrantPos pos, std::map<TheWorld_Utils::QuadrantPos, std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder>>& lockedQuadrants, size_t maxDistance = 1);
		void editModeUnlockQuadrants(std::map<TheWorld_Utils::QuadrantPos, std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder>>& lockedQuadrants);
		//void editModeBlendQuadrantAndNeighbours(TheWorld_Utils::QuadrantPos pos, std::map<TheWorld_Utils::QuadrantPos, std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder>>& lockedQuadrants, size_t maxDistance = 1);
		bool blendAction(TheWorld_Utils::QuadrantPos blendingPos,
							bool sync,
							bool automaticBlending,
							bool saveAndUpload,
							bool updateUI,
							bool lockEditingTerrainData,
							bool& quadrantBusy,
							bool overrideBlendingParameters,
							bool unlockQuadrants,
							std::map<TheWorld_Utils::QuadrantPos, std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder>>& lockedQuadrants);
		bool checkBlend(void);
		bool autoBlendPermitted(void);
		void editModeBlendAction(void);
		void editModeSaveAction(void);
		void editModeUploadAction(void);
		void editModeStopAction(void);
		void editModeResetAction(void);
		void editModeReloadShader(void);
		void editModeSelectTerrainTypeAction(int32_t index);

		void editModeGenerate(void);
		void editModeBlend(void);
		void editModeSave(void);
		void editModeUpload(void);
		bool editModeUploadModifiedWorldQuadrants(GDN_TheWorld_Map* map);
		void editModeUploadMapQuadrantOverrides(GDN_TheWorld_Map* map);
		
		size_t numQuadrantsRequiringMapModding(void);

		void prepareHeightsForQuadrantBlend(Quadrant* quadrant, TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData* quadrantData, TheWorld_Utils::MemoryBuffer* terrainEditValuesBuffer, bool overrideBlendingParameters);
		void prepareHeightsForQuadrantBlend(std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> lockHolder, TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData* quadrantData, TheWorld_Utils::MemoryBuffer* terrainEditValuesBuffer, bool overrideBlendingParameters);
		//QuadTree* prepareHeightsForQuadrantBlend(bool* lockAcquired, TheWorld_Utils::QuadrantPos* pos, TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData* quadrantData, TheWorld_Utils::MemoryBuffer* terrainEditValuesBuffer);
		bool quadrantBlendPostOperations(TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData& quadrantData, Quadrant* quadrant, TheWorld_Utils::MemoryBuffer& terrainEditValuesBuffer);
		bool quadrantBlendPostOperations(TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData& quadrantData, std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> lockHolder, TheWorld_Utils::MemoryBuffer& terrainEditValuesBuffer);
		void manageSaveAndUploadAfterBlending(Quadrant* quadrant);

		godot::Ref<godot::Texture2D> getTextureByTerrainType(TheWorld_Utils::TerrainEdit::TerrainType terrainType);

		void setTerrainEditValues(TheWorld_Utils::TerrainEdit& terrainEdit, TheWorld_Utils::QuadrantPos quadrantPos, bool activateEditingMap = true);

		void editModeMouseEnteredEditPanel(void);
		void editModeMouseExitedEditPanel(void);
		void setUIAcceptFocus(bool b);
		bool UIAcceptingFocus(void) const;

		void resetPanelsOnQuadrantDeselection(void);
		
		void resetSeed(void);
		void setSeed(int seed);
		int seed(void);
		void resetFrequency(void);
		void setFrequency(float frequency);
		float frequency(void);
		void resetOctaves(void);
		void setOctaves(int octaves);
		int octaves(void);
		void resetLacunarity(void);
		void setLacunarity(float lacunarity);
		float lacunarity(void);
		void resetGain(void);
		void setGain(float gain);
		float gain(void);
		void resetWeightedStrength(void);
		void setWeightedStrength(float weightedStrength);
		float weightedStrength(void);
		void resetPingPongStrength(void);
		void setPingPongStrength(float pingPongStrength);
		float pingPongStrength(void);
		bool amplitudeBlendFactorValid(void);
		void resetAmplitudeBlendFactor(void);
		void setAmplitudeBlendFactor(float amplitudeBlendFactor);
		float amplitudeBlendFactor(void);
		void resetBlendingCycles(void);
		void setBlendingCycles(size_t blendingCycles);
		size_t blendingCycles(void);
		void resetBlendingBendingFactor(void);
		void setBlendingBendingFactor(float blendingBendingFactor);
		float blendingBendingFactor(void);
		void resetMinRandomBendingExtension(void);
		void setMinRandomBendingExtension(float minRandomBendingExtension);
		float minRandomBendingExtension(void);
		void resetMaxRandomBendingExtension(void);
		void setMaxRandomBendingExtension(float maxRandomBendingExtension);
		float maxRandomBendingExtension(void);
		void resetAmplitude(void);
		void setAmplitude(unsigned int amplitude);
		unsigned int amplitude(void);
		void resetScaleFactor(void);
		void setScaleFactor(float scaleFactor);
		float scaleFactor(void);
		void resetIgnoreDesideredMinHeight(void);
		void setIgnoreDesideredMinHeight(bool ignoreDesideredMinHeight);
		bool ignoreDesideredMinHeight(void);
		void resetDesideredMinHeight(void);
		void setDesideredMinHeight(float desideredMinHeight);
		float desideredMinHeight(void);
		void resetPostGenerateElevation(void);
		void setPostGenerateElevation(float postGenerateElevation);
		float postGenerateElevation(void);
		void setMinHeight(float minHeight);
		void setMaxHeight(float maxHeight);
		void resetSelectedTerrainType(void);
		void setSelectedTerrainType(enum class TheWorld_Utils::TerrainEdit::TerrainType terrainType);
		void setElapsed(size_t elapsed, bool onGoing);
		//size_t elapsed(void);
		void setCounter(size_t curr, size_t all);
		void setNote1(size_t num);
		void setNote2(std::string msg);
		void resetSlopeVerticalFactor(void);
		void setSlopeVerticalFactor(float value);
		float slopeVerticalFactor(void);
		void resetSlopeFlatFactor(void);
		void setSlopeFlatFactor(float value);
		float slopeFlatFactor(void);
		void resetDirtOnRocksFactor(void);
		void setDirtOnRocksFactor(float value);
		float dirtOnRocksFactor(void);
		void resetSplatMapMode(void);
		void setSplatMapMode(size_t value);
		size_t splatMapMode(void);
		void resetGroundLayout(void);
		void setGroundLayout(std::string layout);
		std::string groundLayout(void);
		void resetTerrainBoxTextures(void);

		bool actionInProgress(void) const
		{
			return m_actionInProgress;
		}

		QuadTree* getQuadrantHit(void) const;
		void setQuadrantHit(QuadTree* quadTree);
		void setMouseHitPos(godot::Vector3 mouseHitPos);
		void setMouseQuadHitLabelText(std::string text);
		void setMouseQuadHitPosLabelText(std::string text);
		void setMouseQuadSelLabelText(std::string text);
		void setMouseQuadSelPosLabelText(std::string text);

		void setNumQuadrantToSave(size_t num);
		void setNumQuadrantToUpload(size_t num, size_t numInMapView);
		void setQuadrantsToBlend(std::list<TheWorld_Utils::QuadrantPos> quadrantsToBlend);
		void setNumQuadrantLocked(size_t num);
		void getQuadrantsToBlend(std::list<TheWorld_Utils::QuadrantPos>& quadrantsToBlend);
		size_t getNumQuadrantsToBlend(void);

		void refreshNumToSaveUpload(size_t& numToSave, size_t& numToUpload, size_t& numToUploadInMapView, size_t& numLocked);

		void setMessage(std::string text, bool add = false);
		void setMessage(godot::String text, bool add = false);
		bool mouseInsideEditPanel(void)
		{
			return m_innerData->m_mouseInsideEditPanel;
		}

		void addToDelayedBlendQuads(TheWorld_Utils::QuadrantPos pos)
		{
			addToDelayedBlendQuadsRef(pos);
		}

		void addToDelayedBlendQuadsRef(TheWorld_Utils::QuadrantPos& pos)
		{
			std::lock_guard<std::recursive_mutex> lock(m_innerData->m_mtxMapQuadToDelayedBlend);
			m_innerData->m_mapQuadToDelayedBlend[pos] = true;
		}

		void removeQuadToDelayedBlend(TheWorld_Utils::QuadrantPos& pos)
		{
			std::lock_guard<std::recursive_mutex> lock(m_innerData->m_mtxMapQuadToDelayedBlend);
			if (m_innerData->m_mapQuadToDelayedBlend.contains(pos))
				m_innerData->m_mapQuadToDelayedBlend.erase(pos);
		}

		size_t numQuadsToDelayedBlend(void)
		{
			std::lock_guard<std::recursive_mutex> lock(m_innerData->m_mtxMapQuadToDelayedBlend);
			return m_innerData->m_mapQuadToDelayedBlend.size();
		}

		// Useless atm
		//TheWorld_Utils::QuadrantPos getFirstQuadToDelayedBlend(void)
		//{
		//	std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToDelayedBlend);
		//	return m_mapQuadToDelayedBlend.begin()->first;
		//}

		void addQuadrantsToBlend(std::vector<TheWorld_Utils::QuadrantPos>& quadsToBlend)
		{
			std::map<TheWorld_Utils::QuadrantPos, bool> _quadsToBlend;
			for (auto& item : quadsToBlend)
				_quadsToBlend[item] = true;
			std::lock_guard<std::recursive_mutex> lock(m_innerData->m_mtxMapQuadrantsToBlend);
			for (auto& item : m_innerData->m_mapQuadrantsToBlend)
				_quadsToBlend[item.first] = true;

			quadsToBlend.clear();
			for (auto& item : _quadsToBlend)
				quadsToBlend.push_back(item.first);
		}

		void addDelayedBlendQuads(std::vector<TheWorld_Utils::QuadrantPos>& quadsToBlend)
		{
			std::map<TheWorld_Utils::QuadrantPos, bool> _quadsToBlend;
			for (auto& item : quadsToBlend)
				_quadsToBlend[item] = true;
			std::lock_guard<std::recursive_mutex> lock(m_innerData->m_mtxMapQuadToDelayedBlend);
			for (auto& item : m_innerData->m_mapQuadToDelayedBlend)
				_quadsToBlend[item.first] = true;

			quadsToBlend.clear();
			for (auto& item : _quadsToBlend)
				quadsToBlend.push_back(item.first);
		}

		bool isDelayedBlend(TheWorld_Utils::QuadrantPos pos)
		{
			return isDelayedBlendRef(pos);
		}
			
		bool isDelayedBlendRef(TheWorld_Utils::QuadrantPos& pos)
		{
			std::lock_guard<std::recursive_mutex> lock(m_innerData->m_mtxMapQuadToDelayedBlend);
			if (m_innerData->m_mapQuadToDelayedBlend.contains(pos))
				return true;
			else
				return false;
		}

		void addQuadToSave(TheWorld_Utils::QuadrantPos& pos, bool manual = true)
		{
			std::lock_guard<std::recursive_mutex> lock(m_innerData->m_mtxMapQuadToSave);
			if (m_innerData->m_mapQuadToSave.contains(pos))
			{
				if (manual)
					if (!m_innerData->m_mapQuadToSave[pos] == manual)
						m_innerData->m_mapQuadToSave[pos] = manual;
			}
			else
				m_innerData->m_mapQuadToSave[pos] = manual;
		}

		void removeQuadToSave(TheWorld_Utils::QuadrantPos& pos)
		{
			std::lock_guard<std::recursive_mutex> lock(m_innerData->m_mtxMapQuadToSave);
			if (m_innerData->m_mapQuadToSave.contains(pos))
				m_innerData->m_mapQuadToSave.erase(pos);
		}
		
		bool isQuadToSave(TheWorld_Utils::QuadrantPos& pos, bool& manual)
		{
			std::lock_guard<std::recursive_mutex> lock(m_innerData->m_mtxMapQuadToSave);
			if (m_innerData->m_mapQuadToSave.contains(pos))
			{
				manual = m_innerData->m_mapQuadToSave[pos];
				return true;
			}
			else
			{
				return false;
			}
		}

		GDN_TheWorld_Globals* Globals(bool useCache = true)
		{
			if (m_viewer == nullptr)
				return nullptr;
			return m_viewer->Globals();
		}

		godot::Size2 getTerrainScrolledPanelSize(void);
		size_t getNumWorkingThread(void)
		{
			size_t maxThreads;
			return m_tp.getNumWorkingThreads(maxThreads);
		}
		void requestedQuit(void)
		{
			m_requestedThreadExit = true;
		}

		bool terrainDataLocked(void) const
		{
			return m_terrainDataLocked;
		}
		void terrainDataLocked(bool b)
		{
			m_terrainDataLocked = b;
		}

		void setServerProcGenStatus(TheWorld_Utils::MapManagerProceduralGenerationStatus& s);
		void resetServerProcGenStatus(void);
		void waitingForServerGen(bool b);
		bool waitingForServerGen(void) const
		{
			return m_innerData->m_waitingForServerGen;
		}

		void mapMode(bool b);

		bool editingMap(void);
		bool editingMap(TheWorld_Utils::TerrainEdit& terrainEdit);
		void editingMap(bool b);
		void setEditingMapMouseShape(bool b);
		bool getEditedTerrainEditValues(TheWorld_Utils::TerrainEdit& terrainEdit);
		void showTerrainPressed(bool b);
		void showSeaPressed(bool b);
		void showFogPressed(bool b);
		void showChunksPressed(bool b);
		void showWireframePressed(bool b);

		void holdBlending(bool b, bool force = false)
		{
			if (m_holdBlending != b or force)
			{
				m_holdBlending = b;
				godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_HOLD_BLENDING, m_holdBlending);
			}
		}
		bool holdBlending(void) const
		{
			return m_holdBlending;
		}

		bool uploadToServerInProgress(void) const;

	private:
		void blendExecution(std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> lockHolder,
			std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> northPosLockHolder,
			std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> southPosLockHolder,
			std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> westPosLockHolder,
			std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> eastPosLockHolder,
			std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> northwestPosLockHolder,
			std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> northeastPosLockHolder,
			std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> southwestPosLockHolder,
			std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> southeastPosLockHolder,
			bool updateUI,
			bool unlockEditingTerrainData,
			bool unlockQuadrants,
			bool saveAndUpload,
			bool overrideBlendingParameters);
		void blendQuadrant(TheWorld_Utils::QuadrantPos pos, std::map<TheWorld_Utils::QuadrantPos, std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder>>& lockedQuadrants, bool lastPhase);

	private:
		bool m_initialized;
		bool m_quitting;
		bool m_ready;
		GDN_TheWorld_Viewer* m_viewer = nullptr;
		std::recursive_mutex m_mtxUI;

		map<TheWorld_Utils::WorldModifierPos, std::unique_ptr<TheWorld_Utils::WorldModifier>> m_wms;
		
		bool m_editorUploadInProgress = false;
		bool m_uploadModifiedWorldQuadrantsInProgress = false;
		bool m_actionInProgress;
		bool m_actionStopRequested;
		TimerMs m_actionClock;
		TimerMs m_refreshUI;
		bool m_onGoingElapsedLabel;
		bool m_uploadMapQuadrantOverridesInProgress = false;
		size_t m_completedItems;
		size_t m_elapsedCompleted;
		size_t m_allItems;
		size_t m_lastElapsed;
		bool m_requiredUIAcceptFocus;
		bool m_UIAcceptingFocus;
		bool m_terrainDataLocked = false;

		std::unique_ptr<InnerData> m_innerData;

		bool m_controlNeedResize;
		bool m_scrollPanelsNeedResize;
		
		bool m_infoLabelTextChanged;
		bool m_messageLabelTextChanged = false;

		QuadTree* m_lastQuadTreeSel = nullptr;
		// filler
		//char filler[72];	// 72 OK (73 no) oppure 32 + 1 std::string (sembra che occupi 40 byte)
		// filler
		
		bool m_lastMessageChanged;

		TheWorld_Utils::ThreadPool m_tp;
		bool m_requestedThreadExit = false;

		godot::Vector2 m_size;

		bool m_holdBlending = false;
	};
}

