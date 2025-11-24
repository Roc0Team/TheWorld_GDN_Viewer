//float gridStepInWU = m_viewer->Globals()->gridStepInWU();
//float numVerticesPerChuckSide = (float)m_viewer->Globals()->numVerticesPerChuckSide();
//item.second->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
//item.second->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, numVerticesPerChuckSide);
//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_INVERSE_TRANSFORM) + String(" t=") + String(t));
//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_NORMAL_BASIS) + String(" b=") + String(b));
//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GRID_STEP) + String(" grid_step=") + String(std::to_string(f).c_str()));
//currentMaterial->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_EDITMODE_SELECTED) + String(" quad_selected=") + String(std::to_string(sel).c_str()));

							// X-axis growing
							//end = mouseHit + Vector3Right * lineSize;
							//m_normalDrawer->drawLine(mouseHit, end);
							//m_normalDrawer->drawSphere(end, radius, GDN_TheWorld_Globals::g_color_red);

							// Y-axis growing
							//end = mouseHit + Vector3Up * lineSize;
							//m_normalDrawer->drawLine(mouseHit, end);
							//m_normalDrawer->drawSphere(end, radius, GDN_TheWorld_Globals::g_color_green);

							// Z-axis growing
							//end = mouseHit + Vector3Back * lineSize;
							//m_normalDrawer->drawLine(mouseHit, end);
							//m_normalDrawer->drawSphere(end, radius, GDN_TheWorld_Globals::g_color_blue);

				//TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData quadrantData;
				//quadrantData.terrainEdit = quadTree->getQuadrant()->getTerrainEdit();
				//quadrantData.meshId = quadTree->getQuadrant()->getMeshCacheBuffer().getMeshId();
				//quadrantData.minHeight = quadTree->getQuadrant()->getTerrainEdit()->minHeight;
				//quadrantData.maxHeight = quadTree->getQuadrant()->getTerrainEdit()->maxHeight;
				//TheWorld_Utils::MemoryBuffer& h16Buffer = quadTree->getQuadrant()->getFloat16HeightsBuffer();
				//quadrantData.heights16Buffer = &h16Buffer;
				//TheWorld_Utils::MemoryBuffer& h32Buffer = quadTree->getQuadrant()->getFloat32HeightsBuffer();
				//quadrantData.heights32Buffer = &h32Buffer;
				//TheWorld_Utils::MemoryBuffer& normals = quadTree->getQuadrant()->getNormalsBuffer();
				//quadrantData.normalsBuffer = &normals;
				//TheWorld_Utils::MemoryBuffer& splatmap = quadTree->getQuadrant()->getSplatmapBuffer();
				//quadrantData.splatmapBuffer = &splatmap;

			//srand((unsigned int)time(NULL));			// random seed
			//int random_i = (rand() % 100) + 1;			// random integer between 1 and 100
			//float random_f = (float)random_i / 100;		// random float from 0.01 and 1.0 (step 0.01)
			//random_f += 0.5f;							// random float from 0.51 and 1.5 (step 0.01)
			//numVerticesToAdjustInEachQuadrant = (size_t)ceilf(heightGapForEachQuandrantOnBorder * random_f);


//
//float gapToFlatten = data.heights32Buffer->at<float>(x, numVerticesToAdjustInEachQuadrant, numVerticesPerSize) - data.heights32Buffer->at<float>(x, 1, numVerticesPerSize);
//float heightStep = gapToFlatten / numVerticesToAdjustInEachQuadrant;
//float heightStepFactor = heightStep * MAX_STEP_MODIFIER_FACTOR;
//size_t idx = 0;
//float sumHeightStep = 0, sumModifiedHeightStep = 0;
//for (int z = (int)numVerticesToAdjustInEachQuadrant; z >= 1; z--)
//{
//	float modifiedHeightStep = heightStep + sin(float(idx) * radiantStepPerVertex) * heightStepFactor;
//	//float h = data.heights32Buffer->at<float>(x, z, numVerticesPerSize);
//	//float h1 = h;
//	//h -= heightStep;
//	//h1 -= modifiedHeightStep;
//	data.heights32Buffer->at<float>(x, z, numVerticesPerSize) = heightStep;
//	if (x == 1024)
//	{
//		sumHeightStep += heightStep;
//		sumModifiedHeightStep += modifiedHeightStep;
//		//PLOG_DEBUG << "z = " << z << ", heightStep = " << heightStep << ", modifiedHeightStep = " << modifiedHeightStep << ", h = " << h << ", h1 = " << h1;
//	}
//	TheWorld_Utils::FLOAT_32 f(data.heights32Buffer->at<float>(x, z, numVerticesPerSize));
//	uint16_t half = half_from_float(f.u32);
//	data.heights16Buffer->at<uint16_t>(x, z, numVerticesPerSize) = half;
//	idx++;
//}
//if (x == 1024)
//{
//	PLOG_DEBUG << "gapToFlatten = " << gapToFlatten << ",sumHeightStep = " << sumHeightStep << ", sumModifiedHeightStep" << sumModifiedHeightStep;
//}
//
//gapToFlatten = northData.heights32Buffer->at<float>(x, maxIndex - numVerticesToAdjustInEachQuadrant, numVerticesPerSize) - data.heights32Buffer->at<float>(x, maxIndex - 1, numVerticesPerSize);
//heightStep = gapToFlatten / numVerticesToAdjustInEachQuadrant;
//heightStepFactor = heightStep * MAX_STEP_MODIFIER_FACTOR;
//idx = 0;
//sumHeightStep = 0, sumModifiedHeightStep = 0;
//for (int z = (int)numVerticesToAdjustInEachQuadrant; z >= 1; z--)
//{
//	float modifiedHeightStep = heightStep + sin(float(idx) * radiantStepPerVertex) * heightStepFactor;
//	//float h = northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize);
//	//float h1 = h;
//	//h -= heightStep;
//	//h1 -= modifiedHeightStep;
//	northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize) -= heightStep;
//	if (x == 1024)
//	{
//		sumHeightStep += heightStep;
//		sumModifiedHeightStep += modifiedHeightStep;
//		//PLOG_DEBUG << "z = " << z << ", heightStep = " << heightStep << ", modifiedHeightStep = " << modifiedHeightStep << ", h = " << h << ", h1 = " << h1;
//	}
//	TheWorld_Utils::FLOAT_32 f(northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize));
//	uint16_t half = half_from_float(f.u32);
//	northData.heights16Buffer->at<uint16_t>(x, maxIndex - z, numVerticesPerSize) = half;
//	idx++;
//}
//if (x == 1024)
//{
//	PLOG_DEBUG << "gapToFlatten = " << gapToFlatten << ",sumHeightStep = " << sumHeightStep << ", sumModifiedHeightStep" << sumModifiedHeightStep;
//}

//#define	MIN_PERCENT					0.2f
//#define	MAX_PERCENT					0.5f
//#define	MAX_STEP_MODIFIER_FACTOR	100.0f	// crescendo incrementa la curvatura
//
//std::hash<std::string> hasher;
//size_t seed = hasher(data.meshId + northData.meshId);
//srand((unsigned int)seed);
//int random_i = (rand() % 100) + 1;			// random integer between 1 and 100
//float random_f = (((MAX_PERCENT - MIN_PERCENT) / 100) * random_i) + MIN_PERCENT;
//numVerticesToAdjustInEachQuadrant = size_t(numVerticesToAdjustInEachQuadrant * random_f);
//float radiantStepPerVertex = TheWorld_Utils::kPi / (numVerticesToAdjustInEachQuadrant);
//
//float gapToFlatten = data.heights32Buffer->at<float>(x, numVerticesToAdjustInEachQuadrant, numVerticesPerSize) - data.heights32Buffer->at<float>(x, 1, numVerticesPerSize);
//float heightStep = gapToFlatten / numVerticesToAdjustInEachQuadrant;
//float heightStepFactor = heightStep * MAX_STEP_MODIFIER_FACTOR;
//size_t idx = 0;
//for (int z = (int)numVerticesToAdjustInEachQuadrant - 1; z >= 0; z--)
//{
//	float modifiedHeightStep = heightStep + sin(float(idx) * radiantStepPerVertex) * heightStepFactor;
//	//float h1 = data.heights32Buffer->at<float>(x, z, numVerticesPerSize);
//	//float h = h1;
//	//h -= heightStep;
//	//h1 -= modifiedHeightStep;
//	data.heights32Buffer->at<float>(x, z, numVerticesPerSize) -= modifiedHeightStep;
//	//data.heights32Buffer->at<float>(x, z, numVerticesPerSize) = h1;
//	//if (x == 1024)
//	//{
//	//	PLOG_DEBUG << "z = " << z << ", heightStep = " << heightStep << ", modifiedHeightStep = " << modifiedHeightStep << ", h = " << h << ", h1 = " << h1;
//	//}
//	TheWorld_Utils::FLOAT_32 f(data.heights32Buffer->at<float>(x, z, numVerticesPerSize));
//	uint16_t half = half_from_float(f.u32);
//	data.heights16Buffer->at<uint16_t>(x, z, numVerticesPerSize) = half;
//	idx++;
//
//	if (z == 1)
//	{
//		float h1 = data.heights32Buffer->at<float>(x, z, numVerticesPerSize);
//		float h2 = northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize);
//		saveHeight = (h1 + h2) / 2;
//	}
//}
////if (x == 1024)
////{
////	PLOG_DEBUG << "gapToFlatten = " << gapToFlatten;
////}
//
//gapToFlatten = northData.heights32Buffer->at<float>(x, maxIndex - numVerticesToAdjustInEachQuadrant, numVerticesPerSize) - northData.heights32Buffer->at<float>(x, maxIndex - 1, numVerticesPerSize);
//heightStep = gapToFlatten / numVerticesToAdjustInEachQuadrant;
//heightStepFactor = heightStep * MAX_STEP_MODIFIER_FACTOR;
//idx = 0;
//for (int z = (int)numVerticesToAdjustInEachQuadrant - 1; z >= 0; z--)
//{
//	float modifiedHeightStep = heightStep + sin(float(idx) * radiantStepPerVertex) * heightStepFactor;
//	//float h1 = northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize);
//	//float h = h1;
//	//h -= heightStep;
//	//h1 -= modifiedHeightStep;
//	northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize) -= modifiedHeightStep;
//	//northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize) = h1;
//	//if (x == 1024)
//	//{
//	//	PLOG_DEBUG << "z = " << z << ", heightStep = " << heightStep << ", modifiedHeightStep = " << modifiedHeightStep << ", h = " << h << ", h1 = " << h1;
//	//}
//	TheWorld_Utils::FLOAT_32 f(northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize));
//	uint16_t half = half_from_float(f.u32);
//	northData.heights16Buffer->at<uint16_t>(x, maxIndex - z, numVerticesPerSize) = half;
//	idx++;
//}
////if (x == 1024)
////{
////	PLOG_DEBUG << "gapToFlatten = " << gapToFlatten;
////}

//std::hash<std::string> hasher;
//size_t seed = hasher(data.meshId + northData.meshId);
//srand((unsigned int)(seed));
//int random_i = (rand() % 100) + 1;			// random integer between 1 and 100
//float random_f = (((MAX_PERCENT - MIN_PERCENT) / 100) * random_i) + MIN_PERCENT;
//numVerticesToAdjustInEachQuadrant = size_t(ceilf(numVerticesToAdjustInEachQuadrant * random_f));
//float radiantStepPerVertex = TheWorld_Utils::kPi / (numVerticesToAdjustInEachQuadrant - 1);
//
//float gapToFlatten1 = data.heights32Buffer->at<float>(x, numVerticesToAdjustInEachQuadrant, numVerticesPerSize) - data.heights32Buffer->at<float>(x, 1, numVerticesPerSize);
//float heightStep1 = gapToFlatten1 / numVerticesToAdjustInEachQuadrant;
//float heightStepFactor1 = heightStep1 * MAX_STEP_MODIFIER_FACTOR;
//float gapToFlatten2 = northData.heights32Buffer->at<float>(x, maxIndex - numVerticesToAdjustInEachQuadrant, numVerticesPerSize) - northData.heights32Buffer->at<float>(x, maxIndex - 1, numVerticesPerSize);
//float heightStep2 = gapToFlatten2 / numVerticesToAdjustInEachQuadrant;
//float heightStepFactor2 = heightStep2 * MAX_STEP_MODIFIER_FACTOR;
//size_t idx = 0;
//for (int z = (int)numVerticesToAdjustInEachQuadrant; z >= 1; z--)
//{
//	float modifiedHeightStep1 = heightStep1 + sin(float(idx) * radiantStepPerVertex) * heightStepFactor1;
//	data.heights32Buffer->at<float>(x, z, numVerticesPerSize) -= modifiedHeightStep1;
//	TheWorld_Utils::FLOAT_32 f1(data.heights32Buffer->at<float>(x, z, numVerticesPerSize));
//	uint16_t half1 = half_from_float(f1.u32);
//	data.heights16Buffer->at<uint16_t>(x, z, numVerticesPerSize) = half1;
//
//	float modifiedHeightStep2 = heightStep2 + sin(float(idx) * radiantStepPerVertex) * heightStepFactor2;
//	northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize) -= modifiedHeightStep2;
//	TheWorld_Utils::FLOAT_32 f2(northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize));
//	uint16_t half2 = half_from_float(f2.u32);
//	northData.heights16Buffer->at<uint16_t>(x, maxIndex - z, numVerticesPerSize) = half2;
//
//	idx++;
//}

//void ShaderTerrainData::loadGroundTextures(godot::String dirName, GDN_TheWorld_Viewer* viewer)
//{
//	TheWorld_Utils::GuardProfiler profiler(std::string("getGroundTextures 1 ") + __FUNCTION__, "all");
//
//	TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
//
//	if (viewer != nullptr)
//		viewer->Globals()->debugPrint("Reading ground textures");
//
//	godot::String _dirName = dirName;
//	if (_dirName.length() == 0)
//	{
//		_dirName = ShaderTerrainData::getGroundTexturesDir().c_str();
//	}
//
//	godot::String globalizedDirName = ProjectSettings::get_singleton()->globalize_path(_dirName);
//
//	std::map<std::string, bool> textures;
//
//	std::string scanningDir = std::string(globalizedDirName.utf8().get_data());
//	if (!fs::exists(scanningDir))
//		return;
//
//	for (const auto& entry : fs::directory_iterator(scanningDir))
//		if (entry.is_directory())
//		{
//			std::string dirname = entry.path().filename().generic_string();
//			if (not dirname.starts_with("_"))
//				textures[dirname] = true;
//		}
//
//	godot::Ref<godot::Texture2DArray> albedoTexture2DArray = memnew(godot::Texture2DArray);
//	godot::Ref<godot::Texture2DArray> normalTexture2DArray = memnew(godot::Texture2DArray);
//	godot::Ref<godot::Texture2DArray> roughnessTexture2DArray = memnew(godot::Texture2DArray);
//	godot::Ref<godot::Texture2DArray> displacementTexture2DArray = memnew(godot::Texture2DArray);
//	godot::Ref<godot::Texture2DArray> ambientOcclusionTexture2DArray = memnew(godot::Texture2DArray);
//	godot::TypedArray<godot::Image> albedoImageArray, normalImageArray, roughnessImageArray, displacementImageArray/*, ambientOcclusionImageArray*/;
//
//	std::map<std::string, int32_t>& groundTextureLayers = ShaderTerrainData::getGroundTextureLayers();
//	groundTextureLayers.clear();
//
//	const int desiredImageSize = 1024;
//	const int texturePreviewDesiredImageSize = 512;
//	const godot::Image::Format desiredImageFormat = godot::Image::Format::FORMAT_RGB8;
//
//	int layer = 0;
//	for (const auto& texture : textures)
//	{
//		std::string textureDir = scanningDir + texture.first;
//		//std::string albedoDisplTextureFilePath, normalRoughnessTextureFilePath;
//		std::string albedoTextureFilePath, normalTextureFilePath, roughnessTextureFilePath, displacementTextureFilePath, ambientOcclusionFilePath, texturePreviewFilePath;
//
//		for (const auto& entry : fs::directory_iterator(textureDir))
//		{
//			if (entry.is_regular_file())
//			{
//				std::string filename = entry.path().filename().generic_string();
//
//				if (GDN_TheWorld_Globals::isImageExt(filename))
//				{
//					if (GDN_TheWorld_Globals::isAlbedoTexture(filename))
//					{
//						albedoTextureFilePath = textureDir + "/" + filename;
//					}
//					else if (GDN_TheWorld_Globals::isNormalTexture(filename))
//					{
//						normalTextureFilePath = textureDir + "/" + filename;
//					}
//					else if (GDN_TheWorld_Globals::isRoughnessTexture(filename))
//					{
//						roughnessTextureFilePath = textureDir + "/" + filename;
//					}
//					else if (GDN_TheWorld_Globals::isDisplacementTexture(filename))
//					{
//						displacementTextureFilePath = textureDir + "/" + filename;
//					}
//					else if (GDN_TheWorld_Globals::isAmbientOcclusionTexture(filename))
//					{
//						ambientOcclusionFilePath = textureDir + "/" + filename;
//					}
//					else if (GDN_TheWorld_Globals::isTexturePreview(filename))
//					{
//						texturePreviewFilePath = textureDir + "/" + filename;
//					}
//				}
//			}
//		}
//
//		if (albedoTextureFilePath.size() > 0)
//		{
//			TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
//
//			s_groundTextures[texture.first] = std::make_unique<ShaderTerrainData::GroundTexture>();
//			ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[texture.first].get();
//
//			groundTexture->m_albedo_tex = loadGroundTexture(albedoTextureFilePath, "ALBEDO", texture.first, desiredImageSize, desiredImageFormat, false, viewer);
//			my_assert(groundTexture->m_albedo_tex != nullptr);
//			albedoImageArray.push_back(groundTexture->m_albedo_tex->get_image());
//
//			groundTexture->m_normal_tex = loadGroundTexture(normalTextureFilePath, "NORMAL", texture.first, desiredImageSize, desiredImageFormat, true, viewer);
//			my_assert(groundTexture->m_normal_tex != nullptr);
//			normalImageArray.push_back(groundTexture->m_normal_tex->get_image());
//
//			groundTexture->m_roughness_tex = loadGroundTexture(roughnessTextureFilePath, "ROUGHNESS", texture.first, desiredImageSize, desiredImageFormat, true, viewer);
//			my_assert(groundTexture->m_roughness_tex != nullptr);
//			roughnessImageArray.push_back(groundTexture->m_roughness_tex->get_image());
//
//			groundTexture->m_displacement_tex = loadGroundTexture(displacementTextureFilePath, "DISPLACEMENT", texture.first, desiredImageSize, desiredImageFormat, true, viewer);
//			my_assert(groundTexture->m_displacement_tex != nullptr);
//			displacementImageArray.push_back(groundTexture->m_displacement_tex->get_image());
//
//			//groundTexture->m_ambient_occlusion_tex = loadGroundTexture(ambientOcclusionFilePath, "AMBIENT OCCLUSION", texture.first, desiredImageSize, desiredImageFormat, true, viewer);
//			//my_assert(groundTexture->m_ambient_occlusion_tex != nullptr);
//			//ambientOcclusionImageArray.push_back(groundTexture->m_ambient_occlusion_tex->get_image());
//
//			if (texturePreviewFilePath.size() > 0)
//			{
//				groundTexture->m_texture_preview = loadGroundTexture(texturePreviewFilePath, "TEXTURE PREVIEW", texture.first, texturePreviewDesiredImageSize, desiredImageFormat, false, viewer);
//				my_assert(groundTexture->m_texture_preview != nullptr);
//			}
//			else
//			{
//				godot::Ref<godot::Image> image = godot::Image::create_from_data(texturePreviewDesiredImageSize, texturePreviewDesiredImageSize, false, groundTexture->m_albedo_tex->get_image()->get_format(), groundTexture->m_albedo_tex->get_image()->get_data());
//				/*if (ambientOcclusionFilePath.size() > 0)
//				{
//					if (not groundTexture->m_ambient_occlusion_tex.is_valid())
//					{
//						groundTexture->m_ambient_occlusion_tex = loadGroundTexture(ambientOcclusionFilePath, "AMBIENT OCCLUSION", texture.first, texturePreviewDesiredImageSize, desiredImageFormat, true, viewer);
//						my_assert(groundTexture->m_ambient_occlusion_tex != nullptr);
//					}
//
//					godot::Ref<godot::Image> aoImage = groundTexture->m_ambient_occlusion_tex->get_image();
//					godot::Rect2i sourceBlendingRect(0, 0, image->get_width(), image->get_height());
//					godot::Vector2i destBlendingRect(0, 0);
//
//					//aoImage->blend_rect(image, sourceBlendingRect, destBlendingRect);		// Alpha-blending is not what I want
//					//image = aoImage;
//					for(int y = 0; y < image->get_height(); y++)
//						for (int x = 0; x < image->get_width(); x++)
//						{
//							godot::Color color = image->get_pixel(x, y);
//							godot::Color aoImageColor = aoImage->get_pixel(x, y);
//							godot::Color mixedColor = color * aoImageColor;
//							image->set_pixel(x, y, mixedColor);
//						}
//				}*/
//
//				texturePreviewFilePath = textureDir + "/" + texture.first + "_texture_preview" + ".png";
//				//if (not godot::Engine::get_singleton()->is_editor_hint())
//				image->save_png(texturePreviewFilePath.c_str());
//				groundTexture->m_texture_preview = godot::ImageTexture::create_from_image(image);
//			}
//
//			groundTextureLayers[texture.first] = layer;
//
//			TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
//			long long elapsed = (now - start).count();
//			if (viewer != nullptr)
//				viewer->Globals()->debugPrint((std::string("Loaded ground_texture [") + texture.first + "] ==> layer = " + std::to_string(layer) + " - Elapsed = " + std::to_string(elapsed) + " ms").c_str());
//
//			layer++;
//		}
//		else
//		{
//			if (viewer != nullptr)
//			{
//				std::string msg = std::string("Ground texture ") + texture.first + " discarded";
//				viewer->Globals()->debugPrint(msg.c_str());
//			}
//
//		}
//	}
//
//	my_assert(albedoImageArray.size() == normalImageArray.size());
//	my_assert(albedoImageArray.size() == roughnessImageArray.size());
//	my_assert(albedoImageArray.size() == displacementImageArray.size());
//	//my_assert(albedoImageArray.size() == ambientOcclusionImageArray.size());
//
//	godot::Error e = albedoTexture2DArray->create_from_images(albedoImageArray);
//	my_assert(e == godot::Error::OK);
//	godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_GROUND_ALBEDO_ARRAY, albedoTexture2DArray);
//	e = normalTexture2DArray->create_from_images(normalImageArray);
//	my_assert(e == godot::Error::OK);
//	godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_GROUND_NORMAL_ARRAY, normalTexture2DArray);
//	e = roughnessTexture2DArray->create_from_images(roughnessImageArray);
//	my_assert(e == godot::Error::OK);
//	godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_GROUND_ROUGHNESS_ARRAY, roughnessTexture2DArray);
//	e = displacementTexture2DArray->create_from_images(displacementImageArray);
//	my_assert(e == godot::Error::OK);
//	godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_GROUND_DISPL_ARRAY, displacementTexture2DArray);
//	//e = ambientOcclusionTexture2DArray->create_from_images(ambientOcclusionImageArray);
//	//my_assert(e == godot::Error::OK);
//	//godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_GROUND_AO_ARRAY, ambientOcclusionTexture2DArray);
//
//	// Free PBR Textures because they are already sent to the global shader variables
//	for (auto& groundTexture : s_groundTextures)
//	{
//		if (groundTexture.second->m_albedo_tex.is_valid())
//			groundTexture.second->m_albedo_tex.unref();
//		if (groundTexture.second->m_normal_tex.is_valid())
//			groundTexture.second->m_normal_tex.unref();
//		if (groundTexture.second->m_roughness_tex.is_valid())
//			groundTexture.second->m_roughness_tex.unref();
//		if (groundTexture.second->m_ambient_occlusion_tex.is_valid())
//			groundTexture.second->m_ambient_occlusion_tex.unref();
//		if (groundTexture.second->m_displacement_tex.is_valid())
//			groundTexture.second->m_displacement_tex.unref();
//	}
//
//	TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
//	long long elapsed = (now - start).count();
//
//	if (viewer != nullptr)
//		viewer->Globals()->debugPrint((std::string("Reading ground textures done! - Elapsed ") + std::to_string(elapsed) + " ms").c_str());
//
//	//String materialPath = "res://addons/twviewer/shaders/m_sea.tres";
//	//s_seaMat = godot::ResourceLoader::get_singleton()->load(materialPath);
//}


//void GDN_TheWorld_Edit::editModeGenNormalsAction(void)
//{
//	if (m_actionInProgress)
//		return;
//
//	m_actionInProgress = true;
//	m_allItems = 0;
//	setElapsed(0, true);
//	setNote1(0);
//
//	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeGenNormals, this);
//	m_tp.QueueJob(f);
//}

//void GDN_TheWorld_Edit::editModeGenNormals(void)
//{
//	editModeGenNormals_1(true, false);
//}
//
//void GDN_TheWorld_Edit::editModeGenNormals_1(bool forceGenSelectedQuad, bool evaluateSelectedQuadOnly)
//{
//	// TO BE FIXED: IT IS IN CONFLICT WITH QUADRANT MODDER
//	
//	if (m_actionClock.counterStarted())
//	{
//		m_actionInProgress = false;
//		return;
//	}
//
//	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1 ") + __FUNCTION__, "ALL");
//
//	setMessage(godot::String("Generating normals..."));
//	
//	m_actionClock.tick();
//
//	QuadTree* quadTreeSel = nullptr;
//	TheWorld_Utils::QuadrantPos quadrantSelPos = m_viewer->getWorldQuadrantSelForEdit(&quadTreeSel);
//
//	bool allCheched = m_innerData->m_allCheckBox->is_pressed();
//
//	if (!allCheched && quadrantSelPos.empty())
//	{
//		m_actionClock.tock();
//		m_actionInProgress = false;
//		setMessage(godot::String("Completed!"), true);
//		return;
//	}
//
//	std::vector<TheWorld_Utils::QuadrantPos> allQuadrantPos;
//	m_viewer->getAllQuadrantPos(allQuadrantPos);
//
//	std::vector<TheWorld_Utils::QuadrantPos> quadrantPosToRegen;
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.1 ") + __FUNCTION__, "Select quads");
//
//		if (evaluateSelectedQuadOnly)
//		{
//			if (quadTreeSel != nullptr)
//			{
//				if (forceGenSelectedQuad)
//				{
//					quadTreeSel->getQuadrant()->getTerrainEdit()->normalsNeedRegen = true;
//					quadTreeSel->getQuadrant()->setNeedUploadToServer(true);
//					quadrantPosToRegen.push_back(quadrantSelPos);
//				}
//				else
//				{
//					TheWorld_Utils::MemoryBuffer& normalsBuffer = quadTreeSel->getQuadrant()->getNormalsBuffer(true);
//					if (normalsBuffer.size() == 0 || quadTreeSel->getQuadrant()->getTerrainEdit()->normalsNeedRegen)
//					{
//						quadTreeSel->getQuadrant()->setNeedUploadToServer(true);
//						quadrantPosToRegen.push_back(quadrantSelPos);
//					}
//				}
//			}
//
//		}
//		else
//		{
//			//if (quadTreeSel != nullptr && forceGenSelectedQuad)
//			//	quadTreeSel->getQuadrant()->getTerrainEdit()->normalsNeedRegen = true;
//
//			int num = 0;
//			for (auto& pos : allQuadrantPos)
//			{
//				num++;
//				QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
//				if (quadTree != nullptr && !quadTree->getQuadrant()->empty())
//				{
//					//quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen = true;		// SUPERDEBUGRIC: force to generate normals
//					TheWorld_Utils::MemoryBuffer* pn = nullptr;
//					{
//						TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.1.1 ") + __FUNCTION__, "getNormalsBuffer quad");
//						TheWorld_Utils::MemoryBuffer& n = quadTree->getQuadrant()->getNormalsBuffer(true);
//						pn = &n;
//					}
//					TheWorld_Utils::MemoryBuffer& normalsBuffer = *pn;
//					if (normalsBuffer.size() == 0 || quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen)
//					{
//						if (allCheched || pos == quadrantSelPos)
//						{
//							quadTree->getQuadrant()->setNeedUploadToServer(true);
//							quadrantPosToRegen.push_back(pos);
//
//							if (!allCheched)
//								break;
//						}
//					}
//				}
//			}
//		}
//	}
//
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.2 ") + __FUNCTION__, "Loop quads");
//
//		m_completedItems = 0;
//		m_elapsedCompleted = 0;
//		m_lastElapsed = 0;
//		m_allItems = quadrantPosToRegen.size();
//		for (auto& pos : quadrantPosToRegen)
//		{
//			if (m_actionStopRequested)
//			{
//				m_actionStopRequested = false;
//				break;
//			}
//
//			QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
//			if (quadTree != nullptr)
//			{
//				bool lockAcquired = quadTree->getQuadrant()->lockInternalData();
//				if (lockAcquired)
//				{
//					editModeGenNormals_2(quadTree);
//					quadTree->getQuadrant()->unlockInternalData();
//				}
//
//				m_completedItems++;
//				size_t partialCount = m_actionClock.partialDuration().count();
//				m_lastElapsed = partialCount - m_elapsedCompleted;
//				m_elapsedCompleted = partialCount;
//			}
//		}
//	}
//
//	m_actionClock.tock();
//
//	m_actionInProgress = false;
//
//	size_t duration = m_actionClock.duration().count();
//	setElapsed(duration, false);
//	setCounter(m_completedItems, m_allItems);
//	setNote1(m_lastElapsed);
//
//	//size_t numToSave = 0;
//	//size_t numToUpload = 0;
//	//size_t numLocked = 0;
//	//refreshNumToSaveUpload(numToSave, numToUpload, numLocked);
//
//	setMessage(godot::String("Completed!"), true);
//}

//void GDN_TheWorld_Edit::editModeGenNormals_2(QuadTree* quadTree)
//{
//	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.2.1 ") + __FUNCTION__, "getNormalsBuffer single quad");
//
//	TheWorld_Utils::MemoryBuffer* pn = nullptr;
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.2.1.1 ") + __FUNCTION__, "getNormalsBuffer quad");
//		TheWorld_Utils::MemoryBuffer& n = quadTree->getQuadrant()->getNormalsBuffer(true);
//		pn = &n;
//	}
//	TheWorld_Utils::MemoryBuffer& normalsBuffer = *pn;
//	if (normalsBuffer.size() == 0 || quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen)
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.2.1.2 ") + __FUNCTION__, "Single QuadTree");
//
//		TheWorld_Utils::QuadrantPos pos = quadTree->getQuadrant()->getPos();
//		TheWorld_Utils::MemoryBuffer& float32HeigthsBuffer = quadTree->getQuadrant()->getFloat32HeightsBuffer(true);
//
//		{
//			TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.2.1.2.1 ") + __FUNCTION__, "Single QuadTree - GPU");
//			generateNormals(quadTree, &float32HeigthsBuffer, &normalsBuffer);
//		}
//
//		//{
//		//	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.2.2.1 ") + __FUNCTION__, "Single QuadTree - CPU");
//		//	cache.generateNormalsForBlendedQuadrants(pos.getNumVerticesPerSize(), pos.getGridStepInWU(), float32HeigthsBuffer, *eastHeights32Buffer, *southHeights32Buffer, normalsBuffer, m_requestedThreadExit);
//		//	
//		//	if (m_requestedThreadExit)
//		//		return;
//		//}
//
//		//std::vector<float> vectGridHeights;
//		//heightsBuffer.populateFloatVector(vectGridHeights);
//		//cache.deprecated_generateNormals(pos.getNumVerticesPerSize(), pos.getGridStepInWU(), vectGridHeights, normalsBuffer);
//
//		{
//			std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
//			//m_mapQuadToSave[pos] = "";
//			bool manual = true;
//			addQuadToSave(pos, manual);
//		}
//		quadTree->getQuadrant()->getTerrainEdit()->needUploadToServer = true;
//		quadTree->getQuadrant()->setNeedUploadToServer(true);
//
//		quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen = false;
//		quadTree->getQuadrant()->setNormalsUpdated(true);
//
//		quadTree->getQuadrant()->resetSplatmapBuffer(__FUNCTION__);
//
//		quadTree->getQuadrant()->resetGlobalmapBuffer(__FUNCTION__);
//
//		quadTree->materialParamsNeedReset(true);
//	}
//}

//void GDN_TheWorld_Edit::generateGlobalmap(QuadTree* quadtree, TheWorld_Utils::MemoryBuffer* globalmapBuffer)
//{
//	my_assert(m_mapModder != nullptr);
//
//	size_t numHeights = (size_t)sqrt(float(quadtree->getQuadrant()->getFloat32HeightsBuffer().size() / sizeof(float)));
//	my_assert(size_t(pow((float)numHeights, 2) * sizeof(float)) == quadtree->getQuadrant()->getFloat32HeightsBuffer().size());
//
//	while (!m_mapModder->ready())
//		Sleep(5);
//
//	m_mapModder->initForGlobalMap(quadtree);
//	while (m_mapModder->working())
//		Sleep(5);
//	TheWorld_Utils::MemoryBuffer* cookedGlobalmap = m_mapModder->getOutBuffer();
//
//	my_assert(size_t(pow((float)numHeights, 2) * sizeof(TheWorld_Utils::_RGB)) == cookedGlobalmap->size());
//	globalmapBuffer->reserve(cookedGlobalmap->size());
//	memcpy(globalmapBuffer->ptr(), cookedGlobalmap->ptr(), cookedGlobalmap->size());
//	globalmapBuffer->adjustSize(cookedGlobalmap->size());
//	m_mapModder->deinitRequired(true);
//}

//void GDN_TheWorld_Edit::generateNormals(QuadTree* quadtree, TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* normalsBuffer)
//{
//	// distance == 0 ==> all the quadrant (distance is ignored atm considering "all the quadrant" by design)
//	my_assert(m_mapModder != nullptr);
//
//	while (!m_mapModder->ready())
//		Sleep(5);
//	
//	size_t numHeights = (size_t)sqrt(float(float32HeigthsBuffer->size() / sizeof(float)));
//	my_assert(size_t(pow((float)numHeights, 2) * sizeof(float)) == float32HeigthsBuffer->size());
//
//	m_mapModder->initForNormals(quadtree, float32HeigthsBuffer, 1024, 1);		// atm border = 0 will be the same
//	while (m_mapModder->working())
//		Sleep(5);
//	TheWorld_Utils::MemoryBuffer* cookedNormals = m_mapModder->getOutBuffer();
//	
//	my_assert(normalsBuffer->size() == 0 || cookedNormals->size() == normalsBuffer->size());
//	size_t numNormals = (size_t)sqrt(float(cookedNormals->size() / sizeof(TheWorld_Utils::_RGB)));
//	my_assert(size_t(pow((float)numNormals, 2) * sizeof(TheWorld_Utils::_RGB)) == cookedNormals->size());
//	my_assert(numHeights == numNormals);
//	normalsBuffer->reserve(cookedNormals->size());
//	memcpy(normalsBuffer->ptr(), cookedNormals->ptr(), cookedNormals->size());
//	normalsBuffer->adjustSize(cookedNormals->size());
//	m_mapModder->deinitRequired(true);
//}

//void GDN_TheWorld_Edit::generateSplatmapForBlendedQuadrants(QuadTree* quadtree, size_t numVerticesPerSize, float gridStepInWU,
//	float x, float z, float distance,
//	TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* normalsBuffer, TheWorld_Utils::MemoryBuffer* splatmapBuffer, TheWorld_Utils::TerrainEdit* terrainEdit,
//	TheWorld_Utils::MemoryBuffer* west_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* west_normalsBuffer, TheWorld_Utils::MemoryBuffer* west_splatmapBuffer, TheWorld_Utils::TerrainEdit* west_terrainEdit,
//	TheWorld_Utils::MemoryBuffer* east_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* east_normalsBuffer, TheWorld_Utils::MemoryBuffer* east_splatmapBuffer, TheWorld_Utils::TerrainEdit* east_terrainEdit,
//	TheWorld_Utils::MemoryBuffer* north_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* north_normalsBuffer, TheWorld_Utils::MemoryBuffer* north_splatmapBuffer, TheWorld_Utils::TerrainEdit* north_terrainEdit,
//	TheWorld_Utils::MemoryBuffer* south_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* south_normalsBuffer, TheWorld_Utils::MemoryBuffer* south_splatmapBuffer, TheWorld_Utils::TerrainEdit* south_terrainEdit,
//	float slopeVerticalFactor,	// raising it slope will have higher maximum and so rocks will be more diffuse on slopes
//	float slopeFlatFactor,		// raising it slope will have lesser mininum and maximum so rocks will be less diffuse on slopes
//	float dirtOnRocksFactor,	// raising it dirt will be more diffuse on rocks
//	float highElevationFactor,	// raising it high elevation amount will be greater at lower altitude
//	float lowElevationFactor,	// raising it low elevation amount will be greater at higher altitude
//	size_t splatMapMode)
//{
//	// distance == 0 ==> all the quadrant (distance is ignored atm considering "all the quadrant" by design)
//	my_assert(m_mapModder != nullptr);
//
//	while (!m_mapModder->ready())
//		Sleep(5);
//
//	size_t numHeights = (size_t)sqrt(float(float32HeigthsBuffer->size() / sizeof(float)));
//	my_assert(size_t(pow((float)numHeights, 2) * sizeof(float)) == float32HeigthsBuffer->size());
//	size_t numNormals = (size_t)sqrt(float(normalsBuffer->size() / sizeof(TheWorld_Utils::_RGB)));
//	my_assert(size_t(pow((float)numNormals, 2) * sizeof(TheWorld_Utils::_RGB)) == normalsBuffer->size());
//	my_assert(numNormals == 0 || numHeights == numNormals);
//
//	m_mapModder->initForSplatmap(quadtree, float32HeigthsBuffer, normalsBuffer, slopeVerticalFactor, slopeFlatFactor,	dirtOnRocksFactor, highElevationFactor,	lowElevationFactor, terrainEdit->minHeight, terrainEdit->maxHeight, 1024);
//	while (m_mapModder->working())
//		Sleep(5);
//	TheWorld_Utils::MemoryBuffer* cookedSplatmap = m_mapModder->getOutBuffer();
//
//	my_assert(splatmapBuffer->size() == 0 || cookedSplatmap->size() == splatmapBuffer->size());
//	size_t numPixels = (size_t)sqrt(float(cookedSplatmap->size() / sizeof(TheWorld_Utils::_RGBA)));
//	my_assert(size_t(pow((float)numPixels, 2) * sizeof(TheWorld_Utils::_RGBA)) == cookedSplatmap->size());
//	my_assert(numHeights == numPixels);
//	splatmapBuffer->reserve(cookedSplatmap->size());
//	memcpy(splatmapBuffer->ptr(), cookedSplatmap->ptr(), cookedSplatmap->size());
//	splatmapBuffer->adjustSize(cookedSplatmap->size());
//	m_mapModder->deinitRequired(true);
//}

//void GDN_TheWorld_Edit::editModeApplyTexturesAction(void)
//{
//	if (m_actionInProgress)
//		return;
//
//	m_actionInProgress = true;
//	m_allItems = 0;
//	setElapsed(0, true);
//	setNote1(0);
//
//	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeApplyTextures, this);
//	m_tp.QueueJob(f);
//}

//void GDN_TheWorld_Edit::editModeApplyTextures(void)
//{
//	editModeApplyTextures_1(true, false);
//}
//
//void GDN_TheWorld_Edit::editModeApplyTextures_1(bool forceApplySelectedQuad, bool evaluateSelectedQuadOnly)
//{
//	// TO BE FIXED: IT IS IN CONFLICT WITH QUADRANT MODDER
//
//	if (m_actionClock.counterStarted())
//	{
//		m_actionInProgress = false;
//		return;
//	}
//
//	m_actionInProgress = true;
//
//	TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1 ") + __FUNCTION__, "ALL");
//
//	setMessage(godot::String("Applying textures..."));
//
//	m_actionClock.tick();
//
//	QuadTree* quadTreeSel = nullptr;
//	TheWorld_Utils::QuadrantPos quadrantSelPos = m_viewer->getWorldQuadrantSelForEdit(&quadTreeSel);
//
//	bool allCheched = m_innerData->m_allCheckBox->is_pressed();
//
//	if (!allCheched && quadrantSelPos.empty())
//	{
//		m_actionClock.tock();
//		m_actionInProgress = false;
//		setMessage(godot::String("Completed!"), true);
//		return;
//	}
//
//	std::vector<TheWorld_Utils::QuadrantPos> allQuadrantPos;
//	m_viewer->getAllQuadrantPos(allQuadrantPos);
//
//	std::vector<TheWorld_Utils::QuadrantPos> quadrantPos;
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.1 ") + __FUNCTION__, "Select Quads");
//
//		if (evaluateSelectedQuadOnly)
//		{
//			if (quadTreeSel != nullptr)
//			{
//				if (forceApplySelectedQuad)
//				{
//					quadTreeSel->getQuadrant()->getTerrainEdit()->splatmapNeedRegen(true);
//					quadTreeSel->getQuadrant()->setNeedUploadToServer(true);
//					quadrantPos.push_back(quadrantSelPos);
//				}
//				else
//				{
//					TheWorld_Utils::MemoryBuffer& splatmapBuffer = quadTreeSel->getQuadrant()->getSplatmapBuffer(true);
//					if (splatmapBuffer.size() == 0 || quadTreeSel->getQuadrant()->getTerrainEdit()->splatmapNeedRegen())
//					{
//						quadTreeSel->getQuadrant()->setNeedUploadToServer(true);
//						quadrantPos.push_back(quadrantSelPos);
//					}
//				}
//			}
//
//		}
//		else
//		{
//			if (quadTreeSel != nullptr && forceApplySelectedQuad)
//				quadTreeSel->getQuadrant()->getTerrainEdit()->splatmapNeedRegen(true);
//
//			{
//				for (auto& pos : allQuadrantPos)
//				{
//					QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
//					if (quadTree != nullptr && !quadTree->getQuadrant()->empty())
//					{
//						//quadTree->getQuadrant()->getTerrainEdit()->extraValues.splatmapNeedRegen = true;		// SUPERDEBUGRIC: force to generate textures
//						TheWorld_Utils::MemoryBuffer* sp = nullptr;
//						{
//							TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.1.1 ") + __FUNCTION__, "getSplatmapBuffer quad");
//							TheWorld_Utils::MemoryBuffer& s = quadTree->getQuadrant()->getSplatmapBuffer(true);
//							sp = &s;
//						}
//						TheWorld_Utils::MemoryBuffer& splatmapBuffer = *sp;
//						if (splatmapBuffer.size() == 0 || quadTree->getQuadrant()->getTerrainEdit()->splatmapNeedRegen())
//						{
//							if (allCheched || pos == quadrantSelPos)
//							{
//								quadrantPos.push_back(pos);
//								quadTree->getQuadrant()->setNeedUploadToServer(true);
//
//								if (!allCheched)
//									break;
//							}
//						}
//					}
//				}
//			}
//		}
//	}
//
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.2 ") + __FUNCTION__, "Loop Quads");
//
//		m_completedItems = 0;
//		m_elapsedCompleted = 0;
//		m_lastElapsed = 0;
//		m_allItems = quadrantPos.size();
//		for (auto& pos : quadrantPos)
//		{
//			if (m_actionStopRequested)
//			{
//				m_actionStopRequested = false;
//				break;
//			}
//
//			QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
//			if (quadTree != nullptr)
//			{
//				bool lockAcquired = quadTree->getQuadrant()->lockInternalData();
//				if (lockAcquired)
//				{
//					editModeApplyTextures_2(quadTree);
//					quadTree->getQuadrant()->unlockInternalData();
//				}
//
//				m_completedItems++;
//				size_t partialCount = m_actionClock.partialDuration().count();
//				m_lastElapsed = partialCount - m_elapsedCompleted;
//				m_elapsedCompleted = partialCount;
//			}
//		}
//	}
//
//	m_actionClock.tock();
//
//	m_actionInProgress = false;
//
//	size_t duration = m_actionClock.duration().count();
//	setElapsed(duration, false);
//	setCounter(m_completedItems, m_allItems);
//	setNote1(m_lastElapsed);
//
//	//size_t numToSave = 0;
//	//size_t numToUpload = 0;
//	//size_t numLocked = 0;
//	//refreshNumToSaveUpload(numToSave, numToUpload, numLocked);
//
//	setMessage(godot::String("Completed!"), true);
//
//}

//void GDN_TheWorld_Edit::editModeApplyTextures_2(QuadTree* quadTree)
//{
//	TheWorld_Utils::MemoryBuffer* sp = nullptr;
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.2.1 ") + __FUNCTION__, "getSplatmapBuffer quad");
//		TheWorld_Utils::MemoryBuffer& s = quadTree->getQuadrant()->getSplatmapBuffer(true);
//		sp = &s;
//	}
//	TheWorld_Utils::MemoryBuffer& splatmapBuffer = *sp;
//	if (splatmapBuffer.size() == 0 || quadTree->getQuadrant()->getTerrainEdit()->splatmapNeedRegen())
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.2.2 ") + __FUNCTION__, "Single QuadTree");
//
//		editModeGenNormals_2(quadTree);
//
//		TheWorld_Utils::QuadrantPos pos = quadTree->getQuadrant()->getPos();
//
//		TheWorld_Utils::MeshCacheBuffer& cache = quadTree->getQuadrant()->getInternalMeshCacheBuffer();
//
//		TheWorld_Utils::MemoryBuffer& normalsBuffer = quadTree->getQuadrant()->getNormalsBuffer(true);
//		TheWorld_Utils::MemoryBuffer& float32HeigthsBuffer = quadTree->getQuadrant()->getFloat32HeightsBuffer();
//		TheWorld_Utils::MemoryBuffer& splatmapBuffer = quadTree->getQuadrant()->getSplatmapBuffer();
//		size_t numElements = float32HeigthsBuffer.size() / sizeof(float);
//		my_assert(numElements == pos.getNumVerticesPerSize() * pos.getNumVerticesPerSize());
//
//		//{
//		//	TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.2.2.1 ") + __FUNCTION__, "Single QuadTree - CPU");
//		//	cache.generateSplatmap(pos.getNumVerticesPerSize(), pos.getGridStepInWU(), quadTree->getQuadrant()->getTerrainEdit(), float32HeigthsBuffer, normalsBuffer, splatmapBuffer,
//		//		slopeVerticalFactor(),
//		//		slopeFlatFactor(),
//		//		dirtOnRocksFactor(),
//		//		highElevationFactor(),
//		//		lowElevationFactor(),
//		//		splatMapMode());
//		//}
//
//		{
//			quadTree->getQuadrant()->getTerrainEdit()->extraValues.slopeVerticalFactor = slopeVerticalFactor();
//			quadTree->getQuadrant()->getTerrainEdit()->extraValues.slopeFlatFactor = slopeFlatFactor();
//			quadTree->getQuadrant()->getTerrainEdit()->extraValues.dirtOnRocksFactor = dirtOnRocksFactor();
//			quadTree->getQuadrant()->getTerrainEdit()->extraValues.highElevationFactor = highElevationFactor();
//			quadTree->getQuadrant()->getTerrainEdit()->extraValues.lowElevationFactor = lowElevationFactor();
//			quadTree->getQuadrant()->getTerrainEdit()->extraValues.splatMapMode = splatMapMode();
//
//			TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.2.2.2 ") + __FUNCTION__, "Single QuadTree - GPU");
//			TheWorld_Utils::MemoryBuffer* westHeights32Buffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* eastHeights32Buffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* northHeights32Buffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* southHeights32Buffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* westNormalsBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* eastNormalsBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* northNormalsBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* southNormalsBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* westSplatmapBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* eastSplatmapBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* northSplatmapBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* southSplatmapBuffer = nullptr;
//			generateSplatmapForBlendedQuadrants(quadTree, pos.getNumVerticesPerSize(), pos.getGridStepInWU(), 0.0f, 0.0f, 0.0f,
//				&float32HeigthsBuffer, &normalsBuffer, &splatmapBuffer, quadTree->getQuadrant()->getTerrainEdit(),
//				westHeights32Buffer, westNormalsBuffer, westSplatmapBuffer, nullptr,
//				eastHeights32Buffer, eastNormalsBuffer, eastSplatmapBuffer, nullptr,
//				northHeights32Buffer, northNormalsBuffer, northSplatmapBuffer, nullptr,
//				southHeights32Buffer, southNormalsBuffer, southSplatmapBuffer, nullptr,
//				quadTree->getQuadrant()->getTerrainEdit()->extraValues.slopeVerticalFactor,
//				quadTree->getQuadrant()->getTerrainEdit()->extraValues.slopeFlatFactor,
//				quadTree->getQuadrant()->getTerrainEdit()->extraValues.dirtOnRocksFactor,
//				quadTree->getQuadrant()->getTerrainEdit()->extraValues.highElevationFactor,
//				quadTree->getQuadrant()->getTerrainEdit()->extraValues.lowElevationFactor,
//				quadTree->getQuadrant()->getTerrainEdit()->extraValues.splatMapMode);
//
//			quadTree->getQuadrant()->resetGlobalmapBuffer(__FUNCTION__);
//		}
//
//		{
//			std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
//			//m_mapQuadToSave[pos] = "";
//			bool manual = true;
//			addQuadToSave(pos, manual);
//		}
//		quadTree->getQuadrant()->getTerrainEdit()->needUploadToServer = true;
//		quadTree->getQuadrant()->setNeedUploadToServer(true);
//
//		quadTree->getQuadrant()->getTerrainEdit()->splatmapNeedRegen(false);
//		quadTree->getQuadrant()->setSplatmapUpdated(true);
//
//		{
//			bool lockAcquired = quadTree->getQuadrant()->lockInternalData();
//			if (lockAcquired)
//			{
//				TheWorld_Utils::MemoryBuffer& globalmap = quadTree->getQuadrant()->getGlobalmapBuffer();
//				generateGlobalmap(quadTree, &globalmap);
//				quadTree->getQuadrant()->unlockInternalData();
//				quadTree->getQuadrant()->getTerrainEdit()->globalmapNeedRegen = false;
//				quadTree->getQuadrant()->setGlobalmapUpdated(true);
//			}
//
//		}
//
//		quadTree->materialParamsNeedReset(true);
//
//		{
//			if (strlen(quadTree->getQuadrant()->getTerrainEdit()->extraValues.lowElevationTexName_r) == 0)
//				quadTree->getQuadrant()->getTerrainEdit()->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::lowElevation);
//			if (strlen(quadTree->getQuadrant()->getTerrainEdit()->extraValues.highElevationTexName_g) == 0)
//				quadTree->getQuadrant()->getTerrainEdit()->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::highElevation);
//			if (strlen(quadTree->getQuadrant()->getTerrainEdit()->extraValues.dirtTexName_b) == 0)
//				quadTree->getQuadrant()->getTerrainEdit()->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::dirt);
//			if (strlen(quadTree->getQuadrant()->getTerrainEdit()->extraValues.rocksTexName_a) == 0)
//				quadTree->getQuadrant()->getTerrainEdit()->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::rocks);
//		}
//	}
//}

		//__declspec(dllexport) static void generateGroundImage(std::string outdir, std::string groundTypeName, size_t imageSize, bool flipY, MemoryBuffer& colorImage, MemoryBuffer& bumpImage, MemoryBuffer& normalImage, MemoryBuffer& roughImage);
		//__declspec(dllexport) static void generateGroundImage(MemoryBuffer& albedoBumpImage, MemoryBuffer& normalRoughnessImage, std::string groundTypeName, size_t imageSize, bool flipY, MemoryBuffer& colorImage, MemoryBuffer& bumpImage, MemoryBuffer& normalImage, MemoryBuffer& roughImage);

	//void TerrainEdit::generateGroundImage(MemoryBuffer& albedoBumpImage, MemoryBuffer& normalRoughnessImage, std::string groundTypeName, size_t imageSize, bool flipY, MemoryBuffer& colorImage, MemoryBuffer& bumpImage, MemoryBuffer& normalImage, MemoryBuffer& roughImage)
	//{
	//	size_t width, heigth;
	//	double d;

	//	// Color image formal controls
	//	d = sqrt(colorImage.size() / sizeof(struct TheWorld_Utils::_RGB));
	//	if (colorImage.size() == 0)
	//		throw(std::exception((std::string(__FUNCTION__) + std::string(" Color image empty")).c_str()));
	//	if (d * d * sizeof(struct TheWorld_Utils::_RGB) != colorImage.size())
	//		throw(std::exception((std::string(__FUNCTION__) + std::string(" Color image length not valid")).c_str()));
	//	width = heigth = (size_t)d;

	//	// Bump/Displ/Depth image formal controls
	//	if (bumpImage.size() > 0)
	//	{
	//		d = sqrt(bumpImage.size());
	//		if (d * d != bumpImage.size())
	//			throw(std::exception((std::string(__FUNCTION__) + std::string(" Normal image length not valid")).c_str()));
	//		if ((size_t)d != width)
	//			throw(std::exception((std::string(__FUNCTION__) + std::string(" Normal image differs from color image size")).c_str()));
	//	}

	//	// Normal image formal controls
	//	if (normalImage.size() > 0)
	//	{
	//		d = sqrt(normalImage.size() / sizeof(struct TheWorld_Utils::_RGB));
	//		if (d * d * sizeof(struct TheWorld_Utils::_RGB) != normalImage.size())
	//			throw(std::exception((std::string(__FUNCTION__) + std::string(" Normal image length not valid")).c_str()));
	//		if ((size_t)d != width)
	//			throw(std::exception((std::string(__FUNCTION__) + std::string(" Normal image differs from color image size")).c_str()));
	//	}

	//	// Rough image formal controls
	//	if (roughImage.size() > 0)
	//	{
	//		d = sqrt(roughImage.size());
	//		if (d * d != roughImage.size())
	//			throw(std::exception((std::string(__FUNCTION__) + std::string(" Rough image length not valid")).c_str()));
	//		if ((size_t)d != width)
	//			throw(std::exception((std::string(__FUNCTION__) + std::string(" Rough image differs from color image size")).c_str()));
	//	}

	//	size_t bufferSize = width * heigth * sizeof(struct TheWorld_Utils::_RGBA);

	//	albedoBumpImage.clear(__FUNCTION__);
	//	albedoBumpImage.reserve(bufferSize);
	//	albedoBumpImage.adjustSize(bufferSize);
	//	struct TheWorld_Utils::_RGBA* rgba = (struct TheWorld_Utils::_RGBA*)albedoBumpImage.ptr();
	//	struct TheWorld_Utils::_RGB* rgb = (struct TheWorld_Utils::_RGB*)colorImage.ptr();
	//	BYTE* a = bumpImage.size() > 0 ? bumpImage.ptr() : nullptr;
	//	for (size_t row = 0; row < heigth; row++)
	//		for (size_t col = 0; col < width; col++)
	//		{
	//			rgba->r = rgb->r;
	//			rgba->g = rgb->g;
	//			rgba->b = rgb->b;

	//			if (a != nullptr)
	//			{
	//				rgba->a = *a;
	//				a++;
	//			}
	//			else
	//				rgba->a = 255;

	//			rgba++;
	//			rgb++;
	//		}

	//	normalRoughnessImage.clear(__FUNCTION__);
	//	if (normalImage.size() > 0 || roughImage.size() > 0)
	//	{
	//		normalRoughnessImage.reserve(bufferSize);
	//		normalRoughnessImage.adjustSize(bufferSize);
	//		struct TheWorld_Utils::_RGBA* rgba = (struct TheWorld_Utils::_RGBA*)normalRoughnessImage.ptr();
	//		struct TheWorld_Utils::_RGB* rgb = normalImage.size() > 0 ? (struct TheWorld_Utils::_RGB*)normalImage.ptr() : nullptr;
	//		BYTE* a = roughImage.size() > 0 ? roughImage.ptr() : nullptr;
	//		for (size_t row = 0; row < heigth; row++)
	//			for (size_t col = 0; col < width; col++)
	//			{
	//				if (rgb != nullptr)
	//				{
	//					rgba->r = rgb->r;
	//					if (flipY)
	//						rgba->g = 255 - rgb->g;
	//					else
	//						rgba->g = rgb->g;
	//					rgba->b = rgb->b;
	//					rgb++;
	//				}
	//				else
	//				{
	//					rgba->r = 127;
	//					rgba->g = 127;
	//					rgba->b = 255;
	//				}

	//				if (a != nullptr)
	//				{
	//					rgba->a = *a;
	//					a++;
	//				}
	//				else
	//					rgba->a = 255;

	//				rgba++;
	//			}
	//	}
	//}

	//void TerrainEdit::generateGroundImage(std::string outdir, std::string groundTypeName, size_t imageSize, bool flipY, MemoryBuffer& colorImage, MemoryBuffer& bumpImage, MemoryBuffer& normalImage, MemoryBuffer& roughImage)
	//{
	//	MemoryBuffer albedoBumpImage;
	//	MemoryBuffer normalRoughnessImage;

	//	generateGroundImage(albedoBumpImage, normalRoughnessImage, groundTypeName, imageSize, flipY, colorImage, bumpImage, normalImage, roughImage);

	//	if (outdir.substr(outdir.size() - 1, 1) != "\\")
	//		outdir += "\\";
	//	FILE* outFile = nullptr;

	//	std::string albedoBumpImagePath = outdir + groundTypeName + "_albedo_bump.ground";
	//	errno_t err = fopen_s(&outFile, albedoBumpImagePath.c_str(), "wb");
	//	if (err != 0)
	//	{
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Open ") + albedoBumpImagePath.c_str() + " in errore - Err=" + std::to_string(err)).c_str()));
	//	}

	//	if (fwrite(albedoBumpImage.ptr(), albedoBumpImage.size(), 1, outFile) != 1)
	//	{
	//		fclose(outFile);
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Error writing ") + albedoBumpImagePath.c_str()).c_str()));
	//	}

	//	fclose(outFile);

	//	std::string normalRoughnessImagePath = outdir + groundTypeName + "_normal_roughness.ground";
	//	err = fopen_s(&outFile, normalRoughnessImagePath.c_str(), "wb");
	//	if (err != 0)
	//	{
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Open ") + normalRoughnessImagePath.c_str() + " in errore - Err=" + std::to_string(err)).c_str()));
	//	}

	//	if (fwrite(normalRoughnessImage.ptr(), normalRoughnessImage.size(), 1, outFile) != 1)
	//	{
	//		fclose(outFile);
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Error writing ") + normalRoughnessImagePath.c_str()).c_str()));
	//	}

	//	fclose(outFile);
	//}

