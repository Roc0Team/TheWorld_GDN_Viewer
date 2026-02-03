//#include "pch.h"

#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Edit.h"
#include "MeshCache.h"
#include "WorldModifier.h"
#include "Utils.h"
#include "FastNoiseLite.h"
#include "half.h"
#include "UnicodeDefine.h"
#include <typeinfo>

#pragma warning (push, 0)
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/style_box_empty.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/classes/plane_mesh.hpp>
#include <godot_cpp/classes/input.hpp>
#pragma warning (pop)

#include <string>
#include <iostream>

#define	SECTION_BUTTON_COLOR	GDN_TheWorld_Globals::g_color_pink_amaranth
#define	BUTTON_COLOR			GDN_TheWorld_Globals::g_color_yellow_amber

#define SERVER_INFO_BUTTON_TEXT L"Server Info"
#define TERRAIN_INFO_BUTTON_TEXT L"Terrain Info"
#define TERRAIN_GENERATION_BUTTON_TEXT L"Terrain Generation"
#define TERRAIN_EDIT_BUTTON_TEXT L"Quadrant Texture Edit"
#define TERRAIN_BLENDING_BUTTON_TEXT L"Terrain Blending"

using namespace godot;

const std::string c_lowElevationEditTextureName = "Low Elevation Texture";
const std::string c_highElevationEditTextureName = "High Elevation Texture";
const std::string c_dirtEditTextureName = "Dirt Texture";
const std::string c_rocksEditTextureName = "Rocks Texture";

void GDN_TheWorld_Edit::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("expand_server_info_panel"), &GDN_TheWorld_Edit::expandServerInfoPanel);
	ClassDB::bind_method(D_METHOD("edit_mode_generate"), &GDN_TheWorld_Edit::editModeGenerateAction);
	ClassDB::bind_method(D_METHOD("edit_mode_set_terr_type"), &GDN_TheWorld_Edit::editModeSetTerrType);
	ClassDB::bind_method(D_METHOD("edit_mode_blend"), &GDN_TheWorld_Edit::editModeBlendAction);
	ClassDB::bind_method(D_METHOD("edit_mode_reset_normals_splatmap"), &GDN_TheWorld_Edit::editModeResetNormalsSplatmapAction);
	//ClassDB::bind_method(D_METHOD("edit_mode_gen_normals"), &GDN_TheWorld_Edit::editModeGenNormalsAction);
	//ClassDB::bind_method(D_METHOD("edit_mode_apply_textures"), &GDN_TheWorld_Edit::editModeApplyTexturesAction);
	ClassDB::bind_method(D_METHOD("edit_mode_save"), &GDN_TheWorld_Edit::editModeSaveAction);
	ClassDB::bind_method(D_METHOD("edit_mode_upload"), &GDN_TheWorld_Edit::editModeUploadAction);
	ClassDB::bind_method(D_METHOD("edit_mode_stop"), &GDN_TheWorld_Edit::editModeStopAction);
	ClassDB::bind_method(D_METHOD("edit_mode_reload_shader"), &GDN_TheWorld_Edit::editModeReloadShader);
	ClassDB::bind_method(D_METHOD("edit_mode_sel_terr_type"), &GDN_TheWorld_Edit::editModeSelectTerrainTypeAction);
	ClassDB::bind_method(D_METHOD("edit_mode_sel_lookdev"), &GDN_TheWorld_Edit::editModeSelectLookDevAction);
	ClassDB::bind_method(D_METHOD("mouse_entered_edit_panel"), &GDN_TheWorld_Edit::editModeMouseEnteredEditPanel);
	ClassDB::bind_method(D_METHOD("mouse_exited_edit_panel"), &GDN_TheWorld_Edit::editModeMouseExitedEditPanel);
	ClassDB::bind_method(D_METHOD("control_need_resize"), &GDN_TheWorld_Edit::controlNeedResize);
	ClassDB::bind_method(D_METHOD("void_signal_manager", "p_object", "p_signal", "p_class_name", "p_instance_id", "p_object_name", "p_custom1", "p_custom2", "p_custom3"), &GDN_TheWorld_Edit::voidSignalManager);
	ClassDB::bind_method(D_METHOD("gui_input_event", "p_event, p_object", "p_signal", "p_class_name", "p_instance_id", "p_object_name", "p_custom1", "p_custom2", "p_custom3"), &GDN_TheWorld_Edit::guiInputEvent);
	ClassDB::bind_method(D_METHOD("toggled_signal_manager", "p_button_pressed", "p_object", "p_signal", "p_class_name", "p_instance_id", "p_object_name", "p_custom1", "p_custom2", "p_custom3"), &GDN_TheWorld_Edit::toggledSignalManager);
}

GDN_TheWorld_Edit::GDN_TheWorld_Edit()
{
	m_quitting = false;
	m_initialized = false;
	m_ready = false;
	m_actionInProgress = false;
	m_actionStopRequested = false;
	m_onGoingElapsedLabel = false;
	m_controlNeedResize = false;
	m_scrollPanelsNeedResize = false;
	m_infoLabelTextChanged = false;
	m_allItems = 0;
	m_completedItems = 0;
	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_UIAcceptingFocus = false;
	m_requiredUIAcceptFocus = false;
	m_lastMessageChanged = false;

	_init();
}

GDN_TheWorld_Edit::~GDN_TheWorld_Edit()
{
	deinit();
}

void GDN_TheWorld_Edit::replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply)
{
	std::string method = reply.getMethod();

	if (reply.error())
	{
		m_viewer->Globals()->setStatus(TheWorldStatus::error);
		m_viewer->Globals()->errorPrint((std::string("GDN_TheWorld_Edit::replyFromServer: error method ") + method + std::string(" rc=") + std::to_string(reply.getErrorCode()) + std::string(" ") + reply.getErrorMessage()).c_str());
		return;
	}

	try
	{
		if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER)
		{
			ClientServerVariant v = reply.getInputParam(0);
			const auto _lowerXGridVertex(std::get_if<float>(&v));
			if (_lowerXGridVertex == NULL)
			{
				std::string m = std::string("Reply MapManager::uploadCacheBuffer did not have a float as first input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float lowerXGridVertex = *_lowerXGridVertex;

			v = reply.getInputParam(1);
			const auto _lowerZGridVertex(std::get_if<float>(&v));
			if (_lowerZGridVertex == NULL)
			{
				std::string m = std::string("Reply MapManager::uploadCacheBuffer did not have a float as second input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float lowerZGridVertex = *_lowerZGridVertex;

			v = reply.getInputParam(2);
			const auto _numVerticesPerSize(std::get_if<int>(&v));
			if (_numVerticesPerSize == NULL)
			{
				std::string m = std::string("Reply MapManager::uploadCacheBuffer did not have an int as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			int numVerticesPerSize = *_numVerticesPerSize;

			v = reply.getInputParam(3);
			const auto _gridStepInWU(std::get_if<float>(&v));
			if (_gridStepInWU == NULL)
			{
				std::string m = std::string("Reply MapManager::uploadCacheBuffer did not have a float as forth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepInWU = *_gridStepInWU;

			v = reply.getInputParam(4);
			const auto _level(std::get_if<int>(&v));
			if (_level == NULL)
			{
				std::string m = std::string("Reply MapManager::uploadCacheBuffer did not have an int as fifth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			int level = *_level;

			v = reply.getReplyParam(0);
			const auto _refreshRequired(std::get_if<bool>(&v));
			if (_refreshRequired == NULL)
			{
				std::string m = std::string("Reply MapManager::uploadCacheBuffer did not have an int as fifth reply param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			bool refreshRequired = *_refreshRequired;

			TheWorld_Utils::QuadrantPos pos(lowerXGridVertex, lowerZGridVertex, level, numVerticesPerSize, gridStepInWU);
			QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
			if (quadTree != nullptr)
			{
				if (refreshRequired)
					quadTree->setStatus(QuadrantStatus::refreshTerrainDataNeeded);
				quadTree->getQuadrant()->setNeedUploadToServer(false);
			}

			m_completedItems++;
			size_t partialCount = m_actionClock.partialDuration().count();
			m_lastElapsed = partialCount - m_elapsedCompleted;
			m_elapsedCompleted = partialCount;
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADQUADOVERRIDES)
		{
			m_uploadMapQuadrantOverridesInProgress = false;
		}
		else
		{
			m_viewer->Globals()->setStatus(TheWorldStatus::error);
			m_viewer->Globals()->errorPrint((std::string("GDN_TheWorld_Edit::replyFromServer: unknow method ") + method).c_str());
		}
	}
	catch (GDN_TheWorld_Exception ex)
	{
		m_viewer->Globals()->setStatus(TheWorldStatus::error);
		m_viewer->Globals()->errorPrint(String(ex.exceptionName()) + String(" caught - ") + ex.what());
	}
	catch (...)
	{
		m_viewer->Globals()->setStatus(TheWorldStatus::error);
		m_viewer->Globals()->errorPrint("GDN_TheWorld_Globals::GDN_TheWorld_Edit: exception caught");
	}
}

void GDN_TheWorld_Edit::editModeMouseEnteredEditPanel(void)
{
	m_requiredUIAcceptFocus = true;
	//setUIAcceptFocus(true);
	if (m_innerData.get() != nullptr)
	{
		m_innerData->m_mouseInsideEditPanel = true;
	}
}

void GDN_TheWorld_Edit::editModeMouseExitedEditPanel(void)
{
	m_requiredUIAcceptFocus = false;
	//setUIAcceptFocus(false);
	if (m_innerData.get() != nullptr)
	{
		m_innerData->m_mouseInsideEditPanel = false;
	}
}

bool GDN_TheWorld_Edit::UIAcceptingFocus(void) const
{
	return m_UIAcceptingFocus;
}

void GDN_TheWorld_Edit::setUIAcceptFocus(bool b)
{
	if (m_UIAcceptingFocus == b)
		return;
		
	enum godot::Control::FocusMode focusMode = godot::Control::FocusMode::FOCUS_ALL;

	if (!b)
	{
		focusMode = godot::Control::FocusMode::FOCUS_NONE;
	}

	//if (b)
	//	setSeed(1);
	//else
	//	setSeed(0);

	//m_innerData->m_allCheckBox->set_focus_mode(focusMode);
	//m_innerData->m_collisonHiResGridCheckBox->set_focus_mode(focusMode);
	//m_innerData->m_collisonLowResGridCheckBox->set_focus_mode(focusMode);
	m_innerData->m_seed->set_focus_mode(focusMode);
	m_innerData->m_frequency->set_focus_mode(focusMode);
	m_innerData->m_fractalOctaves->set_focus_mode(focusMode);
	m_innerData->m_fractalLacunarity->set_focus_mode(focusMode);
	m_innerData->m_fractalGain->set_focus_mode(focusMode);
	m_innerData->m_amplitudeLabel->set_focus_mode(focusMode);
	m_innerData->m_scaleFactorLabel->set_focus_mode(focusMode);
	m_innerData->m_ignoreDesideredMinHeightLabel->set_focus_mode(focusMode);
	m_innerData->m_desideredMinHeightLabel->set_focus_mode(focusMode);
	m_innerData->m_postGenerateElevationLabel->set_focus_mode(focusMode);
	m_innerData->m_fractalWeightedStrength->set_focus_mode(focusMode);
	m_innerData->m_fractalPingPongStrength->set_focus_mode(focusMode);
	m_innerData->m_amplitudeBlendFactor->set_focus_mode(focusMode);
	m_innerData->m_blendCycles->set_focus_mode(focusMode);
	m_innerData->m_blendingBendingFactor->set_focus_mode(focusMode);
	m_innerData->m_minRandomBendingExtension->set_focus_mode(focusMode);
	m_innerData->m_maxRandomBendingExtension->set_focus_mode(focusMode);
	m_innerData->m_slopeVerticalFactor->set_focus_mode(focusMode);
	m_innerData->m_slopeFlatFactor->set_focus_mode(focusMode);
	m_innerData->m_dirtOnRocksFactor->set_focus_mode(focusMode);
	m_innerData->m_highElevationFactor->set_focus_mode(focusMode);
	m_innerData->m_lowElevationFactor->set_focus_mode(focusMode);
	m_innerData->m_splatMapMode->set_focus_mode(focusMode);
	m_innerData->m_groundLayout->set_focus_mode(focusMode);
	m_innerData->m_showGizmo->set_focus_mode(focusMode);
	m_innerData->m_showNormal->set_focus_mode(focusMode);

	m_UIAcceptingFocus = b;
}

void GDN_TheWorld_Edit::setServerProcGenStatus(TheWorld_Utils::MapManagerProceduralGenerationStatus& s)
{
	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
	m_innerData->m_serverProceGenStatusMap[s.m_numVerticesPerSize][s.m_level] = s;
	m_innerData->m_serverProceGenStatusMapChanged = true;
}

void GDN_TheWorld_Edit::resetServerProcGenStatus(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
	m_innerData->m_serverProceGenStatusMap.clear();
	m_innerData->m_serverProceGenStatusMapChanged = true;
}

void GDN_TheWorld_Edit::waitingForServerGen(bool b)
{
	m_innerData->m_waitingForServerGen = b;
	m_innerData->m_serverProceGenStatusMapChanged = true;
}

void GDN_TheWorld_Edit::mapMode(bool b)
{
	if (b)
	{
		if (m_innerData->m_terrTypeButton != nullptr)
		{
			m_innerData->m_terrTypeButton->set_visible(true);
			m_scrollPanelsNeedResize = true;
		}
	}
	else
	{
		if (m_innerData->m_terrTypeButton != nullptr)
		{
			m_innerData->m_terrTypeButton->set_visible(false);
			m_scrollPanelsNeedResize = true;
		}
	}

	//if (m_innerData->m_terrTypeButton != nullptr)
	//{
	//	editingMap(false);
	//}
}

bool GDN_TheWorld_Edit::editingMap(void)
{
	if (m_innerData->m_terrTypeButton != nullptr and m_innerData->m_terrTypeButton->is_pressed())
		return true;
	else
		return false;
}

bool GDN_TheWorld_Edit::editingMap(TheWorld_Utils::TerrainEdit& terrainEdit)
{
	if (editingMap() and getEditedTerrainEditValues(terrainEdit))
		return true;
	else
		return false;
}

void GDN_TheWorld_Edit::showTerrainPressed(bool b)
{
	if (m_innerData->m_showTerrain == nullptr)
		return;

	if (m_innerData->m_showTerrain->is_pressed() != b)
		m_innerData->m_showTerrain->set_pressed(b);
}

void GDN_TheWorld_Edit::showSeaPressed(bool b)
{
	if (m_innerData->m_showSea == nullptr)
		return;

	if (m_innerData->m_showSea->is_pressed() != b)
		m_innerData->m_showSea->set_pressed(b);
}

void GDN_TheWorld_Edit::showFogPressed(bool b)
{
	if (m_innerData->m_showFog == nullptr)
		return;

	if (m_innerData->m_showFog->is_pressed() != b)
		m_innerData->m_showFog->set_pressed(b);
}

void GDN_TheWorld_Edit::showChunksPressed(bool b)
{
	if (m_innerData->m_showChunks == nullptr)
		return;

	if (m_innerData->m_showChunks->is_pressed() != b)
		m_innerData->m_showChunks->set_pressed(b);
}

void GDN_TheWorld_Edit::showWireframePressed(bool b)
{
	if (m_innerData->m_showWireframe == nullptr)
		return;

	if (m_innerData->m_showWireframe->is_pressed() != b)
		m_innerData->m_showWireframe->set_pressed(b);
}

void GDN_TheWorld_Edit::editingMap(bool b)
{
	if (m_innerData->m_terrTypeButton == nullptr)
		return;

	if (b)
	{
		m_innerData->m_terrTypeButton->set_pressed(true);
	}
	else
	{
		m_innerData->m_terrTypeButton->set_pressed(false);
	}
}

void GDN_TheWorld_Edit::setEditingMapMouseShape(bool b)
{
	if (b)
	{
		if (m_innerData->m_dragCursorShape != nullptr)
			godot::Input::get_singleton()->set_custom_mouse_cursor(m_innerData->m_dragCursorShape, godot::Input::CursorShape::CURSOR_ARROW, m_innerData->m_dragCursorShapeSize / 2);
	}
	else
	{
		godot::Input::get_singleton()->set_custom_mouse_cursor(nullptr);
	}
}

void GDN_TheWorld_Edit::init(GDN_TheWorld_Viewer* viewer)
{
	m_innerData = std::make_unique<InnerData>();

	m_viewer = viewer;
	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	holdBlending(false, true);

	//TheWorld_Utils::WorldModifierPos pos(0, 4096, 4096, TheWorld_Utils::WMType::elevator, 0);
	//m_wms[pos] = std::make_unique<TheWorld_Utils::WorldModifier>(pos, TheWorld_Utils::WMFunctionType::ConsiderMinMax, 10000.0f, 0.0f, 200.0f, TheWorld_Utils::WMOrder::MaxEffectOnWM);

	godot::ResourceLoader* resLoader = godot::ResourceLoader::get_singleton();
	
	{
		godot::Ref<godot::Texture2D> tex = resLoader->load("res://addons/twviewer/assets/cursors/grab.png");
		if (tex != nullptr)
		{
			godot::Ref<godot::Image> image = tex->get_image();
			image->resize(16, 16);
			godot::Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
			m_innerData->m_dragCursorShape = tex;
			m_innerData->m_dragCursorShapeSize = m_innerData->m_dragCursorShape->get_size();
		}
		//m_innerData->m_dragCursorShape = resLoader->load("res://addons/twviewer/assets/cursors/grab.png");
		//if (m_innerData->m_dragCursorShape != nullptr)
		//{
		//	m_innerData->m_dragCursorShapeSize = m_innerData->m_dragCursorShape->get_size();
		//}
	}

	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());

	{
		for (int terrainTypeInt = minTerrainTypeValue; terrainTypeInt <= maxTerrainTypeValue; terrainTypeInt++)
		{
			TheWorld_Utils::TerrainEdit::TerrainType terrainType = static_cast<TheWorld_Utils::TerrainEdit::TerrainType>(terrainTypeInt);
			std::string iconName = TheWorld_Utils::TerrainEdit::getIconNameForTerrainType(terrainType);
			if (iconName.size() > 0)
			{
				std::string path = "res://addons/twviewer/assets/terrain_icons/" + iconName;
				m_innerData->m_terrainTypeIcons[terrainType] = resLoader->load(path.c_str());
			}

		}
	}
	
	int32_t border_size = 2;
	{
		godot::Ref<godot::StyleBoxFlat> styleBox = memnew(godot::StyleBoxFlat);
		m_innerData->m_styleBoxSelectedFrame = styleBox;
		Color c = m_innerData->m_styleBoxSelectedFrame->get_bg_color();
		c.a = 0.0f;
		m_innerData->m_styleBoxSelectedFrame->set_bg_color(c);
		m_innerData->m_styleBoxSelectedFrame->set_border_color(GDN_TheWorld_Globals::g_color_yellow_amber);
		m_innerData->m_styleBoxSelectedFrame->set_border_width(godot::Side::SIDE_TOP, border_size);
		m_innerData->m_styleBoxSelectedFrame->set_border_width(godot::Side::SIDE_BOTTOM, border_size);
		m_innerData->m_styleBoxSelectedFrame->set_border_width(godot::Side::SIDE_RIGHT, border_size);
		m_innerData->m_styleBoxSelectedFrame->set_border_width(godot::Side::SIDE_LEFT, border_size);
	}
	{
		godot::Ref<godot::StyleBoxFlat> styleBox = memnew(godot::StyleBoxFlat);
		m_innerData->m_styleBoxHighlightedFrame = styleBox;
		Color c = m_innerData->m_styleBoxHighlightedFrame->get_bg_color();
		c.a = 0.0f;
		m_innerData->m_styleBoxHighlightedFrame->set_bg_color(c);
		m_innerData->m_styleBoxHighlightedFrame->set_border_color(GDN_TheWorld_Globals::g_color_white);
		m_innerData->m_styleBoxHighlightedFrame->set_border_width(godot::Side::SIDE_TOP, border_size);
		m_innerData->m_styleBoxHighlightedFrame->set_border_width(godot::Side::SIDE_BOTTOM, border_size);
		m_innerData->m_styleBoxHighlightedFrame->set_border_width(godot::Side::SIDE_RIGHT, border_size);
		m_innerData->m_styleBoxHighlightedFrame->set_border_width(godot::Side::SIDE_LEFT, border_size);
	}

	godot::Node* parent = m_viewer->get_parent();
	if (parent != nullptr)
		parent->add_child(this);
	else
		m_viewer->add_child(this);
	set_name(THEWORLD_EDIT_MODE_UI_CONTROL_NAME);

	//godot::CheckBox* test1 = memnew(godot::CheckBox);
	//parent->add_child(test1);
	//test1->set_text("TEXT");

	//godot::Button* test2 = memnew(godot::Button);
	//parent->add_child(test2);
	//test2->set_text("TEXT");

	m_mapModder = memnew(GDN_TheWorld_MapModder);
	m_mapModder->custom_ready(m_viewer, this, false);

	//const Color self_modulate = get_self_modulate();
	//const Color self_modulate_to_transparency(1.0f, 1.0f, 1.0f, 0.5f);
	const Color self_modulate_to_transparency(0.0f, 0.0f, 0.0f, 0.5f);
	//const Color self_modulate(1.0f, 1.0f, 1.0f, 1.0f);

	godot::HBoxContainer* hBoxContainer = nullptr;
	godot::VBoxContainer* vBoxContainer = nullptr;
	godot::Control* separator = nullptr;
	godot::Button* button = nullptr;
	godot::Label* label = nullptr;
	godot::HSplitContainer* hSplitContainer = nullptr;
	godot::LineEdit* lineEdit = nullptr;
	godot::Error e;

	m_innerData->m_mainPanelContainer = createControl<godot::PanelContainer>(this, "");

	m_innerData->m_mainTabContainer = createControl<godot::TabContainer>(m_innerData->m_mainPanelContainer, "", "", "", nullptr, "", "", "", "", self_modulate_to_transparency);

	godot::MarginContainer* marginContainer = nullptr;
	marginContainer = createControl<godot::MarginContainer>(m_innerData->m_mainTabContainer, "Terrain");
	if (!IS_EDITOR_HINT())
	{
		marginContainer->add_theme_constant_override("margin_left", 5);
		marginContainer->add_theme_constant_override("margin_right", 5);
	}

	vBoxContainer = createControl<godot::VBoxContainer>(marginContainer);

	m_innerData->m_mainFixedVBoxContainer = createControl<godot::VBoxContainer>(vBoxContainer);

	{
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainFixedVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Quads to save");
		m_innerData->m_numQuadrantToSaveLabel = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_numQuadrantToSaveLabel->set_horizontal_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);
		label = createControl<godot::Label>(hBoxContainer, "", "To upload");
		m_innerData->m_numQuadrantToUploadLabel = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_numQuadrantToUploadLabel->set_horizontal_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);
		label = createControl<godot::Label>(hBoxContainer, "", "To blend");
		m_innerData->m_numQuadrantToBlendLabel = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_numQuadrantToBlendLabel->set_horizontal_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);
		label = createControl<godot::Label>(hBoxContainer, "", "Locked");
		m_innerData->m_numQuadrantLocked = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_numQuadrantLocked->set_horizontal_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainFixedVBoxContainer);
		button = createControl<godot::Button>(hBoxContainer, "", "Save", "pressed", this, "edit_mode_save");
		button->set_self_modulate(BUTTON_COLOR);
		button = createControl<godot::Button>(hBoxContainer, "", "Upload", "pressed", this, "edit_mode_upload");
		button->set_self_modulate(BUTTON_COLOR);
		button = createControl<godot::Button>(hBoxContainer, "", "Stop", "pressed", this, "edit_mode_stop");
		button->set_self_modulate(BUTTON_COLOR);
		button = createControl<godot::Button>(hBoxContainer, "", "Reload shader", "pressed", this, "edit_mode_reload_shader");
		button->set_self_modulate(BUTTON_COLOR);
		button = createControl<godot::Button>(hBoxContainer, "", "Reset Norm/Tex", "pressed", this, "edit_mode_reset_normals_splatmap");
		button->set_self_modulate(BUTTON_COLOR);
		//button = createControl<godot::Button>(hBoxContainer, "", "Apply Texs", "pressed", this, "edit_mode_apply_textures");
		//button->set_self_modulate(BUTTON_COLOR);
		//hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainFixedVBoxContainer);
		//m_innerData->m_allCheckBox = createControl<godot::CheckBox>(hBoxContainer);
		//m_innerData->m_allCheckBox->set_text("All");
		//m_innerData->m_allCheckBox->set_toggle_mode(true);
		//hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainFixedVBoxContainer);
		//m_innerData->m_collisonHiResGridCheckBox = createControl<godot::CheckBox>(hBoxContainer, "CollisonHiResGridCheckBox");
		//m_innerData->m_collisonHiResGridCheckBox->set_text("Coll. High Res Grid");
		//m_innerData->m_collisonHiResGridCheckBox->set_toggle_mode(true);
		//e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_collisonHiResGridCheckBox, "godot::CheckBox", "toggled", this, "toggled_signal_manager");
		//m_innerData->m_collisonLowResGridCheckBox = createControl<godot::CheckBox>(hBoxContainer, "CollisonLowResGridCheckBox");
		//m_innerData->m_collisonLowResGridCheckBox->set_text("Coll. Low Res Grid");
		//m_innerData->m_collisonLowResGridCheckBox->set_toggle_mode(true);
		//e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_collisonLowResGridCheckBox, "godot::CheckBox", "toggled", this, "toggled_signal_manager");

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainFixedVBoxContainer);
		m_innerData->m_message = createControl<godot::Label>(hBoxContainer);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainFixedVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Elapsed");
		m_innerData->m_elapsedLabel = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_elapsed1Label = createControl<godot::Label>(hBoxContainer);
	}
	//setMessage(std::string("AAAAAAAAAAAAAAAAAAAAAAA"));

	// Server Info (content show/hide in fixed VBox container)
	{
		m_innerData->m_serverInfoButton = createControl<godot::Button>(m_innerData->m_mainFixedVBoxContainer, "ServerInfoPanelToggle", (std::wstring(RIGHT_ARROW) + L" " + SERVER_INFO_BUTTON_TEXT).c_str(), "pressed", this, "void_signal_manager");
		m_innerData->m_serverInfoButton->set_text_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);
		m_innerData->m_serverInfoButton->set_self_modulate(SECTION_BUTTON_COLOR);

		m_innerData->m_serverInfoVBoxContainer = createControl<godot::VBoxContainer>(m_innerData->m_mainFixedVBoxContainer);
		m_innerData->m_serverInfoVBoxContainer->hide();

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_serverInfoVBoxContainer);
		m_innerData->m_procgenHeader = createControl<godot::Label>(hBoxContainer, "", "");
		m_innerData->m_procgenStatus = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_procgenClient = createControl<godot::Label>(hBoxContainer);

		separator = createControl<godot::HSeparator>(m_innerData->m_serverInfoVBoxContainer);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_serverInfoVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Lvl");
		m_innerData->m_procgenLevel = createControl<godot::Label>(hBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Heights");
		m_innerData->m_procgenHeightsProgress = createControl<godot::Label>(hBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Blend");
		m_innerData->m_procgenBlendProgress = createControl<godot::Label>(hBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "To Cli");
		m_innerData->m_procgenClientElabProgress = createControl<godot::Label>(hBoxContainer);
	}

	// Terrain Info (content show/hide in fixed VBox container)
	{
		m_innerData->m_terrainInfoButton = createControl<godot::Button>(m_innerData->m_mainFixedVBoxContainer, "TerrainInfoPanelToggle", (std::wstring(DOWN_ARROW) + L" " + TERRAIN_INFO_BUTTON_TEXT).c_str(), "pressed", this, "void_signal_manager");
		m_innerData->m_terrainInfoButton->set_text_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);
		m_innerData->m_terrainInfoButton->set_self_modulate(SECTION_BUTTON_COLOR);

		m_innerData->m_terrainInfoVBoxContainer = createControl<godot::VBoxContainer>(m_innerData->m_mainFixedVBoxContainer);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrainInfoVBoxContainer);
		m_innerData->m_showGizmo = createControl<godot::CheckBox>(hBoxContainer, "ShowGizmoCheckBox");
		m_innerData->m_showGizmo->set_text("Show Gizmo");
		m_innerData->m_showGizmo->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_showGizmo, "godot::CheckBox", "toggled", this, "toggled_signal_manager");
		m_innerData->m_showNormal = createControl<godot::CheckBox>(hBoxContainer, "ShowNormalCheckBox");
		m_innerData->m_showNormal->set_text("Show Normal");
		m_innerData->m_showNormal->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_showNormal, "godot::CheckBox", "toggled", this, "toggled_signal_manager");
		m_innerData->m_showPosition = createControl<godot::CheckBox>(hBoxContainer, "ShowPositionCheckBox");
		m_innerData->m_showPosition->set_text("Show Position");
		m_innerData->m_showPosition->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_showPosition, "godot::CheckBox", "toggled", this, "toggled_signal_manager");

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrainInfoVBoxContainer);
		m_innerData->m_showTerrain = createControl<godot::CheckBox>(hBoxContainer, "ShowTerrainCheckBox");
		m_innerData->m_showTerrain->set_text("Show Terrain");
		m_innerData->m_showTerrain->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_showTerrain, "godot::CheckBox", "toggled", this, "toggled_signal_manager");
		if (globals->getParamShowTerrain())
			m_innerData->m_showTerrain->set_pressed(true);
		m_innerData->m_showSea = createControl<godot::CheckBox>(hBoxContainer, "ShowSeaCheckBox");
		m_innerData->m_showSea->set_text("Show Sea");
		m_innerData->m_showSea->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_showSea, "godot::CheckBox", "toggled", this, "toggled_signal_manager");
		if (globals->getParamShowSea())
			m_innerData->m_showSea->set_pressed(true);
		m_innerData->m_showFog = createControl<godot::CheckBox>(hBoxContainer, "ShowFogCheckBox");
		m_innerData->m_showFog->set_text("Show Fog");
		m_innerData->m_showFog->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_showFog, "godot::CheckBox", "toggled", this, "toggled_signal_manager");
		if (globals->getParamShowFog())
			m_innerData->m_showFog->set_pressed(true);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrainInfoVBoxContainer);
		m_innerData->m_highlightHitQuadrant = createControl<godot::CheckBox>(hBoxContainer, "HighlightHitQuadrantCheckBox");
		m_innerData->m_highlightHitQuadrant->set_text("Highlight Hit Quad");
		m_innerData->m_highlightHitQuadrant->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_highlightHitQuadrant, "godot::CheckBox", "toggled", this, "toggled_signal_manager");
		m_innerData->m_showChunks = createControl<godot::CheckBox>(hBoxContainer, "ShowChunksCheckBox");
		m_innerData->m_showChunks->set_text("Show Chunks");
		m_innerData->m_showChunks->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_showChunks, "godot::CheckBox", "toggled", this, "toggled_signal_manager");
		if (globals->getParamShowChunks())
			m_innerData->m_showChunks->set_pressed(true);
		m_innerData->m_showWireframe = createControl<godot::CheckBox>(hBoxContainer, "ShowWireframeCheckBox");
		m_innerData->m_showWireframe->set_text("Wireframe");
		m_innerData->m_showWireframe->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_showWireframe, "godot::CheckBox", "toggled", this, "toggled_signal_manager");
		if (globals->getParamShowWireframe())
			m_innerData->m_showWireframe->set_pressed(true);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrainInfoVBoxContainer);
		m_innerData->m_gridON = createControl<godot::CheckBox>(hBoxContainer, "GridOnCheckBox");
		m_innerData->m_gridON->set_text("Grid ON");
		m_innerData->m_gridON->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_gridON, "godot::CheckBox", "toggled", this, "toggled_signal_manager");
		m_innerData->m_castRay = createControl<godot::CheckBox>(hBoxContainer, "CastRayCheckBox");
		m_innerData->m_castRay->set_text("Cast Ray");
		m_innerData->m_castRay->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_castRay, "godot::CheckBox", "toggled", this, "toggled_signal_manager");

		separator = createControl<HSeparator>(m_innerData->m_terrainInfoVBoxContainer);
		m_innerData->m_mouseHitInfoLabel = createControl<godot::Label>(m_innerData->m_terrainInfoVBoxContainer);
		separator = createControl<HSeparator>(m_innerData->m_terrainInfoVBoxContainer);
		m_innerData->m_hitQuadInfoLabel = createControl<godot::Label>(m_innerData->m_terrainInfoVBoxContainer);
		separator = createControl<HSeparator>(m_innerData->m_terrainInfoVBoxContainer);
		m_innerData->m_selQuadInfoLabel = createControl<godot::Label>(m_innerData->m_terrainInfoVBoxContainer);
		m_innerData->m_selQuadSeaCheckBox = createControl<godot::CheckBox>(m_innerData->m_terrainInfoVBoxContainer, "SelectedQuadSeaCheckBox");
		m_innerData->m_selQuadSeaCheckBox->set_text("Sea");
		m_innerData->m_selQuadSeaCheckBox->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_selQuadSeaCheckBox, "godot::CheckBox", "toggled", this, "toggled_signal_manager");
		m_innerData->m_selQuadSeaCheckBox->set_disabled(true);
	}

	separator = createControl<HSeparator>(vBoxContainer);

	m_innerData->m_mainTerrainScrollContainer = createControl<godot::ScrollContainer>(vBoxContainer);	// m_innerData->m_mainTabContainer
	m_innerData->m_mainTerrainScrollContainer->set_h_size_flags(godot::Control::SizeFlags::SIZE_FILL);
	m_innerData->m_mainTerrainScrollContainer->set_v_size_flags(godot::Control::SizeFlags::SIZE_FILL);
	m_innerData->m_mainTerrainScrollContainer->set_horizontal_scroll_mode(godot::ScrollContainer::SCROLL_MODE_AUTO);
	m_innerData->m_mainTerrainScrollContainer->set_vertical_scroll_mode(godot::ScrollContainer::SCROLL_MODE_AUTO);

	m_innerData->m_mainScrolledVBoxContainer = createControl<godot::VBoxContainer>(m_innerData->m_mainTerrainScrollContainer);	// marginContainer

	// Lookdev Option Button (in scrollable VBox container)
	{
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainScrolledVBoxContainer);
		m_innerData->m_lookDevOptionButton = createControl<godot::OptionButton>(hBoxContainer, "", "", "item_selected", this, "edit_mode_sel_lookdev");
		m_innerData->m_lookDevOptionButton->set_h_size_flags(godot::OptionButton::SizeFlags::SIZE_EXPAND_FILL);
		m_innerData->m_lookDevOptionButton->set_toggle_mode(true);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev disabled", (int64_t)ShaderTerrainData::LookDev::NotSet);
		m_innerData->m_lookDevOptionButton->add_separator();
		m_innerData->m_lookDevOptionButton->add_item("Lookdev heights", (int64_t)ShaderTerrainData::LookDev::Heights);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev normals", (int64_t)ShaderTerrainData::LookDev::Normals);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev splatmap", (int64_t)ShaderTerrainData::LookDev::Splat);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev colors", (int64_t)ShaderTerrainData::LookDev::Color);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev globalmap", (int64_t)ShaderTerrainData::LookDev::Global);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev chunk lod", (int64_t)ShaderTerrainData::LookDev::ChunkLod);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev universe", (int64_t)ShaderTerrainData::LookDev::Universe);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev shader art", (int64_t)ShaderTerrainData::LookDev::ShaderArt);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev lightning", (int64_t)ShaderTerrainData::LookDev::Lightning);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev star nest", (int64_t)ShaderTerrainData::LookDev::StarNest);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev flaring star", (int64_t)ShaderTerrainData::LookDev::FlaringStar);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev animated diamond", (int64_t)ShaderTerrainData::LookDev::AnimatedDiamond);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev plasma waves", (int64_t)ShaderTerrainData::LookDev::PlasmaWaves);
	}

	// Terrain Generation (content show/hide in scrollable VBox container)
	{
		m_innerData->m_terrGenerationButton = createControl<godot::Button>(m_innerData->m_mainScrolledVBoxContainer, "NoisePanelToggle", (std::wstring(RIGHT_ARROW) + L" " + TERRAIN_GENERATION_BUTTON_TEXT).c_str(), "pressed", this, "void_signal_manager");
		m_innerData->m_terrGenerationButton->set_text_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);
		m_innerData->m_terrGenerationButton->set_self_modulate(SECTION_BUTTON_COLOR);

		m_innerData->m_terrGenerationVBoxContainer = createControl<godot::VBoxContainer>(m_innerData->m_mainScrolledVBoxContainer);
		m_innerData->m_terrGenerationVBoxContainer->hide();

		separator = createControl<HSeparator>(m_innerData->m_terrGenerationVBoxContainer);
		
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrGenerationVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", " ");
		godot::Vector2 size(140, 140);
		m_innerData->m_terrTypeButton = createControl<godot::CheckButton>(hBoxContainer, "", "", "pressed", this, "edit_mode_set_terr_type");
		m_innerData->m_terrTypeButton->set_custom_minimum_size(size);
		m_innerData->m_terrTypeButton->set_expand_icon(true);
		m_innerData->m_terrTypeButton->set_visible(false);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrGenerationVBoxContainer);
		m_innerData->m_terrTypeOptionButton = createControl<godot::OptionButton>(hBoxContainer, "", "", "item_selected", this, "edit_mode_sel_terr_type");
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::notset).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::notset);
		m_innerData->m_terrTypeOptionButton->add_separator();
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::flat).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::flat);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::empty).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::empty);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::campaign_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::campaign_1);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::plateau_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::plateau_1);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::low_hills).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::low_hills);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_hills).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_hills);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::low_mountains).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::low_mountains);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::low_mountains_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::low_mountains_grow);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_1);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_1_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_1_grow);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_2).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_2);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_2_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_2_grow);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_3).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_3);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_3_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_3_grow);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_4).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_4);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_4_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_4_grow);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::desert_flat_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::desert_flat_1);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::desert_low_dunes).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::desert_low_dunes);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::desert_low_mountains).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::desert_low_mountains);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::desert_low_mountains_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::desert_low_mountains_grow);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::desert_high_mountains_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::desert_high_mountains_1);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::desert_high_mountains_1_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::desert_high_mountains_1_grow);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::coastal).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::coastal);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::sea_abyss_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::sea_abyss_1);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::sea_abyss_2).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::sea_abyss_2);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::sea_abyss_3).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::sea_abyss_3);
		m_innerData->m_terrTypeOptionButton->add_separator();
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::noise_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::noise_1);
		m_innerData->m_terrTypeOptionButton->set_disabled(true);

		separator = createControl<godot::VSeparator>(hBoxContainer);
		m_innerData->m_generateTerrButton = createControl<godot::Button>(hBoxContainer, "", "Generate", "pressed", this, "edit_mode_generate");
		m_innerData->m_generateTerrButton->set_self_modulate(BUTTON_COLOR);
		m_innerData->m_generateTerrButton->set_disabled(true);
		separator = createControl<godot::VSeparator>(hBoxContainer);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrGenerationVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Seed");
		m_innerData->m_seed = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_seed->set_editable(false);
		label = createControl<godot::Label>(hBoxContainer, "", "Frequency");
		m_innerData->m_frequency = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_frequency->set_editable(false);
		label = createControl<godot::Label>(hBoxContainer, "", "Gain");
		m_innerData->m_fractalGain = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_fractalGain->set_editable(false);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrGenerationVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Octaves");
		m_innerData->m_fractalOctaves = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_fractalOctaves->set_editable(false);
		label = createControl<godot::Label>(hBoxContainer, "", "Lacunarity");
		m_innerData->m_fractalLacunarity = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_fractalLacunarity->set_editable(false);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrGenerationVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Weighted Str.");
		m_innerData->m_fractalWeightedStrength = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_fractalWeightedStrength->set_editable(false);
		label = createControl<godot::Label>(hBoxContainer, "", "Ping Pong Str.");
		m_innerData->m_fractalPingPongStrength = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_fractalPingPongStrength->set_editable(false);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrGenerationVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Ampl.");
		m_innerData->m_amplitudeLabel = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_amplitudeLabel->set_editable(false);
		label = createControl<godot::Label>(hBoxContainer, "", "Jag Int.");
		m_innerData->m_scaleFactorLabel = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_scaleFactorLabel->set_editable(false);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrGenerationVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Min Height");
		m_innerData->m_desideredMinHeightLabel = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_desideredMinHeightLabel->set_editable(false);
		m_innerData->m_ignoreDesideredMinHeightLabel = createControl<godot::CheckBox>(hBoxContainer);
		m_innerData->m_ignoreDesideredMinHeightLabel->set_disabled(true);
		m_innerData->m_ignoreDesideredMinHeightLabel->set_text("Ignore Min Height");
		m_innerData->m_ignoreDesideredMinHeightLabel->set_toggle_mode(true);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrGenerationVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Post Generate Elevation");
		m_innerData->m_postGenerateElevationLabel = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_postGenerateElevationLabel->set_editable(false);

		separator = createControl<HSeparator>(m_innerData->m_terrGenerationVBoxContainer);
	}

	// Terrain Blending (content show/hide in scrollable VBox container)
	{
		m_innerData->m_terrBlendingButton = createControl<godot::Button>(m_innerData->m_mainScrolledVBoxContainer, "TerrBlendingPanelToggle", (std::wstring(RIGHT_ARROW) + L" " + TERRAIN_BLENDING_BUTTON_TEXT).c_str(), "pressed", this, "void_signal_manager");
		m_innerData->m_terrBlendingButton->set_text_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);
		m_innerData->m_terrBlendingButton->set_self_modulate(SECTION_BUTTON_COLOR);
		//m_innerData->m_terrBlendingButton->add_theme_color_override("font_color", GDN_TheWorld_Globals::g_color_yellow_apricot);

		m_innerData->m_terrBlendingVBoxContainer = createControl<godot::VBoxContainer>(m_innerData->m_mainScrolledVBoxContainer);
		m_innerData->m_terrBlendingVBoxContainer->hide();

		separator = createControl<HSeparator>(m_innerData->m_terrBlendingVBoxContainer);

		godot::Size2 labelMinimumSize(250, 0);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrBlendingVBoxContainer);
		button = createControl<godot::Button>(hBoxContainer, "", "Blend", "pressed", this, "edit_mode_blend");
		button->set_self_modulate(BUTTON_COLOR);
		//m_innerData->m_blendAllCheckBox = createControl<godot::CheckBox>(hBoxContainer);
		//m_innerData->m_blendAllCheckBox->set_text("All");
		//m_innerData->m_blendAllCheckBox->set_toggle_mode(true);
		m_innerData->m_checkBlend = createControl<godot::CheckBox>(hBoxContainer, "CheckBlendCheckBox");
		m_innerData->m_checkBlend->set_text("Check Blend");
		m_innerData->m_checkBlend->set_toggle_mode(true);
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_checkBlend, "godot::CheckBox", "toggled", this, "toggled_signal_manager");
		m_innerData->m_autoBlendCheckBox = createControl<godot::CheckBox>(hBoxContainer);
		m_innerData->m_autoBlendCheckBox->set_text("Auto Blend (Save/Upload)");
		m_innerData->m_autoBlendCheckBox->set_toggle_mode(true);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrBlendingVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Amplitude Factor");
		label->set_custom_minimum_size(labelMinimumSize);
		m_innerData->m_amplitudeBlendFactor = createControl<godot::LineEdit>(hBoxContainer);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrBlendingVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Cycles");
		label->set_custom_minimum_size(labelMinimumSize);
		m_innerData->m_blendCycles = createControl<godot::LineEdit>(hBoxContainer);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrBlendingVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Bending Modifier Factor");
		label->set_custom_minimum_size(labelMinimumSize);
		//label->set_visible(false);		// DISABLED
		m_innerData->m_blendingBendingFactor = createControl<godot::LineEdit>(hBoxContainer);
		//m_innerData->m_blendingBendingFactor->set_visible(false);		// DISABLED

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrBlendingVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Min Rand. Bending Extension");
		//label->set_visible(false);		// DISABLED
		label->set_custom_minimum_size(labelMinimumSize);
		m_innerData->m_minRandomBendingExtension = createControl<godot::LineEdit>(hBoxContainer);
		//m_innerData->m_minRandomBendingExtension->set_visible(false);		// DISABLED

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_terrBlendingVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Max Rand. Bending Extension");
		//label->set_visible(false);		// DISABLED
		label->set_custom_minimum_size(labelMinimumSize);
		m_innerData->m_maxRandomBendingExtension = createControl<godot::LineEdit>(hBoxContainer);
		//m_innerData->m_maxRandomBendingExtension->set_visible(false);		// DISABLED

		separator = createControl<HSeparator>(m_innerData->m_terrBlendingVBoxContainer);
	}

	// Quadrant Texture Edit (content show/hide in scrollable VBox container)
	{
		m_innerData->m_quadrantTextureEditButton = createControl<godot::Button>(m_innerData->m_mainScrolledVBoxContainer, "TerrEditPanelToggle", (std::wstring(RIGHT_ARROW) + L" " + TERRAIN_EDIT_BUTTON_TEXT).c_str(), "pressed", this, "void_signal_manager");
		m_innerData->m_quadrantTextureEditButton->set_text_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);
		m_innerData->m_quadrantTextureEditButton->set_self_modulate(SECTION_BUTTON_COLOR);

		m_innerData->m_quadrantTextureEditVBoxContainer = createControl<godot::VBoxContainer>(m_innerData->m_mainScrolledVBoxContainer);
		m_innerData->m_quadrantTextureEditVBoxContainer->hide();

		godot::Vector2 size(70, 70);
		
		separator = createControl<HSeparator>(m_innerData->m_quadrantTextureEditVBoxContainer);

		godot::Size2 labelMinimumSize(180, 0);
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_quadrantTextureEditVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "High Elev. Factor");
		label->set_custom_minimum_size(labelMinimumSize);
		m_innerData->m_highElevationFactor = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_highElevationFactor->set_editable(false);
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_quadrantTextureEditVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Low Elev. Factor");
		label->set_custom_minimum_size(labelMinimumSize);
		m_innerData->m_lowElevationFactor = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_lowElevationFactor->set_editable(false);
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_quadrantTextureEditVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Slope Vert. Factor");
		label->set_custom_minimum_size(labelMinimumSize);
		m_innerData->m_slopeVerticalFactor = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_slopeVerticalFactor->set_editable(false);
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_quadrantTextureEditVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Slope Flat Factor");
		label->set_custom_minimum_size(labelMinimumSize);
		m_innerData->m_slopeFlatFactor = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_slopeFlatFactor->set_editable(false);
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_quadrantTextureEditVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Dirt On Rocks Factor");
		label->set_custom_minimum_size(labelMinimumSize);
		m_innerData->m_dirtOnRocksFactor = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_dirtOnRocksFactor->set_editable(false);
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_quadrantTextureEditVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Splatmap Mode");
		label->set_custom_minimum_size(labelMinimumSize);
		m_innerData->m_splatMapMode = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_splatMapMode->set_editable(false);
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_quadrantTextureEditVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Ground Layout");
		label->set_custom_minimum_size(labelMinimumSize);
		m_innerData->m_groundLayout = createControl<godot::LineEdit>(hBoxContainer);
		m_innerData->m_groundLayout->set_editable(false);

		separator = createControl<HSeparator>(m_innerData->m_quadrantTextureEditVBoxContainer);

		label = createControl<godot::Label>(m_innerData->m_quadrantTextureEditVBoxContainer, "", c_lowElevationEditTextureName.c_str());
		m_innerData->m_lowElevationTexPanel = createControl<godot::PanelContainer>(m_innerData->m_quadrantTextureEditVBoxContainer, "LowElevationTexPanel");
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_lowElevationTexPanel, "godot::PanelContainer", "mouse_entered", this, "void_signal_manager", c_lowElevationEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_lowElevationTexPanel, "godot::PanelContainer", "mouse_exited", this, "void_signal_manager", c_lowElevationEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_lowElevationTexPanel, "godot::PanelContainer", "gui_input", this, "gui_input_event", c_lowElevationEditTextureName.c_str());
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_lowElevationTexPanel);
		m_innerData->m_lowElevationTex = createControl<godot::TextureRect>(hBoxContainer);
		m_innerData->m_lowElevationTex->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
		m_innerData->m_lowElevationTex->set_custom_minimum_size(size);
		m_innerData->m_lowElevationTex->set_stretch_mode(godot::TextureRect::STRETCH_SCALE);
		m_innerData->m_lowElevationTexName = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_lowElevationLayoutName = createControl<godot::Label>(hBoxContainer);
		//button = createControl<godot::Button>(hBoxContainer, "", "Change", "pressed", this, "void_signal_manager");
		//button->set_self_modulate(BUTTON_COLOR);

		label = createControl<godot::Label>(m_innerData->m_quadrantTextureEditVBoxContainer, "", c_highElevationEditTextureName.c_str());
		m_innerData->m_highElevationTexPanel = createControl<godot::PanelContainer>(m_innerData->m_quadrantTextureEditVBoxContainer, "HighElevationTexPanel");
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_highElevationTexPanel, "godot::PanelContainer", "mouse_entered", this, "void_signal_manager", c_highElevationEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_highElevationTexPanel, "godot::PanelContainer", "mouse_exited", this, "void_signal_manager", c_highElevationEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_highElevationTexPanel, "godot::PanelContainer", "gui_input", this, "gui_input_event", c_highElevationEditTextureName.c_str());
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_highElevationTexPanel);
		m_innerData->m_highElevationTex = createControl<godot::TextureRect>(hBoxContainer);
		m_innerData->m_highElevationTex->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
		m_innerData->m_highElevationTex->set_custom_minimum_size(size);
		m_innerData->m_highElevationTex->set_stretch_mode(godot::TextureRect::STRETCH_SCALE);
		m_innerData->m_highElevationTexName = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_highElevationLayoutName = createControl<godot::Label>(hBoxContainer);
		//button = createControl<godot::Button>(hBoxContainer, "", "Change", "pressed", this, "void_signal_manager");
		//button->set_self_modulate(BUTTON_COLOR);

		label = createControl<godot::Label>(m_innerData->m_quadrantTextureEditVBoxContainer, "", c_dirtEditTextureName.c_str());
		m_innerData->m_dirtTexPanel = createControl<godot::PanelContainer>(m_innerData->m_quadrantTextureEditVBoxContainer, "DirtTexPanel");
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_dirtTexPanel, "godot::PanelContainer", "mouse_entered", this, "void_signal_manager", c_dirtEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_dirtTexPanel, "godot::PanelContainer", "mouse_exited", this, "void_signal_manager", c_dirtEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_dirtTexPanel, "godot::PanelContainer", "gui_input", this, "gui_input_event", c_dirtEditTextureName.c_str());
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_dirtTexPanel);
		m_innerData->m_dirtTex = createControl<godot::TextureRect>(hBoxContainer);
		m_innerData->m_dirtTex->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
		m_innerData->m_dirtTex->set_custom_minimum_size(size);
		m_innerData->m_dirtTex->set_stretch_mode(godot::TextureRect::STRETCH_SCALE);
		m_innerData->m_dirtTexName = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_dirtLayoutName = createControl<godot::Label>(hBoxContainer);
		//button = createControl<godot::Button>(hBoxContainer, "", "Change", "pressed", this, "void_signal_manager");
		//button->set_self_modulate(BUTTON_COLOR);

		label = createControl<godot::Label>(m_innerData->m_quadrantTextureEditVBoxContainer, "", c_rocksEditTextureName.c_str());
		m_innerData->m_rocksTexPanel = createControl<godot::PanelContainer>(m_innerData->m_quadrantTextureEditVBoxContainer, "RocksTexPanel");
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_rocksTexPanel, "godot::PanelContainer", "mouse_entered", this, "void_signal_manager", c_rocksEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_rocksTexPanel, "godot::PanelContainer", "mouse_exited", this, "void_signal_manager", c_rocksEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_rocksTexPanel, "godot::PanelContainer", "gui_input", this, "gui_input_event", c_rocksEditTextureName.c_str());
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_rocksTexPanel);
		m_innerData->m_rocksTex = createControl<godot::TextureRect>(hBoxContainer);
		m_innerData->m_rocksTex->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
		m_innerData->m_rocksTex->set_custom_minimum_size(size);
		m_innerData->m_rocksTex->set_stretch_mode(godot::TextureRect::STRETCH_SCALE);
		m_innerData->m_rocksTexName = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_rocksLayoutName = createControl<godot::Label>(hBoxContainer);
		//button = createControl<godot::Button>(hBoxContainer, "", "Change", "pressed", this, "void_signal_manager");
		//button->set_self_modulate(BUTTON_COLOR);

		separator = createControl<HSeparator>(m_innerData->m_quadrantTextureEditVBoxContainer);
	}
	
	m_innerData->m_dummyLabel = createControl<Label>(m_innerData->m_mainScrolledVBoxContainer, "", " ");

	controlNeedResize();

	size_t numToSave;
	size_t numToUpload;
	size_t numLocked;
	refreshNumToSaveUpload(numToSave, numToUpload, numLocked);

	m_tp.Start("EditModeWorker", 1, this);

	m_initialized = true;
}

template <class T>
T* GDN_TheWorld_Edit::createControl(godot::Node* parent, std::string name, godot::String text, std::string signal, godot::Object* callableObject, std::string callableMethod, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3, Color selfModulateColor)
{
	T* control = memnew(T);
	parent->add_child(control);
	godot::Error e = control->connect("mouse_entered", Callable(this, "mouse_entered_edit_panel"));
	e = control->connect("mouse_exited", Callable(this, "mouse_exited_edit_panel"));

	if (name.length() > 0)
		control->set_name(name.c_str());

	//Color c = control->get_self_modulate();
	if (selfModulateColor != Color(0.0f, 0.0f, 0.0f, 0.0f))
		control->set_self_modulate(selfModulateColor);
	//c = control->get_self_modulate();

	if constexpr (std::is_same_v<T, godot::Label>)
	{
		if (text.length() > 0)
			control->set_text(text);
		control->set_horizontal_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);
	}
	else if constexpr (std::is_same_v<T, godot::MarginContainer>)
	{
		control->set_h_size_flags(godot::MarginContainer::SizeFlags::SIZE_EXPAND_FILL);
		control->set_v_size_flags(godot::MarginContainer::SizeFlags::SIZE_EXPAND_FILL);
	}
	else if constexpr (std::is_same_v<T, godot::VBoxContainer>)
	{
		control->set_h_size_flags(godot::VBoxContainer::SizeFlags::SIZE_EXPAND_FILL);
		control->set_v_size_flags(godot::VBoxContainer::SizeFlags::SIZE_EXPAND_FILL);
	}
	else if constexpr (std::is_same_v<T, godot::Button> || std::is_same_v<T, godot::OptionButton> || std::is_same_v<T, godot::CheckButton>)
	{
		if (text.length() > 0)
			control->set_text(text);
		if (signal.length() > 0 && callableObject != nullptr && callableMethod.length() > 0 && callableMethod != "void_signal_manager")
			e = control->connect(signal.c_str(), Callable(callableObject, callableMethod.c_str()));
		control->set_focus_mode(godot::Control::FocusMode::FOCUS_NONE);
		//control->set_flat(true);

	}
	else if constexpr (std::is_same_v<T, godot::LineEdit>)
	{
		control->set_horizontal_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);
	}

	if (signal.length() > 0 && callableObject != nullptr && callableMethod.length() > 0 && callableMethod == "void_signal_manager")
	{
		std::string typeName = typeid(T).name();
		e = GDN_TheWorld_Globals::connectSignal(control, typeName.c_str(), signal.c_str(), callableObject, callableMethod.c_str(), custom1, custom2, custom3);
	}

	return control;
}

void GDN_TheWorld_Edit::deinit(void)
{
	if (m_initialized)
	{
		Node* parent = m_mapModder->get_parent();
		if (parent != nullptr)
			parent->remove_child(m_mapModder);
		m_mapModder->queue_free();
		m_mapModder = nullptr;

		m_tp.Stop();
		m_innerData.reset();
		m_initialized = false;

		parent = get_parent();
		if (parent != nullptr)
		{
			//parent->call_deferred("remove_child", this);		// Mah
			parent->remove_child(this);
		}
		queue_free();
	}
}

void GDN_TheWorld_Edit::setSizeUI(void)
{
	set_anchor(godot::Side::SIDE_RIGHT, 1.0);
	set_anchor(godot::Side::SIDE_LEFT, 1.0);
	//set_offset(godot::Side::SIDE_RIGHT, 0.0);
	//set_offset(godot::Side::SIDE_TOP, 0.0);
	//set_offset(godot::Side::SIDE_BOTTOM, 0.0);

#define CONTROL_DISTANCE_FROM_BORDER 5
#define CONTROL_SIZE 420

	//if (IS_EDITOR_HINT())
	//{
	//	std::string classsName;
	//	godot::Node* parent = get_parent();
	//	if (parent != nullptr)
	//	{
	//		godot::String s = parent->get_class();
	//		classsName = m_viewer->to_string(s);
	//	}
	//	if (classsName == "Control")
	//	{
	//		set_offset(godot::Side::SIDE_LEFT, ((godot::Control*)get_parent())->get_size().x - 350);
	//	}
	//	else
	//	{
	//		set_offset(godot::Side::SIDE_LEFT, get_viewport()->get_visible_rect().get_size().x - 350);
	//	}
	//}
	//else
	//{
	//	set_offset(godot::Side::SIDE_LEFT, get_viewport()->get_visible_rect().get_size().x - CONTROL_SIZE - CONTROL_DISTANCE_FROM_BORDER);
	//}
	set_offset(godot::Side::SIDE_LEFT, -(CONTROL_SIZE + CONTROL_DISTANCE_FROM_BORDER));
	set_offset(godot::Side::SIDE_RIGHT, -CONTROL_DISTANCE_FROM_BORDER);
	set_offset(godot::Side::SIDE_TOP, CONTROL_DISTANCE_FROM_BORDER);

	m_scrollPanelsNeedResize = true;
	m_infoLabelTextChanged = true;
}

bool GDN_TheWorld_Edit::getEditedTerrainEditValues(TheWorld_Utils::TerrainEdit& terrainEdit)
{
	TheWorld_Utils::TerrainEdit nullTerrainEdit(TheWorld_Utils::TerrainEdit::TerrainType::notset);

	int64_t id = m_innerData->m_terrTypeOptionButton->get_selected_id();
	if (id == -1)
	{
		terrainEdit = nullTerrainEdit;
		return false;
	}

	TheWorld_Utils::TerrainEdit::TerrainType selectedTerrainType = (TheWorld_Utils::TerrainEdit::TerrainType)id;
	if (selectedTerrainType == TheWorld_Utils::TerrainEdit::TerrainType::notset)
	{
		terrainEdit = nullTerrainEdit;
		return false;
	}

	TheWorld_Utils::TerrainEdit t(selectedTerrainType);
	t.noise.noiseSeed = seed();
	t.noise.frequency = frequency();
	t.noise.fractalOctaves = octaves();
	t.noise.fractalLacunarity = lacunarity();
	t.noise.fractalGain = gain();
	t.noise.fractalWeightedStrength = weightedStrength();
	t.noise.fractalPingPongStrength = pingPongStrength();
	t.noise.heightsAmplitude = amplitude();
	t.noise.scaleFactor_JagIntensity = scaleFactor();
	t.noise.desideredMinHeight = desideredMinHeight();
	t.extraValues.ignoreDesideredMinHeight = ignoreDesideredMinHeight();
	t.extraValues.postGenerateElevation = postGenerateElevation();
	
	terrainEdit = t;
	return true;
}

godot::Ref<godot::Texture2D> GDN_TheWorld_Edit::getTextureByTerrainType(TheWorld_Utils::TerrainEdit::TerrainType terrainType)
{
	if (m_innerData->m_terrainTypeIcons.contains(terrainType))
		return m_innerData->m_terrainTypeIcons[terrainType];
	else
		return nullptr;
}

void GDN_TheWorld_Edit::setEmptyTerrainEditValues(void)
{
	m_innerData->m_terrTypeOptionButton->select(m_innerData->m_terrTypeOptionButton->get_item_index((int64_t)TheWorld_Utils::TerrainEdit::TerrainType::notset));
	godot::Ref<godot::Texture2D> emptyTexture;
	m_innerData->m_terrTypeButton->set_button_icon(emptyTexture);
	editingMap(false);
	//m_innerData->m_selectedTerrainType = TheWorld_Utils::TerrainEdit::TerrainType::notset;
	setSelectedTerrainType(TheWorld_Utils::TerrainEdit::TerrainType::notset);

	m_infoLabelTextChanged = true;

	setSeed(0);
	setFrequency(0.0f);
	setOctaves(0);
	setLacunarity(0.0f);
	setGain(0.0f);
	setWeightedStrength(0.0f);
	setPingPongStrength(0.0f);
	setAmplitudeBlendFactor(0.0f);
	setBlendingCycles(0);
	setBlendingBendingFactor(0.0f);
	setMinRandomBendingExtension(0.0f);
	setMaxRandomBendingExtension(0.0f);
	setAmplitude(0);
	setScaleFactor(1.0f);
	setIgnoreDesideredMinHeight(false);
	setDesideredMinHeight(0.0f);
	setPostGenerateElevation(0.0f);
	setMinHeight(0.0f);
	setMaxHeight(0.0f);
	//setCanContainSea(false);
	setSlopeVerticalFactor(0.0f);
	setSlopeFlatFactor(0.0f);
	setDirtOnRocksFactor(0.0f);
	setHighElevationFactor(0.0f);
	setLowElevationFactor(0.0f);
	setSplatMapMode(1);
	setGroundLayout("");

	m_innerData->m_lowElevationTexName->set_text("");
	m_innerData->m_highElevationTexName->set_text("");
	m_innerData->m_dirtTexName->set_text("");
	m_innerData->m_rocksTexName->set_text("");
	m_innerData->m_lowElevationLayoutName->set_text("");
	m_innerData->m_highElevationLayoutName->set_text("");
	m_innerData->m_dirtLayoutName->set_text("");
	m_innerData->m_rocksLayoutName->set_text("");
	m_innerData->m_lowElevationTex->set_texture(emptyTexture);
	m_innerData->m_highElevationTex->set_texture(emptyTexture);
	m_innerData->m_dirtTex->set_texture(emptyTexture);
	m_innerData->m_rocksTex->set_texture(emptyTexture);

	m_innerData->m_selQuadSeaCheckBox->set_disabled(true);
}

void GDN_TheWorld_Edit::setTerrainEditValues(TheWorld_Utils::TerrainEdit& terrainEdit, TheWorld_Utils::QuadrantPos quadrantPos, bool activateEditingMap)
{
	int64_t id = m_innerData->m_terrTypeOptionButton->get_selected_id();
	if (id != -1)
	{
		TheWorld_Utils::TerrainEdit::TerrainType selectedTerrainType = (TheWorld_Utils::TerrainEdit::TerrainType)id;
		if (selectedTerrainType != terrainEdit.terrainType)
		{
			//std::string text = TheWorld_Utils::TerrainEdit::terrainTypeString(terrainEdit.terrainType);
			//m_terrTypeOptionButton->set_text(text.c_str());
			m_innerData->m_terrTypeOptionButton->select(m_innerData->m_terrTypeOptionButton->get_item_index((int64_t)terrainEdit.terrainType));
			//m_innerData->m_selectedTerrainType = terrainEdit.terrainType;
		}
		setSelectedTerrainType(terrainEdit.terrainType);
	}

	godot::Ref<godot::Texture2D> emptyTexture;
	if (m_innerData->m_terrainTypeIcons.contains(terrainEdit.terrainType))
	{
		m_innerData->m_terrTypeButton->set_button_icon(m_innerData->m_terrainTypeIcons[terrainEdit.terrainType]);
		if (activateEditingMap)
			editingMap(true);
	}
	else
	{
		m_innerData->m_terrTypeButton->set_button_icon(emptyTexture);
		editingMap(false);
	}

	m_infoLabelTextChanged = true;

	setSeed(terrainEdit.noise.noiseSeed);
	setFrequency(terrainEdit.noise.frequency);
	setOctaves(terrainEdit.noise.fractalOctaves);
	setLacunarity(terrainEdit.noise.fractalLacunarity);
	setGain(terrainEdit.noise.fractalGain);
	setWeightedStrength(terrainEdit.noise.fractalWeightedStrength);
	setPingPongStrength(terrainEdit.noise.fractalPingPongStrength);
	setAmplitudeBlendFactor(terrainEdit.extraValues.amplitudeBlendFactor);
	setBlendingCycles(terrainEdit.extraValues.numBlendingCycles);
	setBlendingBendingFactor(terrainEdit.extraValues.blendingBendingFactor);
	setMinRandomBendingExtension(terrainEdit.extraValues.minRandomBendingExtension);
	setMaxRandomBendingExtension(terrainEdit.extraValues.maxRandomBendingExtension);
	setAmplitude(terrainEdit.noise.heightsAmplitude);
	setScaleFactor(terrainEdit.noise.scaleFactor_JagIntensity);
	setIgnoreDesideredMinHeight(terrainEdit.extraValues.ignoreDesideredMinHeight);
	setDesideredMinHeight(terrainEdit.noise.desideredMinHeight);
	setPostGenerateElevation(terrainEdit.extraValues.postGenerateElevation);
	setMinHeight(terrainEdit.minHeight);
	setMaxHeight(terrainEdit.maxHeight);
	//setCanContainSea((terrainEdit.extraValues.seaInfoSet == QUADRANT_SEA_INFO_TRUE && terrainEdit.extraValues.canContainSea == QUADRANT_SEA_INFO_TRUE) ? true : false);

	//setSlopeVerticalFactor(4.0f);
	//setSlopeFlatFactor(1.0f);
	//setDirtOnRocksFactor(2.0f);
	//setHighElevationFactor(4.0f);
	//setLowElevationFactor(2.0f);
	//setSplatMapMode(1);
	setSlopeVerticalFactor(terrainEdit.extraValues.slopeVerticalFactor);
	setSlopeFlatFactor(terrainEdit.extraValues.slopeFlatFactor);
	setDirtOnRocksFactor(terrainEdit.extraValues.dirtOnRocksFactor);
	setHighElevationFactor(terrainEdit.extraValues.highElevationFactor);
	setLowElevationFactor(terrainEdit.extraValues.lowElevationFactor);
	setSplatMapMode(terrainEdit.extraValues.splatMapMode);
	//std::string layout = ShaderTerrainData::layoutFromContinentId(terrainEdit.extraValues1.continentId);
	
	std::string layout;
	if (not quadrantPos.empty())
	{
		std::string continentName;
		layout = ShaderTerrainData::layoutFromQuadrantPos(quadrantPos, continentName);
		setGroundLayout(layout);
	}

	ShaderTerrainData::GroundTextures& groundTextures = ShaderTerrainData::getGroundTextures();
	std::string textureLayout;

	textureLayout = "";
	if (layout.size() > 0)
	{
		godot::Ref<godot::Texture2D> t = groundTextures.getTexturePreview(layout, terrainEdit.extraValues.lowElevationTexName_r, textureLayout);
		m_innerData->m_lowElevationTex->set_texture(t);
	}
	std::string s(terrainEdit.extraValues.lowElevationTexName_r);
	m_innerData->m_lowElevationTexName->set_text(s.c_str());
	if (textureLayout.size() == 0)
		textureLayout = "unknown";
	m_innerData->m_lowElevationLayoutName->set_text((std::string("(Layout: ") + textureLayout + ")").c_str());

	textureLayout = "";
	if (layout.size() > 0)
	{
		godot::Ref<godot::Texture2D> t = groundTextures.getTexturePreview(layout, terrainEdit.extraValues.highElevationTexName_g, textureLayout);
		m_innerData->m_highElevationTex->set_texture(t);
	}
	s = terrainEdit.extraValues.highElevationTexName_g;
	m_innerData->m_highElevationTexName->set_text(s.c_str());
	if (textureLayout.size() == 0)
		textureLayout = "unknown";
	m_innerData->m_highElevationLayoutName->set_text((std::string("(Layout: ") + textureLayout + ")").c_str());

	textureLayout = "";
	if (layout.size() > 0)
	{
		godot::Ref<godot::Texture2D> t = groundTextures.getTexturePreview(layout, terrainEdit.extraValues.dirtTexName_b, textureLayout);
		m_innerData->m_dirtTex->set_texture(t);
	}
	s = terrainEdit.extraValues.dirtTexName_b;
	m_innerData->m_dirtTexName->set_text(s.c_str());
	if (textureLayout.size() == 0)
		textureLayout = "unknown";
	m_innerData->m_dirtLayoutName->set_text((std::string("(Layout: ") + textureLayout + ")").c_str());

	textureLayout = "";
	if (layout.size() > 0)
	{
		godot::Ref<godot::Texture2D> t = groundTextures.getTexturePreview(layout, terrainEdit.extraValues.rocksTexName_a, textureLayout);
		m_innerData->m_rocksTex->set_texture(t);
	}
	s = terrainEdit.extraValues.rocksTexName_a;
	m_innerData->m_rocksTexName->set_text(s.c_str());
	if (textureLayout.size() == 0)
		textureLayout = "unknown";
	m_innerData->m_rocksLayoutName->set_text((std::string("(Layout: ") + textureLayout + ")").c_str());

	m_innerData->m_selQuadSeaCheckBox->set_disabled(false);
}

void GDN_TheWorld_Edit::resetPanelsOnQuadrantDeselection()
{
	resetSelectedTerrainType();
	resetSeed();
	resetFrequency();
	resetOctaves();
	resetLacunarity();
	resetGain();
	resetWeightedStrength();
	resetPingPongStrength();
	resetAmplitudeBlendFactor();
	resetBlendingCycles();
	resetBlendingBendingFactor();
	resetMinRandomBendingExtension();
	resetMaxRandomBendingExtension();
	resetAmplitude();
	resetScaleFactor();
	resetIgnoreDesideredMinHeight();
	resetDesideredMinHeight();
	resetPostGenerateElevation();

	resetSlopeVerticalFactor();
	resetSlopeFlatFactor();
	resetDirtOnRocksFactor();
	resetHighElevationFactor();
	resetLowElevationFactor();
	resetSplatMapMode();
	resetGroundLayout();

	resetTerrainBoxTextures();
}

void GDN_TheWorld_Edit::resetSeed(void)
{
	m_innerData->m_seed->set_text("");
	m_innerData->m_seed->set_editable(false);
}

void GDN_TheWorld_Edit::setSeed(int seed)
{
	m_innerData->m_seed->set_text(std::to_string(seed).c_str());
	m_innerData->m_seed->set_editable(true);
}

int GDN_TheWorld_Edit::seed(void)
{
	std::string s(m_innerData->m_seed->get_text().utf8().get_data());
	int ret = 0;
	if (s.length() > 0)
		ret = std::stoi(s);
	return ret;
}

void GDN_TheWorld_Edit::resetFrequency(void)
{
	m_innerData->m_frequency->set_text("");
	m_innerData->m_frequency->set_editable(false);
}

void GDN_TheWorld_Edit::setFrequency(float frequency)
{
	m_innerData->m_frequency->set_text(std::to_string(frequency).c_str());
	m_innerData->m_frequency->set_editable(true);
}

float GDN_TheWorld_Edit::frequency(void)
{
	std::string s(m_innerData->m_frequency->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetOctaves(void)
{
	m_innerData->m_fractalOctaves->set_text("");
	m_innerData->m_fractalOctaves->set_editable(false);
}

void GDN_TheWorld_Edit::setOctaves(int octaves)
{
	m_innerData->m_fractalOctaves->set_text(std::to_string(octaves).c_str());
	m_innerData->m_fractalOctaves->set_editable(true);
}

int GDN_TheWorld_Edit::octaves(void)
{
	std::string s(m_innerData->m_fractalOctaves->get_text().utf8().get_data());
	int ret = 0;
	if (s.length() > 0)
		ret = std::stoi(s);
	return ret;
}

void GDN_TheWorld_Edit::resetLacunarity(void)
{
	m_innerData->m_fractalLacunarity->set_text("");
	m_innerData->m_fractalLacunarity->set_editable(false);
}

void GDN_TheWorld_Edit::setLacunarity(float lacunarity)
{
	m_innerData->m_fractalLacunarity->set_text(std::to_string(lacunarity).c_str());
	m_innerData->m_fractalLacunarity->set_editable(true);
}

float GDN_TheWorld_Edit::lacunarity(void)
{
	std::string s(m_innerData->m_fractalLacunarity->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetGain(void)
{
	m_innerData->m_fractalGain->set_text("");
	m_innerData->m_fractalGain->set_editable(false);
}

void GDN_TheWorld_Edit::setGain(float gain)
{
	m_innerData->m_fractalGain->set_text(std::to_string(gain).c_str());
	m_innerData->m_fractalGain->set_editable(true);
}

float GDN_TheWorld_Edit::gain(void)
{
	std::string s(m_innerData->m_fractalGain->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetWeightedStrength(void)
{
	m_innerData->m_fractalWeightedStrength->set_text("");
	m_innerData->m_fractalWeightedStrength->set_editable(false);
}

void GDN_TheWorld_Edit::setWeightedStrength(float weightedStrength)
{
	m_innerData->m_fractalWeightedStrength->set_text(std::to_string(weightedStrength).c_str());
	m_innerData->m_fractalWeightedStrength->set_editable(true);
}

float GDN_TheWorld_Edit::weightedStrength(void)
{
	std::string s(m_innerData->m_fractalWeightedStrength->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetPingPongStrength(void)
{
	m_innerData->m_fractalPingPongStrength->set_text("");
	m_innerData->m_fractalPingPongStrength->set_editable(false);
}

void GDN_TheWorld_Edit::setPingPongStrength(float pingPongStrength)
{
	m_innerData->m_fractalPingPongStrength->set_text(std::to_string(pingPongStrength).c_str());
	m_innerData->m_fractalPingPongStrength->set_editable(true);
}

float GDN_TheWorld_Edit::pingPongStrength(void)
{
	std::string s(m_innerData->m_fractalPingPongStrength->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetAmplitudeBlendFactor(void)
{
	m_innerData->m_amplitudeBlendFactor->set_text("");
	m_innerData->m_amplitudeBlendFactor->set_editable(false);
}

void GDN_TheWorld_Edit::setAmplitudeBlendFactor(float amplitudeBlendFactor)
{
	m_innerData->m_amplitudeBlendFactor->set_text(std::to_string(amplitudeBlendFactor).c_str());
	m_innerData->m_amplitudeBlendFactor->set_editable(true);
}

bool GDN_TheWorld_Edit::amplitudeBlendFactorValid(void)
{
	if (strlen(m_innerData->m_amplitudeBlendFactor->get_text().utf8().get_data()) > 0)
		return true;
	else
		return false;
}

float GDN_TheWorld_Edit::amplitudeBlendFactor(void)
{
	std::string s(m_innerData->m_amplitudeBlendFactor->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetBlendingCycles(void)
{
	m_innerData->m_blendCycles->set_text("");
	m_innerData->m_blendCycles->set_editable(false);
}

void GDN_TheWorld_Edit::setBlendingCycles(size_t blendingCycles)
{
	m_innerData->m_blendCycles->set_text(std::to_string(blendingCycles).c_str());
	m_innerData->m_blendCycles->set_editable(true);
}

size_t GDN_TheWorld_Edit::blendingCycles(void)
{
	std::string s(m_innerData->m_blendCycles->get_text().utf8().get_data());
	size_t ret = 0;
	if (s.length() > 0)
		ret = (size_t)std::stol(s);
	return ret;
}

void GDN_TheWorld_Edit::resetBlendingBendingFactor(void)
{
	m_innerData->m_blendingBendingFactor->set_text("");
	m_innerData->m_blendingBendingFactor->set_editable(false);
}

void GDN_TheWorld_Edit::setBlendingBendingFactor(float blendingBendingFactor)
{
	m_innerData->m_blendingBendingFactor->set_text(std::to_string(blendingBendingFactor).c_str());
	m_innerData->m_blendingBendingFactor->set_editable(true);
}

float GDN_TheWorld_Edit::blendingBendingFactor(void)
{
	std::string s(m_innerData->m_blendingBendingFactor->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetMinRandomBendingExtension(void)
{
	m_innerData->m_minRandomBendingExtension->set_text("");
	m_innerData->m_minRandomBendingExtension->set_editable(false);
}

void GDN_TheWorld_Edit::setMinRandomBendingExtension(float minRandomBendingExtension)
{
	m_innerData->m_minRandomBendingExtension->set_text(std::to_string(minRandomBendingExtension).c_str());
	m_innerData->m_minRandomBendingExtension->set_editable(true);
}

float GDN_TheWorld_Edit::minRandomBendingExtension(void)
{
	std::string s(m_innerData->m_minRandomBendingExtension->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetMaxRandomBendingExtension(void)
{
	m_innerData->m_maxRandomBendingExtension->set_text("");
	m_innerData->m_maxRandomBendingExtension->set_editable(false);
}

void GDN_TheWorld_Edit::setMaxRandomBendingExtension(float maxRandomBendingExtension)
{
	m_innerData->m_maxRandomBendingExtension->set_text(std::to_string(maxRandomBendingExtension).c_str());
	m_innerData->m_maxRandomBendingExtension->set_editable(true);
}

float GDN_TheWorld_Edit::maxRandomBendingExtension(void)
{
	std::string s(m_innerData->m_maxRandomBendingExtension->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetAmplitude(void)
{
	m_innerData->m_amplitudeLabel->set_text("");
	m_innerData->m_amplitudeLabel->set_editable(false);
}

void GDN_TheWorld_Edit::setAmplitude(unsigned int amplitude)
{
	m_innerData->m_amplitudeLabel->set_text(std::to_string(amplitude).c_str());
	m_innerData->m_amplitudeLabel->set_editable(true);
}

unsigned int GDN_TheWorld_Edit::amplitude(void)
{
	std::string s(m_innerData->m_amplitudeLabel->get_text().utf8().get_data());
	unsigned int ret = 0;
	if (s.length() > 0)
		ret = std::stoi(s);
	return ret;
}

void GDN_TheWorld_Edit::resetScaleFactor(void)
{
	m_innerData->m_scaleFactorLabel->set_text("");
	m_innerData->m_scaleFactorLabel->set_editable(false);
}

void GDN_TheWorld_Edit::setScaleFactor(float scaleFactor)
{
	m_innerData->m_scaleFactorLabel->set_text(std::to_string(scaleFactor).c_str());
	m_innerData->m_scaleFactorLabel->set_editable(true);
}

float GDN_TheWorld_Edit::scaleFactor(void)
{
	std::string s(m_innerData->m_scaleFactorLabel->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetIgnoreDesideredMinHeight(void)
{
	m_innerData->m_ignoreDesideredMinHeightLabel->set_pressed(false);
	m_innerData->m_ignoreDesideredMinHeightLabel->set_disabled(true);
}

void GDN_TheWorld_Edit::setIgnoreDesideredMinHeight(bool ignoreDesideredMinHeight)
{
	m_innerData->m_ignoreDesideredMinHeightLabel->set_pressed(ignoreDesideredMinHeight);
	m_innerData->m_ignoreDesideredMinHeightLabel->set_disabled(false);
}

bool GDN_TheWorld_Edit::ignoreDesideredMinHeight(void)
{
	return m_innerData->m_ignoreDesideredMinHeightLabel->is_pressed();
}

void GDN_TheWorld_Edit::resetDesideredMinHeight(void)
{
	m_innerData->m_desideredMinHeightLabel->set_text("");
	m_innerData->m_desideredMinHeightLabel->set_editable(false);
}

void GDN_TheWorld_Edit::setDesideredMinHeight(float desideredMinHeight)
{
	m_innerData->m_desideredMinHeightLabel->set_text(std::to_string(desideredMinHeight).c_str());
	m_innerData->m_desideredMinHeightLabel->set_editable(true);
}

float GDN_TheWorld_Edit::desideredMinHeight(void)
{
	std::string s(m_innerData->m_desideredMinHeightLabel->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetPostGenerateElevation(void)
{
	m_innerData->m_postGenerateElevationLabel->set_text("");
	m_innerData->m_postGenerateElevationLabel->set_editable(false);
}

void GDN_TheWorld_Edit::setPostGenerateElevation(float postGenerateElevation)
{
	m_innerData->m_postGenerateElevationLabel->set_text(std::to_string(postGenerateElevation).c_str());
	m_innerData->m_postGenerateElevationLabel->set_editable(true);
}

float GDN_TheWorld_Edit::postGenerateElevation(void)
{
	std::string s(m_innerData->m_postGenerateElevationLabel->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetSlopeVerticalFactor(void)
{
	m_innerData->m_slopeVerticalFactor->set_text("");
	m_innerData->m_slopeVerticalFactor->set_editable(false);
}

void GDN_TheWorld_Edit::setSlopeVerticalFactor(float value)
{
	m_innerData->m_slopeVerticalFactor->set_text(std::to_string(value).c_str());
	m_innerData->m_slopeVerticalFactor->set_editable(true);
}

float GDN_TheWorld_Edit::slopeVerticalFactor(void)
{
	std::string s(m_innerData->m_slopeVerticalFactor->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetSlopeFlatFactor(void)
{
	m_innerData->m_slopeFlatFactor->set_text("");
	m_innerData->m_slopeFlatFactor->set_editable(false);
}

void GDN_TheWorld_Edit::setSlopeFlatFactor(float value)
{
	m_innerData->m_slopeFlatFactor->set_text(std::to_string(value).c_str());
	m_innerData->m_slopeFlatFactor->set_editable(true);
}

float GDN_TheWorld_Edit::slopeFlatFactor(void)
{
	std::string s(m_innerData->m_slopeFlatFactor->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetDirtOnRocksFactor(void)
{
	m_innerData->m_dirtOnRocksFactor->set_text("");
	m_innerData->m_dirtOnRocksFactor->set_editable(false);
}

void GDN_TheWorld_Edit::setDirtOnRocksFactor(float value)
{
	m_innerData->m_dirtOnRocksFactor->set_text(std::to_string(value).c_str());
	m_innerData->m_dirtOnRocksFactor->set_editable(true);
}

float GDN_TheWorld_Edit::dirtOnRocksFactor(void)
{
	std::string s(m_innerData->m_dirtOnRocksFactor->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetHighElevationFactor(void)
{
	m_innerData->m_highElevationFactor->set_text("");
	m_innerData->m_highElevationFactor->set_editable(false);
}

void GDN_TheWorld_Edit::setHighElevationFactor(float value)
{
	m_innerData->m_highElevationFactor->set_text(std::to_string(value).c_str());
	m_innerData->m_highElevationFactor->set_editable(true);
}

float GDN_TheWorld_Edit::highElevationFactor(void)
{
	std::string s(m_innerData->m_highElevationFactor->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() > 0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetLowElevationFactor(void)
{
	m_innerData->m_lowElevationFactor->set_text("");
	m_innerData->m_lowElevationFactor->set_editable(false);
}

void GDN_TheWorld_Edit::setLowElevationFactor(float value)
{
	m_innerData->m_lowElevationFactor->set_text(std::to_string(value).c_str());
	m_innerData->m_lowElevationFactor->set_editable(true);
}

float GDN_TheWorld_Edit::lowElevationFactor(void)
{
	std::string s(m_innerData->m_lowElevationFactor->get_text().utf8().get_data());
	float ret = 0;
	if (s.length() >0)
		ret = std::stof(s);
	return ret;
}

void GDN_TheWorld_Edit::resetSplatMapMode(void)
{
	m_innerData->m_splatMapMode->set_text("");
	m_innerData->m_splatMapMode->set_editable(false);
}

void GDN_TheWorld_Edit::setSplatMapMode(size_t value)
{
	m_innerData->m_splatMapMode->set_text(std::to_string(value).c_str());
	m_innerData->m_splatMapMode->set_editable(true);
}

size_t GDN_TheWorld_Edit::splatMapMode(void)
{
	
	std::string s(m_innerData->m_splatMapMode->get_text().utf8().get_data());
	size_t ret = 0;
	if (s.length() > 0)
		ret = size_t(std::stoi(s));
	return ret;
}

void GDN_TheWorld_Edit::resetGroundLayout(void)
{
	m_innerData->m_groundLayout->set_text("");
	m_innerData->m_groundLayout->set_editable(false);
}

void GDN_TheWorld_Edit::setGroundLayout(std::string layout)
{
	m_innerData->m_groundLayout->set_text(layout.c_str());
	m_innerData->m_groundLayout->set_editable(true);
}

std::string GDN_TheWorld_Edit::groundLayout(void)
{
	return m_innerData->m_groundLayout->get_text().utf8().get_data();
}

void GDN_TheWorld_Edit::resetTerrainBoxTextures(void)
{
	godot::Ref<godot::Texture2D> t;
	m_innerData->m_lowElevationTex->set_texture(t);
	m_innerData->m_highElevationTex->set_texture(t);
	m_innerData->m_dirtTex->set_texture(t);
	m_innerData->m_rocksTex->set_texture(t);
}

void GDN_TheWorld_Edit::setMinHeight(float minHeight)
{
	m_innerData->m_minHeightStr = std::format("{:.2f}", minHeight);
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::resetSelectedTerrainType(void)
{
	m_innerData->m_selectedTerrainType = TheWorld_Utils::TerrainEdit::TerrainType::notset;
	m_innerData->m_terrTypeOptionButton->set_disabled(true);
	m_innerData->m_generateTerrButton->set_disabled(true);
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setSelectedTerrainType(enum class TheWorld_Utils::TerrainEdit::TerrainType terrainType)
{
	m_innerData->m_selectedTerrainType = terrainType;
	m_innerData->m_terrTypeOptionButton->set_disabled(false);
	m_innerData->m_generateTerrButton->set_disabled(false);
	m_infoLabelTextChanged = true;
}

//float GDN_TheWorld_Edit::minHeight(void)
//{
//	float ret = std::stof(m_innerData->m_minHeightStr);		// WARNING real minHeight is rounded to 2 decimal places
//	return ret;
//	return 0;
//}

void GDN_TheWorld_Edit::setMaxHeight(float maxHeight)
{
	m_innerData->m_maxHeightStr = std::format("{:.2f}", maxHeight);
	m_infoLabelTextChanged = true;
}

//void GDN_TheWorld_Edit::setCanContainSea(bool canContainSea)
//{
//	m_innerData->m_canContainSeaStr = std::to_string(canContainSea);
//	m_infoLabelTextChanged = true;
//}

//float GDN_TheWorld_Edit::maxHeight(void)
//{
//	float ret = std::stof(m_innerData->m_maxHeightStr);		// WARNING real minHeight is rounded to 2 decimal places
//	return ret;
//	return 0;
//}

void GDN_TheWorld_Edit::setElapsed(size_t elapsed, bool onGoing)
{
	//std::lock_guard<std::recursive_mutex> lock(m_mtxUI);

	m_innerData->m_onGoing = onGoing;

	//if (m_innerData->m_onGoing)
	//{
	//	if (!m_onGoingElapsedLabel)
	//	{
	//		m_innerData->m_elapsedLabel->add_theme_color_override("font_color", Color(1, 0, 0));
	//		m_onGoingElapsedLabel = true;
	//	}
	//}
	//else
	//{
	//	if (m_onGoingElapsedLabel)
	//	{
	//		m_innerData->m_elapsedLabel->add_theme_color_override("font_color", Color(1, 1, 1));	
	//		m_onGoingElapsedLabel = false;
	//	}
	//}
	
	if (elapsed == 0)
	{
		//m_innerData->m_elapsedLabel->set_text("");
		m_innerData->m_elapsed = "";
	}
	else
	{
		//m_innerData->m_elapsedLabel->set_text(std::to_string(elapsed).c_str());
		m_innerData->m_elapsed = std::to_string(elapsed);
	}

	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setCounter(size_t current, size_t all)
{
	m_innerData->m_counterStr = std::to_string(current) + "/" + std::to_string(all);
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setNote1(size_t num)
{
	if (num == 0)
	{
		m_innerData->m_note1Str = "";
	}
	else
	{
		m_innerData->m_note1Str = std::to_string(num);
	}
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setNote1(std::string msg)
{
	m_innerData->m_note1Str = msg;
	m_infoLabelTextChanged = true;
}

size_t GDN_TheWorld_Edit::elapsed(void)
{
	//godot::String s = m_innerData->m_elapsedLabel->get_text();
	//const char* str = s.utf8().get_data();
	//size_t ret = std::stoll(std::string(str));
	size_t ret = std::stoll(std::string(m_innerData->m_elapsedLabel->get_text().utf8().get_data()));
	return ret;
}

void GDN_TheWorld_Edit::setMouseHitPos(godot::Vector3 mouseHitPos)
{
	if (m_innerData != nullptr)
	{
		m_innerData->m_mouseHitPos = mouseHitPos;
		m_infoLabelTextChanged = true;
	}
}

QuadTree* GDN_TheWorld_Edit::getQuadrantHit(void) const
{
	if (m_innerData != nullptr)
		return m_innerData->m_quadTreeHit;
	else
		return nullptr;
}

void GDN_TheWorld_Edit::setQuadrantHit(QuadTree* quadTree)
{
	if (m_innerData != nullptr)
	{
		m_innerData->m_quadTreeHit = quadTree;
		m_infoLabelTextChanged = true;
	}
}

void GDN_TheWorld_Edit::setMouseQuadHitLabelText(std::string text)
{
	if (m_innerData != nullptr)
	{
		m_innerData->m_mouseQuadHitStr = text;
		m_infoLabelTextChanged = true;
	}
}

void GDN_TheWorld_Edit::setMouseQuadHitPosLabelText(std::string text)
{
	m_innerData->m_mouseQuadHitPosStr = text;
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setMouseQuadSelLabelText(std::string text)
{
	m_innerData->m_mouseQuadSelStr = text;
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setMouseQuadSelPosLabelText(std::string text)
{
	m_innerData->m_mouseQuadSelPosStr = text;
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setNumQuadrantToSave(size_t num)
{
	m_innerData->m_numQuadrantToSaveLabel->set_text(std::to_string(num).c_str());
}

void GDN_TheWorld_Edit::setNumQuadrantToUpload(size_t num)
{
	m_innerData->m_numQuadrantToUploadLabel->set_text(std::to_string(num).c_str());
}

void GDN_TheWorld_Edit::setQuadrantsToBlend(std::list<TheWorld_Utils::QuadrantPos> quadrantsToBlend)
{
	std::lock_guard<std::recursive_mutex> lock(m_innerData->m_mtxMapQuadrantsToBlend);
	size_t numQuadrantsToBlend = m_innerData->m_mapQuadrantsToBlend.size();
	m_innerData->m_mapQuadrantsToBlend.clear();
	for (auto& quadrantToBlend : quadrantsToBlend)
	{
		m_innerData->m_mapQuadrantsToBlend[quadrantToBlend] = true;
	}
	if (numQuadrantsToBlend != m_innerData->m_mapQuadrantsToBlend.size())
		m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::getQuadrantsToBlend(std::list<TheWorld_Utils::QuadrantPos>& quadrantsToBlend)
{
	quadrantsToBlend.clear();
	std::lock_guard<std::recursive_mutex> lock(m_innerData->m_mtxMapQuadrantsToBlend);
	for (auto& quadrantToBlendItem : m_innerData->m_mapQuadrantsToBlend)
	{
		quadrantsToBlend.push_back(quadrantToBlendItem.first);
	}
}

size_t GDN_TheWorld_Edit::getNumQuadrantsToBlend(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_innerData->m_mtxMapQuadrantsToBlend);
	return m_innerData->m_mapQuadrantsToBlend.size();
}

void GDN_TheWorld_Edit::setNumQuadrantLocked(size_t num)
{
	m_innerData->m_numQuadrantLocked->set_text(std::to_string(num).c_str());
}

void GDN_TheWorld_Edit::refreshNumToSaveUpload(size_t& numToSave, size_t& numToUpload, size_t& numLocked)
{
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
		numToSave = m_mapQuadToSave.size();
	}

	numToUpload = 0;
	numLocked = 0;

	std::vector<TheWorld_Utils::QuadrantPos> allQuadrantPos;
	m_viewer->getAllQuadrantPos(allQuadrantPos, true);

	for (auto& pos : allQuadrantPos)
	{
		QuadTree* q = m_viewer->getQuadTree(pos, false, false, true);
		if (q != nullptr)
		{
			if (q->getQuadrant()->getTerrainEdit()->needUploadToServer)
			numToUpload++;
		}
		if (q != nullptr)
		{
			if (q->getQuadrant()->internalDataLocked())
				numLocked++;
		}
	}

	setNumQuadrantToSave(numToSave);
	setNumQuadrantToUpload(numToUpload);
	setNumQuadrantLocked(numLocked);
}

void GDN_TheWorld_Edit::_init(void)
{
	//Cannot find Globals pointer as current node is not yet in the scene
	//godot::UtilityFunctions::print("GDN_TheWorld_Edit::Init");

	set_name(THEWORLD_EDIT_MODE_UI_CONTROL_NAME);
}

void GDN_TheWorld_Edit::_ready(void)
{
	//Cannot find Globals pointer as current node is not yet in the scene
	//godot::UtilityFunctions::print("GDN_TheWorld_Edit::_ready");

	m_ready = true;
}

void GDN_TheWorld_Edit::controlNeedResize(void)
{
	m_controlNeedResize = true;
}

void GDN_TheWorld_Edit::_input(const Ref<InputEvent>& event)
{
	if (!m_initialized)
		return;
	
	const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	if (m_innerData->m_mouseInsideEditPanel)
	{
		if (IS_EDITOR_HINT())
		{
			if (eventMouseButton != nullptr && eventMouseButton->is_pressed()
				&& (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_WHEEL_DOWN || eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_WHEEL_UP))
			{
				accept_event();

				if (m_innerData->m_mainTerrainScrollContainer != nullptr)
				{
					int vStep = 30;
					int vScroll = m_innerData->m_mainTerrainScrollContainer->get_v_scroll();
					double vStep1 = m_innerData->m_mainTerrainScrollContainer->get_vertical_custom_step();
					if (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_WHEEL_DOWN)
						m_innerData->m_mainTerrainScrollContainer->set_v_scroll(vScroll + vStep);
					else
						m_innerData->m_mainTerrainScrollContainer->set_v_scroll(vScroll - vStep);
					//int hStep = 10;
					//int hScroll = m_innerData->m_mainTerrainScrollContainer->get_h_scroll();
					//if (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_WHEEL_DOWN)
					//	m_innerData->m_mainTerrainScrollContainer->set_h_scroll(hScroll + hStep);
					//else
					//	m_innerData->m_mainTerrainScrollContainer->set_h_scroll(hScroll - hStep);
				}
			}
		}
		else
		{
			//if (eventMouseButton != nullptr && eventMouseButton->is_pressed()
			//	&& (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_WHEEL_DOWN || eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_WHEEL_UP))
			//{
			//	accept_event();
			//}
		}
	}
}

void GDN_TheWorld_Edit::_unhandled_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr && eventMouseButton->is_pressed()
	//	&& (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_WHEEL_DOWN || eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_WHEEL_UP)
	//	&& m_innerData->m_mouseInsideEditPanel)
	//{
	//	accept_event();
	//}
}

void GDN_TheWorld_Edit::_notification(int p_what)
{
	switch (p_what)
	{
	case NOTIFICATION_ENTER_TREE:
	{
		std::string classsName;
		godot::Node* parent = get_parent();
		if (parent != nullptr)
		{
			godot::String s = parent->get_class();
			classsName = m_viewer->to_string(s);
		}
		if (IS_EDITOR_HINT())
		{
			if (classsName == "Control")
			{
				//godot::Error e = parent->connect("size_changed", this, "setSizeUI");
				//std::string s = "";
			}
		}
	}
	break;
	case NOTIFICATION_WM_CLOSE_REQUEST:
	{
		m_quitting = true;
	}
	break;
	case NOTIFICATION_PREDELETE:
	{
		if (m_ready && m_viewer != nullptr)
		{
			GDN_TheWorld_Globals* globals = m_viewer->Globals();
			if (globals != nullptr)
				if (m_quitting)
					globals->debugPrint("GDN_TheWorld_Edit::_notification (NOTIFICATION_PREDELETE) - Destroy Edit", false);
				else
					globals->debugPrint("GDN_TheWorld_Edit::_notification (NOTIFICATION_PREDELETE) - Destroy Edit");
		}
	}
	break;
	case NOTIFICATION_RESIZED:
	{
		//setSizeUI();
		godot::Vector2 newSize = get_size();
		controlNeedResize();
		m_size = newSize;
	}
	break;
	}
}

godot::Size2 GDN_TheWorld_Edit::getTerrainScrolledPanelSize(void)
{
	//return godot::Size2(0, 400);
	
	if (m_viewer == nullptr || m_innerData->m_mainScrolledVBoxContainer == nullptr)
		return godot::Size2(0.0f, 0.0f);

	godot::Size2 viewportSize = m_viewer->getViewportSize();
	godot::Rect2 scrollableContentRect = m_innerData->m_mainScrolledVBoxContainer->get_global_rect();
	//godot::Size2 scrollableContentSize = m_innerData->m_mainScrolledVBoxContainer->get_rect().size;

	godot::Rect2 fixedContentRect = m_innerData->m_mainFixedVBoxContainer->get_global_rect();
	float sizeInViewportForScrollableContainer = viewportSize.y - (fixedContentRect.position.y + fixedContentRect.size.y);

	float lookdevOptionButtonHeight = (float)m_innerData->m_lookDevOptionButton->get_rect().size.height + 3;
	float terrGenerationButtonHeight = (float)m_innerData->m_terrGenerationButton->get_rect().size.height + 3;
	float terrBlendingButtonHeight = (float)m_innerData->m_terrBlendingButton->get_rect().size.height + 3;
	float quadrantTextureEditButtonHeight = (float)m_innerData->m_quadrantTextureEditButton->get_rect().size.height + 3;
	float dummyLabelHeight = (float)m_innerData->m_dummyLabel->get_rect().size.height + 3;

	//static float terrGenerationVBoxCExpandedHeight = 0;
	float terrGenerationVBoxCExpandedHeight = 0;
	float terrGenerationVBoxCHeight;
	if (m_innerData->m_terrGenerationVBoxContainer->is_visible())
	{
		if (terrGenerationVBoxCExpandedHeight == 0)
		{
			terrGenerationVBoxCExpandedHeight = (float)m_innerData->m_terrGenerationVBoxContainer->get_rect().size.height + 4;
		}
		terrGenerationVBoxCHeight = terrGenerationVBoxCExpandedHeight;
	}
	else
		terrGenerationVBoxCHeight = 0;

	//static float terrBlendingVBoxcExpandedHeight = 0;
	float terrBlendingVBoxcExpandedHeight = 0;
	float terrBlendingVBoxcHeight;
	if (m_innerData->m_terrBlendingVBoxContainer->is_visible())
	{
		if (terrBlendingVBoxcExpandedHeight == 0)
		{
			terrBlendingVBoxcExpandedHeight = (float)m_innerData->m_terrBlendingVBoxContainer->get_rect().size.height + 4;
		}
		terrBlendingVBoxcHeight = terrBlendingVBoxcExpandedHeight;
	}
	else
		terrBlendingVBoxcHeight = 0;

	//static float quadrantTextureEditVBoxcExpandedHeight = 0;
	float quadrantTextureEditVBoxcExpandedHeight = 0;
	float quadrantTextureEditVBoxcHeight;
	if (m_innerData->m_quadrantTextureEditVBoxContainer->is_visible())
	{
		if (quadrantTextureEditVBoxcExpandedHeight == 0)
		{
			quadrantTextureEditVBoxcExpandedHeight = (float)m_innerData->m_quadrantTextureEditVBoxContainer->get_rect().size.height + 4;
		}
		quadrantTextureEditVBoxcHeight = quadrantTextureEditVBoxcExpandedHeight;
	}
	else
		quadrantTextureEditVBoxcHeight = 0;

	float scrollableContentHeightCalc = lookdevOptionButtonHeight + terrGenerationButtonHeight + terrGenerationVBoxCHeight + terrBlendingButtonHeight + terrBlendingVBoxcHeight + quadrantTextureEditButtonHeight + quadrantTextureEditVBoxcHeight + dummyLabelHeight;

	godot::Size2 ret;

#define	MIN_WIDTH	425

	//if (scrollableContentRect.position.y + scrollableContentHeightCalc < viewportSize.y)
	//{
	//	ret = godot::Size2(MIN_WIDTH, scrollableContentHeightCalc);
	//}
	//else
	//	ret = godot::Size2(MIN_WIDTH, viewportSize.y - scrollableContentRect.position.y);
	if (sizeInViewportForScrollableContainer > scrollableContentHeightCalc)
		ret = godot::Size2(MIN_WIDTH, scrollableContentHeightCalc);
	else
		ret = godot::Size2(MIN_WIDTH, sizeInViewportForScrollableContainer);
	
	//ret.width = 0;	// ???
	
	return ret;
}

void GDN_TheWorld_Edit::_physics_process(double _delta)
{
	if (m_viewer == nullptr)
		return;

	if (m_lastMessageChanged)
	{
		std::unique_lock<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex(), std::try_to_lock);
		if (lock.owns_lock())
		{
			if (m_lastMessageChanged)
			{
				m_innerData->m_message->set_text(m_innerData->m_lastMessage);

				size_t numToSave, numToUpload, numLocked;
				refreshNumToSaveUpload(numToSave, numToUpload, numLocked);

				m_lastMessageChanged = false;
			}
		}
	}
}

void GDN_TheWorld_Edit::_process(double _delta)
{
	//if (is_visible_in_tree())
	//{
	//	godot::Vector2 mousePos = get_viewport()->get_mouse_position();
	//	
	//	godot::Vector2 globalPos = m_mainTabContainer->get_global_position();
	//	godot::Vector2 globalMousePos = m_mainTabContainer->get_global_mouse_position();
	//	godot::Rect2 viewportRect = m_mainTabContainer->get_viewport_rect();
	//	godot::Transform2D viewportTransform = m_mainTabContainer->get_viewport_transform();

	//	globalPos = m_mainPanelContainer->get_global_position();
	//	globalMousePos = m_mainPanelContainer->get_global_mouse_position();
	//	viewportRect = m_mainPanelContainer->get_viewport_rect();
	//	viewportTransform = m_mainPanelContainer->get_viewport_transform();

	//	viewportTransform = m_mainTabContainer->get_viewport_transform();
	//}

	if (!m_initialized)
		return;

	if (m_viewer == nullptr)
		return;

	if (not m_viewer->editMode())
	{
		setEditingMapMouseShape(false);
		return;
	}

	if (m_viewer->worldMode())
	{
		setEditingMapMouseShape(false);
	}
	else
	{
		if (mouseInsideEditPanel())
		{
			setEditingMapMouseShape(false);
		}
		else
		{
			if (m_innerData->m_terrTypeButton != nullptr)
			{
				if (m_innerData->m_terrTypeButton->is_pressed())
					setEditingMapMouseShape(true);
				else
					setEditingMapMouseShape(false);
			}
		}
	}

	std::unique_lock<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex(), std::try_to_lock);
	if (not lock.owns_lock())
	{
		return;
	}

	QuadTree* quadTreeSel = nullptr;
	TheWorld_Utils::QuadrantPos quadrantSelPos = m_viewer->getWorldQuadrantSelForEdit(&quadTreeSel);

	//if (m_innerData->m_mainScrolledVBoxContainer != nullptr)
	//{
	//	float scrollableContentHeight = m_innerData->m_mainScrolledVBoxContainer->get_global_rect().size.height;
	//	float scrollableContentHeight2 = m_innerData->m_mainScrolledVBoxContainer->get_rect().size.height;
	//	
	//	float lookdevOptionButtonHeight = m_innerData->m_lookDevOptionButton->get_rect().size.height + 3;
	//	float noiseButtonHeight = m_innerData->m_terrGenerationButton->get_rect().size.height + 3;
	//	float noiseVBoxCHeight = m_innerData->m_terrGenerationVBoxContainer->is_visible() ? m_innerData->m_terrGenerationVBoxContainer->get_rect().size.height + 4 : 0;
	//	float terrEditButtonHeight = m_innerData->m_quadrantTextureEditButton->get_rect().size.height + 3;
	//	float terrEditVBoxcHeight = m_innerData->m_quadrantTextureEditVBoxContainer->is_visible() ? m_innerData->m_quadrantTextureEditVBoxContainer->get_rect().size.height + 4 : 0;
	//	float dummyLabelHeight = m_innerData->m_dummyLabel->get_rect().size.height + 3;
	//	float scrollableContentHeightCalc = lookdevOptionButtonHeight + noiseButtonHeight + noiseVBoxCHeight + terrEditButtonHeight + terrEditVBoxcHeight + dummyLabelHeight;
	//	float size2 = 0;
	//}

	if (m_controlNeedResize)
	{
		setSizeUI();
		m_controlNeedResize = false;
	}

	if (m_scrollPanelsNeedResize)
	{
		if (m_innerData->m_mainScrolledVBoxContainer != nullptr && m_innerData->m_mainScrolledVBoxContainer->get_rect().size != godot::Size2(0.0f, 0.0f))
		{
			m_innerData->m_mainTerrainScrollContainer->set_custom_minimum_size(getTerrainScrolledPanelSize());
			m_scrollPanelsNeedResize = false;
		}
	}

	if (m_viewer->editMode() && (!m_refreshUI.counterStarted() || m_refreshUI.partialDuration().count() > 1000))
	{
		m_refreshUI.tick();

		size_t numToSave = 0, numToUpload = 0, numLocked = 0;
		refreshNumToSaveUpload(numToSave, numToUpload, numLocked);
	}

	if (m_requiredUIAcceptFocus != m_UIAcceptingFocus)
	{
		setUIAcceptFocus(m_requiredUIAcceptFocus);
	}

	if (m_actionInProgress)
	{
		setElapsed(m_actionClock.partialDuration().count(), true);
		if (m_allItems != 0)
		{
			setCounter(m_completedItems, m_allItems);
			setNote1(m_lastElapsed);
		}
	}

	if (m_innerData->m_serverProceGenStatusMapChanged && m_viewer->editMode())
	{
		bool running = false;
		size_t numVerticesPerSize = 0;
		int level = 0;
		for (auto& item : m_innerData->m_serverProceGenStatusMap)
			for (auto& item1 : item.second)
				if (item1.second.m_proceduralProcessingInProgress)
				{
					running = true;
					numVerticesPerSize = item1.second.m_numVerticesPerSize;
					level = item1.second.m_level;
					break;
				}

		//m_innerData->m_procgenStatus->set_text((std::string("Proc. Gen (Vert ") + std::to_string(m_innerData->m_serverProceGenStatusMap[numVerticesPerSize][level].m_numVerticesPerSize) + "):").c_str());
		m_innerData->m_procgenClient->set_text((std::string("Ext Que ") + std::to_string(m_viewer->getExtraQuadsMapSize())).c_str());
		if (running)
		{
			waitingForServerGen(false);

			if (!m_innerData->m_procgenClock.counterStarted())
				m_innerData->m_procgenClock.tick();
			
			size_t procgenDurationMs = m_innerData->m_procgenClock.partialDuration().count();
			std::string procgenDurationStr = " " + TheWorld_Utils::milliseconds_to_hhhmmss(procgenDurationMs);;

			// WARNIG: DISPLAING ONLY FIRST ELEMENET OF MAP, ASSUMING ATM ONLY 1 LEVEL IS PRESENT
			m_innerData->m_procgenHeader->set_text((std::string("Proc. Gen (Vert ") + std::to_string(m_innerData->m_serverProceGenStatusMap[numVerticesPerSize][level].m_numVerticesPerSize) + ")").c_str());
			m_innerData->m_procgenStatus->set_text((std::string("RUNNING") + procgenDurationStr).c_str());
			m_innerData->m_procgenLevel->set_text(std::to_string(m_innerData->m_serverProceGenStatusMap[numVerticesPerSize][level].m_level).c_str());
			std::string s = std::to_string(m_innerData->m_serverProceGenStatusMap[numVerticesPerSize][level].m_quadrantsWithHeigthsGenerated) + "/" + std::to_string(m_innerData->m_serverProceGenStatusMap[numVerticesPerSize][level].m_quadrantsToGenerateHeights) + " (" + std::to_string(m_innerData->m_serverProceGenStatusMap[numVerticesPerSize][level].m_currentHeightGenCycle) + ")";
			m_innerData->m_procgenHeightsProgress->set_text(s.c_str());
			s = std::to_string(m_innerData->m_serverProceGenStatusMap[numVerticesPerSize][level].m_numBlendedQuadrants) + "/" + std::to_string(m_innerData->m_serverProceGenStatusMap[numVerticesPerSize][level].m_numQuadrantsToBlend) + " (" + std::to_string(m_innerData->m_serverProceGenStatusMap[numVerticesPerSize][level].m_currentBlendingCycle) + "/" + std::to_string(m_innerData->m_serverProceGenStatusMap[numVerticesPerSize][level].m_numBlendingCycles) + ")";
			m_innerData->m_procgenBlendProgress->set_text(s.c_str());
			m_innerData->m_procgenClientElabProgress->set_text(std::to_string(m_innerData->m_serverProceGenStatusMap[numVerticesPerSize][level].m_numQuadrantsToElabClientSide).c_str());
		}
		else
		{
			bool clientWorkNeeded = false;
			for (auto& item : m_innerData->m_serverProceGenStatusMap)
				for (auto& item1 : item.second)
					if (item1.second.m_numQuadrantsToElabClientSide > 0)
					{
						clientWorkNeeded = true;
						numVerticesPerSize = item1.second.m_numVerticesPerSize;
						level = item1.second.m_level;
						break;
					}

			if (m_innerData->m_procgenClock.counterStarted() && !clientWorkNeeded)
				m_innerData->m_procgenClock.tock();

			size_t procgenDurationMs = m_innerData->m_procgenClock.counterStarted() ? m_innerData->m_procgenClock.partialDuration().count() : m_innerData->m_procgenClock.duration().count();
			std::string procgenDurationStr = " Elapsed=" + TheWorld_Utils::milliseconds_to_hhhmmss(procgenDurationMs);

			m_innerData->m_procgenHeader->set_text("Proc. Gen.");
			if (waitingForServerGen())
			{
				m_innerData->m_procgenStatus->set_text("WAITING");
			}
			else
				m_innerData->m_procgenStatus->set_text(m_innerData->m_procgenClock.counterEverStarted() ? (std::string("STOPPED") + procgenDurationStr).c_str() : "");
			m_innerData->m_procgenLevel->set_text("-");
			m_innerData->m_procgenHeightsProgress->set_text("-");
			m_innerData->m_procgenBlendProgress->set_text("-");
			if (clientWorkNeeded)
				m_innerData->m_procgenClientElabProgress->set_text(std::to_string(m_innerData->m_serverProceGenStatusMap[numVerticesPerSize][level].m_numQuadrantsToElabClientSide).c_str());
			else
				m_innerData->m_procgenClientElabProgress->set_text("-");
		}

		m_innerData->m_serverProceGenStatusMapChanged = false;
	}
	
	if (m_lastMessageChanged)
	{
		m_innerData->m_message->set_text(m_innerData->m_lastMessage);

		size_t numToSave, numToUpload, numLocked;
		refreshNumToSaveUpload(numToSave, numToUpload, numLocked);

		m_lastMessageChanged = false;
	}

	if (m_innerData->m_selTexturePanel->m_closeSelTexturesRequired)
	{
		if (m_innerData->m_selTexturePanel->m_wnd != nullptr)
		{
			m_innerData->m_selTexturePanel->m_wnd->hide();
			godot::Node* parent = m_innerData->m_selTexturePanel->m_wnd->get_parent();
			if (parent != nullptr)
			{
				//parent->remove_child(m_innerData->m_selTexturePanel->m_wnd);
				parent->call_deferred("remove_child", m_innerData->m_selTexturePanel->m_wnd);		// Mah
			}
			m_innerData->m_selTexturePanel->m_wnd->queue_free();
			m_innerData->m_selTexturePanel->m_wnd = nullptr;
			m_innerData->m_selTexturePanel->m_textureItems.clear();

			m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
			m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
			m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
			m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");
			
			if (!m_innerData->m_selTexturePanel->m_openSelTexturesRequired)
			{
				m_innerData->m_selTexturePanel->m_selectedTexSlotName = "";
				m_innerData->m_lowElevationTexSelected = false;
				m_innerData->m_highElevationTexSelected = false;
				m_innerData->m_dirtTexSelected = false;
				m_innerData->m_rocksTexSelected = false;
			}
		}

		m_innerData->m_selTexturePanel->m_closeSelTexturesRequired = false;
	}

	if (m_innerData->m_selTexturePanel->m_openSelTexturesRequired)
	{
		if (m_innerData->m_selTexturePanel->m_wnd == nullptr)
		{
			if (m_innerData->m_lowElevationTexSelected)
				m_innerData->m_lowElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			else if (m_innerData->m_highElevationTexSelected)
				m_innerData->m_highElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			else if (m_innerData->m_dirtTexSelected)
				m_innerData->m_dirtTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			else if (m_innerData->m_rocksTexSelected)
				m_innerData->m_rocksTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);

			m_innerData->m_selTexturePanel->m_wnd = initSelTexturePanel();
		}

		m_innerData->m_selTexturePanel->m_openSelTexturesRequired = false;
	}

	if (m_innerData->m_selTexturePanel->m_changeTextureRequired)
	{
		changeTextureAction();
		m_innerData->m_selTexturePanel->m_changeTextureRequired = false;
	}
	
	if (is_visible() && (m_viewer->Globals()->status() == TheWorldStatus::worldDeployInProgress || m_viewer->Globals()->status() == TheWorldStatus::minimalWorldDeployed || m_viewer->Globals()->status() == TheWorldStatus::worldDeployed) && m_innerData->m_quadTreeHit != nullptr)
	{
		int optimizationLevel = (int)m_innerData->m_quadTreeHit->getQuadrant()->optimizationLevel();
		if (optimizationLevel != m_innerData->m_lastOptimizationLevelDisplayed)
		{
			m_innerData->m_lastOptimizationLevelDisplayed = optimizationLevel;
			m_infoLabelTextChanged = true;
		}

		bool hasHiResData = m_innerData->m_quadTreeHit->getQuadrant()->getCollider()->hasHiResData();
		int hiResCollisionGrid = (hasHiResData ? 1 : 0);
		if (hiResCollisionGrid != m_innerData->m_lastHiResCollisionGridDisplayed)
		{
			m_innerData->m_lastHiResCollisionGridDisplayed = hiResCollisionGrid;
			m_infoLabelTextChanged = true;
		}
	}
	
	//if (m_innerData->m_quadTreeHit != nullptr)	// #####
	//	if (m_innerData->m_quadTreeHit->getQuadrant()->getPos().getIndexPosX() == -7 and m_innerData->m_quadTreeHit->getQuadrant()->getPos().getIndexPosZ() == -16)	// #####
	//		assert(m_innerData->m_quadTreeHit != nullptr);									// #####

	if (m_infoLabelTextChanged && (m_viewer->Globals()->status() == TheWorldStatus::worldDeployInProgress || m_viewer->Globals()->status() == TheWorldStatus::minimalWorldDeployed || m_viewer->Globals()->status() == TheWorldStatus::worldDeployed) /* && m_innerData->m_quadTreeHit != nullptr*/)
	{
		m_innerData->m_numQuadrantToBlendLabel->set_text(std::to_string(getNumQuadrantsToBlend() + numQuadsToDelayedBlend()).c_str());

		std::string quadHitOptimizationLevelInfoStr, quadHitColliderInfoStr, quadHitCanContainSeaStr, quadHitTerrainTypeStr, quadHitMinHeightStr, quadHitMaxHeightStr, quadHitEmptyStr, quadHitContinentStr, needToUploadStr, needToSaveStr, DepthStr, quadsFromCameraQuadStr, numVisibleChunksStr;
		enum class TheWorld_Utils::TerrainEdit::TerrainType quadHitTerrainType = TheWorld_Utils::TerrainEdit::TerrainType::notset;
		float quadHitMinHeight = 0, quadHitMaxHeight = 0;
		if (m_innerData->m_quadTreeHit != nullptr)
		{
			quadHitOptimizationLevelInfoStr = std::to_string(m_innerData->m_quadTreeHit->getQuadrant()->optimizationLevel());
			if (m_innerData->m_quadTreeHit->getQuadrant()->getCollider() != nullptr)
				quadHitColliderInfoStr = m_innerData->m_quadTreeHit->getQuadrant()->getCollider()->hasHiResData() ? "HIRES" : "LOWRES";
			quadHitCanContainSeaStr = std::to_string(m_innerData->m_quadTreeHit->canContainSea());
			quadHitTerrainType = m_innerData->m_quadTreeHit->getQuadrant()->getTerrainEdit()->terrainType;
			quadHitMinHeight = m_innerData->m_quadTreeHit->getQuadrant()->getTerrainEdit()->minHeight;
			quadHitMaxHeight = m_innerData->m_quadTreeHit->getQuadrant()->getTerrainEdit()->maxHeight;
			quadHitTerrainTypeStr = TheWorld_Utils::TerrainEdit::terrainTypeString(quadHitTerrainType);
			quadHitMinHeightStr = std::format("{:.2f}", quadHitMinHeight);
			quadHitMaxHeightStr = std::format("{:.2f}", quadHitMaxHeight);
			quadHitContinentStr = m_innerData->m_quadTreeHit->getQuadrant()->continentName();
			quadHitEmptyStr = m_innerData->m_quadTreeHit->getQuadrant()->empty() ? "True" : "False";
			needToUploadStr = (m_innerData->m_quadTreeHit->getQuadrant()->getTerrainEdit()->needUploadToServer ? "True" : "False");
			needToSaveStr = (m_mapQuadToSave.contains(m_innerData->m_quadTreeHit->getQuadrant()->getPos()) ? "True" : "False");
			DepthStr = QuadTree::quadrantDepthToString(m_innerData->m_quadTreeHit->quadrantDepth());
			quadsFromCameraQuadStr = std::to_string(m_innerData->m_quadTreeHit->numQuadrantsFromCameraQuadrant());
			numVisibleChunksStr = std::to_string(m_innerData->m_quadTreeHit->getNumChunks(true, QuadType::NotSet));
		}
		else
		{
			quadHitOptimizationLevelInfoStr = "-";
			quadHitColliderInfoStr = "-";
			quadHitCanContainSeaStr = "-";
			quadHitTerrainTypeStr = "-";
			quadHitMinHeightStr = "-";
			quadHitMaxHeightStr = "-";
			quadHitContinentStr = "-";
			quadHitEmptyStr = "-";
			needToUploadStr = "-";
			needToSaveStr = "-";
			DepthStr = "-";
			quadsFromCameraQuadStr = "-";
			numVisibleChunksStr = "-";
		}

		std::string quadSelOptimizationLevelInfoStr, quadSelColliderInfoStr, quadSelCanContainSeaStr, quadSelContinentStr;
		bool quadSelCanContainSea = false;
		if (quadTreeSel != nullptr)
		{
			quadSelOptimizationLevelInfoStr = std::to_string(quadTreeSel->getQuadrant()->optimizationLevel());
			if (quadTreeSel->getQuadrant()->getCollider() != nullptr)
				quadSelColliderInfoStr = quadTreeSel->getQuadrant()->getCollider()->hasHiResData() ? "HIRES" : "LOWRES";
			quadSelCanContainSeaStr = std::to_string(quadTreeSel->canContainSea());
			quadSelCanContainSea = (quadTreeSel->canContainSea() == QUADRANT_SEA_INFO_TRUE);
			quadSelContinentStr = quadTreeSel->getQuadrant()->continentName();
		}

		std::string mouseHitInfoLabelText = "Hit: ";
		if (m_viewer->worldMode())
			mouseHitInfoLabelText += (std::string("X=") + std::format("{:.2f}", m_innerData->m_mouseHitPos.x) + " Y=" + std::format("{:.2f}", m_innerData->m_mouseHitPos.y) + " Z=" + std::format("{:.2f}", m_innerData->m_mouseHitPos.z));
		else
			mouseHitInfoLabelText += (std::string("X=") + std::format("{:.2f}", m_innerData->m_mouseHitPos.x) + " Z=" + std::format("{:.2f}", m_innerData->m_mouseHitPos.z));
		
		m_innerData->m_mouseHitInfoLabel->set_text(mouseHitInfoLabelText.c_str());


		std::string hitQuadInfoLabelText;

		//if (m_viewer->mapMode())
		hitQuadInfoLabelText =
			"Quad Hit: " + m_innerData->m_mouseQuadHitStr + " - Sea: " + quadHitCanContainSeaStr + "\n"
			+ "Pos: " + m_innerData->m_mouseQuadHitPosStr + "\n"
			+ /*"Type: " +*/ quadHitTerrainTypeStr + " - Min: " + quadHitMinHeightStr + " Max: " + quadHitMaxHeightStr + "\n"
			+ "Opt. Lvl: " + quadHitOptimizationLevelInfoStr + " Coll: " + quadHitColliderInfoStr + " Chunks: " + numVisibleChunksStr + " " + quadHitContinentStr + "\n"
			+ "To Upload: " + needToUploadStr + " To Save: " + needToSaveStr + " - Empty: " + quadHitEmptyStr + "\n"
			+ "Depth: " + DepthStr + " - Quads from Camera quad: " + quadsFromCameraQuadStr;

		m_innerData->m_hitQuadInfoLabel->set_text(hitQuadInfoLabelText.c_str());

		std::string selQuadInfoLabelText;
		if (quadTreeSel != nullptr)
		{
			selQuadInfoLabelText =
				"Quad Sel: " + m_innerData->m_mouseQuadSelStr + " - Sea: " + quadSelCanContainSeaStr + "\n"
				+ "Pos: " + m_innerData->m_mouseQuadSelPosStr + "\n"
				+ /*"Type: " +*/ TheWorld_Utils::TerrainEdit::terrainTypeString(m_innerData->m_selectedTerrainType) + " - Min: " + m_innerData->m_minHeightStr + " Max: " + m_innerData->m_maxHeightStr + "\n"
				+ "Opt. Lvl: " + quadSelOptimizationLevelInfoStr + " Coll: " + quadSelColliderInfoStr + " " + quadSelContinentStr + "\n"
				+ "To Upload: " + (quadTreeSel->getQuadrant()->getTerrainEdit()->needUploadToServer ? "True" : "False") + " To Save: " + (m_mapQuadToSave.contains(quadTreeSel->getQuadrant()->getPos()) ? "True" : "False") + " - Empty: " + (quadTreeSel->getQuadrant()->empty() ? "True" : "False");
		}
		m_innerData->m_selQuadInfoLabel->set_text(selQuadInfoLabelText.c_str());

		if (m_lastQuadTreeSel != quadTreeSel and quadTreeSel == nullptr)
		{
			// DeSelection of quadrant
			resetPanelsOnQuadrantDeselection();
		}
		
		if (quadSelCanContainSea && !m_innerData->m_selQuadSeaCheckBox->is_pressed())	
		{
			m_innerData->m_pressedViaCode = true;
			m_innerData->m_selQuadSeaCheckBox->set_pressed(true);
		}
		else if (!quadSelCanContainSea && m_innerData->m_selQuadSeaCheckBox->is_pressed())
		{
			m_innerData->m_pressedViaCode = true;
			m_innerData->m_selQuadSeaCheckBox->set_pressed(false);
		}

		if (m_innerData->m_onGoing)
		{
			if (!m_onGoingElapsedLabel)
			{
				m_innerData->m_elapsedLabel->add_theme_color_override("font_color", Color(1, 0, 0));
				m_onGoingElapsedLabel = true;
			}
		}
		else
		{
			if (m_onGoingElapsedLabel)
			{
				m_innerData->m_elapsedLabel->add_theme_color_override("font_color", Color(1, 1, 1));
				m_onGoingElapsedLabel = false;
			}
		}
		m_innerData->m_elapsedLabel->set_text(m_innerData->m_elapsed.c_str());
		m_innerData->m_elapsed1Label->set_text((m_innerData->m_counterStr + " " + m_innerData->m_note1Str).c_str());
		
		//m_scrollPanelsNeedResize = true;

		m_infoLabelTextChanged = false;
	}

	if (m_mapModder != nullptr && m_mapModder->startProcessRequired())
	{
		m_mapModder->startProcessing();
		//m_mapModder->startProcessRequired(false);
	}

	if (m_mapModder != nullptr && m_mapModder->deinitRequired())
	{
		m_mapModder->deinit();
		//m_mapModder->deinitRequired(false);
	}

	if (m_mapModder != nullptr)
	{
		m_mapModder->debug_process(_delta);
	}

	m_lastQuadTreeSel = quadTreeSel;
}

void GDN_TheWorld_Edit::changeTextureAction()
{
	QuadTree* quadTreeSel = nullptr;
	TheWorld_Utils::QuadrantPos quadrantSelPos = m_viewer->getWorldQuadrantSelForEdit(&quadTreeSel);
	if (quadTreeSel == nullptr)
		return;

	std::string selectedLayout = groundLayout();

	ShaderTerrainData::GroundTextures& groundTextures = ShaderTerrainData::getGroundTextures();
	std::string textureLayout;
	godot::Ref<godot::Texture2D> texturePreview = groundTextures.getTexturePreview(selectedLayout, m_innerData->m_selTexturePanel->m_newTexName, textureLayout);
	if (texturePreview == nullptr)
		return;

	TheWorld_Utils::TerrainEdit* terrainEdit = quadTreeSel->getQuadrant()->getTerrainEdit();

	if (m_innerData->m_selTexturePanel->m_newTexSlotName == c_lowElevationEditTextureName)
	{
		strcpy_s(terrainEdit->extraValues.lowElevationTexName_r, sizeof(terrainEdit->extraValues.lowElevationTexName_r), m_innerData->m_selTexturePanel->m_newTexName.c_str());
		m_innerData->m_lowElevationTexName->set_text(m_innerData->m_selTexturePanel->m_newTexName.c_str());
		m_innerData->m_lowElevationLayoutName->set_text(textureLayout.c_str());
		m_innerData->m_lowElevationTex->set_texture(texturePreview);
	}
	else if (m_innerData->m_selTexturePanel->m_newTexSlotName == c_highElevationEditTextureName)
	{
		strcpy_s(terrainEdit->extraValues.highElevationTexName_g, sizeof(terrainEdit->extraValues.highElevationTexName_g), m_innerData->m_selTexturePanel->m_newTexName.c_str());
		m_innerData->m_highElevationTexName->set_text(m_innerData->m_selTexturePanel->m_newTexName.c_str());
		m_innerData->m_highElevationLayoutName->set_text(textureLayout.c_str());
		m_innerData->m_highElevationTex->set_texture(texturePreview);
	}
	else if (m_innerData->m_selTexturePanel->m_newTexSlotName == c_dirtEditTextureName)
	{
		strcpy_s(terrainEdit->extraValues.dirtTexName_b, sizeof(terrainEdit->extraValues.dirtTexName_b), m_innerData->m_selTexturePanel->m_newTexName.c_str());
		m_innerData->m_dirtTexName->set_text(m_innerData->m_selTexturePanel->m_newTexName.c_str());
		m_innerData->m_dirtLayoutName->set_text(textureLayout.c_str());
		m_innerData->m_dirtTex->set_texture(texturePreview);
	}
	else if (m_innerData->m_selTexturePanel->m_newTexSlotName == c_rocksEditTextureName)
	{
		strcpy_s(terrainEdit->extraValues.rocksTexName_a, sizeof(terrainEdit->extraValues.rocksTexName_a), m_innerData->m_selTexturePanel->m_newTexName.c_str());
		m_innerData->m_rocksTexName->set_text(m_innerData->m_selTexturePanel->m_newTexName.c_str());
		m_innerData->m_rocksLayoutName->set_text(textureLayout.c_str());
		m_innerData->m_rocksTex->set_texture(texturePreview);
	}

	// Quadrant has to be saved and uploaded
	quadTreeSel->getQuadrant()->setNeedUploadToServer(true);
	terrainEdit->needUploadToServer = true;
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
		bool manual = true;
		addQuadToSave(quadrantSelPos, manual);
	}

	GDN_TheWorld_Map* map = m_viewer->Map();
	GDN_TheWorld_Map::MapQuadrantOverride* MapQuadrantOverride = map->getMapQuadrantOverride(quadrantSelPos);
	if (MapQuadrantOverride != nullptr)
	{
		MapQuadrantOverride->terrainEdit(*terrainEdit);
	}


	// Shader need refresh
	quadTreeSel->getQuadrant()->setTextureUpdated(true);
	quadTreeSel->getQuadrant()->getShaderTerrainData()->materialParamsNeedReset(true);
	quadTreeSel->getQuadrant()->northQuadrantData().initialized(false);
	quadTreeSel->getQuadrant()->southQuadrantData().initialized(false);
	quadTreeSel->getQuadrant()->westQuadrantData().initialized(false);
	quadTreeSel->getQuadrant()->eastQuadrantData().initialized(false);
	quadTreeSel->getQuadrant()->northWestQuadrantData().initialized(false);
	quadTreeSel->getQuadrant()->southEastQuadrantData().initialized(false);
}

godot::Window* GDN_TheWorld_Edit::initSelTexturePanel(void)
{
	godot::Error e;

	m_innerData->m_selTexturePanel->m_textureItems.clear();
	
	godot::Window* wnd = memnew(godot::Window);
	this->add_child(wnd);
	godot::Size2 windowSize(300.0f, 400.0f);
	wnd->set_flag(godot::Window::Flags::FLAG_ALWAYS_ON_TOP, true);
	wnd->set_initial_position(godot::Window::WindowInitialPosition::WINDOW_INITIAL_POSITION_ABSOLUTE);
	wnd->set_position(get_viewport_rect().size * godot::Size2(0.5f, 0.3f));
	wnd->set_flag(godot::Window::Flags::FLAG_BORDERLESS, true);
	wnd->set_min_size(windowSize);
	e = GDN_TheWorld_Globals::connectSignal(wnd, "godot::Window", "window_input", this, "gui_input_event", "SelTexturePanel_Window");
	
	godot::PanelContainer* panelContainer = createControl<godot::PanelContainer>(wnd, "SelTexturesPanel");
	godot::VBoxContainer* vBoxContainer = createControl<godot::VBoxContainer>(panelContainer);
	godot::Label* label = createControl<godot::Label>(vBoxContainer, "", m_innerData->m_selTexturePanel->m_selectedTexSlotName.c_str());
	godot::HBoxContainer* hBoxContainer = createControl<godot::HBoxContainer>(vBoxContainer);
	godot::Button* button = createControl<godot::Button>(hBoxContainer, "", "Set");
	e = GDN_TheWorld_Globals::connectSignal(button, "godot::Button", "pressed", this, "void_signal_manager", "SelTexturePanel_SetButton");
	button = createControl<godot::Button>(hBoxContainer, "", "Cancel");
	e = GDN_TheWorld_Globals::connectSignal(button, "godot::Button", "pressed", this, "void_signal_manager", "SelTexturePanel_CancelButton");
	godot::ScrollContainer* scrollContainer = createControl<godot::ScrollContainer>(vBoxContainer);
	godot::MarginContainer* marginContainer = createControl<godot::MarginContainer>(scrollContainer);
	scrollContainer->set_custom_minimum_size(windowSize - godot::Size2(0, 40));
	scrollContainer->set_h_size_flags(godot::Control::SizeFlags::SIZE_FILL);
	scrollContainer->set_v_size_flags(godot::Control::SizeFlags::SIZE_FILL);
	vBoxContainer = createControl<godot::VBoxContainer>(marginContainer);


	godot::Vector2 size(70, 70);

	std::string selectedLayout = groundLayout();


	ShaderTerrainData::GroundTextures& groundTextures = ShaderTerrainData::getGroundTextures();
	std::map<std::string, bool> textures;
	for (const auto& groundTexture : groundTextures)
		textures[groundTexture.second->textureName()] = true;

	int32_t idx = 0;
	for (const auto& texture : textures)
	{
		{
			m_innerData->m_selTexturePanel->m_textureItems[texture.first] = std::make_unique<InnerData::SelTexturePanel::TextureItemPanel>();
			InnerData::SelTexturePanel::TextureItemPanel* texItemPanel = m_innerData->m_selTexturePanel->m_textureItems[texture.first].get();

			texItemPanel->m_panel = createControl<godot::PanelContainer>(vBoxContainer, (std::string("SelTexturePanel_TexFrame_") + std::to_string(idx)).c_str());
			texItemPanel->m_idx = idx;
			if (m_innerData->m_selTexturePanel->m_selectedTexName == texture.first)
			{
				texItemPanel->m_panel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				texItemPanel->m_selected = true;
			}
			//std::string s(groundTexture.second->textureName());
			e = GDN_TheWorld_Globals::connectSignal(texItemPanel->m_panel, "godot::PanelContainer", "gui_input", this, "gui_input_event", "SelTexturePanel_ItemPanelGuiInput", texture.first.c_str(), idx);
			e = GDN_TheWorld_Globals::connectSignal(texItemPanel->m_panel, "godot::PanelContainer", "mouse_entered", this, "void_signal_manager", "SelTexturePanel_ItemPanelGuiInput", texture.first.c_str(), idx);
			e = GDN_TheWorld_Globals::connectSignal(texItemPanel->m_panel, "godot::PanelContainer", "mouse_exited", this, "void_signal_manager", "SelTexturePanel_ItemPanelGuiInput", texture.first.c_str(), idx);
			godot::HBoxContainer* hBoxContainer = createControl<godot::HBoxContainer>(texItemPanel->m_panel);
			//int i = hBoxContainer->get_theme_constant("separation");
			godot::TextureRect* tex = createControl<godot::TextureRect>(hBoxContainer);
			tex->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
			tex->set_custom_minimum_size(size);
			tex->set_stretch_mode(godot::TextureRect::STRETCH_SCALE);
			std::string textureLayout;
			tex->set_texture(groundTextures.getTexturePreview(selectedLayout, texture.first, textureLayout));
			godot::Label* label = createControl<godot::Label>(hBoxContainer, "", texture.first.c_str());

			idx++;
		}
	}

	label = createControl<godot::Label>(vBoxContainer, "", " ");

	return wnd;
}

void GDN_TheWorld_Edit::expandServerInfoPanel(void)
{
	bool visible = m_innerData->m_serverInfoVBoxContainer->is_visible();
	if (not visible)
	{
		m_innerData->m_serverInfoVBoxContainer->show();
		m_innerData->m_serverInfoButton->set_text((std::wstring(DOWN_ARROW) + L" " + SERVER_INFO_BUTTON_TEXT).c_str());
		m_scrollPanelsNeedResize = true;
	}
}

void GDN_TheWorld_Edit::compressServerInfoPanel(void)
{
	if (m_innerData->m_serverInfoVBoxContainer->is_visible())
	{
		m_innerData->m_serverInfoVBoxContainer->hide();
		m_innerData->m_serverInfoButton->set_text((std::wstring(RIGHT_ARROW) + L" " + SERVER_INFO_BUTTON_TEXT).c_str());
		m_scrollPanelsNeedResize = true;
	}
}

//void GDN_TheWorld_Edit::setAllChecked(bool allChecked)
//{
//	m_innerData->m_allCheckBox->set_pressed(allChecked);
//}

void GDN_TheWorld_Edit::expandTerrainGenerationPanel(void)
{
	if (not m_innerData->m_terrGenerationVBoxContainer->is_visible())
	{
		m_innerData->m_terrGenerationVBoxContainer->show();
		m_innerData->m_terrGenerationButton->set_text((std::wstring(DOWN_ARROW) + L" " + TERRAIN_GENERATION_BUTTON_TEXT).c_str());
		m_scrollPanelsNeedResize = true;
	}
}

void GDN_TheWorld_Edit::compressTerrainGenerationPanel(void)
{
	if (m_innerData->m_terrGenerationVBoxContainer->is_visible())
	{
		m_innerData->m_terrGenerationVBoxContainer->hide();
		m_innerData->m_terrGenerationButton->set_text((std::wstring(RIGHT_ARROW) + L" " + TERRAIN_GENERATION_BUTTON_TEXT).c_str());
		m_scrollPanelsNeedResize = true;
	}
}

void GDN_TheWorld_Edit::voidSignalManager(godot::Object* obj, godot::String signal, godot::String className, int instanceId, godot::String objectName, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3)
{
	//m_viewer->Globals()->debugPrint("GDN_TheWorld_Edit::voidSignalManager", true);

	std::string _signal = signal.utf8().get_data();
	std::string _className = className.utf8().get_data();
	std::string _objectName = objectName.utf8().get_data();

	if (TheWorld_Utils::to_lower(_signal) == "pressed")
	{
		if (_objectName == "NoisePanelToggle" && _className == "class godot::Button")
		{
			if (m_innerData->m_terrGenerationVBoxContainer->is_visible())
			{
				m_innerData->m_terrGenerationVBoxContainer->hide();
				((godot::Button*)obj)->set_text((std::wstring(RIGHT_ARROW) + L" " + TERRAIN_GENERATION_BUTTON_TEXT).c_str());
			}
			else
			{
				m_innerData->m_terrGenerationVBoxContainer->show();
				((godot::Button*)obj)->set_text((std::wstring(DOWN_ARROW) + L" " + TERRAIN_GENERATION_BUTTON_TEXT).c_str());
			}

			m_scrollPanelsNeedResize = true;
		}
		else if (_objectName == "ServerInfoPanelToggle" && _className == "class godot::Button")
		{
			if (m_innerData->m_serverInfoVBoxContainer->is_visible())
			{
				m_innerData->m_serverInfoVBoxContainer->hide();
				((godot::Button*)obj)->set_text((std::wstring(RIGHT_ARROW) + L" " + SERVER_INFO_BUTTON_TEXT).c_str());
			}
			else
			{
				m_innerData->m_serverInfoVBoxContainer->show();
				((godot::Button*)obj)->set_text((std::wstring(DOWN_ARROW) + L" " + SERVER_INFO_BUTTON_TEXT).c_str());
			}

			m_scrollPanelsNeedResize = true;
		}
		else if (_objectName == "TerrainInfoPanelToggle" && _className == "class godot::Button")
		{
			if (m_innerData->m_terrainInfoVBoxContainer->is_visible())
			{
				m_innerData->m_terrainInfoVBoxContainer->hide();
				((godot::Button*)obj)->set_text((std::wstring(RIGHT_ARROW) + L" " + TERRAIN_INFO_BUTTON_TEXT).c_str());
			}
			else
			{
				m_innerData->m_terrainInfoVBoxContainer->show();
				((godot::Button*)obj)->set_text((std::wstring(DOWN_ARROW) + L" " + TERRAIN_INFO_BUTTON_TEXT).c_str());
			}

			m_scrollPanelsNeedResize = true;
		}
		else if (_objectName == "TerrBlendingPanelToggle" && _className == "class godot::Button")
		{
			if (m_innerData->m_terrBlendingVBoxContainer->is_visible())
			{
				m_innerData->m_terrBlendingVBoxContainer->hide();
				((godot::Button*)obj)->set_text((std::wstring(RIGHT_ARROW) + L" " + TERRAIN_BLENDING_BUTTON_TEXT).c_str());
			}
			else
			{
				m_innerData->m_terrBlendingVBoxContainer->show();
				((godot::Button*)obj)->set_text((std::wstring(DOWN_ARROW) + L" " + TERRAIN_BLENDING_BUTTON_TEXT).c_str());
			}

			m_scrollPanelsNeedResize = true;
		}
		else if (_objectName == "TerrEditPanelToggle" && _className == "class godot::Button")
		{
			if (m_innerData->m_quadrantTextureEditVBoxContainer->is_visible())
			{
				m_innerData->m_quadrantTextureEditVBoxContainer->hide();
				((godot::Button*)obj)->set_text((std::wstring(RIGHT_ARROW) + L" " + TERRAIN_EDIT_BUTTON_TEXT).c_str());
			}
			else
			{
				m_innerData->m_quadrantTextureEditVBoxContainer->show();
				((godot::Button*)obj)->set_text((std::wstring(DOWN_ARROW) + L" " + TERRAIN_EDIT_BUTTON_TEXT).c_str());
			}

			m_scrollPanelsNeedResize = true;
		}
		else if (godot::String(custom1) == "SelTexturePanel_SetButton")
		{
			for (auto& texturePanel : m_innerData->m_selTexturePanel->m_textureItems)
			{
				if (texturePanel.second->m_selected)
				{
					m_innerData->m_selTexturePanel->m_changeTextureRequired = true;
					m_innerData->m_selTexturePanel->m_newTexSlotName = m_innerData->m_selTexturePanel->m_selectedTexSlotName;
					m_innerData->m_selTexturePanel->m_newTexName = texturePanel.first;
					break;
				}
			}
		}
		else if (godot::String(custom1) == "SelTexturePanel_CancelButton")
		{
			m_innerData->m_selTexturePanel->m_closeSelTexturesRequired = true;
		}
	}
	else if (TheWorld_Utils::to_lower(_signal) == "mouse_entered")
	{
		if (_className == "godot::PanelContainer")
		{
			if (godot::String(custom1) == c_lowElevationEditTextureName.c_str())
			{
				if (!m_innerData->m_lowElevationTexSelected)
					m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_highElevationTexSelected)
					m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_dirtTexSelected)
					m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_rocksTexSelected)
					m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");

				if (m_innerData->m_lowElevationTexSelected)
					m_innerData->m_lowElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				else
					m_innerData->m_lowElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxHighlightedFrame);

				m_innerData->m_mouseInsideEditPanel = true;
			}
			else if (godot::String(custom1) == c_highElevationEditTextureName.c_str())
			{
				if (!m_innerData->m_lowElevationTexSelected)
					m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_highElevationTexSelected)
					m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_dirtTexSelected)
					m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_rocksTexSelected)
					m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");

				if (m_innerData->m_highElevationTexSelected)
					m_innerData->m_highElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				else
					m_innerData->m_highElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxHighlightedFrame);

				m_innerData->m_mouseInsideEditPanel = true;
			}
			else if (godot::String(custom1) == c_dirtEditTextureName.c_str())
			{
				if (!m_innerData->m_lowElevationTexSelected)
					m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_highElevationTexSelected)
					m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_dirtTexSelected)
					m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_rocksTexSelected)
					m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");

				if (m_innerData->m_dirtTexSelected)
					m_innerData->m_dirtTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				else
					m_innerData->m_dirtTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxHighlightedFrame);

				m_innerData->m_mouseInsideEditPanel = true;
			}
			else if (godot::String(custom1) == c_rocksEditTextureName.c_str())
			{
				if (!m_innerData->m_lowElevationTexSelected)
					m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_highElevationTexSelected)
					m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_dirtTexSelected)
					m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_rocksTexSelected)
					m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");

				if (m_innerData->m_rocksTexSelected)
					m_innerData->m_rocksTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				else
					m_innerData->m_rocksTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxHighlightedFrame);

				m_innerData->m_mouseInsideEditPanel = true;
			}
			else if (godot::String(custom1) == "SelTexturePanel_ItemPanelGuiInput")
			{
				std::string textureName = godot::String(custom2).utf8().get_data();
				if (m_innerData->m_selTexturePanel->m_textureItems.contains(textureName) && !m_innerData->m_selTexturePanel->m_textureItems[textureName]->m_selected)
					m_innerData->m_selTexturePanel->m_textureItems[textureName]->m_panel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxHighlightedFrame);

				m_innerData->m_mouseInsideEditPanel = true;
			}
		}
	}
	else if (TheWorld_Utils::to_lower(_signal) == "mouse_exited")
	{
		if (_className == "godot::PanelContainer")
		{
			if (godot::String(custom1) == c_lowElevationEditTextureName.c_str()
				|| godot::String(custom1) == c_highElevationEditTextureName.c_str()
				|| godot::String(custom1) == c_dirtEditTextureName.c_str()
				|| godot::String(custom1) == c_rocksEditTextureName.c_str())
			{
				if (!m_innerData->m_lowElevationTexSelected)
					m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_highElevationTexSelected)
					m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_dirtTexSelected)
					m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_rocksTexSelected)
					m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");

				m_innerData->m_mouseInsideEditPanel = false;
			}
			else if (godot::String(custom1) == "SelTexturePanel_ItemPanelGuiInput")
			{
				std::string textureName = godot::String(custom2).utf8().get_data();
				if (m_innerData->m_selTexturePanel->m_textureItems.contains(textureName) && !m_innerData->m_selTexturePanel->m_textureItems[textureName]->m_selected)
					m_innerData->m_selTexturePanel->m_textureItems[textureName]->m_panel->remove_theme_stylebox_override("panel");

				m_innerData->m_mouseInsideEditPanel = false;
			}
		}
	}
}

void GDN_TheWorld_Edit::guiInputEvent(const godot::Ref<godot::InputEvent>& event, godot::Object* obj, godot::String signal, godot::String className, int instanceId, godot::String objectName, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3)
{
	//m_viewer->Globals()->debugPrint("GDN_TheWorld_Edit::guiInputEvent", true);

	std::string _signal = signal.utf8().get_data();
	std::string _className = className.utf8().get_data();
	std::string _objectName = objectName.utf8().get_data();

	if (TheWorld_Utils::to_lower(_signal) == "gui_input")
	{
		const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
		const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
		
		if ( (_objectName == "LowElevationTexPanel" || _objectName == "HighElevationTexPanel" || _objectName == "DirtTexPanel" || _objectName == "RocksTexPanel")
			&& eventMouseButton != nullptr && eventMouseButton->is_double_click() && eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_LEFT
			&& m_viewer->isWorldQuadrantSelectedForEdit())
		{
			//accept_event();
			
			m_innerData->m_selTexturePanel->m_closeSelTexturesRequired = true;
			m_innerData->m_selTexturePanel->m_openSelTexturesRequired = true;
			m_innerData->m_selTexturePanel->m_selectedTexSlotName = godot::String(custom1).utf8().get_data();
			if (_objectName == "LowElevationTexPanel")
			{
				m_innerData->m_selTexturePanel->m_selectedTexName = m_innerData->m_lowElevationTexName->get_text().utf8().get_data();
				m_innerData->m_lowElevationTexSelected = true;
				m_innerData->m_highElevationTexSelected = false;
				m_innerData->m_dirtTexSelected = false;
				m_innerData->m_rocksTexSelected = false;
			}
			else if (_objectName == "HighElevationTexPanel")
			{
				m_innerData->m_selTexturePanel->m_selectedTexName = m_innerData->m_highElevationTexName->get_text().utf8().get_data();
				m_innerData->m_lowElevationTexSelected = false;
				m_innerData->m_highElevationTexSelected = true;
				m_innerData->m_dirtTexSelected = false;
				m_innerData->m_rocksTexSelected = false;
			}
			else if (_objectName == "DirtTexPanel")
			{
				m_innerData->m_selTexturePanel->m_selectedTexName = m_innerData->m_dirtTexName->get_text().utf8().get_data();
				m_innerData->m_lowElevationTexSelected = false;
				m_innerData->m_highElevationTexSelected = false;
				m_innerData->m_dirtTexSelected = true;
				m_innerData->m_rocksTexSelected = false;
			}
			else if (_objectName == "RocksTexPanel")
			{
				m_innerData->m_selTexturePanel->m_selectedTexName = m_innerData->m_rocksTexName->get_text().utf8().get_data();
				m_innerData->m_lowElevationTexSelected = false;
				m_innerData->m_highElevationTexSelected = false;
				m_innerData->m_dirtTexSelected = false;
				m_innerData->m_rocksTexSelected = true;
			}

			//m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
			//m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
			//m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
			//m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");
			//if (m_innerData->m_lowElevationTexSelected)
			//	m_innerData->m_lowElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			//else if (m_innerData->m_highElevationTexSelected)
			//	m_innerData->m_highElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			//else if (m_innerData->m_dirtTexSelected)
			//	m_innerData->m_dirtTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			//else if (m_innerData->m_rocksTexSelected)
			//	m_innerData->m_rocksTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
		}
		else if (godot::String(custom1) == "SelTexturePanel_ItemPanelGuiInput")
		{
			if (eventMouseButton != nullptr && eventMouseButton->is_double_click() && eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_LEFT)
			{
				//accept_event();

				std::string selectedLayout = groundLayout();

				std::string selectedTextureName = godot::String(custom2).utf8().get_data();
				ShaderTerrainData::GroundTextures& groundTextures = ShaderTerrainData::getGroundTextures();
				int32_t idx = int32_t(custom3);		// DEBUG ONLY
				if (groundTextures.containTexture(selectedLayout, selectedTextureName))
				{
					for (auto& texturePanel : m_innerData->m_selTexturePanel->m_textureItems)
					{
						if (texturePanel.second->m_selected)
						{
							texturePanel.second->m_selected = false;
							texturePanel.second->m_panel->remove_theme_stylebox_override("panel");
						}
					}
					m_innerData->m_selTexturePanel->m_textureItems[selectedTextureName]->m_selected = true;
					m_innerData->m_selTexturePanel->m_textureItems[selectedTextureName]->m_panel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
					m_innerData->m_selTexturePanel->m_changeTextureRequired = true;
					m_innerData->m_selTexturePanel->m_newTexSlotName = m_innerData->m_selTexturePanel->m_selectedTexSlotName;
					m_innerData->m_selTexturePanel->m_newTexName = selectedTextureName;
				}
			}
			else if (eventMouseButton != nullptr && eventMouseButton->is_pressed() && eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_LEFT)
			{
				//accept_event();
				
				std::string selectedLayout = groundLayout();

				std::string selectedTextureName = godot::String(custom2).utf8().get_data();
				ShaderTerrainData::GroundTextures& groundTextures = ShaderTerrainData::getGroundTextures();
				int32_t idx = int32_t(custom3);			// DEBUG ONLY
				if (groundTextures.containTexture(selectedLayout, selectedTextureName))
				{
					if (m_innerData->m_selTexturePanel->m_textureItems.contains(selectedTextureName) && !m_innerData->m_selTexturePanel->m_textureItems[selectedTextureName]->m_selected)
					{
						for (auto& texturePanel : m_innerData->m_selTexturePanel->m_textureItems)
						{
							if (texturePanel.second->m_selected)
							{
								texturePanel.second->m_selected = false;
								texturePanel.second->m_panel->remove_theme_stylebox_override("panel");
							}
						}
						m_innerData->m_selTexturePanel->m_textureItems[selectedTextureName]->m_selected = true;
						m_innerData->m_selTexturePanel->m_textureItems[selectedTextureName]->m_panel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
					}
				}
			}
		}
	}
	else if (TheWorld_Utils::to_lower(_signal) == "window_input")
	{
		if (godot::String(custom1) == "SelTexturePanel_Window")
		{
			const godot::InputEventKey* eventKey = godot::Object::cast_to<godot::InputEventKey>(event.ptr());

			if (eventKey != nullptr && eventKey->is_pressed() && eventKey->get_keycode() == godot::Key::KEY_ESCAPE)
			{
				m_innerData->m_selTexturePanel->m_closeSelTexturesRequired = true;
			}
		}
	}
}

//bool GDN_TheWorld_Edit::hiResCollisionGridEnabled(void)
//{
//	return m_innerData->m_collisonHiResGridCheckBox->is_pressed();
//}

//bool GDN_TheWorld_Edit::lowResCollisionGridEnabled(void)
//{
//	return m_innerData->m_collisonLowResGridCheckBox->is_pressed();
//}

void GDN_TheWorld_Edit::toggledSignalManager(const bool pressed, godot::Object* obj, godot::String signal, godot::String className, int instanceId, godot::String objectName, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3)
{
	std::string _signal = signal.utf8().get_data();
	std::string _className = className.utf8().get_data();
	std::string _objectName = objectName.utf8().get_data();

	if (TheWorld_Utils::to_lower(_signal) != "toggled")
		return;

	if (m_viewer == nullptr)
		return;

	if (_objectName == "ShowGizmoCheckBox")
	{
		if (pressed)
			m_viewer->gizmoVisible(true);
		else
			m_viewer->gizmoVisible(false);
	}
	else if (_objectName == "CheckBlendCheckBox")
	{
		if (pressed)
			Globals()->setParamShowNeedBlending(true);
		else
			Globals()->setParamShowNeedBlending(false);
	}
	else if (_objectName == "ShowNormalCheckBox")
	{
		if (pressed)
			m_viewer->normalVisible(true);
		else
			m_viewer->normalVisible(false);
	}
	else if (_objectName == "ShowPositionCheckBox")
	{
		if (pressed)
		{
			m_viewer->showPositionOnCursorHit(true);
		}
		else
		{
			m_viewer->showPositionOnCursorHit(false);
			m_innerData->m_gridON->set_pressed(false);
		}
	}
	else if (_objectName == "ShowTerrainCheckBox")
	{
		if (pressed)
			Globals()->setParamShowTerrain(true);
		else
			Globals()->setParamShowTerrain(false);
	}
	else if (_objectName == "ShowSeaCheckBox")
	{
		if (pressed)
			Globals()->setParamShowSea(true);
		else
			Globals()->setParamShowSea(false);
	}
	else if (_objectName == "ShowFogCheckBox")
	{
		if (pressed)
			Globals()->setParamShowFog(true);
		else
			Globals()->setParamShowFog(false);
	}
	else if (_objectName == "ShowChunksCheckBox")
	{
		if (pressed)
			Globals()->setParamShowChunks(true);
		else
			Globals()->setParamShowChunks(false);
	}
	else if (_objectName == "ShowWireframeCheckBox")
	{
		if (pressed)
			Globals()->setParamShowWireframe(true);
		else
			Globals()->setParamShowWireframe(false);
	}
	else if (_objectName == "HighlightHitQuadrantCheckBox")
	{
		if (pressed)
			m_viewer->highlightHitQuadrant(true);
		else
			m_viewer->highlightHitQuadrant(false);
	}
	else if (_objectName == "GridOnCheckBox")
	{
		if (pressed)
			m_viewer->gridOn(true);
		else
			m_viewer->gridOn(false);
	}
	else if (_objectName == "CastRayCheckBox")
	{
		if (pressed)
			m_viewer->castRayFromCamera(true);
		else
			m_viewer->castRayFromCamera(false);
	}
	else if (_objectName == "SelectedQuadSeaCheckBox")
	{
		if (!m_innerData->m_pressedViaCode)
		{
			QuadTree* quadTreeSel = nullptr;
			TheWorld_Utils::QuadrantPos quadrantSelPos = m_viewer->getWorldQuadrantSelForEdit(&quadTreeSel);
			if (quadTreeSel != nullptr)
			{
				TheWorld_Utils::TerrainEdit* terrainEdit = quadTreeSel->getQuadrant()->getTerrainEdit();
				if (pressed)
				{
					terrainEdit->extraValues.seaInfoSet = QUADRANT_SEA_INFO_TRUE;
					terrainEdit->extraValues.canContainSea = QUADRANT_SEA_INFO_TRUE;
					quadTreeSel->canContainSea(QUADRANT_SEA_INFO_TRUE);
				}
				else
				{
					terrainEdit->extraValues.seaInfoSet = QUADRANT_SEA_INFO_TRUE;
					terrainEdit->extraValues.canContainSea = QUADRANT_SEA_INFO_FALSE;
					quadTreeSel->canContainSea(QUADRANT_SEA_INFO_FALSE);
				}
				Chunk::SeaInfoChangedChunkAction action;
				quadTreeSel->ForAllChunk(action, QuadType::NotSet);

				//quadTreeSel->refreshSeaInfo();
				//quadTreeSel->updateForAllChunksForced(QuadType::Sea, true);

				quadTreeSel->getQuadrant()->setNeedUploadToServer(true);
				terrainEdit->needUploadToServer = true;
				{
					std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
					bool manual = true;
					addQuadToSave(quadrantSelPos, manual);
				}
			}
		}

		m_innerData->m_pressedViaCode = false;
	}
	//else if (_objectName == "CollisonHiResGridCheckBox")
	//{
	//	if (pressed && m_innerData->m_collisonLowResGridCheckBox->is_pressed())
	//		m_innerData->m_collisonLowResGridCheckBox->set_pressed(false);

	//	m_viewer->setColliderNeedUpdateForEveryQuadrant();
	//}
	//else if (_objectName == "CollisonLowResGridCheckBox")
	//{
	//	if (pressed && m_innerData->m_collisonHiResGridCheckBox->is_pressed())
	//		m_innerData->m_collisonHiResGridCheckBox->set_pressed(false);

	//	m_viewer->setColliderNeedUpdateForEveryQuadrant();
	//}
}

void GDN_TheWorld_Edit::editModeStopAction(void)
{
	if (!m_actionInProgress)
		return;

	m_actionStopRequested = true;
}

void GDN_TheWorld_Edit::editModeReloadShader(void)
{
	//bool allCheched = m_innerData->m_allCheckBox->is_pressed();
	bool allCheched = false;

	if (allCheched)
		m_viewer->ReloadAllShadersRequired(true);
	else
		m_viewer->ReloadSelectedQuadrantShaderRequired(true);
}

void GDN_TheWorld_Edit::editModeSaveAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	m_allItems = 0;
	setElapsed(0, true);
	setNote1(0);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeSave, this);
	m_tp.QueueJob(f);
}

size_t GDN_TheWorld_Edit::numQuadrantsRequiringMapModding(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
	
	std::vector<TheWorld_Utils::QuadrantPos> allQuadrantPos;
	m_viewer->getAllQuadrantPos(allQuadrantPos);

	size_t num = 0;

	for (auto& pos : allQuadrantPos)
	{
		QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
		if (quadTree != nullptr)
		{
			if (quadTree->getQuadrant()->mapModdingRequired())
				num++;
		}
	}

	return num;
}

void GDN_TheWorld_Edit::editModeSave(void)
{
	size_t num = numQuadrantsRequiringMapModding();
	if (num > 0)
	{
		setMessage(godot::String("Cannot save quadrants map modding required!"));
		m_actionInProgress = false;
		return;
	}

	if (m_actionClock.counterStarted())
	{
		m_actionInProgress = false;
		return;
	}

	setMessage(godot::String("Saving world quadrants..."));

	m_actionClock.tick();

	TheWorld_Utils::GuardProfiler profiler(std::string("EditSave 1 ") + __FUNCTION__, "ALL");

	m_completedItems = 0;
	m_elapsedCompleted = 0;
	m_lastElapsed = 0;

	{
		std::vector<TheWorld_Utils::QuadrantPos> quadToSave;
		{
			std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
			for (auto& item : m_mapQuadToSave)
				quadToSave.push_back(item.first);
		}

		m_allItems = quadToSave.size();
		for (auto& pos : quadToSave)
		{
			if (m_actionStopRequested)
			{
				m_actionStopRequested = false;
				break;
			}

			QuadTree* quadToSave = nullptr;
			{
				std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
				quadToSave = m_viewer->getQuadTree(pos, false);
				if (quadToSave != nullptr /* and !quadToSave->getQuadrant()->empty()*/)	// empty buffer must be saved because terrain datas may be changed (for instance needblend true/false)
				{
					bool lockAcquired = quadToSave->getQuadrant()->lockInternalData();
					if (!lockAcquired)
						quadToSave = nullptr;
				}
			}
			
			if (quadToSave != nullptr)
			{
				bool saved = quadToSave->getQuadrant()->saveQuadrantToDiskCache2(true);
				my_assert(saved);
				if (!saved)
					throw(GDN_TheWorld_Exception(__FUNCTION__, "Quadrant not saved!"));

				m_completedItems++;
				size_t partialCount = m_actionClock.partialDuration().count();
				m_lastElapsed = partialCount - m_elapsedCompleted;
				m_elapsedCompleted = partialCount;
				m_lastElapsed = m_actionClock.partialDuration().count() - m_lastElapsed;
				
				removeQuadToSave(pos);
				
				quadToSave->getQuadrant()->unlockInternalData();

				{
					std::string s = "Saving world quadrants... " + std::to_string(m_completedItems) + "/" + std::to_string(m_allItems);
					setMessage(godot::String(s.c_str()));
					//Sleep(10);
				}
			}

		}
	}

	m_actionClock.tock();

	m_actionInProgress = false;

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	//size_t numToSave = 0;
	//size_t numToUpload = 0;
	//size_t numLocked = 0;
	//refreshNumToSaveUpload(numToSave, numToUpload, numLocked);

	setMessage(godot::String("Completed!"), true);
}

void GDN_TheWorld_Edit::editModeUploadAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	m_allItems = 0;
	setElapsed(0, true);
	setNote1(0);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeUpload, this);
	m_tp.QueueJob(f);
}

bool GDN_TheWorld_Edit::editModeUploadModifiedWorldQuadrants(GDN_TheWorld_Map* map)
{
	bool allUploaded = true;

	size_t num = numQuadrantsRequiringMapModding();
	if (num > 0)
	{
		setMessage(godot::String("Cannot upload map modding required!"));
		m_actionInProgress = false;
		allUploaded = false;
		return allUploaded;
	}

	if (m_actionClock.counterStarted() or m_uploadModifiedWorldQuadrantsInProgress)
	{
		setMessage(godot::String("Uploading world quadrants already in progress!"));
		m_actionInProgress = false;
		allUploaded = false;
		return allUploaded;
	}


	m_uploadModifiedWorldQuadrantsInProgress = true;

	editModeSave();

	setMessage(godot::String("Uploading world quadrants..."));

	m_actionClock.tick();

	TheWorld_Utils::GuardProfiler profiler(std::string("EditUpload 1 ") + __FUNCTION__, "ALL");

	{
		std::vector<TheWorld_Utils::QuadrantPos> quadrantPosToUpload;

		{
			std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());

			QuadTree* quadTreeSel = nullptr;
			TheWorld_Utils::QuadrantPos quadrantSelPos = m_viewer->getWorldQuadrantSelForEdit(&quadTreeSel);
			//bool allCheched = m_innerData->m_allCheckBox->is_pressed();
			bool allCheched = true;
			if (!allCheched && (quadrantSelPos.empty() or !quadTreeSel->getQuadrant()->getTerrainEdit()->needUploadToServer))
			{
				m_actionClock.tock();
				m_actionInProgress = false;
				setMessage(godot::String("Completed!"), true);
				m_uploadModifiedWorldQuadrantsInProgress = false;
				return allUploaded;
			}

			if (allCheched)
			{
				std::vector<TheWorld_Utils::QuadrantPos> allQuadrantPos;
				m_viewer->getAllQuadrantPos(allQuadrantPos);

				for (auto& pos : allQuadrantPos)
				{
					QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
					if (quadTree != nullptr)
					{
						if (quadTree->getQuadrant()->needUploadToServer())
						{
							quadrantPosToUpload.push_back(pos);
						}
					}
				}
			}
			else
				quadrantPosToUpload.push_back(quadrantSelPos);
		}

		m_completedItems = 0;
		m_elapsedCompleted = 0;
		m_lastElapsed = 0;
		m_allItems = quadrantPosToUpload.size();
		size_t uploadedItems = 0;
		for (auto& pos : quadrantPosToUpload)
		{
			if (m_actionStopRequested)
			{
				m_actionStopRequested = false;
				break;
			}

			QuadTree* quadTree = nullptr;
			{
				std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
				quadTree = m_viewer->getQuadTree(pos, false);
				if (quadTree != nullptr)
				{
					bool lockAcquired = quadTree->getQuadrant()->lockInternalData();
					if (!lockAcquired)
						quadTree = nullptr;
				}
			}
			if (quadTree == nullptr)
				allUploaded = false;
			else
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("EditUpload 1.1 ") + __FUNCTION__, "Single QuadTree");

				std::string buffer;
				bool saved = quadTree->getQuadrant()->saveQuadrantToDiskCache2(buffer, false, true, false);
				my_assert(saved);

				if (!saved)
				{
					m_uploadModifiedWorldQuadrantsInProgress = false;
					throw(GDN_TheWorld_Exception(__FUNCTION__, "Quadrant not saved!"));
				}

				float lowerXGridVertex = pos.getLowerXGridVertex();
				float lowerZGridVertex = pos.getLowerZGridVertex();
				int numVerticesPerSize = pos.getNumVerticesPerSize();
				float gridStepInWU = pos.getGridStepInWU();
				int lvl = pos.getLevel();
				quadTree->Viewer()->Globals()->Client()->MapManagerUploadBuffer(m_viewer->isInEditor(), lowerXGridVertex, lowerZGridVertex, numVerticesPerSize, gridStepInWU, lvl, buffer, m_viewer->getNumQuadrantsPerSize(), m_viewer->getQuadrantGlobalSize(), m_viewer->getQuadrantWorldSize());

				quadTree->getQuadrant()->unlockInternalData();

				uploadedItems++;

				{
					std::string s = "Uploading world quadrants... " + std::to_string(uploadedItems) + "/" + std::to_string(m_allItems);
					setMessage(godot::String(s.c_str()));
					//Sleep(10);
				}
			}
		}

		//std::list<TheWorld_Utils::QuadrantPos> list;
		//map->getAllMapQuadrantOverridePos(list, true);
		//if (list.size() > 0)
		//{
		//	for (auto& item : list)
		//	{
		//		map->removeEditedMapQuadrantOverride(item);
		//	}
		//}
	}

	size_t completedItems = 0;
	while (!m_actionStopRequested && m_allItems >= m_completedItems)
	{
		if (m_completedItems != completedItems)
		{
			std::string s = "Getting ack ... " + std::to_string(m_completedItems) + "/" + std::to_string(m_allItems);
			setMessage(godot::String(s.c_str()));
			completedItems = m_completedItems;
		}
		if (m_allItems > m_completedItems)
			Sleep(10);
		else
			break;
	}

	m_actionClock.tock();

	m_actionInProgress = false;

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	setMessage(godot::String("Completed!"), true);

	m_uploadModifiedWorldQuadrantsInProgress = false;
	return allUploaded;
}

void GDN_TheWorld_Edit::editModeUploadMapQuadrantOverrides(GDN_TheWorld_Map* map)
{
	size_t num = numQuadrantsRequiringMapModding();
	if (num > 0)
	{
		setMessage(godot::String("Cannot upload map modding required!"));
		m_actionInProgress = false;
		return;
	}

	if (m_actionClock.counterStarted() or m_uploadMapQuadrantOverridesInProgress)
	{
		setMessage(godot::String("Uploading map quadrant overrides already in progress!"));
		m_actionInProgress = false;
		return;
	}

	setMessage(godot::String("Uploading map quadrant overrides..."));

	m_actionClock.tick();

	std::list<TheWorld_Utils::QuadrantPos> list;
	map->getAllMapQuadrantOverridePos(list, true);
	if (list.size() > 0)
	{
		call_deferred("expand_server_info_panel");
		waitingForServerGen(true);

		std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());

		m_uploadMapQuadrantOverridesInProgress = true;

		TheWorld_Utils::MemoryBuffer buffer;
		buffer.reserve(10000);		// preallocation for optimization purpose

		*buffer.ptr() = '0';
		size_t cursor = 1;
		buffer.adjustSize(cursor);

		size_t size;
		TheWorld_Utils::serializeToByteStream<size_t>(list.size(), buffer.ptr() + cursor, size);
		buffer.adjustCursorAndSize(cursor, size);

		for (auto& item : list)
		{
			{
				// TheWorld_Utils::QuadrantPos

				TheWorld_Utils::MemoryBuffer quadrantPosBuffer;
				item.serialize(quadrantPosBuffer);
				buffer.reserve(cursor + sizeof(size_t));				// to avoid memory overflow
				TheWorld_Utils::serializeToByteStream<size_t>(quadrantPosBuffer.size(), buffer.ptr() + cursor, size);
				buffer.adjustCursorAndSize(cursor, size);
				buffer.reserve(cursor + quadrantPosBuffer.size());		// to avoid memory overflow
				memcpy(buffer.ptr() + cursor, quadrantPosBuffer.ptr(), quadrantPosBuffer.size());
				buffer.adjustCursorAndSize(cursor, quadrantPosBuffer.size());
			}

			GDN_TheWorld_Map::MapQuadrantOverride* mapQuadrantOverride = map->getMapQuadrantOverride(item);
			TheWorld_Utils::TerrainEdit terrainEdit = mapQuadrantOverride->terrainEdit();

			{
				// TheWorld_Utils::TerrainEdit

				TheWorld_Utils::MemoryBuffer terrainEditBuffer;
				terrainEdit.serialize(terrainEditBuffer);
				buffer.reserve(cursor + sizeof(size_t));				// to avoid memory overflow
				TheWorld_Utils::serializeToByteStream<size_t>(terrainEditBuffer.size(), buffer.ptr() + cursor, size);
				buffer.adjustCursorAndSize(cursor, size);
				buffer.reserve(cursor + terrainEditBuffer.size());		// to avoid memory overflow
				memcpy(buffer.ptr() + cursor, terrainEditBuffer.ptr(), terrainEditBuffer.size());
				buffer.adjustCursorAndSize(cursor, terrainEditBuffer.size());
			}
		}

		int numVerticesPerSize = (int)m_viewer->Globals()->numVerticesPerQuadrantSize();
		float gridStepInWU = m_viewer->Globals()->gridStepInWU();
		int lvl = m_viewer->getLevel();
		std::string strBuffer;
		buffer.toString(strBuffer);
		m_viewer->Globals()->Client()->MapManagerUploadQuadrantOverrides(m_viewer->isInEditor(), numVerticesPerSize, gridStepInWU, lvl, strBuffer);
	}

	while (m_uploadMapQuadrantOverridesInProgress)
		Sleep(10);

	m_actionClock.tock();

	m_actionInProgress = false;

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	setMessage(godot::String("Completed!"), true);
}

void GDN_TheWorld_Edit::editModeUpload(void)
{
	size_t num = numQuadrantsRequiringMapModding();
	if (num > 0)
	{
		setMessage(godot::String("Cannot upload map modding required!"));
		m_actionInProgress = false;
		return;
	}

	m_editorUploadInProgress = true;

	GDN_TheWorld_Map* map = m_viewer->Map();
	if (m_viewer->worldMode())
	{
		editModeUploadModifiedWorldQuadrants(map);
	}
	else
	{
		editModeUploadModifiedWorldQuadrants(map);
		editModeUploadMapQuadrantOverrides(map);
	}

	m_editorUploadInProgress = false;
}

void GDN_TheWorld_Edit::editModeGenerateAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	m_allItems = 0;
	setElapsed(0, true);
	setNote1(0);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeGenerate, this);
	m_tp.QueueJob(f);
}

void GDN_TheWorld_Edit::editModeSetTerrType(void)
{
	// signal "pressed"
	int64_t id = m_innerData->m_terrTypeOptionButton->get_selected_id();
	if (id != -1)
	{
		TheWorld_Utils::TerrainEdit::TerrainType selectedTerrainType = (TheWorld_Utils::TerrainEdit::TerrainType)id;
		if (selectedTerrainType == TheWorld_Utils::TerrainEdit::TerrainType::notset)
			m_innerData->m_terrTypeButton->set_pressed(false);
	}

	if (m_innerData->m_terrTypeButton->is_pressed())
	{
		editingMap(true);
	}
	else
	{
		editingMap(false);
	}
}

void GDN_TheWorld_Edit::editModeGenerate(void)
{
	bool holdBlending = GDN_TheWorld_Edit::holdBlending();

	m_viewer->getMainProcessingMutex().lock();
	if (m_actionClock.counterStarted())
	{
		m_viewer->getMainProcessingMutex().unlock();
		m_actionInProgress = false;
		return;
	}

	if (terrainDataLocked())
	{
		m_viewer->getMainProcessingMutex().unlock();
		setMessage(godot::String("Terrain data locked wait..."));
		return;
	}
	terrainDataLocked(true);
	m_viewer->getMainProcessingMutex().unlock();

	setMessage(godot::String("Generating heights..."));

	m_actionClock.tick();

	QuadTree* quadTreeSel = nullptr;
	TheWorld_Utils::QuadrantPos quadrantSelPos = m_viewer->getWorldQuadrantSelForEdit(&quadTreeSel);

	if (quadrantSelPos.empty())
	{
		m_actionClock.tock();
		m_actionInProgress = false;
		setMessage(godot::String("No quad selected!"));
		terrainDataLocked(false);
		return;
	}

	m_completedItems = 0;
	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_allItems = 1;

	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1 ") + __FUNCTION__, "ALL");

	int64_t id = m_innerData->m_terrTypeOptionButton->get_selected_id();
	enum class TheWorld_Utils::TerrainEdit::TerrainType terrainType = (enum class TheWorld_Utils::TerrainEdit::TerrainType)id;

	TheWorld_Utils::TerrainEdit* terrainEdit = quadTreeSel->getQuadrant()->getTerrainEdit();
	terrainEdit->terrainType = terrainType;
	terrainEdit->noise.noiseSeed = seed();
	terrainEdit->noise.frequency = frequency();
	terrainEdit->noise.fractalOctaves = octaves();
	terrainEdit->noise.fractalLacunarity = lacunarity();
	terrainEdit->noise.fractalGain = gain();
	terrainEdit->noise.fractalWeightedStrength = weightedStrength();
	terrainEdit->noise.fractalPingPongStrength = pingPongStrength();
	terrainEdit->noise.heightsAmplitude = amplitude();
	terrainEdit->noise.scaleFactor_JagIntensity = scaleFactor();
	terrainEdit->noise.desideredMinHeight = desideredMinHeight();
	terrainEdit->extraValues.ignoreDesideredMinHeight = ignoreDesideredMinHeight();
	terrainEdit->extraValues.postGenerateElevation = postGenerateElevation();
	//terrainEdit->needUploadToServer = true;

	int level = quadrantSelPos.getLevel();
	size_t numVerticesPerSize = quadrantSelPos.getNumVerticesPerSize();
	size_t numVertices = numVerticesPerSize * numVerticesPerSize;
	float gridStepInWU = quadrantSelPos.getGridStepInWU();
	float sizeInWU = quadrantSelPos.getSizeInWU();
	float lowerXGridVertex = quadrantSelPos.getLowerXGridVertex();
	float lowerZGridVertex = quadrantSelPos.getLowerZGridVertex();
	
	// 1. Lock internal data to prevent shader using them while they are incomplete 
	bool lockAcquired = quadTreeSel->getQuadrant()->lockInternalData();
	if (!lockAcquired)
	{
		m_actionClock.tock();
		m_actionInProgress = false;
		setMessage(godot::String("Quadrant locked!"));
		terrainDataLocked(false);
		return;
	}

	// 2. generate new heights
	std::vector<float> vectGridHeights;
	TheWorld_Utils::MeshCacheBuffer& cache = quadTreeSel->getQuadrant()->getInternalMeshCacheBuffer();
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1.1 ") + __FUNCTION__, "Generate Heights");
		cache.generateHeightsWithNoise(numVerticesPerSize, gridStepInWU, lowerXGridVertex, lowerZGridVertex, terrainEdit, vectGridHeights);

		for (auto& item : m_wms)
		{
			cache.applyWorldModifier(level, numVerticesPerSize, gridStepInWU, lowerXGridVertex, lowerZGridVertex, terrainEdit, vectGridHeights, *item.second.get());
		}
	}

	// 3. remember quadrant has to be saved and uploaded
	quadTreeSel->getQuadrant()->setNeedUploadToServer(true);
	terrainEdit->needUploadToServer = true;
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
		bool manual = true;
		addQuadToSave(quadrantSelPos, manual);
	}

	// 4. refresh new heights on internal datas 16-bit heights (for shader), 32-bit heigths (for collider), terrain data for editing and set flag according (new heigths have to be passed to shader)
	terrainEdit->northSideZMinus.needBlend = true;
	terrainEdit->southSideZPlus.needBlend = true;
	terrainEdit->westSideXMinus.needBlend = true;
	terrainEdit->eastSideXPlus.needBlend = true;

	TheWorld_Utils::MemoryBuffer& heights16Buffer = quadTreeSel->getQuadrant()->getFloat16HeightsBuffer(false);
	size_t heights16BufferSize = numVertices * sizeof(uint16_t);
	heights16Buffer.reserve(heights16BufferSize);
	uint16_t* movingHeights16Buffer = (uint16_t*)heights16Buffer.ptr();
	
	TheWorld_Utils::MemoryBuffer& heights32Buffer = quadTreeSel->getQuadrant()->getFloat32HeightsBuffer(false);
	size_t heights32BufferSize = numVertices * sizeof(float);
	heights32Buffer.reserve(heights32BufferSize);

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1.2 ") + __FUNCTION__, "Move Heights to float16/float32 buffer");

		size_t idx = 0;
		for (int z = 0; z < numVerticesPerSize; z++)
			for (int x = 0; x < numVerticesPerSize; x++)
			{
				float altitude = vectGridHeights[idx];
				TheWorld_Utils::FLOAT_32 f(altitude);
				*movingHeights16Buffer = half_from_float(f.u32);
				movingHeights16Buffer++;
				idx++;
			}

		memcpy(heights32Buffer.ptr(), &vectGridHeights[0], heights32BufferSize);
	}
	my_assert((BYTE*)movingHeights16Buffer - heights16Buffer.ptr() == heights16BufferSize);

	heights16Buffer.adjustSize(heights16BufferSize);
	heights32Buffer.adjustSize(heights32BufferSize);

	quadTreeSel->getQuadrant()->heightmapChanged();
	quadTreeSel->getQuadrant()->sendHeightsToShader(true);
	terrainEdit->heightsNeedRegen = false;

	// 5. renew MeshId
	TheWorld_Utils::MeshCacheBuffer& quadrantCache = quadTreeSel->getQuadrant()->getInternalMeshCacheBuffer();
	quadrantCache.generateNewInternalMeshId();
	quadTreeSel->getQuadrant()->setMeshId(quadrantCache.getMeshId());

	// 6. invalidata normals and textures as heigths have been changed and set flags according (normals need regen and empty normals have to be passed to shader)
	quadTreeSel->getQuadrant()->resetNormalsBuffer(__FUNCTION__);
	quadTreeSel->getQuadrant()->resetSplatmapBuffer(__FUNCTION__);
	quadTreeSel->getQuadrant()->resetColormapBuffer(__FUNCTION__);
	quadTreeSel->getQuadrant()->resetGlobalmapBuffer(__FUNCTION__);

	// 7. prepare new heigths for collider (PackedFloat32Array) - // WARNING useless ? Same instructions are executed in Collider::setData called in QuadTree::updateMaterialParams
	{
#ifdef REAL_T_IS_DOUBLE
		PackedFloat64Array& heightsForCollider = quadTreeSel->getQuadrant()->getHeightsForCollider();
		heightsForCollider.resize((int)numVertices);
		float* f = (float*)heights32Buffer.ptr();
		for (int idx = 0; idx < numVertices; idx++)
		{
			heightsForCollider.set(idx, (real_t)*f);
			f++;
		}
#else
		PackedFloat32Array& heightsForCollider = quadTreeSel->getQuadrant()->getHeightsForCollider();
		heightsForCollider.resize((int)numVertices);
		memcpy((char*)heightsForCollider.ptrw(), heights32Buffer.ptr(), heights32Buffer.size());
#endif
	}


	// 8. calc new quadrant AABB for collider
	Vector3 startPosition(lowerXGridVertex, terrainEdit->minHeight, lowerZGridVertex);
	Vector3 endPosition(startPosition.x + sizeInWU, terrainEdit->maxHeight, startPosition.z + sizeInWU);
	Vector3 size = endPosition - startPosition;
	quadTreeSel->getQuadrant()->getGlobalCoordAABB().set_position(startPosition);
	quadTreeSel->getQuadrant()->getGlobalCoordAABB().set_size(size);

	
	// 9. notify all chunk of the quadrant that heigths have been changed (invalidates all cached chunk and quad AABB)
	//Chunk::HeightsChangedChunkAction action(m_viewer->is_visible_in_tree());
	Chunk::TerrainHeightsChangedChunkAction action;
	m_viewer->m_refreshRequired = true;
	quadTreeSel->ForAllChunk(action, QuadType::NotSet);

	// 10. update quadrant flags
	//quadTreeSel->getQuadrant()->setEmpty(false);
	quadTreeSel->getQuadrant()->sendColorsToShader(true);
	quadTreeSel->materialParamsNeedReset(true);
	quadTreeSel->colliderNeedReset(true, QuadType::NotSet);
	quadTreeSel->getQuadrant()->northQuadrantData().initialized(false);
	quadTreeSel->getQuadrant()->southQuadrantData().initialized(false);
	quadTreeSel->getQuadrant()->westQuadrantData().initialized(false);
	quadTreeSel->getQuadrant()->eastQuadrantData().initialized(false);
	quadTreeSel->getQuadrant()->northWestQuadrantData().initialized(false);
	quadTreeSel->getQuadrant()->southEastQuadrantData().initialized(false);

	// 12. set texture according to editor
	//strcpy_s(terrainEdit->extraValues.lowElevationTexName_r, sizeof(terrainEdit->extraValues.lowElevationTexName_r), m_innerData->m_selTexturePanel->m_newTexName.c_str());
	//strcpy_s(terrainEdit->extraValues.highElevationTexName_g, sizeof(terrainEdit->extraValues.highElevationTexName_g), m_innerData->m_selTexturePanel->m_newTexName.c_str());
	//strcpy_s(terrainEdit->extraValues.dirtTexName_b, sizeof(terrainEdit->extraValues.dirtTexName_b), m_innerData->m_selTexturePanel->m_newTexName.c_str());
	//strcpy_s(terrainEdit->extraValues.rocksTexName_a, sizeof(terrainEdit->extraValues.rocksTexName_a), m_innerData->m_selTexturePanel->m_newTexName.c_str());
	strcpy_s(terrainEdit->extraValues.lowElevationTexName_r, sizeof(terrainEdit->extraValues.lowElevationTexName_r), m_innerData->m_lowElevationTexName->get_text().utf8().get_data());
	strcpy_s(terrainEdit->extraValues.highElevationTexName_g, sizeof(terrainEdit->extraValues.highElevationTexName_g), m_innerData->m_highElevationTexName->get_text().utf8().get_data());
	strcpy_s(terrainEdit->extraValues.dirtTexName_b, sizeof(terrainEdit->extraValues.dirtTexName_b), m_innerData->m_dirtTexName->get_text().utf8().get_data());
	strcpy_s(terrainEdit->extraValues.rocksTexName_a, sizeof(terrainEdit->extraValues.rocksTexName_a), m_innerData->m_rocksTexName->get_text().utf8().get_data());
	setMinHeight(terrainEdit->minHeight);
	setMaxHeight(terrainEdit->maxHeight);
	setSelectedTerrainType(terrainEdit->terrainType);
	//setCanContainSea((terrainEdit->extraValues.seaInfoSet == QUADRANT_SEA_INFO_TRUE && terrainEdit->extraValues.canContainSea == QUADRANT_SEA_INFO_TRUE) ? true : false);

	// 13. mark quadrant to be blended
	addQuadToDelayedBlend(quadTreeSel->getQuadrant()->getPos());

	// 11. unlock internal datas so that they can be passed to the shader/collider
	quadTreeSel->getQuadrant()->unlockInternalData();

	// 14. blend selected quadrant
	if (holdBlending)
	{
		m_completedItems++;
	}
	else
	{
		setMessage(godot::String("Heights generated, blending quadrant[s]..."));

		std::vector<TheWorld_Utils::QuadrantPos> quadrantPos;
		addQuadsToDelayedBlend(quadrantPos);

		m_lastElapsed = 0;
		m_allItems = quadrantPos.size();

		for (auto& pos : quadrantPos)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1.3 ") + __FUNCTION__, "Single QuadTree");

			if (m_actionStopRequested)
			{
				m_actionStopRequested = false;
				break;
			}

			editModeBlendQuadrantAndNeighbours(pos, 2);

			removeQuadToDelayedBlend(pos);

			size_t partialCount = m_actionClock.partialDuration().count();
			m_lastElapsed = partialCount - m_elapsedCompleted;
			m_elapsedCompleted = partialCount;
		}
	}

	GDN_TheWorld_Map* map = m_viewer->Map();
	if (map != nullptr)
	{
		map->removeEditedMapQuadrantOverride(quadrantSelPos);
	}

	// final actions
	setMinHeight(terrainEdit->minHeight);
	setMaxHeight(terrainEdit->maxHeight);

	setMessage(godot::String("Completed!"), true);

	size_t partialCount = m_actionClock.partialDuration().count();
	m_lastElapsed = partialCount - m_elapsedCompleted;
	m_elapsedCompleted = partialCount;

	m_actionClock.tock();

	m_actionInProgress = false;

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	terrainDataLocked(false);
}

bool GDN_TheWorld_Edit::checkBlend(void)
{
	return m_innerData->m_checkBlend->is_pressed();
}

bool GDN_TheWorld_Edit::autoBlendPermitted(void)
{
	return m_innerData->m_autoBlendCheckBox->is_pressed();
}

void GDN_TheWorld_Edit::editModeBlendQuadrantAndNeighbours(TheWorld_Utils::QuadrantPos pos, size_t maxDistance)
{
	bool processed = false, quadrantBusy = true;
	while (not processed and quadrantBusy)
	{
		processed = blendAction(pos, true, false, false, false, false, quadrantBusy, true);
		if (not processed and quadrantBusy)
			Sleep(1);
	}
	if (processed)
		m_completedItems++;

	// blend neighbour quadrants
	{
		std::vector<TheWorld_Utils::QuadrantPos> neighbours;
		pos.getNeighbours(neighbours, maxDistance);

		m_allItems += neighbours.size();
		for (auto& pos : neighbours)
		{
			//if (pos.getIndexPosX() == -14 and pos.getIndexPosZ() == 24)		// #####
			//	assert(m_allItems > 0);									// #####

			QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
			if (quadTree != nullptr)
			{
				bool blendNeeded = quadTree->getQuadrant()->checkBlendNeeded();
				if (blendNeeded)
				{
					bool processed = false, quadrantBusy = true;
					while (not processed and quadrantBusy)
					{
						processed = blendAction(pos, true, false, false, false, false, quadrantBusy, true);
						if (not processed and quadrantBusy)
							Sleep(1);
					}
					if (processed)
						m_completedItems++;
				}
				else
					m_completedItems++;
			}
		}
	}
}

bool GDN_TheWorld_Edit::blendAction(TheWorld_Utils::QuadrantPos blendingPos, bool sync, bool automaticBlending, bool saveAndUpload, bool updateUI, bool lock, bool& quadrantBusy, bool overrideBlendingParameters)
{
	bool processed = false;
	quadrantBusy = false;

	//if (blendingPos.getIndexPosX() == -14 and blendingPos.getIndexPosZ() == 21)	// #####
	//	assert(processed == false);												// #####

	bool autoBlendPermitted = GDN_TheWorld_Edit::autoBlendPermitted();
	if (automaticBlending and not autoBlendPermitted)
		return processed;

	if (!m_viewer->canProcessWorldDataUpdate())
	{
		return processed;
	}

	m_viewer->getMainProcessingMutex().lock();

	if (lock)
	{
		if (m_actionInProgress)
		{
			m_viewer->getMainProcessingMutex().unlock();
			return processed;
		}
		m_actionInProgress = true;

		if (not initialized() or terrainDataLocked())
		{
			m_actionInProgress = false;
			m_viewer->getMainProcessingMutex().unlock();
			return processed;
		}
		terrainDataLocked(true);
	}

	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> lockHolder = std::make_shared<Quadrant::InternalQuadrantDataLockHolder>(blendingPos, m_viewer);
	if (lockHolder->quadrant() != nullptr and not lockHolder->lockAcquired())
	{
		if (lock)
		{
			m_actionInProgress = false;
			terrainDataLocked(false);
		}
		m_viewer->getMainProcessingMutex().unlock();
		quadrantBusy = true;
		return processed;
	}
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> northPosLockHolder = std::make_shared<Quadrant::InternalQuadrantDataLockHolder>(blendingPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinus), m_viewer);
	if (northPosLockHolder->quadrant() != nullptr and not northPosLockHolder->lockAcquired())
	{
		if (lock)
		{
			m_actionInProgress = false;
			terrainDataLocked(false);
		}
		m_viewer->getMainProcessingMutex().unlock();
		quadrantBusy = true;
		return processed;
	}
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> southPosLockHolder = std::make_shared<Quadrant::InternalQuadrantDataLockHolder>(blendingPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlus), m_viewer);
	if (southPosLockHolder->quadrant() != nullptr and not southPosLockHolder->lockAcquired())
	{
		if (lock)
		{
			m_actionInProgress = false;
			terrainDataLocked(false);
		}
		m_viewer->getMainProcessingMutex().unlock();
		quadrantBusy = true;
		return processed;
	}
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> westPosLockHolder = std::make_shared<Quadrant::InternalQuadrantDataLockHolder>(blendingPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XMinus), m_viewer);
	if (westPosLockHolder->quadrant() != nullptr and not westPosLockHolder->lockAcquired())
	{
		if (lock)
		{
			m_actionInProgress = false;
			terrainDataLocked(false);
		}
		m_viewer->getMainProcessingMutex().unlock();
		quadrantBusy = true;
		return processed;
	}
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> eastPosLockHolder = std::make_shared<Quadrant::InternalQuadrantDataLockHolder>(blendingPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XPlus), m_viewer);
	if (eastPosLockHolder->quadrant() != nullptr and not eastPosLockHolder->lockAcquired())
	{
		if (lock)
		{
			m_actionInProgress = false;
			terrainDataLocked(false);
		}
		m_viewer->getMainProcessingMutex().unlock();
		quadrantBusy = true;
		return processed;
	}
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> northwestPosLockHolder = std::make_shared<Quadrant::InternalQuadrantDataLockHolder>(blendingPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinusXMinus), m_viewer);
	if (northwestPosLockHolder->quadrant() != nullptr and not northwestPosLockHolder->lockAcquired())
	{
		if (lock)
		{
			m_actionInProgress = false;
			terrainDataLocked(false);
		}
		m_viewer->getMainProcessingMutex().unlock();
		quadrantBusy = true;
		return processed;
	}
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> northeastPosLockHolder = std::make_shared<Quadrant::InternalQuadrantDataLockHolder>(blendingPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinusXPlus), m_viewer);
	if (northeastPosLockHolder->quadrant() != nullptr and not northeastPosLockHolder->lockAcquired())
	{
		if (lock)
		{
			m_actionInProgress = false;
			terrainDataLocked(false);
		}
		m_viewer->getMainProcessingMutex().unlock();
		quadrantBusy = true;
		return processed;
	}
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> southwestPosLockHolder = std::make_shared<Quadrant::InternalQuadrantDataLockHolder>(blendingPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlusXMinus), m_viewer);
	if (southwestPosLockHolder->quadrant() != nullptr and not southwestPosLockHolder->lockAcquired())
	{
		if (lock)
		{
			m_actionInProgress = false;
			terrainDataLocked(false);
		}
		m_viewer->getMainProcessingMutex().unlock();
		quadrantBusy = true;
		return processed;
	}
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> southeastPosLockHolder = std::make_shared<Quadrant::InternalQuadrantDataLockHolder>(blendingPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlusXPlus), m_viewer);
	if (southeastPosLockHolder->quadrant() != nullptr and not southeastPosLockHolder->lockAcquired())
	{
		if (lock)
		{
			m_actionInProgress = false;
			terrainDataLocked(false);
		}
		m_viewer->getMainProcessingMutex().unlock();
		quadrantBusy = true;
		return processed;
	}

	m_viewer->getMainProcessingMutex().unlock();

	if (updateUI)
	{
		m_actionClock.tick();
		m_allItems = 0;
		setElapsed(0, true);
		setNote1(0);
	}

	processed = true;
	if (sync)
	{
		blendExecution(lockHolder,
			northPosLockHolder,
			southPosLockHolder,
			westPosLockHolder,
			eastPosLockHolder,
			northwestPosLockHolder,
			northeastPosLockHolder,
			southwestPosLockHolder,
			southeastPosLockHolder,
			updateUI,
			lock,
			saveAndUpload,
			overrideBlendingParameters);
	}
	else
	{
		std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::blendExecution, this,
			lockHolder,
			northPosLockHolder,
			southPosLockHolder,
			westPosLockHolder,
			eastPosLockHolder,
			northwestPosLockHolder,
			northeastPosLockHolder,
			southwestPosLockHolder,
			southeastPosLockHolder,
			updateUI,
			lock,
			saveAndUpload,
			overrideBlendingParameters);
		m_tp.QueueJob(f);
	}

	return processed;
}

bool GDN_TheWorld_Edit::uploadToServerInProgress(void) const
{
	if (m_editorUploadInProgress or m_uploadModifiedWorldQuadrantsInProgress or m_uploadMapQuadrantOverridesInProgress)
		return true;
	else
		return false;
}

void GDN_TheWorld_Edit::blendExecution(std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> lockHolder,
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> northPosLockHolder,
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> southPosLockHolder,
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> westPosLockHolder,
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> eastPosLockHolder,
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> northwestPosLockHolder,
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> northeastPosLockHolder,
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> southwestPosLockHolder,
	std::shared_ptr<Quadrant::InternalQuadrantDataLockHolder> southeastPosLockHolder,
	bool updateUI,
	bool unlock,
	bool saveAndUpload,
	bool overrideBlendingParameters)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("Blend execution 1 ") + __FUNCTION__, "ALL");

	std::stringstream str;
	str << "Blending heigths for quadrant " << lockHolder->quadrant()->getPos().getIndexPosX() << "," << lockHolder->quadrant()->getPos().getIndexPosZ();
	PLOG_DEBUG << str.str();
	if (updateUI)
		setMessage(godot::String(str.str().c_str()));

	size_t numRounds = lockHolder->quadrant()->getTerrainEdit()->extraValues.numBlendingCycles;
	for (size_t round = 0; round < numRounds; round++)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("Blend execution 1.1 ") + __FUNCTION__, "Single round");

		bool lastPhase = false;
		if (round == numRounds - 1)
			lastPhase = true;

		TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer; TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData quadrantData;
		prepareHeightsForQuadrantBlend(lockHolder->quadrant(), &quadrantData, &terrainEditValuesBuffer, overrideBlendingParameters);
		TheWorld_Utils::MemoryBuffer northTerrainEditValuesBuffer; TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData northQuadrantData;
		prepareHeightsForQuadrantBlend(northPosLockHolder->quadrant(), &northQuadrantData, &northTerrainEditValuesBuffer, overrideBlendingParameters);
		TheWorld_Utils::MemoryBuffer southTerrainEditValuesBuffer; TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData southQuadrantData;
		prepareHeightsForQuadrantBlend(southPosLockHolder->quadrant(), &southQuadrantData, &southTerrainEditValuesBuffer, overrideBlendingParameters);
		TheWorld_Utils::MemoryBuffer westTerrainEditValuesBuffer; TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData westQuadrantData;
		prepareHeightsForQuadrantBlend(westPosLockHolder->quadrant(), &westQuadrantData, &westTerrainEditValuesBuffer, overrideBlendingParameters);
		TheWorld_Utils::MemoryBuffer eastTerrainEditValuesBuffer; TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData eastQuadrantData;
		prepareHeightsForQuadrantBlend(eastPosLockHolder->quadrant(), &eastQuadrantData, &eastTerrainEditValuesBuffer, overrideBlendingParameters);
		TheWorld_Utils::MemoryBuffer northwestTerrainEditValuesBuffer; TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData northwestQuadrantData;
		prepareHeightsForQuadrantBlend(northwestPosLockHolder->quadrant(), &northwestQuadrantData, &northwestTerrainEditValuesBuffer, overrideBlendingParameters);
		TheWorld_Utils::MemoryBuffer northeastTerrainEditValuesBuffer; TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData northeastQuadrantData;
		prepareHeightsForQuadrantBlend(northeastPosLockHolder->quadrant(), &northeastQuadrantData, &northeastTerrainEditValuesBuffer, overrideBlendingParameters);
		TheWorld_Utils::MemoryBuffer southwestTerrainEditValuesBuffer; TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData southwestQuadrantData;
		prepareHeightsForQuadrantBlend(southwestPosLockHolder->quadrant(), &southwestQuadrantData, &southwestTerrainEditValuesBuffer, overrideBlendingParameters);
		TheWorld_Utils::MemoryBuffer southeastTerrainEditValuesBuffer; TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData southeastQuadrantData;
		prepareHeightsForQuadrantBlend(southeastPosLockHolder->quadrant(), &southeastQuadrantData, &southeastTerrainEditValuesBuffer, overrideBlendingParameters);

		{
			TheWorld_Utils::GuardProfiler profiler(std::string("Blend execution 1.1.1 ") + __FUNCTION__, "Single QuadTree blendQuadrant");

			//lastPhase = false;	// DEBUG
			bool updated = lockHolder->quadrant()->getInternalMeshCacheBuffer().blendQuadrant(lockHolder->quadrant()->getPos().getNumVerticesPerSize(), lockHolder->quadrant()->getPos().getGridStepInWU(), lastPhase,
				quadrantData,
				northQuadrantData,
				southQuadrantData,
				westQuadrantData,
				eastQuadrantData,
				northwestQuadrantData,
				northeastQuadrantData,
				southwestQuadrantData,
				southeastQuadrantData);
		}

		manageUpdatedHeightsAfterQuadrantBlend(quadrantData, lockHolder->quadrant(), terrainEditValuesBuffer);
		manageUpdatedHeightsAfterQuadrantBlend(northQuadrantData, northPosLockHolder->quadrant(), northTerrainEditValuesBuffer);
		manageUpdatedHeightsAfterQuadrantBlend(southQuadrantData, southPosLockHolder->quadrant(), southTerrainEditValuesBuffer);
		manageUpdatedHeightsAfterQuadrantBlend(westQuadrantData, westPosLockHolder->quadrant(), westTerrainEditValuesBuffer);
		manageUpdatedHeightsAfterQuadrantBlend(eastQuadrantData, eastPosLockHolder->quadrant(), eastTerrainEditValuesBuffer);
		manageUpdatedHeightsAfterQuadrantBlend(northwestQuadrantData, northwestPosLockHolder->quadrant(), northwestTerrainEditValuesBuffer);
		manageUpdatedHeightsAfterQuadrantBlend(northeastQuadrantData, northeastPosLockHolder->quadrant(), northeastTerrainEditValuesBuffer);
		manageUpdatedHeightsAfterQuadrantBlend(southwestQuadrantData, southwestPosLockHolder->quadrant(), southwestTerrainEditValuesBuffer);
		manageUpdatedHeightsAfterQuadrantBlend(southeastQuadrantData, southeastPosLockHolder->quadrant(), southeastTerrainEditValuesBuffer);
	}
	
	if (saveAndUpload)
	{
		manageSaveAndUploadAfterBlending(lockHolder->quadrant());
		manageSaveAndUploadAfterBlending(northPosLockHolder->quadrant());
		manageSaveAndUploadAfterBlending(southPosLockHolder->quadrant());
		manageSaveAndUploadAfterBlending(westPosLockHolder->quadrant());
		manageSaveAndUploadAfterBlending(eastPosLockHolder->quadrant());
		manageSaveAndUploadAfterBlending(northwestPosLockHolder->quadrant());
		manageSaveAndUploadAfterBlending(northeastPosLockHolder->quadrant());
		manageSaveAndUploadAfterBlending(southwestPosLockHolder->quadrant());
		manageSaveAndUploadAfterBlending(southeastPosLockHolder->quadrant());
	}

	// final operations
	m_viewer->getMainProcessingMutex().lock();
	lockHolder->unlock();
	northPosLockHolder->unlock();
	southPosLockHolder->unlock();
	westPosLockHolder->unlock();
	eastPosLockHolder->unlock();
	northwestPosLockHolder->unlock();
	northeastPosLockHolder->unlock();
	southwestPosLockHolder->unlock();
	southeastPosLockHolder->unlock();
	if (unlock)
	{
		terrainDataLocked(false);
		m_actionInProgress = false;
	}
	size_t duration = m_actionClock.partialDuration().count();
	if (updateUI)
		m_actionClock.tock();
	m_viewer->getMainProcessingMutex().unlock();

	str.str("");
	str.clear();
	str << "Blending heigths for quadrant " << lockHolder->quadrant()->getPos().getIndexPosX() << "," << lockHolder->quadrant()->getPos().getIndexPosZ() << " - Completed!";
	PLOG_DEBUG << str.str() << " - " << duration << " ms";
	if (updateUI)
	{
		setElapsed(duration, false);
		setMessage(godot::String(str.str().c_str()), false);
	}
}

void GDN_TheWorld_Edit::manageSaveAndUploadAfterBlending(Quadrant* quadrant)
{
	if (quadrant == nullptr)
		return;

	TheWorld_Utils::QuadrantPos quadrantPos = quadrant->getPos();
	bool manual = true;
	if (isQuadToSave(quadrantPos, manual))
	{
		quadrant->saveQuadrantToDiskCache2(true, false);
		removeQuadToSave(quadrantPos);
		quadrant->getTerrainEdit()->needUploadToServer = false;
		std::string buffer;
		quadrant->getQuadrantBufferForCache(buffer);
		m_viewer->Globals()->Client()->MapManagerUploadBuffer(m_viewer->isInEditor(), quadrantPos.getLowerXGridVertex(), quadrantPos.getLowerZGridVertex(), quadrantPos.getNumVerticesPerSize(), quadrantPos.getGridStepInWU(), quadrantPos.getLevel(), buffer, m_viewer->getNumQuadrantsPerSize(), m_viewer->getQuadrantGlobalSize(), m_viewer->getQuadrantWorldSize());
	}
}

void GDN_TheWorld_Edit::editModeBlendAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	m_allItems = 0;
	setElapsed(0, true);
	setNote1(0);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeBlendNew, this);
	m_tp.QueueJob(f);
}

void GDN_TheWorld_Edit::editModeBlendNew(void)
{
	m_viewer->getMainProcessingMutex().lock();
	if (m_actionClock.counterStarted())
	{
		m_actionInProgress = false;
		m_viewer->getMainProcessingMutex().unlock();
		return;
	}
	if (terrainDataLocked())
	{
		m_viewer->getMainProcessingMutex().unlock();
		setMessage(godot::String("Terrain data locked wait..."));
		return;
	}
	terrainDataLocked(true);
	m_viewer->getMainProcessingMutex().unlock();

	setMessage(godot::String("Blending heigths..."));

	m_actionClock.tick();

	TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1 ") + __FUNCTION__, "ALL");

	bool allCheched = false; // m_innerData->m_blendAllCheckBox->is_pressed();

	QuadTree* quadTreeSel = nullptr;
	TheWorld_Utils::QuadrantPos quadrantSelPos = m_viewer->getWorldQuadrantSelForEdit(&quadTreeSel);

	if (quadrantSelPos.empty() and numQuadsToDelayedBlend() == 0 and getNumQuadrantsToBlend() == 0)
	{
		m_actionClock.tock();
		m_actionInProgress = false;
		setMessage(godot::String("No quadrant to blend!"), true);
		terrainDataLocked(false);
		return;
	}

	std::vector<TheWorld_Utils::QuadrantPos> quadrantPos;
	if (not quadrantSelPos.empty())
		quadrantPos.push_back(quadrantSelPos);
	addQuadsToDelayedBlend(quadrantPos);
	addQuadrantsToBlend(quadrantPos);

	setMessage(godot::String("Blending quadrants..."));

	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_allItems = quadrantPos.size();

	for (auto& pos : quadrantPos)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1 ") + __FUNCTION__, "Single QuadTree");

		if (m_actionStopRequested)
		{
			m_actionStopRequested = false;
			break;
		}

		editModeBlendQuadrantAndNeighbours(pos, 2);

		removeQuadToDelayedBlend(pos);

		size_t partialCount = m_actionClock.partialDuration().count();
		m_lastElapsed = partialCount - m_elapsedCompleted;
		m_elapsedCompleted = partialCount;
	}

	if (quadTreeSel != nullptr)
	{
		setMinHeight(quadTreeSel->getQuadrant()->getTerrainEdit()->minHeight);
		setMaxHeight(quadTreeSel->getQuadrant()->getTerrainEdit()->maxHeight);
		//setCanContainSea((quadTreeSel->getQuadrant()->getTerrainEdit()->extraValues.seaInfoSet == QUADRANT_SEA_INFO_TRUE && quadTreeSel->getQuadrant()->getTerrainEdit()->extraValues.canContainSea == QUADRANT_SEA_INFO_TRUE) ? true : false);
	}

	size_t partialCount = m_actionClock.partialDuration().count();
	m_lastElapsed = partialCount - m_elapsedCompleted;
	m_elapsedCompleted = partialCount;

	m_actionClock.tock();

	m_actionInProgress = false;

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	setMessage(godot::String("Completed!"), true);

	terrainDataLocked(false);
}

void GDN_TheWorld_Edit::editModeBlend(void)
{
	m_viewer->getMainProcessingMutex().lock();
	if (m_actionClock.counterStarted())
	{
		m_actionInProgress = false;
		m_viewer->getMainProcessingMutex().unlock();
		return;
	}
	if (terrainDataLocked())
	{
		m_viewer->getMainProcessingMutex().unlock();
		setMessage(godot::String("Terrain data locked wait..."));
		return;
	}
	terrainDataLocked(true);
	m_viewer->getMainProcessingMutex().unlock();

	setMessage(godot::String("Blending heigths..."));
	
	m_actionClock.tick();

	TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1 ") + __FUNCTION__, "ALL");

	bool allCheched = false;	// m_innerData->m_blendAllCheckBox->is_pressed();

	QuadTree* quadTreeSel = nullptr;
	TheWorld_Utils::QuadrantPos quadrantSelPos = m_viewer->getWorldQuadrantSelForEdit(&quadTreeSel);

	if (quadrantSelPos.empty())
	{
		m_actionClock.tock();
		m_actionInProgress = false;
		setMessage(godot::String("Must select a quadrant!"), true);
		return;
	}

	std::vector<TheWorld_Utils::QuadrantPos> allQuadrantPos;
	m_viewer->getAllQuadrantPos(allQuadrantPos);

	std::vector<TheWorld_Utils::QuadrantPos> quadrantPos;
	for (auto& pos : allQuadrantPos)
	{
		QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
		if (quadTree != nullptr)
		{
			if (allCheched || pos == quadrantSelPos)
			{
				quadrantPos.push_back(pos);

				if (!allCheched)
					break;
			}
		}
	}

	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_allItems = quadrantPos.size();

	size_t numRounds = blendingCycles();
	for (size_t round = 0; round < numRounds; round++)
	{
		bool lastPhase = false;
		if (round == numRounds - 1)
			lastPhase = true;

		m_completedItems = 0;

		bool cannotExitLoop = false;
		
		for (auto& pos : quadrantPos)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1 ") + __FUNCTION__, "Single QuadTree");

			if (m_actionStopRequested)
			{
				m_actionStopRequested = false;
				break;
			}

			TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
			TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData quadrantData;

			bool lockAcquired;

			QuadTree* quadTree = nullptr;
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.1 ") + __FUNCTION__, "Single QuadTree prepare data for blending");
				quadTree = prepareHeightsForQuadrantBlend(&lockAcquired , &pos, &quadrantData, &terrainEditValuesBuffer);
				if (!lockAcquired && lastPhase)
					cannotExitLoop = true;
			}
				
			if (quadTree != nullptr)
			{
				//std::vector<std::thread> threads;
				//threads.resize(8);
								
				TheWorld_Utils::MemoryBuffer northTerrainEditValuesBuffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData northQuadrantData;
				TheWorld_Utils::QuadrantPos northPos = pos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinus);
				QuadTree* northQuadTree = nullptr;
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.2 ") + __FUNCTION__, "Single QuadTree start thread prepare data for blending");
					//threads[0] = std::thread(&GDN_TheWorld_Edit::prepareHeightsForQuadrantBlendThread, this, &northQuadTree, &northPos, &northQuadrantData, &northTerrainEditValuesBuffer, &northHeights16Buffer, &northHeights32Buffer);
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.1 ") + __FUNCTION__, "Single QuadTree prepare data for blending");
					northQuadTree = prepareHeightsForQuadrantBlend(&lockAcquired, &northPos, &northQuadrantData, &northTerrainEditValuesBuffer);
					if (!lockAcquired && lastPhase)
						cannotExitLoop = true;
				}

				TheWorld_Utils::MemoryBuffer southTerrainEditValuesBuffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData southQuadrantData;
				TheWorld_Utils::QuadrantPos southPos = pos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlus);
				QuadTree* southQuadTree = nullptr;
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.2 ") + __FUNCTION__, "Single QuadTree start thread prepare data for blending");
					//threads[1] = std::thread(&GDN_TheWorld_Edit::prepareHeightsForQuadrantBlendThread, this, &southQuadTree, &southPos, &southQuadrantData, &southTerrainEditValuesBuffer, &southHeights16Buffer, &southHeights32Buffer);
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.1 ") + __FUNCTION__, "Single QuadTree prepare data for blending");
					southQuadTree = prepareHeightsForQuadrantBlend(&lockAcquired, &southPos, &southQuadrantData, &southTerrainEditValuesBuffer);
					if (!lockAcquired && lastPhase)
						cannotExitLoop = true;
				}

				TheWorld_Utils::MemoryBuffer westTerrainEditValuesBuffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData westQuadrantData;
				TheWorld_Utils::QuadrantPos westPos = pos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XMinus);
				QuadTree* westQuadTree = nullptr;
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.2 ") + __FUNCTION__, "Single QuadTree start thread prepare data for blending");
					//threads[2] = std::thread(&GDN_TheWorld_Edit::prepareHeightsForQuadrantBlendThread, this, &westQuadTree, &westPos, &westQuadrantData, &westTerrainEditValuesBuffer, &westHeights16Buffer, &westHeights32Buffer);
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.1 ") + __FUNCTION__, "Single QuadTree prepare data for blending");
					westQuadTree = prepareHeightsForQuadrantBlend(&lockAcquired, &westPos, &westQuadrantData, &westTerrainEditValuesBuffer);
					if (!lockAcquired && lastPhase)
						cannotExitLoop = true;
				}

				TheWorld_Utils::MemoryBuffer eastTerrainEditValuesBuffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData eastQuadrantData;
				TheWorld_Utils::QuadrantPos eastPos = pos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XPlus);
				QuadTree* eastQuadTree = nullptr;
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.2 ") + __FUNCTION__, "Single QuadTree start thread prepare data for blending");
					//threads[3] = std::thread(&GDN_TheWorld_Edit::prepareHeightsForQuadrantBlendThread, this, &eastQuadTree, &eastPos, &eastQuadrantData, &eastTerrainEditValuesBuffer, &eastHeights16Buffer, &eastHeights32Buffer);
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.1 ") + __FUNCTION__, "Single QuadTree prepare data for blending");
					eastQuadTree = prepareHeightsForQuadrantBlend(&lockAcquired, &eastPos, &eastQuadrantData, &eastTerrainEditValuesBuffer);
					if (!lockAcquired && lastPhase)
						cannotExitLoop = true;
				}

				TheWorld_Utils::MemoryBuffer northwestTerrainEditValuesBuffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData northwestQuadrantData;
				TheWorld_Utils::QuadrantPos northwestPos = pos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinusXMinus);
				QuadTree* northwestQuadTree = nullptr;
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.2 ") + __FUNCTION__, "Single QuadTree start thread prepare data for blending");
					//threads[4] = std::thread(&GDN_TheWorld_Edit::prepareHeightsForQuadrantBlendThread, this, &northwestQuadTree, &northwestPos, &northwestQuadrantData, &northwestTerrainEditValuesBuffer, &northwestHeights16Buffer, &northwestHeights32Buffer);
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.1 ") + __FUNCTION__, "Single QuadTree prepare data for blending");
					northwestQuadTree = prepareHeightsForQuadrantBlend(&lockAcquired, &northwestPos, &northwestQuadrantData, &northwestTerrainEditValuesBuffer);
					if (!lockAcquired && lastPhase)
						cannotExitLoop = true;
				}

				TheWorld_Utils::MemoryBuffer northeastTerrainEditValuesBuffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData northeastQuadrantData;
				TheWorld_Utils::QuadrantPos northeastPos = pos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinusXPlus);
				QuadTree* northeastQuadTree = nullptr;
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.2 ") + __FUNCTION__, "Single QuadTree start thread prepare data for blending");
					//threads[5] = std::thread(&GDN_TheWorld_Edit::prepareHeightsForQuadrantBlendThread, this, &northeastQuadTree, &northeastPos, &northeastQuadrantData, &northeastTerrainEditValuesBuffer, &northeastHeights16Buffer, &northeastHeights32Buffer);
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.1 ") + __FUNCTION__, "Single QuadTree prepare data for blending");
					northeastQuadTree = prepareHeightsForQuadrantBlend(&lockAcquired, &northeastPos, &northeastQuadrantData, &northeastTerrainEditValuesBuffer);
					if (!lockAcquired && lastPhase)
						cannotExitLoop = true;
				}

				TheWorld_Utils::MemoryBuffer southwestTerrainEditValuesBuffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData southwestQuadrantData;
				TheWorld_Utils::QuadrantPos southwestPos = pos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlusXMinus);
				QuadTree* southwestQuadTree = nullptr;
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.2 ") + __FUNCTION__, "Single QuadTree start thread prepare data for blending");
					//threads[6] = std::thread(&GDN_TheWorld_Edit::prepareHeightsForQuadrantBlendThread, this, &southwestQuadTree, &southwestPos, &southwestQuadrantData, &southwestTerrainEditValuesBuffer, &southwestHeights16Buffer, &southwestHeights32Buffer);
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.1 ") + __FUNCTION__, "Single QuadTree prepare data for blending");
					southwestQuadTree = prepareHeightsForQuadrantBlend(&lockAcquired, &southwestPos, &southwestQuadrantData, &southwestTerrainEditValuesBuffer);
					if (!lockAcquired && lastPhase)
						cannotExitLoop = true;
				}

				TheWorld_Utils::MemoryBuffer southeastTerrainEditValuesBuffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData southeastQuadrantData;
				TheWorld_Utils::QuadrantPos southeastPos = pos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlusXPlus);
				QuadTree* southeastQuadTree = nullptr;
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.2 ") + __FUNCTION__, "Single QuadTree start thread prepare data for blending");
					//threads[7] = std::thread(&GDN_TheWorld_Edit::prepareHeightsForQuadrantBlendThread, this, &southeastQuadTree, &southeastPos, &southeastQuadrantData, &southeastTerrainEditValuesBuffer, &southeastHeights16Buffer, &southeastHeights32Buffer);
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.1 ") + __FUNCTION__, "Single QuadTree prepare data for blending");
					southeastQuadTree = prepareHeightsForQuadrantBlend(&lockAcquired, &southeastPos, &southeastQuadrantData, &southeastTerrainEditValuesBuffer);
					if (!lockAcquired && lastPhase)
						cannotExitLoop = true;
				}

				//for (auto& t : threads)
				//	t.join();

				{
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.3 ") + __FUNCTION__, "Single QuadTree blendQuadrant");

					//lastPhase = false;	// DEBUG
					bool updated = quadTree->getQuadrant()->getInternalMeshCacheBuffer().blendQuadrant(pos.getNumVerticesPerSize(), pos.getGridStepInWU(), lastPhase,
						quadrantData,
						northQuadrantData,
						southQuadrantData,
						westQuadrantData,
						eastQuadrantData,
						northwestQuadrantData,
						northeastQuadrantData,
						southwestQuadrantData,
						southeastQuadrantData);

					//if (updated && lastPhase)
					//	cannotExitLoop = true;
				}

				{
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.4 ") + __FUNCTION__, "Single QuadTree manage blended data");
					manageUpdatedHeightsAfterQuadrantBlend(quadrantData, quadTree, terrainEditValuesBuffer);
					quadTree->getQuadrant()->unlockInternalData();
				}

				if (northQuadTree != nullptr)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.4 ") + __FUNCTION__, "Single QuadTree manage blended data");
					manageUpdatedHeightsAfterQuadrantBlend(northQuadrantData, northQuadTree, northTerrainEditValuesBuffer);
					northQuadTree->getQuadrant()->unlockInternalData();
				}
				if (southQuadTree != nullptr)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.4 ") + __FUNCTION__, "Single QuadTree manage blended data");
					manageUpdatedHeightsAfterQuadrantBlend(southQuadrantData, southQuadTree, southTerrainEditValuesBuffer);
					southQuadTree->getQuadrant()->unlockInternalData();
				}
				if (westQuadTree != nullptr)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.4 ") + __FUNCTION__, "Single QuadTree manage blended data");
					manageUpdatedHeightsAfterQuadrantBlend(westQuadrantData, westQuadTree, westTerrainEditValuesBuffer);
					westQuadTree->getQuadrant()->unlockInternalData();
				}
				if (eastQuadTree != nullptr)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.4 ") + __FUNCTION__, "Single QuadTree manage blended data");
					manageUpdatedHeightsAfterQuadrantBlend(eastQuadrantData, eastQuadTree, eastTerrainEditValuesBuffer);
					eastQuadTree->getQuadrant()->unlockInternalData();
				}
				if (northwestQuadTree != nullptr)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.4 ") + __FUNCTION__, "Single QuadTree manage blended data");
					manageUpdatedHeightsAfterQuadrantBlend(northwestQuadrantData, northwestQuadTree, northwestTerrainEditValuesBuffer);
					northwestQuadTree->getQuadrant()->unlockInternalData();
				}
				if (northeastQuadTree != nullptr)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.4 ") + __FUNCTION__, "Single QuadTree manage blended data");
					manageUpdatedHeightsAfterQuadrantBlend(northeastQuadrantData, northeastQuadTree, northeastTerrainEditValuesBuffer);
					northeastQuadTree->getQuadrant()->unlockInternalData();
				}
				if (southwestQuadTree != nullptr)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.4 ") + __FUNCTION__, "Single QuadTree manage blended data");
					manageUpdatedHeightsAfterQuadrantBlend(southwestQuadrantData, southwestQuadTree, southwestTerrainEditValuesBuffer);
					southwestQuadTree->getQuadrant()->unlockInternalData();
				}
				if (southeastQuadTree != nullptr)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.4 ") + __FUNCTION__, "Single QuadTree manage blended data");
					manageUpdatedHeightsAfterQuadrantBlend(southeastQuadrantData, southeastQuadTree, southeastTerrainEditValuesBuffer);
					southeastQuadTree->getQuadrant()->unlockInternalData();
				}

				m_completedItems++;
				size_t partialCount = m_actionClock.partialDuration().count();
				m_lastElapsed = partialCount - m_elapsedCompleted;
				m_elapsedCompleted = partialCount;
			}
		}

		if (cannotExitLoop && round == numRounds - 1)
			round--;
	}

	setMinHeight(quadTreeSel->getQuadrant()->getTerrainEdit()->minHeight);
	setMaxHeight(quadTreeSel->getQuadrant()->getTerrainEdit()->maxHeight);
	//setCanContainSea((quadTreeSel->getQuadrant()->getTerrainEdit()->extraValues.seaInfoSet == QUADRANT_SEA_INFO_TRUE && quadTreeSel->getQuadrant()->getTerrainEdit()->extraValues.canContainSea == QUADRANT_SEA_INFO_TRUE) ? true : false);

	size_t partialCount = m_actionClock.partialDuration().count();
	m_lastElapsed = partialCount - m_elapsedCompleted;
	m_elapsedCompleted = partialCount;

	m_actionClock.tock();

	m_actionInProgress = false;

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	//size_t numToSave = 0;
	//size_t numToUpload = 0;
	//size_t numLocked = 0;
	//refreshNumToSaveUpload(numToSave, numToUpload, numLocked);

	setMessage(godot::String("Completed!"), true);

	terrainDataLocked(false);
}

//void GDN_TheWorld_Edit::prepareHeightsForQuadrantBlendThread(QuadTree** quadTree, bool* lockAcquired, TheWorld_Utils::QuadrantPos* pos, TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData* quadrantData, TheWorld_Utils::MemoryBuffer* terrainEditValuesBuffer)
//{
//	TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1.2.1 ") + __FUNCTION__, "Single QuadTree prepare data for blending in thread");
//	*quadTree = prepareHeightsForQuadrantBlend(lockAcquired, pos, quadrantData, terrainEditValuesBuffer);
//}

void GDN_TheWorld_Edit::prepareHeightsForQuadrantBlend(Quadrant* quadrant, TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData* quadrantData, TheWorld_Utils::MemoryBuffer* terrainEditValuesBuffer, bool overrideBlendingParameters)
{
	if (quadrant != nullptr)
	{
		if (overrideBlendingParameters and amplitudeBlendFactorValid())
		{
			quadrant->getTerrainEdit()->extraValues.amplitudeBlendFactor = amplitudeBlendFactor();
			quadrant->getTerrainEdit()->extraValues.numBlendingCycles = blendingCycles();
			quadrant->getTerrainEdit()->extraValues.blendingBendingFactor = blendingBendingFactor();
			quadrant->getTerrainEdit()->extraValues.minRandomBendingExtension = minRandomBendingExtension();
			quadrant->getTerrainEdit()->extraValues.maxRandomBendingExtension = maxRandomBendingExtension();
		}
		quadrant->getTerrainEdit()->serialize(*terrainEditValuesBuffer);

		quadrantData->meshId = quadrant->getInternalMeshCacheBuffer().getMeshId();
		quadrantData->terrainEditValues = terrainEditValuesBuffer;
		quadrantData->minHeight = quadrant->getTerrainEdit()->minHeight;
		quadrantData->maxHeight = quadrant->getTerrainEdit()->maxHeight;
		TheWorld_Utils::MemoryBuffer& h16Buffer = quadrant->getFloat16HeightsBuffer();
		quadrantData->heights16Buffer = &h16Buffer;
		TheWorld_Utils::MemoryBuffer& h32Buffer = quadrant->getFloat32HeightsBuffer();
		quadrantData->heights32Buffer = &h32Buffer;
		quadrantData->normalsBuffer = nullptr;
		quadrantData->splatmapBuffer = nullptr;
		quadrantData->colormapBuffer = nullptr;
		quadrantData->globalmapBuffer = nullptr;
	}
}

QuadTree* GDN_TheWorld_Edit::prepareHeightsForQuadrantBlend(bool* lockAcquired, TheWorld_Utils::QuadrantPos* pos, TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData* quadrantData, TheWorld_Utils::MemoryBuffer* terrainEditValuesBuffer)
{
	QuadTree* quadTree = nullptr;

	{
		//std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
		quadTree = m_viewer->getQuadTree(*pos, false);
	}

	if (quadTree != nullptr)
	{
		*lockAcquired = quadTree->getQuadrant()->lockInternalData();

		if (*lockAcquired)
		{
			quadTree->getQuadrant()->getTerrainEdit()->extraValues.amplitudeBlendFactor  = amplitudeBlendFactor();
			quadTree->getQuadrant()->getTerrainEdit()->extraValues.numBlendingCycles = blendingCycles();
			quadTree->getQuadrant()->getTerrainEdit()->extraValues.blendingBendingFactor = blendingBendingFactor();
			quadTree->getQuadrant()->getTerrainEdit()->extraValues.minRandomBendingExtension = minRandomBendingExtension();
			quadTree->getQuadrant()->getTerrainEdit()->extraValues.maxRandomBendingExtension = maxRandomBendingExtension();
			quadTree->getQuadrant()->getTerrainEdit()->serialize(*terrainEditValuesBuffer);

			quadrantData->meshId = quadTree->getQuadrant()->getInternalMeshCacheBuffer().getMeshId();
			quadrantData->terrainEditValues = terrainEditValuesBuffer;
			quadrantData->minHeight = quadTree->getQuadrant()->getTerrainEdit()->minHeight;
			quadrantData->maxHeight = quadTree->getQuadrant()->getTerrainEdit()->maxHeight;
			TheWorld_Utils::MemoryBuffer& h16Buffer = quadTree->getQuadrant()->getFloat16HeightsBuffer();
			quadrantData->heights16Buffer = &h16Buffer;
			TheWorld_Utils::MemoryBuffer& h32Buffer = quadTree->getQuadrant()->getFloat32HeightsBuffer();
			quadrantData->heights32Buffer = &h32Buffer;
			quadrantData->normalsBuffer = nullptr;
			quadrantData->splatmapBuffer = nullptr;
			quadrantData->colormapBuffer = nullptr;
			quadrantData->globalmapBuffer = nullptr;
		}
		else
			quadTree = nullptr;
	}

	return quadTree;
}

void GDN_TheWorld_Edit::manageUpdatedHeightsAfterQuadrantBlend(TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData& quadrantData, Quadrant* quadrant, TheWorld_Utils::MemoryBuffer& terrainEditValuesBuffer)
{
	if (quadrantData.heightsUpdated)
	{
		// renew MeshId
		TheWorld_Utils::MeshCacheBuffer& quadrantCache = quadrant->getInternalMeshCacheBuffer();
		quadrantCache.generateNewInternalMeshId();
		quadrant->setMeshId(quadrantCache.getMeshId());

		quadrant->getTerrainEdit()->deserialize(terrainEditValuesBuffer);

		{
			std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
			//m_mapQuadToSave[quadrant->getPos()] = "";
			TheWorld_Utils::QuadrantPos pos = quadrant->getPos();
			bool manual = true;
			addQuadToSave(pos, manual);
		}

		//quadrant->setEmpty(false);
		quadrant->getTerrainEdit()->needUploadToServer = true;
		quadrant->setNeedUploadToServer(true);
		quadrant->heightmapChanged();
		quadrant->sendHeightsToShader(true);
		quadrant->resetNormalsBuffer(__FUNCTION__);
		quadrant->resetSplatmapBuffer(__FUNCTION__);
		quadrant->resetGlobalmapBuffer(__FUNCTION__);

		{
			my_assert(quadrant->getFloat32HeightsBuffer().size() == quadrant->getPos().getNumVerticesPerSize() * quadrant->getPos().getNumVerticesPerSize() * sizeof(float));
#ifdef REAL_T_IS_DOUBLE
			godot::PackedFloat64Array& heightsForCollider = quadrant->getHeightsForCollider(false);
			heightsForCollider.resize((int64_t)(quadrant->getPos().getNumVerticesPerSize() * quadrant->getPos().getNumVerticesPerSize()));
			float* f = (float*)quadrant->getFloat32HeightsBuffer().ptr();
			for (int idx = 0; idx < quadrant->getPos().getNumVerticesPerSize(); idx++)
			{
				heightsForCollider.set(idx, (real_t)*f);
				f++;
			}
#else
			godot::PackedFloat32Array& heightsForCollider = quadrant->getHeightsForCollider(false);
			heightsForCollider.resize((int64_t)(quadrant->getPos().getNumVerticesPerSize() * quadrant->getPos().getNumVerticesPerSize()));
			memcpy((char*)heightsForCollider.ptrw(), quadrant->getFloat32HeightsBuffer().ptr(), quadrant->getFloat32HeightsBuffer().size());
#endif
		}

		Vector3 startPosition(quadrant->getPos().getLowerXGridVertex(), quadrant->getTerrainEdit()->minHeight, quadrant->getPos().getLowerZGridVertex());
		Vector3 endPosition(startPosition.x + quadrant->getPos().getSizeInWU(), quadrant->getTerrainEdit()->maxHeight, startPosition.z + quadrant->getPos().getSizeInWU());
		Vector3 size = endPosition - startPosition;
		quadrant->getGlobalCoordAABB().set_position(startPosition);
		quadrant->getGlobalCoordAABB().set_size(size);

		//Chunk::HeightsChangedChunkAction action(m_viewer->is_visible_in_tree());
		Chunk::TerrainHeightsChangedChunkAction action;
		m_viewer->m_refreshRequired = true;
		quadrant->getQuadTree()->ForAllChunk(action, QuadType::NotSet);

		quadrant->getQuadTree()->materialParamsNeedReset(true);
		quadrant->getQuadTree()->colliderNeedReset(true, QuadType::NotSet);
		//quadrant->northQuadrantData().initialized(false);
		//quadrant->southQuadrantData().initialized(false);
		//quadrant->westQuadrantData().initialized(false);
		//quadrant->eastQuadrantData().initialized(false);
	}
	else
	{
		if (quadrant != nullptr and not quadrant->empty())
		{
			if (terrainEditValuesBuffer.size() > 0)
			{
				// renew MeshId
				TheWorld_Utils::MeshCacheBuffer& quadrantCache = quadrant->getInternalMeshCacheBuffer();
				quadrantCache.generateNewInternalMeshId();
				quadrant->setMeshId(quadrantCache.getMeshId());

				quadrant->getTerrainEdit()->deserialize(terrainEditValuesBuffer);

				// we need to save quadrant and every surrounding even if heights are not changed becasue terrain datas may be changed during blending
				{
					std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
					//m_mapQuadToSave[quadrant->getPos()] = "";
					TheWorld_Utils::QuadrantPos pos = quadrant->getPos();
					bool manual = true;
					addQuadToSave(pos, manual);
				}
			}
		}
	}
}

void GDN_TheWorld_Edit::manageUpdatedHeightsAfterQuadrantBlend(TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData& quadrantData, QuadTree* quadTree, TheWorld_Utils::MemoryBuffer& terrainEditValuesBuffer)
{
	//if (quadTree->getQuadrant()->getPos().getIndexPosX() == -7 and quadTree->getQuadrant()->getPos().getIndexPosZ() == -15)	// #####
	//	assert(quadTree != nullptr);									// #####

	if (quadrantData.heightsUpdated)
	{
		// renew MeshId
		TheWorld_Utils::MeshCacheBuffer& quadrantCache = quadTree->getQuadrant()->getInternalMeshCacheBuffer();
		quadrantCache.generateNewInternalMeshId();
		quadTree->getQuadrant()->setMeshId(quadrantCache.getMeshId());

		quadTree->getQuadrant()->getTerrainEdit()->deserialize(terrainEditValuesBuffer);
		
		{
			std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
			//m_mapQuadToSave[quadTree->getQuadrant()->getPos()] = "";
			TheWorld_Utils::QuadrantPos pos = quadTree->getQuadrant()->getPos();
			bool manual = true;
			addQuadToSave(pos, manual);
		}
		 
		//quadTree->getQuadrant()->setEmpty(false);
		quadTree->getQuadrant()->getTerrainEdit()->needUploadToServer = true;
		quadTree->getQuadrant()->setNeedUploadToServer(true);

		quadTree->getQuadrant()->heightmapChanged();
		quadTree->getQuadrant()->sendHeightsToShader(true);

		quadTree->getQuadrant()->resetNormalsBuffer(__FUNCTION__);

		quadTree->getQuadrant()->resetSplatmapBuffer(__FUNCTION__);

		quadTree->getQuadrant()->resetGlobalmapBuffer(__FUNCTION__);


		{
			my_assert(quadTree->getQuadrant()->getFloat32HeightsBuffer().size() == quadTree->getQuadrant()->getPos().getNumVerticesPerSize() * quadTree->getQuadrant()->getPos().getNumVerticesPerSize() * sizeof(float));
#ifdef REAL_T_IS_DOUBLE
			godot::PackedFloat64Array& heightsForCollider = quadTree->getQuadrant()->getHeightsForCollider(false);
			heightsForCollider.resize((int64_t)(quadTree->getQuadrant()->getPos().getNumVerticesPerSize() * quadTree->getQuadrant()->getPos().getNumVerticesPerSize()));
			float* f = (float*)quadTree->getQuadrant()->getFloat32HeightsBuffer().ptr();
			for (int idx = 0; idx < quadTree->getQuadrant()->getPos().getNumVerticesPerSize(); idx++)
			{
				heightsForCollider.set(idx, (real_t)*f);
				f++;
			}
#else
			godot::PackedFloat32Array& heightsForCollider = quadTree->getQuadrant()->getHeightsForCollider(false);
			heightsForCollider.resize((int64_t)(quadTree->getQuadrant()->getPos().getNumVerticesPerSize() * quadTree->getQuadrant()->getPos().getNumVerticesPerSize()));
			memcpy((char*)heightsForCollider.ptrw(), quadTree->getQuadrant()->getFloat32HeightsBuffer().ptr(), quadTree->getQuadrant()->getFloat32HeightsBuffer().size());
#endif
		}

		Vector3 startPosition(quadTree->getQuadrant()->getPos().getLowerXGridVertex(), quadTree->getQuadrant()->getTerrainEdit()->minHeight, quadTree->getQuadrant()->getPos().getLowerZGridVertex());
		Vector3 endPosition(startPosition.x + quadTree->getQuadrant()->getPos().getSizeInWU(), quadTree->getQuadrant()->getTerrainEdit()->maxHeight, startPosition.z + quadTree->getQuadrant()->getPos().getSizeInWU());
		Vector3 size = endPosition - startPosition;
		quadTree->getQuadrant()->getGlobalCoordAABB().set_position(startPosition);
		quadTree->getQuadrant()->getGlobalCoordAABB().set_size(size);

		//Chunk::HeightsChangedChunkAction action(m_viewer->is_visible_in_tree());
		Chunk::TerrainHeightsChangedChunkAction action;
		m_viewer->m_refreshRequired = true;
		quadTree->ForAllChunk(action, QuadType::NotSet);

		quadTree->materialParamsNeedReset(true);
		quadTree->colliderNeedReset(true, QuadType::NotSet);
		//quadTree->getQuadrant()->northQuadrantData().initialized(false);
		//quadTree->getQuadrant()->southQuadrantData().initialized(false);
		//quadTree->getQuadrant()->westQuadrantData().initialized(false);
		//quadTree->getQuadrant()->eastQuadrantData().initialized(false);
	}
	else
	{
		if (not quadTree->getQuadrant()->empty())
		{
			if (terrainEditValuesBuffer.size() > 0)
			{
				// renew MeshId
				TheWorld_Utils::MeshCacheBuffer& quadrantCache = quadTree->getQuadrant()->getInternalMeshCacheBuffer();
				quadrantCache.generateNewInternalMeshId();
				quadTree->getQuadrant()->setMeshId(quadrantCache.getMeshId());

				quadTree->getQuadrant()->getTerrainEdit()->deserialize(terrainEditValuesBuffer);

				// we need to save quadrant and every surrounding even if heights are not changed becasue terrain datas may be changed during blending
				{
					std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
					//m_mapQuadToSave[quadTree->getQuadrant()->getPos()] = "";
					TheWorld_Utils::QuadrantPos pos = quadTree->getQuadrant()->getPos();
					bool manual = true;
					addQuadToSave(pos, manual);
				}
			}
		}
	}
}

void GDN_TheWorld_Edit::editModeResetNormalsSplatmapAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	m_allItems = 0;
	setElapsed(0, true);
	setNote1(0);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeResetNormalsSplatmap, this);
	m_tp.QueueJob(f);
}

void GDN_TheWorld_Edit::editModeResetNormalsSplatmap(void)
{
	if (m_actionClock.counterStarted())
	{
		m_actionInProgress = false;
		return;
	}

	setMessage(godot::String("Reset in progress..."));

	m_actionClock.tick();

	QuadTree* quadTreeSel = nullptr;
	TheWorld_Utils::QuadrantPos quadrantSelPos = m_viewer->getWorldQuadrantSelForEdit(&quadTreeSel);
	//bool allCheched = m_innerData->m_allCheckBox->is_pressed();
	bool allCheched = false;
	if (!allCheched && quadrantSelPos.empty())
	{
		m_actionClock.tock();
		m_actionInProgress = false;
		setMessage(godot::String("Completed!"), true);
		return;
	}

	std::vector<TheWorld_Utils::QuadrantPos> quadrantPosToRegen;
	if (allCheched)
		m_viewer->getAllQuadrantPos(quadrantPosToRegen);
	else
		quadrantPosToRegen.push_back(quadrantSelPos);

	m_allItems = quadrantPosToRegen.size();
	for (auto& pos : quadrantPosToRegen)
	{
		if (m_actionStopRequested)
		{
			m_actionStopRequested = false;
			break;
		}

		QuadTree* quadTree = m_viewer->getQuadTree(pos, true, true);
		if (quadTree != nullptr)
		{
			quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen = true;
			quadTree->getQuadrant()->getNormalsBuffer().clear(__FUNCTION__);
			quadTree->getQuadrant()->updateTerrainEditTextureLayers();
			quadTree->getQuadrant()->unlockInternalData();
			quadTree->autoSaveRequired(false);
			m_completedItems++;
			size_t partialCount = m_actionClock.partialDuration().count();
			m_lastElapsed = partialCount - m_elapsedCompleted;
			m_elapsedCompleted = partialCount;
		}
	}

	m_actionClock.tock();
	m_actionInProgress = false;
	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	setMessage(godot::String("Reset completed!"));
}

void GDN_TheWorld_Edit::editModeSelectLookDevAction(int32_t index)
{
	enum class ShaderTerrainData::LookDev lookDev = (enum class ShaderTerrainData::LookDev)m_innerData->m_lookDevOptionButton->get_item_id(index);
	m_viewer->setDesideredLookDev(lookDev);
}

void GDN_TheWorld_Edit::editModeSelectTerrainTypeAction(int32_t index)
{
	enum class TheWorld_Utils::TerrainEdit::TerrainType terrainType = (enum class TheWorld_Utils::TerrainEdit::TerrainType)m_innerData->m_terrTypeOptionButton->get_item_id(index);
	std::string s = TheWorld_Utils::TerrainEdit::terrainTypeString(terrainType);

	TheWorld_Utils::TerrainEdit terrainEdit(terrainType);

	QuadTree* selectedQuadTree = nullptr;
	TheWorld_Utils::QuadrantPos selectedQuadrantPos = m_viewer->getWorldQuadrantSelForEdit(&selectedQuadTree);
		
	Quadrant* quadrant = nullptr;
	if (!selectedQuadrantPos.empty() && selectedQuadTree != nullptr)
	{
		TheWorld_Utils::TerrainEdit* northSideTerrainEdit = nullptr;
		TheWorld_Utils::QuadrantPos pos = selectedQuadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus);
		QuadTree* q = m_viewer->getQuadTree(pos, false);
		if (q != nullptr /* && !q->getQuadrant()->empty()*/)
		{
			northSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
		}
		TheWorld_Utils::TerrainEdit* southSideTerrainEdit = nullptr;
		pos = selectedQuadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus);
		q = m_viewer->getQuadTree(pos, false);
		if (q != nullptr /* && !q->getQuadrant()->empty()*/)
		{
			southSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
		}
		TheWorld_Utils::TerrainEdit* westSideTerrainEdit = nullptr;
		pos = selectedQuadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus);
		q = m_viewer->getQuadTree(pos, false);
		if (q != nullptr /* && !q->getQuadrant()->empty()*/)
		{
			westSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
		}
		TheWorld_Utils::TerrainEdit* eastSideTerrainEdit = nullptr;
		pos = selectedQuadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus);
		q = m_viewer->getQuadTree(pos, false);
		if (q != nullptr /* && !q->getQuadrant()->empty()*/)
		{
			eastSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
		}
		terrainEdit.adjustValues(northSideTerrainEdit, southSideTerrainEdit, westSideTerrainEdit, eastSideTerrainEdit);
	}

	TheWorld_Utils::QuadrantPos p;
	if (selectedQuadTree != nullptr)
		p = selectedQuadTree->getQuadrant()->getPos();
	setTerrainEditValues(terrainEdit, p);

	// auto generate of terrain on terrain type selection
	if (!selectedQuadrantPos.empty() && selectedQuadTree != nullptr and m_viewer->worldMode())
		editModeGenerateAction();
}

void GDN_TheWorld_Edit::setMessage(std::string text, bool add)
{
	setMessage(godot::String(text.c_str()), add);
}

void GDN_TheWorld_Edit::setMessage(godot::String text, bool add)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxUI);

	if (add)
	{
		//godot::String t = m_innerData->m_message->get_text();
		godot::String t = m_innerData->m_lastMessage;
		t += " ";
		t += text;
		m_innerData->m_lastMessage = t;
		//m_message->set_text(t);
	}
	else
		m_innerData->m_lastMessage = text;
		//m_message->set_text(text);

	m_lastMessageChanged = true;
}
