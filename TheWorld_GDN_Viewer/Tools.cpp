#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "Tools.h"
#include "Utils.h"
#include "Viewer_Utils.h"

#pragma warning(push, 0)
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/mesh.hpp>
#pragma warning(pop)

using namespace godot;

GDN_TheWorld_ShaderTexture::GDN_TheWorld_ShaderTexture()
{
}

GDN_TheWorld_ShaderTexture::~GDN_TheWorld_ShaderTexture()
{
	deinit();
}

void GDN_TheWorld_ShaderTexture::init(Node* parent, godot::Vector2i size, std::string shaderPath)
{
	deinit();

	m_subviewport = memnew(godot::SubViewport);
	parent->add_child(m_subviewport);
	m_subviewport->set_size(size);
	//m_subviewport->set_update_mode(godot::SubViewport::UpdateMode::UPDATE_WHEN_VISIBLE);
	//m_subviewport->set_clear_mode(godot::SubViewport::ClearMode::CLEAR_MODE_ALWAYS);
	godot::Ref<godot::World3D> w = memnew(godot::World3D);
	m_subviewport->set_world_3d(w);
	m_subviewport->set_use_own_world_3d(true);

	godot::ResourceLoader* resLoader = godot::ResourceLoader::get_singleton();
	godot::Ref<godot::ShaderMaterial> mat = memnew(godot::ShaderMaterial);
	godot::Ref<godot::Shader> shader = resLoader->load(shaderPath.c_str()/*, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE*/);
	mat->set_shader(shader);
	m_mat = mat;

	godot::ColorRect* colorRect = memnew(godot::ColorRect);
	m_subviewport->add_child(colorRect);
	colorRect->set_size(m_subviewport->get_size());
	colorRect->set_anchors_preset(godot::Control::LayoutPreset::PRESET_FULL_RECT);
	colorRect->set_material(mat);
}

void GDN_TheWorld_ShaderTexture::deinit(void)
{
	if (m_subviewport != nullptr)
	{
		Node* parent = m_subviewport->get_parent();
		if (parent != nullptr)
			parent->remove_child(m_subviewport);
		m_subviewport->queue_free();
		m_subviewport = nullptr;
		m_mat.unref();
	}
}

godot::Ref<godot::ViewportTexture> GDN_TheWorld_ShaderTexture::getTexture(void)
{
	return m_subviewport->get_texture();
}

godot::Ref<godot::ShaderMaterial> GDN_TheWorld_ShaderTexture::getMaterial(void)
{
	return m_mat;
}

GDN_TheWorld_Gizmo3d::GDN_TheWorld_Gizmo3d()
{
}

GDN_TheWorld_Gizmo3d::~GDN_TheWorld_Gizmo3d()
{
}

void GDN_TheWorld_Gizmo3d::_bind_methods()
{
}

void GDN_TheWorld_Gizmo3d::_ready(void)
{
	const double pixel_size = 0.05;		// 0.005
	const int32_t font_size = 128;		// 32
	const int32_t outline_size = 36;	// 12

	// X Axis GDN_TheWorld_Globals::g_color_red 
	godot::Ref<godot::CylinderMesh> xAzisMesh = memnew(godot::CylinderMesh);
	xAzisMesh->set_cap_bottom(false);
	xAzisMesh->set_cap_top(false);
	xAzisMesh->set_bottom_radius(m_arrowRadius);
	xAzisMesh->set_top_radius(m_arrowRadius);
	xAzisMesh->set_height(m_arrowSize);
	godot::Ref<godot::StandardMaterial3D> xAxisMat = memnew(godot::StandardMaterial3D);
	xAxisMat->set_albedo(GDN_TheWorld_Globals::g_color_red);
	m_xAzisMesh = memnew(godot::MeshInstance3D);
	m_xAzisMesh->set_mesh(xAzisMesh);
	m_xAzisMesh->set_material_override(xAxisMat);
	add_child(m_xAzisMesh);
	m_xAzisMesh->set_rotation_degrees(godot::Vector3(0.0f, 0.0f, 270.0f));
	m_xAzisMesh->set_position(godot::Vector3(m_arrowSize / 2, 0.0f, 0.0f));

	godot::Ref<godot::CylinderMesh> xAzisArrowheadMesh = memnew(godot::CylinderMesh);
	xAzisArrowheadMesh->set_cap_bottom(true);
	xAzisArrowheadMesh->set_cap_top(false);
	xAzisArrowheadMesh->set_bottom_radius(m_arrowRadius * m_arrowheadRadiusMultiplier);
	xAzisArrowheadMesh->set_top_radius(0.0f);
	xAzisArrowheadMesh->set_height(m_arrowheadSize);
	m_xAxisArrowheadMesh = memnew(godot::MeshInstance3D);
	m_xAxisArrowheadMesh->set_mesh(xAzisArrowheadMesh);
	m_xAxisArrowheadMesh->set_material_override(xAxisMat);
	add_child(m_xAxisArrowheadMesh);
	m_xAxisArrowheadMesh->set_rotation_degrees(godot::Vector3(0.0f, 0.0f, 270.0f));
	m_xAxisArrowheadMesh->set_position(godot::Vector3(m_arrowheadSize / 2 + m_arrowSize, 0.0f, 0.0f));

	m_xAxisLabelPosIn3d = godot::Vector3(m_arrowheadSize + m_arrowSize + 5, 0.0f, 0.0f);
	//m_xAxisLabel3d = memnew(godot::Label3D);
	//add_child(m_xAxisLabel3d);
	//m_xAxisLabel3d->set_text("X");
	//m_xAxisLabel3d->set_position(m_xAxisLabelPosIn3d);
	//m_xAxisLabel3d->set_font_size(font_size);
	//m_xAxisLabel3d->set_pixel_size(pixel_size);
	//m_xAxisLabel3d->set_outline_size(outline_size);
	//m_xAxisLabel3d->set_modulate(GDN_TheWorld_Globals::g_color_red);
	//m_xAxisLabel3d->set_outline_modulate(godot::Color(0.27f, 0.27f, 0.27f, 1.0f));
	m_xAxisLabel2d = memnew(godot::Label);
	add_child(m_xAxisLabel2d);
	m_xAxisLabel2d->set_text("X EAST");
	m_xAxisLabel2d->set_modulate(GDN_TheWorld_Globals::g_color_red);
	//m_xAxisLabel2d->add_theme_color_override("font_color", GDN_TheWorld_Globals::g_color_red);

	// Y Axis GDN_TheWorld_Globals::g_color_green 
	godot::Ref<godot::CylinderMesh> yAzisMesh = memnew(godot::CylinderMesh);
	yAzisMesh->set_cap_bottom(false);
	yAzisMesh->set_cap_top(false);
	yAzisMesh->set_bottom_radius(m_arrowRadius);
	yAzisMesh->set_top_radius(m_arrowRadius);
	yAzisMesh->set_height(m_arrowSize);
	godot::Ref<godot::StandardMaterial3D> yAxisMat = memnew(godot::StandardMaterial3D);
	yAxisMat->set_albedo(GDN_TheWorld_Globals::g_color_green);
	m_yAzisMesh = memnew(godot::MeshInstance3D);
	m_yAzisMesh->set_mesh(yAzisMesh);
	m_yAzisMesh->set_material_override(yAxisMat);
	add_child(m_yAzisMesh);
	m_yAzisMesh->set_position(godot::Vector3(0.0f, m_arrowSize / 2, 0.0f));

	godot::Ref<godot::CylinderMesh> yAzisArrowheadMesh = memnew(godot::CylinderMesh);
	yAzisArrowheadMesh->set_cap_bottom(true);
	yAzisArrowheadMesh->set_cap_top(false);
	yAzisArrowheadMesh->set_bottom_radius(m_arrowRadius * m_arrowheadRadiusMultiplier);
	yAzisArrowheadMesh->set_top_radius(0.0f);
	yAzisArrowheadMesh->set_height(m_arrowheadSize);
	m_yAxisArrowheadMesh = memnew(godot::MeshInstance3D);
	m_yAxisArrowheadMesh->set_mesh(yAzisArrowheadMesh);
	m_yAxisArrowheadMesh->set_material_override(yAxisMat);
	add_child(m_yAxisArrowheadMesh);
	m_yAxisArrowheadMesh->set_position(godot::Vector3(0.0f, m_arrowheadSize / 2 + m_arrowSize, 0.0f));

	m_yAxisLabelPosIn3d = godot::Vector3(0.0f, m_arrowheadSize + m_arrowSize + 5, 0.0f);
	//m_yAxisLabel3d = memnew(godot::Label3D);
	//add_child(m_yAxisLabel3d);
	//m_yAxisLabel3d->set_text("Y");
	//m_yAxisLabel3d->set_position(m_yAxisLabelPosIn3d);
	//m_yAxisLabel3d->set_font_size(font_size);
	//m_yAxisLabel3d->set_pixel_size(pixel_size);
	//m_yAxisLabel3d->set_outline_size(outline_size);
	//m_yAxisLabel3d->set_modulate(GDN_TheWorld_Globals::g_color_green);
	//m_yAxisLabel3d->set_outline_modulate(godot::Color(0.27f, 0.27f, 0.27f, 1.0f));
	m_yAxisLabel2d = memnew(godot::Label);
	add_child(m_yAxisLabel2d);
	m_yAxisLabel2d->set_text("Y UP");
	m_yAxisLabel2d->set_modulate(GDN_TheWorld_Globals::g_color_green);
	//m_yAxisLabel2d->add_theme_color_override("font_color", GDN_TheWorld_Globals::g_color_green);

	// Z Axis GDN_TheWorld_Globals::g_color_blue 
	godot::Ref<godot::CylinderMesh> zAzisMesh = memnew(godot::CylinderMesh);
	zAzisMesh->set_cap_bottom(false);
	zAzisMesh->set_cap_top(true);
	zAzisMesh->set_bottom_radius(m_arrowRadius);
	zAzisMesh->set_top_radius(m_arrowRadius);
	zAzisMesh->set_height(m_arrowSize);
	godot::Ref<godot::StandardMaterial3D> zAxisMat = memnew(godot::StandardMaterial3D);
	zAxisMat->set_albedo(GDN_TheWorld_Globals::g_color_blue);
	m_zAzisMesh = memnew(godot::MeshInstance3D);
	m_zAzisMesh->set_mesh(zAzisMesh);
	m_zAzisMesh->set_material_override(zAxisMat);
	add_child(m_zAzisMesh);
	m_zAzisMesh->set_rotation_degrees(godot::Vector3(90.0f, 0.0f, 0.0f));
	m_zAzisMesh->set_position(godot::Vector3(0.0f, 0.0f, m_arrowSize / 2));

	godot::Ref<godot::CylinderMesh> zAzisArrowheadMesh = memnew(godot::CylinderMesh);
	zAzisArrowheadMesh->set_cap_bottom(true);
	zAzisArrowheadMesh->set_cap_top(false);
	zAzisArrowheadMesh->set_bottom_radius(m_arrowRadius * m_arrowheadRadiusMultiplier);
	zAzisArrowheadMesh->set_top_radius(0.0f);
	zAzisArrowheadMesh->set_height(m_arrowheadSize);
	m_zAxisArrowheadMesh = memnew(godot::MeshInstance3D);
	m_zAxisArrowheadMesh->set_mesh(zAzisArrowheadMesh);
	m_zAxisArrowheadMesh->set_material_override(zAxisMat);
	add_child(m_zAxisArrowheadMesh);
	m_zAxisArrowheadMesh->set_rotation_degrees(godot::Vector3(90.0f, 0.0f, 0.0f));
	m_zAxisArrowheadMesh->set_position(godot::Vector3(0.0f, 0.0f, m_arrowheadSize / 2 + m_arrowSize));

	m_zAxisLabelPosIn3d = godot::Vector3(0.0f, 0.0f, m_arrowheadSize + m_arrowSize + 5);
	//m_zAxisLabel3d = memnew(godot::Label3D);
	//add_child(m_zAxisLabel3d);
	//m_zAxisLabel3d->set_text("Z");
	//m_zAxisLabel3d->set_position(m_zAxisLabelPosIn3d);
	//m_zAxisLabel3d->set_font_size(font_size);
	//m_zAxisLabel3d->set_pixel_size(pixel_size);
	//m_zAxisLabel3d->set_outline_size(outline_size);
	//m_zAxisLabel3d->set_modulate(GDN_TheWorld_Globals::g_color_blue);
	//m_zAxisLabel3d->set_outline_modulate(godot::Color(0.27f, 0.27f, 0.27f, 1.0f));
	m_zAxisLabel2d = memnew(godot::Label);
	add_child(m_zAxisLabel2d);
	m_zAxisLabel2d->set_text("Z SOUTH");
	m_zAxisLabel2d->set_modulate(GDN_TheWorld_Globals::g_color_blue);
	//m_zAxisLabel2d->add_theme_color_override("font_color", GDN_TheWorld_Globals::g_color_blue);

	m_localTransformBasis = get_transform().basis;
}

void GDN_TheWorld_Gizmo3d::_process(double _delta)
{
	if (m_camera != nullptr && is_visible())
	{
		godot::Vector3 cameraPos = m_camera->get_global_transform().origin;
		
		//m_xAxisLabel3d->look_at(cameraPos, godot::Vector3(0, 1, 0));
		//m_xAxisLabel3d->rotate_y(TheWorld_Utils::kPi);
		//m_yAxisLabel3d->look_at(cameraPos, godot::Vector3(0, 1, 0));
		//m_yAxisLabel3d->rotate_y(TheWorld_Utils::kPi);
		//m_zAxisLabel3d->look_at(cameraPos, godot::Vector3(0, 1, 0));
		//m_zAxisLabel3d->rotate_y(TheWorld_Utils::kPi);

		float distanceFromCamera = (float)cameraPos.distance_to(get_global_transform().origin);
		float scaleFactor = 1.2f;
		if (distanceFromCamera > 5000)
			scaleFactor = distanceFromCamera / 400.0f;
		else if (distanceFromCamera > 1000)
				scaleFactor = distanceFromCamera / 350.0f;
		else if (distanceFromCamera > 500)
			scaleFactor = distanceFromCamera / 300.0f;

		godot::Transform3D t = godot::Transform3D(m_localTransformBasis.scaled(godot::Vector3(scaleFactor, scaleFactor, scaleFactor)), get_transform().origin);
		set_transform(t);

		godot::Vector2 offset(0.0f, -15.0f);
		godot::Vector3 xAxisLabelGlobalPosIn3d = m_xAxisLabelPosIn3d * scaleFactor + get_global_position();
		if (m_xAxisLabel2d != nullptr && !m_camera->is_position_behind(xAxisLabelGlobalPosIn3d))
		{
			godot::Vector2 viewportPos = m_camera->unproject_position(xAxisLabelGlobalPosIn3d) + offset;
			m_xAxisLabel2d->set_position(viewportPos);
			m_xAxisLabel2d->set_visible(true);
		}
		godot::Vector3 yAxisLabelGlobalPosIn3d = m_yAxisLabelPosIn3d * scaleFactor + get_global_position();
		if (m_yAxisLabel2d != nullptr && !m_camera->is_position_behind(yAxisLabelGlobalPosIn3d))
		{
			godot::Vector2 viewportPos = m_camera->unproject_position(yAxisLabelGlobalPosIn3d) + offset;
			m_yAxisLabel2d->set_position(viewportPos);
			m_yAxisLabel2d->set_visible(true);
		}
		godot::Vector3 zAxisLabelGlobalPosIn3d = m_zAxisLabelPosIn3d * scaleFactor + get_global_position();
		if (m_zAxisLabel2d != nullptr && !m_camera->is_position_behind(zAxisLabelGlobalPosIn3d))
		{
			godot::Vector2 viewportPos = m_camera->unproject_position(zAxisLabelGlobalPosIn3d) + offset;
			m_zAxisLabel2d->set_position(viewportPos);
			m_zAxisLabel2d->set_visible(true);
		}
	}

	if (!is_visible())
	{
		m_xAxisLabel2d->set_visible(false);
		m_yAxisLabel2d->set_visible(false);
		m_zAxisLabel2d->set_visible(false);
	}
}

GDN_TheWorld_Drawer::Drawing::Drawing()
{
}

GDN_TheWorld_Drawer::Drawing::~Drawing()
{
	//if (label3d != nullptr)
	//{
	//	Node* parent = label3d->get_parent();
	//	if (parent != nullptr)
	//		parent->remove_child(label3d);
	//	label3d->queue_free();
	//	label3d = nullptr;
	//}

	//if (label2d != nullptr)
	//{
	//	Node* parent = label2d->get_parent();
	//	if (parent != nullptr)
	//		parent->remove_child(label2d);
	//	label2d->queue_free();
	//	label2d = nullptr;
	//}
}

GDN_TheWorld_Drawer::GDN_TheWorld_Drawer()
{
}

GDN_TheWorld_Drawer::~GDN_TheWorld_Drawer()
{
}

void GDN_TheWorld_Drawer::_bind_methods()
{
}

void GDN_TheWorld_Drawer::_ready(void)
{
	godot::Ref<godot::StandardMaterial3D> mat = memnew(godot::StandardMaterial3D);
	m_mat = mat;
	m_mat->set("use_point_size", false);
	m_mat->set("point_size", 5.0f);
	m_mat->set("no_depth_test", false);
	//m_mat->set("shading_mode", (int)godot::BaseMaterial3D::ShadingMode::SHADING_MODE_PER_PIXEL);
	m_mat->set("shading_mode", (int)godot::BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);
	m_mat->set("vertex_color_use_as_albedo", true);
	//m_mat->set("transparency", (int)godot::BaseMaterial3D::Transparency::TRANSPARENCY_ALPHA);
	m_mat->set("transparency", (int)godot::BaseMaterial3D::Transparency::TRANSPARENCY_DISABLED);

	godot::Ref<godot::ImmediateMesh> mesh = memnew(godot::ImmediateMesh);
	m_mesh = mesh;
	//m_mesh->surface_set_material(0, m_mat);

	set_mesh(m_mesh);
	set_material_override(m_mat);
}

void GDN_TheWorld_Drawer::_process(double _delta)
{
	if (!m_drawings.empty())
	{
		if (is_visible())
		{
			const double pixel_size = 0.05;		// 0.005
			const int32_t font_size = 128;		// 32
			const int32_t outline_size = 36;	// 12

			m_mesh->clear_surfaces();
			for (auto& item : m_drawings)
			{
				godot::Vector3 start = item.second->start;
				godot::Vector3 end = item.second->end;

				if (item.second->drawingType == Drawing::DrawingType::line && start != end)
				{
					if (start == godot::Vector3() && m_camera != nullptr)
						start = m_viewer->globalPosToWorldPos(m_camera->get_global_position());
					if (end == godot::Vector3() && m_camera != nullptr)
						end = m_viewer->globalPosToWorldPos(m_camera->get_global_position());

					drawLine(start, end, item.second->color);
					if (item.second->labelPos == Drawing::LabelPos::not_set)
					{
						if (item.second->label3d != nullptr)
						{
							Node* parent = item.second->label3d->get_parent();
							if (parent != nullptr)
								parent->remove_child(item.second->label3d);
							item.second->label3d->queue_free();
							item.second->label3d = nullptr;
						}
						if (item.second->label2d != nullptr)
						{
							Node* parent = item.second->label2d->get_parent();
							if (parent != nullptr)
								parent->remove_child(item.second->label2d);
							item.second->label2d->queue_free();
							item.second->label2d = nullptr;
						}
					}
					else
					{
						if (item.second->label_3d)
						{
							if (item.second->label3d == nullptr)
							{
								item.second->label3d = memnew(godot::Label3D);
								add_child(item.second->label3d);
								item.second->label3d->set_font_size(font_size);
								item.second->label3d->set_pixel_size(float(pixel_size));
								item.second->label3d->set_outline_size(outline_size);
								item.second->label3d->set_outline_modulate(godot::Color(0.27f, 0.27f, 0.27f, 1.0f));
							}

							item.second->label3d->set_text(item.second->labelText.c_str());
							item.second->label3d->set_modulate(item.second->color);

							if (item.second->labelPos == Drawing::LabelPos::start)
							{
								// TODO
							}
							else if (item.second->labelPos == Drawing::LabelPos::center)
							{
								// TODO
							}
							else if (item.second->labelPos == Drawing::LabelPos::end)
							{
								item.second->label3d->set_position(end + (end - start).normalized() * item.second->label3dOffset);
							}

							if (m_camera != nullptr)
							{
								godot::Vector3 cameraPos = m_camera->get_global_transform().origin;
								item.second->label3d->look_at(cameraPos, godot::Vector3(0, 1, 0));
								item.second->label3d->rotate_y(TheWorld_Utils::kPi);
							}
						}

						if (item.second->label_2d)
						{
							if (m_camera != nullptr && m_viewer != nullptr)
							{
								//godot::Vector3 position;
								godot::Vector2 viewportPos;
								bool label_visible = false;
								if (item.second->labelPos == Drawing::LabelPos::start)
								{
									// TODO
									label_visible = false;
								}
								else if (item.second->labelPos == Drawing::LabelPos::center)
								{
									godot::Vector3 globalStart = m_viewer->worldPosToGlobalPos(start);
									godot::Vector3 globalEnd = m_viewer->worldPosToGlobalPos(end);
									godot::Vector2 endOnViewport = m_camera->unproject_position(globalEnd);
									godot::Vector2 startOnViewport = m_camera->unproject_position(globalStart);
									godot::Size2 viewportSize = m_viewer->getViewportSize();
									bool startBehindCamera = m_camera->is_position_behind(globalStart);
									bool endBehindCamera = m_camera->is_position_behind(globalEnd);

									if (startBehindCamera && endBehindCamera)
										label_visible = false;
									else
									{
										label_visible = true;

										if (startBehindCamera)
										{
											godot::Vector3 newStart = globalStart;
											while (startBehindCamera)
											{
												newStart = newStart + (globalEnd - newStart) / 10;
												startBehindCamera = m_camera->is_position_behind(newStart);
											}
											startOnViewport = m_camera->unproject_position(newStart);
										}

										viewportPos = startOnViewport + (endOnViewport - startOnViewport) / 2;

										if (viewportPos.x < 0 || viewportPos.x > viewportSize.x || viewportPos.y < 0 || viewportPos.y > viewportSize.y)
										{
											// pos outside viewport

											if (endOnViewport.x >= 0 && endOnViewport.x <= viewportSize.x && endOnViewport.y >= 0 && endOnViewport.y <= viewportSize.y)
											{
												// end inside viewport: we need to approach end until label will be inside viewport
												while (viewportPos.x < 0 || viewportPos.x > viewportSize.x || viewportPos.y < 0 || viewportPos.y > viewportSize.y)
													viewportPos = viewportPos + (endOnViewport - viewportPos) / 2;
											}
											else if (startOnViewport.x >= 0 && startOnViewport.x <= viewportSize.x && startOnViewport.y >= 0 && startOnViewport.y <= viewportSize.y)
											{
												// start inside viewport: we need to approach start until label will be inside viewport
												while (viewportPos.x < 0 || viewportPos.x > viewportSize.x || viewportPos.y < 0 || viewportPos.y > viewportSize.y)
													viewportPos = viewportPos + (startOnViewport - viewportPos) / 2;
											}
											else
											{
												// label not visible
												label_visible = false;
											}
										}
									}
								}
								else if (item.second->labelPos == Drawing::LabelPos::end)
								{
									godot::Vector3 position = end + (end - start).normalized() * item.second->label3dOffset;
									viewportPos = m_camera->unproject_position(m_viewer->worldPosToGlobalPos(position));
									label_visible = true;
								}

								if (label_visible)
								{
									if (item.second->label2d == nullptr)
									{
										item.second->label2d = memnew(godot::Label);
										add_child(item.second->label2d);
									}
									item.second->label2d->set_text(item.second->labelText.c_str());
									item.second->label2d->add_theme_color_override("font_color", item.second->color);
								}
								else
								{
									if (item.second->label2d != nullptr)
									{
										Node* parent = item.second->label2d->get_parent();
										if (parent != nullptr)
											parent->remove_child(item.second->label2d);
										item.second->label2d->queue_free();
										item.second->label2d = nullptr;
									}
								}

								if (item.second->label2d != nullptr)
								{
									viewportPos += item.second->label2dOffset;
									item.second->label2d->set_position(viewportPos);
								}
							}
						}
					}
				}
				else if (item.second->drawingType == Drawing::DrawingType::sphere && item.second->radius != 0.0f)
					drawSphere(start, item.second->radius, item.second->color);
				else if (item.second->drawingType == Drawing::DrawingType::label2d)
				{
					if (m_camera != nullptr && m_viewer != nullptr)
					{
						godot::Vector3 pos = m_viewer->worldPosToGlobalPos(start);

						bool label_visible = true;
						if (m_camera->is_position_behind(pos))
							label_visible = false;
						
						if (label_visible)
						{
							if (item.second->label2d == nullptr)
							{
								item.second->label2d = memnew(godot::Label);
								add_child(item.second->label2d);
							}
							godot::Vector2 viewportPos = m_camera->unproject_position(pos);
							viewportPos += item.second->label2dOffset;
							item.second->label2d->set_position(viewportPos);
							item.second->label2d->set_text(item.second->labelText.c_str());
							item.second->label2d->add_theme_color_override("font_color", item.second->color);
						}
						else
						{
							if (item.second->label2d != nullptr)
							{
								Node* parent = item.second->label2d->get_parent();
								if (parent != nullptr)
									parent->remove_child(item.second->label2d);
								item.second->label2d->queue_free();
								item.second->label2d = nullptr;
							}
						}
					}
				}
			}
		}
		else
		{
			for (auto& item : m_drawings)
			{
				if (item.second->label3d != nullptr)
				{
					Node* parent = item.second->label3d->get_parent();
					if (parent != nullptr)
						parent->remove_child(item.second->label3d);
					item.second->label3d->queue_free();
					item.second->label3d = nullptr;
				}
				if (item.second->label2d != nullptr)
				{
					Node* parent = item.second->label2d->get_parent();
					if (parent != nullptr)
						parent->remove_child(item.second->label2d);
					item.second->label2d->queue_free();
					item.second->label2d = nullptr;
				}
			}
		}
	}
}

void GDN_TheWorld_Drawer::clearDrawings()
{
	m_mesh->clear_surfaces();
}

void GDN_TheWorld_Drawer::drawLine(godot::Vector3 start, godot::Vector3 end, godot::Color c)
{
	m_mesh->surface_begin(godot::Mesh::PrimitiveType::PRIMITIVE_LINES);
	m_mesh->surface_set_color(c);
	m_mesh->surface_add_vertex(start);
	m_mesh->surface_add_vertex(end);
	m_mesh->surface_end();
}

void GDN_TheWorld_Drawer::drawSphere(godot::Vector3 center, float radius, godot::Color c)
{
	float numSteps = 16;
	float stepSize = (TheWorld_Utils::kPi2) / numSteps;

	// move here from the center
	std::vector<godot::Vector3> primaryAxis;
	primaryAxis.push_back(Vector3Up);
	primaryAxis.push_back(Vector3Right);
	primaryAxis.push_back(Vector3Forward);

	// then rotate around this axis
	std::vector<godot::Vector3> rotationAxis;
	rotationAxis.push_back(Vector3Right);
	rotationAxis.push_back(Vector3Forward);
	rotationAxis.push_back(Vector3Up);

	m_mesh->surface_begin(godot::Mesh::PrimitiveType::PRIMITIVE_LINE_STRIP);
	m_mesh->surface_set_color(c);

	for (int axis = 0; axis < primaryAxis.size(); axis++)
	{
		// ad an extra step to close the gap on the final disc of each rotation
		for (float i = 0; i <= numSteps; i++)
		{
			Vector3 vert = (primaryAxis[axis] * radius);
			vert = vert.rotated(rotationAxis[axis], stepSize * i);
			vert += center;
			m_mesh->surface_add_vertex(vert);
		}
		// throw in an extra center vert to tidy up the lines between axes
		m_mesh->surface_add_vertex(center);

	}
	m_mesh->surface_end();
}

int32_t GDN_TheWorld_Drawer::addLine(godot::Vector3 start, godot::Vector3 end, std::string labelText, bool label3d, bool label2d, enum class Drawing::LabelPos labelPos, float label3dOffset, godot::Vector2i label2dOffset, godot::Color c)
{
	int32_t idx = m_firstAvailableIdx++;
	m_drawings[idx] = std::make_unique<Drawing>();
	Drawing* drawing = m_drawings[idx].get();

	drawing->drawingType = Drawing::DrawingType::line;
	drawing->start = start;
	drawing->end = end;
	drawing->color = c;
	drawing->labelPos = labelPos;
	drawing->label_3d = label3d;
	drawing->label_2d = label2d;
	drawing->label3dOffset = label3dOffset;
	drawing->label2dOffset = label2dOffset;
	if (labelText == "@")
		drawing->labelText = "";
	else
		drawing->labelText = labelText;

	return idx;
}

void GDN_TheWorld_Drawer::updateLine(int32_t idx, godot::Vector3 start, godot::Vector3 end, std::string labelText, enum class Drawing::LabelPos labelPos, float label3dOffset, godot::Vector2i label2dOffset, godot::Color c)
{
	if (!m_drawings.contains(idx))
		return;

	Drawing* drawing = m_drawings[idx].get();

	if (drawing->drawingType != Drawing::DrawingType::line)
		return;

	drawing->start = start;
	drawing->end = end;
	if (c.a != 0.0f)
		drawing->color = c;
	if (labelPos != Drawing::LabelPos::not_set)
		drawing->labelPos = labelPos;
	if (label3dOffset >= 0)
		drawing->label3dOffset = label3dOffset;
	if (label2dOffset != godot::Vector2i(0, 0))
		drawing->label2dOffset = label2dOffset;
	if (labelText != "@")
		drawing->labelText = labelText;
	if (start == end)
	{
		if (drawing->label3d != nullptr)
		{
			Node* parent = drawing->label3d->get_parent();
			if (parent != nullptr)
				parent->remove_child(drawing->label3d);
			drawing->label3d->queue_free();
			drawing->label3d = nullptr;
		}

		if (drawing->label2d != nullptr)
		{
			Node* parent = drawing->label2d->get_parent();
			if (parent != nullptr)
				parent->remove_child(drawing->label2d);
			drawing->label2d->queue_free();
			drawing->label2d = nullptr;
		}

	}
}

int32_t GDN_TheWorld_Drawer::addSphere(godot::Vector3 center, float radius, godot::Color c)
{
	int32_t idx = m_firstAvailableIdx++;
	m_drawings[idx] = std::make_unique<Drawing>();
	Drawing* drawing = m_drawings[idx].get();

	drawing->drawingType = Drawing::DrawingType::sphere;
	drawing->start = center;
	drawing->radius = radius;
	drawing->color = c;

	return idx;
}

void GDN_TheWorld_Drawer::updateSphere(int32_t idx, godot::Vector3 center, float radius, godot::Color c)
{
	if (!m_drawings.contains(idx))
		return;

	Drawing* drawing = m_drawings[idx].get();

	if (drawing->drawingType != Drawing::DrawingType::sphere)
		return;

	drawing->start = center;
	if (radius >= 0)
		drawing->radius = radius;
	if (c.a != 0.0f)
		drawing->color = c;
}

int32_t GDN_TheWorld_Drawer::addLabel2d(godot::Vector3 pos, godot::Vector2i offset, std::string labelText, godot::Color c)
{
	int32_t idx = m_firstAvailableIdx++;
	m_drawings[idx] = std::make_unique<Drawing>();
	Drawing* drawing = m_drawings[idx].get();

	drawing->drawingType = Drawing::DrawingType::label2d;
	drawing->start = pos;
	if (labelText == "@")
		drawing->labelText = "";
	else
		drawing->labelText = labelText;
	drawing->label_3d = false;
	drawing->label_2d = true;
	drawing->label2dOffset = offset;
	drawing->color = c;

	return idx;
}

void GDN_TheWorld_Drawer::updateLabel2d(int32_t idx, godot::Vector3 pos, std::string labelText, godot::Color c)
{
	if (!m_drawings.contains(idx))
		return;

	Drawing* drawing = m_drawings[idx].get();

	if (drawing->drawingType != Drawing::DrawingType::label2d)
		return;

	drawing->start = pos;
	if (labelText != "@")
		drawing->labelText = labelText;

	if (c.a != 0.0f)
		drawing->color = c;
}

void GDN_TheWorld_Drawer::removeDrawing(int32_t idx)
{
	if (m_drawings.contains(idx))
		m_drawings.erase(idx);
}

void SegmentLine3D::_bind_methods()
{
	// Registra i metodi se vuoi esporli a GDScript (opzionale)
	//godot::ClassDB::bind_method(godot::D_METHOD("set_points", "points"), &SegmentLine3D::set_points);
	//godot::ClassDB::bind_method(godot::D_METHOD("add_point", "point"), &SegmentLine3D::add_point);
	//godot::ClassDB::bind_method(godot::D_METHOD("clear_points"), &SegmentLine3D::clear_points);
	//godot::ClassDB::bind_method(godot::D_METHOD("redraw"), &SegmentLine3D::redraw);
}

SegmentLine3D::SegmentLine3D()
{
	// Inizializza la mesh immediata e il materiale
	imm_mesh.instantiate();
	material.instantiate();

	// Imposta il materiale in modalità Unshaded e dagli un colore (es. Rosso)
	material->set_shading_mode(godot::BaseMaterial3D::SHADING_MODE_UNSHADED);
	material->set_albedo(godot::Color(1.0f, 0.0f, 0.0f));
}

SegmentLine3D::~SegmentLine3D() {}

void SegmentLine3D::_ready()
{
	// Assegna la mesh al nodo MeshInstance3D
	set_mesh(imm_mesh);
}

void SegmentLine3D::set_points(const godot::Vector<godot::Vector3>& p_points)
{
	points = p_points;
	redraw();
}

void SegmentLine3D::add_point(const godot::Vector3& p_point)
{
	points.push_back(p_point);
	redraw();
}

void SegmentLine3D::clear_points()
{
	points.clear();
	imm_mesh->clear_surfaces();
}

void SegmentLine3D::redraw()
{
	// Pulisci la mesh precedente
	imm_mesh->clear_surfaces();

	if (points.size() < 2)
	{
   		return; // Servono almeno due punti per fare un segmento
	}

	// Inizia a disegnare usando la primitiva LINE_STRIP (sequenza di segmenti collegati)
	imm_mesh->surface_begin(godot::Mesh::PRIMITIVE_LINE_STRIP, material);

	for (int i = 0; i < points.size(); ++i)
	{
   		imm_mesh->surface_add_vertex(points[i]);
	}

	imm_mesh->surface_end();
}

void DynamicTrail3D::_bind_methods()
{
    //godot::ClassDB::bind_method(godot::D_METHOD("add_segment_point", "point"), &DynamicTrail3D::add_segment_point);
    //godot::ClassDB::bind_method(godot::D_METHOD("clear_segments"), &DynamicTrail3D::clear_segments);

    //godot::ClassDB::bind_method(godot::D_METHOD("set_thickness", "thickness"), &DynamicTrail3D::set_thickness);
    //godot::ClassDB::bind_method(godot::D_METHOD("get_thickness"), &DynamicTrail3D::get_thickness);

    //ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "thickness"), "set_thickness", "get_thickness");
}

DynamicTrail3D::DynamicTrail3D()
{
    array_mesh.instantiate();
    material.instantiate();

    // Configura il materiale di base (puoi esporlo all'Ispettore se preferisci)
    material->set_cull_mode(godot::BaseMaterial3D::CULL_DISABLED); // Visibile da entrambi i lati
    material->set_albedo(godot::Color(0.0f, 0.8f, 1.0f));          // Colore Ciano
}

DynamicTrail3D::~DynamicTrail3D() {}

void DynamicTrail3D::_ready()
{
    set_mesh(array_mesh);
    set_material_override(material);
}

void DynamicTrail3D::_process(double delta)
{
    // Esegui la ricostruzione della mesh ad ogni frame
    rebuild_mesh();
}

void DynamicTrail3D::add_segment_point(const godot::Vector3& p_point)
{
    points.push_back(p_point);
}

void DynamicTrail3D::clear_segments()
{
    points.clear();
    array_mesh->clear_surfaces();
}

void DynamicTrail3D::set_thickness(float p_thickness)
{
    thickness = p_thickness;
}

float DynamicTrail3D::get_thickness() const
{
    return thickness;
}

void DynamicTrail3D::rebuild_mesh()
{
	array_mesh->clear_surfaces();

    int total_points = int(points.size());
    if (total_points < 2)
	{
        return; // Servono almeno due punti per generare dei triangoli 3D
    }

    godot::PackedVector3Array vertices;
    godot::PackedVector2Array uvs;
    godot::PackedInt32Array indices;

    // Riserva memoria per evitare riallocazioni continue ad ogni frame
    vertices.resize(total_points * 2);
    uvs.resize(total_points * 2);

    // Calcola i vettori di estrusione
    for (int i = 0; i < total_points; ++i)
	{
        godot::Vector3 forward;

        if (i == 0)
		{
            forward = (points[1] - points[0]).normalized();
        }
        else if (i == total_points - 1)
		{
            forward = (points[i] - points[i - 1]).normalized();
        }
        else
		{
            godot::Vector3 dir1 = (points[i] - points[i - 1]).normalized();
            godot::Vector3 dir2 = (points[i + 1] - points[i]).normalized();
            forward = (dir1 + dir2).normalized(); // Direzione media del nodo
        }

        // Trova un vettore perpendicolare per lo spessore (es. asse Y)
        // Nota: Per risultati perfetti a 360°, potresti voler calcolare questo vettore rispetto alla Camera
        godot::Vector3 up = godot::Vector3(0, 1, 0);
        if (godot::Math::abs(forward.dot(up)) > 0.99f)
		{
            up = godot::Vector3(1, 0, 0); // Previene il gimbal lock se la linea va dritta in verticale
        }

        godot::Vector3 right = forward.cross(up).normalized();
        godot::Vector3 offset = right * (thickness * 0.5f);

        // Genera i due vertici per formare lo spessore del segmento corrente
        vertices[i * 2] = points[i] - offset;
        vertices[i * 2 + 1] = points[i] + offset;

        // Calcola le coordinate UV (utili se vuoi applicare texture animate lungo i segmenti)
        float progress = (float)i / (float)(total_points - 1);
        uvs[i * 2] = godot::Vector2(progress, 0.0f);
        uvs[i * 2 + 1] = godot::Vector2(progress, 1.0f);

        // Costruisci gli indici dei triangoli (Quad) che collegano il punto attuale al precedente
        if (i > 0)
		{
            int curr_left = i * 2;
            int curr_right = i * 2 + 1;
            int prev_left = (i - 1) * 2;
            int prev_right = (i - 1) * 2 + 1;

            // Primo triangolo del segmento
            indices.push_back(prev_left);
            indices.push_back(curr_left);
            indices.push_back(prev_right);

            // Secondo triangolo del segmento
            indices.push_back(prev_right);
            indices.push_back(curr_left);
            indices.push_back(curr_right);
        }
    }

    // Prepara l'array globale di Godot per la generazione della superficie mesh
    godot::Array mesh_arrays;
    mesh_arrays.resize(godot::Mesh::ARRAY_MAX);
    mesh_arrays[godot::Mesh::ARRAY_VERTEX] = vertices;
    mesh_arrays[godot::Mesh::ARRAY_TEX_UV] = uvs;
    mesh_arrays[godot::Mesh::ARRAY_INDEX] = indices;

    // Invia i vettori alla GPU in un colpo solo
    array_mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES, mesh_arrays);
}

void DynamicTube3D::_bind_methods()
{
	//ClassDB::bind_method(D_METHOD("add_segment_point", "point"), &DynamicTube3D::add_segment_point);
	//ClassDB::bind_method(D_METHOD("clear_segments"), &DynamicTube3D::clear_segments);

	//ClassDB::bind_method(D_METHOD("set_mode", "mode"), &DynamicTube3D::set_mode);
	//ClassDB::bind_method(D_METHOD("get_mode"), &DynamicTube3D::get_mode);
	//ClassDB::bind_method(D_METHOD("set_thickness", "thickness"), &DynamicTube3D::set_thickness);
	//ClassDB::bind_method(D_METHOD("get_thickness"), &DynamicTube3D::get_thickness);
	//ClassDB::bind_method(D_METHOD("set_tube_sides", "sides"), &DynamicTube3D::set_tube_sides);
	//ClassDB::bind_method(D_METHOD("get_tube_sides"), &DynamicTube3D::get_tube_sides);
	//ClassDB::bind_method(D_METHOD("set_lifetime", "lifetime"), &DynamicTube3D::set_lifetime);
	//ClassDB::bind_method(D_METHOD("get_lifetime"), &DynamicTube3D::get_lifetime);

	//ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Infinite,Timed"), "set_mode", "get_mode");
	//ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "thickness"), "set_thickness", "get_thickness");
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "tube_sides"), "set_tube_sides", "get_tube_sides");
	//ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lifetime"), "set_lifetime", "get_lifetime");
}

DynamicTube3D::DynamicTube3D()
{
	array_mesh.instantiate();
	material.instantiate();
	material->set_cull_mode(BaseMaterial3D::CULL_BACK);
	//material->set_albedo(Color(0.2f, 0.6f, 1.0f));
	material->set_albedo(Color(1.0f, 0.0f, 0.0f));
}

DynamicTube3D::~DynamicTube3D() {}

void DynamicTube3D::_ready()
{
	set_mesh(array_mesh);
	set_material_override(material);
}

void DynamicTube3D::_process(double delta)
{
	time_accumulator += delta;

	// Se siamo in modalità TIMED, rimuoviamo i punti troppo vecchi
	if (mode == MODE_TIMED && points.size() > 0)
	{
		int points_to_remove = 0;
		for (int i = 0; i < points.size(); ++i)
		{
			if (time_accumulator - points[i].time_created > lifetime)
			{
				points_to_remove++;
			}
			else
			{
				break; // I punti sono ordinati per tempo, se questo è valido lo sono anche i successivi
			}
		}

		if (points_to_remove > 0)
		{
			// Rimuove i primi N punti obsoleti dal vettore
			//points.remove_at(0, points_to_remove);
			for (int i = 0; i < points_to_remove; ++i) {
				points.remove_at(i);
			}
		}
	}

	// Rigenera la geometria del tubo per questo frame
	rebuild_mesh();
}

void DynamicTube3D::add_segment_point(const Vector3& p_point)
{
	TrailPoint tp;
	tp.position = p_point;
	tp.time_created = time_accumulator;
	points.push_back(tp);
}

void DynamicTube3D::clear_segments()
{
	points.clear();
	array_mesh->clear_surfaces();
}

void DynamicTube3D::set_mode(TrailMode p_mode) { mode = p_mode; }
DynamicTube3D::TrailMode DynamicTube3D::get_mode() const { return mode; }
void DynamicTube3D::set_thickness(float p_thickness) { thickness = p_thickness; }
float DynamicTube3D::get_thickness() const { return thickness; }
void DynamicTube3D::set_tube_sides(int p_sides) { tube_sides = p_sides < 3 ? 3 : p_sides; }
int DynamicTube3D::get_tube_sides() const { return tube_sides; }
void DynamicTube3D::set_lifetime(float p_lifetime) { lifetime = p_lifetime; }
float DynamicTube3D::get_lifetime() const { return lifetime; }

void DynamicTube3D::rebuild_mesh()
{
	array_mesh->clear_surfaces();

	int total_points = int(points.size());
	if (total_points < 2) return;

	PackedVector3Array vertices;
	PackedVector3Array normals;
	PackedVector2Array uvs;
	PackedInt32Array indices;

	int verts_per_ring = tube_sides + 1;
	vertices.resize(total_points * verts_per_ring);
	normals.resize(total_points * verts_per_ring);
	uvs.resize(total_points * verts_per_ring);

	for (int i = 0; i < total_points; ++i)
	{
		Vector3 forward;
		if (i == 0)
		{
			forward = (points[1].position - points[0].position).normalized();
		}
		else if (i == total_points - 1)
		{
			forward = (points[i].position - points[i - 1].position).normalized();
		}
		else
		{
			Vector3 dir1 = (points[i].position - points[i - 1].position).normalized();
			Vector3 dir2 = (points[i + 1].position - points[i].position).normalized();
			forward = (dir1 + dir2).normalized();
		}

		Vector3 up = Vector3(0, 1, 0);
		if (Math::abs(forward.dot(up)) > 0.99f)
		{
			up = Vector3(1, 0, 0);	// forward and up are parallel, choose a different up vector to avoid gimbal lock
		}
		Vector3 right = forward.cross(up).normalized();
		up = right.cross(forward).normalized();				// recalculate up to ensure orthogonality with forward and right

		for (int side = 0; side <= tube_sides; ++side)
		{
			float angle = float(((float)side / (float)tube_sides) * godot::Math::TAU);
			float cos_a = godot::Math::cos(angle);
			float sin_a = godot::Math::sin(angle);

			Vector3 normal = (right * cos_a + up * sin_a).normalized();
			Vector3 vertex_pos = points[i].position + (normal * thickness);

			int vert_idx = i * verts_per_ring + side;
			vertices[vert_idx] = vertex_pos;
			normals[vert_idx] = normal;

			float u = (float)i / (float)(total_points - 1);
			float v = (float)side / (float)tube_sides;
			uvs[vert_idx] = Vector2(u, v);

			if (i > 0 && side < tube_sides)
			{
				int current_ring = i * verts_per_ring;
				int previous_ring = (i - 1) * verts_per_ring;

				int tl = previous_ring + side;
				int tr = previous_ring + side + 1;
				int bl = current_ring + side;
				int br = current_ring + side + 1;

				indices.push_back(tl);
				indices.push_back(bl);
				indices.push_back(tr);

				indices.push_back(tr);
				indices.push_back(bl);
				indices.push_back(br);
			}
		}
	}

	Array mesh_arrays;
	mesh_arrays.resize(Mesh::ARRAY_MAX);
	mesh_arrays[Mesh::ARRAY_VERTEX] = vertices;
	mesh_arrays[Mesh::ARRAY_NORMAL] = normals;
	mesh_arrays[Mesh::ARRAY_TEX_UV] = uvs;
	mesh_arrays[Mesh::ARRAY_INDEX] = indices;

	array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mesh_arrays);
}