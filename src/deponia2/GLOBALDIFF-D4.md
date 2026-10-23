# Texts and numbers that Deponia 2 and Deponia 4 do not share

Generated with `python tools/globaldiff.py build/cross/original-Deponia2.pkl build/cross/original-Deponia4.pkl`: the text literals and numbers (0x30 to 0xFFFFF) that the code of the
`T*` classes refers to in one build and nowhere in the other. "First" is Deponia 2, "second" Deponia 4. The texts that are the names of the data fields (`InitXMLNames`), of the Lua hooks
(`...Hook`, `engineEvent`, `movieEvent`), the shaders and the console commands are the additions of the newer builds; see `VERSIONS.md` here.

```
texts in T* code: first 1522, second 1731; only first 55, only second 193
== ONLY IN THE FIRST
  ' Actions'   in TDiagnostic::DrawDiagram()
  '%ld'   in TFieldValue::ToString() const
  '(*it)->lpTexture == NULL'   in TGraphicsInterface::~TGraphicsInterface(
  '--------------------------------------------------------------------------------\n'   in TComposedFile::WriteToDisk(wxFileName co
  '--------------------------------------------------------------------------------\n\n'   in TComposedFile::WriteToDisk(wxFileName co
  '1.6.2'   in TPicturePNG::ReadHeader(TFile&, int*, in, TPicturePNG::ReadHeader(char*, unsigned , TPicturePNG::Wri
  'AL_FORMAT_51CHN16'   in TSoundFFMPEG::Play(wxFileName const&, in
  'AL_FORMAT_61CHN16'   in TSoundFFMPEG::Play(wxFileName const&, in
  'AL_FORMAT_71CHN16'   in TSoundFFMPEG::Play(wxFileName const&, in
  'AL_FORMAT_QUAD16'   in TSoundFFMPEG::Play(wxFileName const&, in
  'An unknown OpenGL error occured. %d'   in TGraphicsOGL::CheckLastError(bool) const
  'AnyObject'   in TVisionaire::GetAnyObject() const
  'Codec not found. File: %s'   in TSoundFFMPEG::Play(wxFileName const&, in
  "Container '%s'\n"   in TComposedFile::WriteToDisk(wxFileName co
  'Could not open codec . File: %s'   in TSoundFFMPEG::Play(wxFileName const&, in
  'Failed to create the audio context'   in TSoundFFMPEG::AdjustVolume(int, int, int, TSoundFFMPEG::Play(wxFileName const&, in
  'Failed to create the audio device'   in TSoundFFMPEG::AdjustVolume(int, int, int, TSoundFFMPEG::Play(wxFileName const&, in
  "Failed to initialize the composed file '%s'."   in TComposedFile::InitEntries()
  'Failed to make context current'   in TSoundFFMPEG::AdjustVolume(int, int, int, TSoundFFMPEG::Play(wxFileName const&, in
  'Failed to read buffer of project file. File is corrupt. Error: %s Position: %s'   in TXMLProjectReader::ReadXMLDoc(TVedFile&,
  'File contained more channels than we support (%d).\n'   in TSoundFFMPEG::Play(wxFileName const&, in
  'Framebuffers not supported. Make sure your OpenGL driver is up-to-date.'   in TGraphicsOGL::ResizeScene(wxSize const&,
  'MainObject'   in TVisionaire::GetGame() const
  'Ogg/Opus failed'   in TSoundFFMPEG::Play(wxFileName const&, in
  'Ogg/Vorbis failed'   in TSoundFFMPEG::Play(wxFileName const&, in
  'STEAMREMOTESTORAGE_INTERFACE_VERSION013'   in TSteamSDK::Init()
  'Speichern'   in TVisionaire::BinarySave(wxFileName const
  'Unknown Space '   in TDiagnostic::DrawDiagram()
  'Used '   in TDiagnostic::DrawDiagram()
  'Using Framebuffer with texture.'   in TGraphicsOGL::ResizeScene(wxSize const&,
  'Using Framebuffer without texture.'   in TGraphicsOGL::ResizeScene(wxSize const&,
  'Wasted Space '   in TDiagnostic::DrawDiagram()
  'aqy4rnsmsd8jTw'   in TGameControl::PreLoad(wxString&, wxStrin
  'avformat_open_input failed. File: %s'   in TSoundFFMPEG::Play(wxFileName const&, in
  'basic_string::_S_construct null not valid'   in TData::SerializeParam(char const*, char , TextBox::ProcessEvent(Event*)
  'basic_string::at'   in TCFont::PrintTextLines(std::list<wxStrin, TCFont::TCFont(TVisObjRef&, TFontManager, TSText::Calculat
  'basic_string::erase'   in TextBox::ProcessEvent(Event*)
  'basic_string::substr'   in TConsole::ParseObjectName(int), TMSavegame::GetExistingSaveGames(std::ve, TSText::CalculateRestText(
  'composedfile'   in TComposedFile::InitEntries(), TComposedFile::Open(TFile&, wxFileName c
  "couldn't alloc avioContext. File: %s"   in TSoundFFMPEG::Play(wxFileName const&, in
  'evv.vid'   in TMovie::PlayCutScene(wxFileName const&, 
  'flags'   in TArgParser::ParseArguments(TArgSyntax co
  'g_lookupTable.pEntries'   in TTypeGroup::AddType(TTypeData const&)
  'lightmap'   in TGraphicsOGL::DrawWithLightMap(void*, wx
  'lmrect'   in TGraphicsOGL::DrawWithLightMap(void*, wx
  'pass'   in TGraphicsOGL::AfterDrawScene(bool, bool)
  'picture'   in TPictureIO::EnsureSizeValid(), TPictureIO::LoadPicture(wxFileName, TPic, TPictureIO::LoadPicture(wxF
  'source'   in TGraphicsOGL::DrawWithLightMap(void*, wx
  'surfacefactor'   in TGraphicsOGL::DrawWithLightMap(void*, wx
  'texOffset'   in TGraphicsOGL::FinishDraw()
  'texSize'   in TGraphicsOGL::FinishDraw()
  'texture'   in TGraphicsOGL::DrawWithLightMap(void*, wx
  'vector::_M_range_check'   in TArgTypeArray::Item(int), TArgTypeArray::Item(int) const, TCommandPtrArray::Item(int), TCommandPtrAr
  'vector::reserve'   in TGraphicsOGL::Draw(TParticleEmitter&, vo, TTable::EndPaste(), TVisionaire::CreateTables(int), TXMLPr
  'vector<bool>::_M_insert_aux'   in TGWaySystem::SetWaySystem(TVisObjRef con
== ONLY IN THE SECOND
  ' loop'   in TDiagnostic::DrawDiagram()
  ' true'   in TDiagnostic::DrawDiagram()
  '#ifdef GL_ES\nprecision lowp float;\nprecision lowp int;\n#endif\nuniform mat4 mvp_mat;\nuniform sampler2'   in TTShader::OnCreate(TVisionaireObject*)
  '#ifdef GL_ES\nprecision lowp float;\nprecision lowp int;\n#endif\nuniform vec4 color;\nuniform sampler2D '   in TTShader::OnCreate(TVisionaireObject*)
  '%.1f ms %dM %dM'   in TDiagnostic::DrawDiagram()
  '%02x '   in TPictureWebP::ReadData(TFile&, TPictureM, TPictureWebP::ReadHeader(TFile&, int*, i
  '%08x '   in TPictureWebP::ReadData(TFile&, TPictureM, TPictureWebP::ReadHeader(TFile&, int*, i
  '%f %f %d %d'   in TDiagnostic::DrawDiagram()
  '%f;'   in TCPDebuggerClient::NextFrame()
  '%s %s %s %s'   in TGraphicsSubSystemGL::GetAdapterInfo()
  '%s = %f'   in TGameControl::Update()
  '%s could not be opened'   in TComposedFileManager::Open(TFile&, wxFil
  '%s#%03d#%06d#'   in TComposedFile::AddFile(wxFileName, wxFil
  '%s#%03d#m00000#'   in TComposedFile::AddFile(wxFileName, wxFil
  '-------------------------------------------------------------------------------'   in TComposedFile::WriteToDisk(wxFileName co
  '--------------------------------------------------------------------------------'   in TComposedFile::WriteToDisk(wxFileName co
  '.vc<3i>'   in TVisionaireGame::UpdateVersion(int, bool
  '.vi<3i>'   in TVisionaireGame::UpdateVersion(int, bool
  '.vs<3i>'   in TVisionaireGame::UpdateVersion(int, bool
  '.vv<3c>;copy'   in TVisionaireGame::UpdateVersion(int, bool
  '/Visionaire Editor'   in TStandardPaths::GetConfigDir()
  '1.6.20'   in TPicturePNG::ReadHeader(TFile&, int*, in, TPicturePNG::ReadHeader(char*, unsigned , TPicturePNG::Wri
  ';copy'   in TVisionaireGame::UpdateVersion(int, bool
  ';engineEvent:'   in TGameControl::SaveEventHandlers()
  'After '   in TDiagnostic::EndFixedRegion()
  'AnimationEndDirection'   in TVisionaireGame::InitXMLNames()
  'AnimationModelFile'   in TVisionaireGame::InitXMLNames()
  'AnimationModelName'   in TVisionaireGame::InitXMLNames()
  'AnimationStartedHook'   in TGAnimation::Start(bool, float)
  'AnimationStoppedHook'   in TGAnimation::ContinueAnimations(), TGAnimation::HideAnimation(std::_List_it
  'Arial'   in TDiagnostic::DrawDiagram()
  'AudioBus'   in TVisionaireClipboard::TVisionaireClipboa, TVisionaireGame::InitXMLNames(), TVisionaireGame::TVisiona
  'AudioBusEvents'   in TVisionaireGame::InitXMLNames()
  'AudioBusLinks'   in TVisionaireGame::InitXMLNames()
  'AudioBusSettings'   in TVisionaireGame::InitXMLNames()
  'AudioBusType'   in TVisionaireGame::InitXMLNames()
  'AudioBusValues'   in TVisionaireGame::InitXMLNames()
  'AudioBusses'   in TVisionaireClipboard::TVisionaireClipboa, TVisionaireGame::InitXMLNames(), TVisionaireGame::TVisiona
  'B00'   in TConsole::FillOptions(TConsoleStateEnum)
  'B01'   in TConsole::FillOptions(TConsoleStateEnum)
  'B02'   in TConsole::FillOptions(TConsoleStateEnum)
  'B03'   in TConsole::FillOptions(TConsoleStateEnum)
  'B04'   in TConsole::FillOptions(TConsoleStateEnum)
  'B05'   in TConsole::FillOptions(TConsoleStateEnum)
  'B06'   in TConsole::FillOptions(TConsoleStateEnum)
  'B07'   in TConsole::FillOptions(TConsoleStateEnum)
  'B08'   in TConsole::FillOptions(TConsoleStateEnum)
  'ButtonMatrixId'   in TVisionaireGame::InitXMLNames()
  'ButtonRotation'   in TVisionaireGame::InitXMLNames()
  'ButtonRotationCenter'   in TVisionaireGame::InitXMLNames()
  'ButtonScale'   in TVisionaireGame::InitXMLNames()
  'ButtonScaleX'   in TVisionaireGame::InitXMLNames()
  'ButtonScaleY'   in TVisionaireGame::InitXMLNames()
  'ButtonShaderSet'   in TVisionaireGame::InitXMLNames()
  'C%f'   in TFieldValue::ToString() const
  'CharacterDirectionHook'   in TGCharacter::GetDirectionIndex(int, TCha
  'CharacterHarmonizeWalkAnimations'   in TVisionaireGame::InitXMLNames()
  'CharacterLightmapActive'   in TVisionaireGame::InitXMLNames()
  'CharacterShaderSet'   in TVisionaireGame::InitXMLNames()
  "Container '%s'"   in TComposedFile::WriteToDisk(wxFileName co
  "Could not open file '%s' in container '%s'"   in TComposedFileManager::GetMemoryFile(TMem, TComposedFileManager::Open(TFile&, wxFil
  'DeleteActions'   in TGameControl::Update()
  'DialogCharacter'   in TVisionaireGame::InitXMLNames()
  'Draw Buffers '   in TDiagnostic::DrawDiagram()
  'EngineEventHandler'   in TGameControl::HandleEngineEvent(std::bas
  'Error reading header from image %s'   in TPictureIO::ReadPictureFile()
  'Event'   in TVisionaireClipboard::TVisionaireClipboa, TVisionaireGame::InitXMLNames(), TVisionaireGame::TVisiona
  'Events'   in TVisionaireClipboard::TVisionaireClipboa, TVisionaireGame::InitXMLNames(), TVisionaireGame::TVisiona
  "Failed to initialize the composed file '%s'. Could not open."   in TComposedFile::InitEntries()
  "Failed to initialize the composed file '%s'. Invalid header. Should be VIS3, but was %s"   in TComposedFile::InitEntries()
  'Framebuffer '   in TDiagnostic::DrawDiagram()
  'GalaxyOverlay state: %d'   in TGalaxySDKOverlayInitListener::OnOverlay
  'GameAudioContainerLinks'   in TVisionaireGame::InitXMLNames()
  'GameAudioMixLinks'   in TVisionaireGame::InitXMLNames()
  'GameBuildOptions'   in TVisionaireGame::InitXMLNames()
  'GameBuildRules'   in TVisionaireGame::InitXMLNames()
  'GameContainers'   in TVisionaireGame::InitXMLNames()
  'GameIconFile'   in TVisionaireGame::InitXMLNames()
  'GameShaderLinks'   in TVisionaireGame::InitXMLNames()
  'GameSmoothScrolling'   in TVisionaireGame::InitXMLNames()
  'GetActionTextHook'   in TGObjectManager::GetActionText() const
  'InterfaceMatrixId'   in TVisionaireGame::InitXMLNames()
  'KeyEventHandler'   in TGameControl::HandleKeyEvent(TKeyboardMe
  'LOAD'   in TConsole::FillOptions(TConsoleStateEnum), TConsole::ParseCommand()
  'LoadingSavegameFailed'   in TGameControl::LoadGame(TMSavegame*)
  'LoadingSavegameSuccess'   in TGameControl::LoadGame(TMSavegame*)
  'MainLoop'   in TGameControl::Update()
  'MouseEventHandler'   in TMasterControl::ProcessMessage(TMouseMes
  'MouseMoveHandler'   in TMasterControl::ProcessMessage(TMouseMes
  'ObjectRenderHook'   in TManagedObject::Draw()
  'ObjectScaleX'   in TVisionaireGame::InitXMLNames()
  'ObjectScaleY'   in TVisionaireGame::InitXMLNames()
  'OutfitModelFiles'   in TVisionaireGame::InitXMLNames()
  'OutfitTurnAnimations'   in TVisionaireGame::InitXMLNames()
  'ParticleContainerPreviewScene'   in TVisionaireGame::InitXMLNames()
  'ParticleContainerSettings'   in TVisionaireGame::InitXMLNames()
  'Pause'   in TDiagnostic::DrawDiagram()
  'Plays'   in TDiagnostic::DrawDiagram()
  'S00'   in TConsole::FillOptions(TConsoleStateEnum)
  'S01'   in TConsole::FillOptions(TConsoleStateEnum)
  'S02'   in TConsole::FillOptions(TConsoleStateEnum)
  'S03'   in TConsole::FillOptions(TConsoleStateEnum)
  'S04'   in TConsole::FillOptions(TConsoleStateEnum)
  'S05'   in TConsole::FillOptions(TConsoleStateEnum)
  'S06'   in TConsole::FillOptions(TConsoleStateEnum)
  'S07'   in TConsole::FillOptions(TConsoleStateEnum)
  'S08'   in TConsole::FillOptions(TConsoleStateEnum)
  'SAVE'   in TConsole::FillOptions(TConsoleStateEnum), TConsole::ParseCommand()
  'STEAMAPPS_INTERFACE_VERSION008'   in TSteamSDK::Init()
  'STEAMREMOTESTORAGE_INTERFACE_VERSION014'   in TSteamSDK::Init()
  'SceneAudioBusses'   in TVisionaireGame::InitXMLNames()
  'SceneShaderSet'   in TVisionaireGame::InitXMLNames()
  'Shader'   in TVisionaireClipboard::TVisionaireClipboa, TVisionaireGame::InitXMLNames(), TVisionaireGame::TVisiona
  'ShaderCompiled'   in TVisionaireGame::InitXMLNames()
  'ShaderFS'   in TVisionaireGame::InitXMLNames()
  'ShaderVS'   in TVisionaireGame::InitXMLNames()
  'Shaders'   in TVisionaireClipboard::TVisionaireClipboa, TVisionaireGame::InitXMLNames(), TVisionaireGame::TVisiona
  'Sound'   in TGameControl::Update()
  'Sounds'   in TDiagnostic::DrawDiagram()
  'Steam: RequestCurrentStats failed'   in TSteamSDK::Init()
  'SteamAPI_RegisterCallback'   in TSteamSDK::GetSymbols()
  'SteamAPI_RegisterCallback not found in: %s'   in TSteamSDK::GetSymbols()
  'SteamAPI_RunCallbacks'   in TSteamSDK::GetSymbols()
  'SteamAPI_RunCallbacks not found in: %s'   in TSteamSDK::GetSymbols()
  'SteamController005'   in TSteamSDK::Init()
  'SteamUser019'   in TSteamSDK::Init()
  'SteamUtils008'   in TSteamSDK::Init()
  'Stopped'   in TDiagnostic::DrawDiagram()
  "SubSystem doesn't support runtime shaders"   in TShader::CompileFromCode(char const*, ch
  'TMemoryBuffer::Decompress failed (srclen: %d, destlen: %d)'   in TMemoryBuffer::Uncompress(TMemoryBuffer&
  'TextPositionHook'   in TGText::CalculateTextPos()
  'TextRenderHook'   in TGText::Draw(float)
  'TextStartedHook'   in TGText::TextStarted()
  'TextStoppedHook'   in TGText::TextStopped()
  'TextTextHook'   in TGText::CalculateRestText()
  'Tweens'   in TGameControl::Update()
  'Used  '   in TDiagnostic::DrawDiagram()
  'ValueFloat'   in TVisionaireGame::InitXMLNames()
  'Vis Audio Thread'   in TSoundFFMPEG::TSoundFFMPEG()
  'Visionaire Editor'   in TStandardPaths::GetLogFileDir()
  '[0]'   in TShaderConstantBuffer::SetMat4(char cons
  '\\eActionParts(VActionPartCommand=66);'   in TVisionaireGame::UpdateVersion(int, bool
  '\\eCharacters;'   in TVisionaireGame::UpdateVersion(int, bool
  '\\eGame;<Main.vis>'   in TVisionaireGame::UpdateVersion(int, bool
  '\\eInterfaces;'   in TVisionaireGame::UpdateVersion(int, bool
  '\\eObjects;'   in TVisionaireGame::UpdateVersion(int, bool
  '\\eScenes;'   in TVisionaireGame::UpdateVersion(int, bool
  '_t_'   in TShaderConstantBuffer::SetTexture(char c, TShaderConstantBuffer::SetTexture(char c
  "can't open %s"   in TVisionaire::BinarySave(wxFileName const
  'caret'   in TextBox::DrawSelections()
  'color.bin'   in TGraphicsOGL::ResizeScene(wxSize const&,
  'color2'   in TGraphicsOGL::DrawFrame(wxRect const&, u, TGraphicsOGL::DrawLine(wxPoint const&, w
  'color2.bin'   in TGraphicsOGL::ResizeScene(wxSize const&,
  'commitShift.bin'   in TGraphicsOGL::AfterDrawScene(bool, bool)
  'controller'   in TGameControl::InitScripts()
  'controller.lua'   in TGameControl::InitScripts()
  'create '   in TGameControl::Update()
  'disconnected'   in TCPDebuggerServer::Update()
  'eAudioBusses'   in TVisionaireClipboard::TVisionaireClipboa, TVisionaireGame::TVisionaireGame()
  'eEvents'   in TVisionaireClipboard::TVisionaireClipboa, TVisionaireGame::TVisionaireGame()
  'eShaders'   in TVisionaireClipboard::TVisionaireClipboa, TVisionaireGame::TVisionaireGame()
  'engineEvent'   in TGameControl::LoadEventHandlers()
  'error VP8 detected but no chunk sign'   in TPictureWebP::ReadHeader(TFile&, int*, i
  'error invalid size %dx%d'   in TPictureWebP::ReadHeader(TFile&, int*, i
  'getpeername failed'   in TCPDebuggerServer::GetClientIP()
  'lightmap.bin'   in TGraphicsOGL::DrawWithLightMap(void*, wx
  'movieEvent'   in TMasterControl::MovieEvent(void*)
  'not found'   in TComposedFile::InitEntries()
  'particles.bin'   in TGraphicsOGL::ResizeScene(wxSize const&,
  'renderpass'   in TGraphicsOGL::AfterDrawScene(bool, bool)
  'resolution'   in TGraphicsOGL::Draw(void*, wxRect const&,
  'return particleSystem:new('   in TGObject::SetActive(bool), TGScene::BeginScene()
  'selection'   in TextBox::TextBox()
  'sha1'   in TGameControl::InitScripts()
  'size'   in TGraphicsOGL::ConvertYUVToRGBA(void*, ch
  "size valid '%s', %dms"   in TPictureIO::EnsureSizeValid()
  'stipple.bin'   in TGraphicsOGL::ResizeScene(wxSize const&,
  'texcoord'   in TGraphicsOGL::ConvertYUVToRGBA(void*, ch
  'text.bin'   in TGraphicsOGL::ResizeScene(wxSize const&,
  'textbox'   in TextBox::TextBox()
  'textured.bin'   in TGraphicsOGL::ResizeScene(wxSize const&,
  'texu'   in TGraphicsOGL::ConvertYUVToRGBA(void*, ch
  'texv'   in TGraphicsOGL::ConvertYUVToRGBA(void*, ch
  'texy'   in TGraphicsOGL::ConvertYUVToRGBA(void*, ch
  'tooltip'   in Tooltip::Tooltip()
  'trough'   in TDiagnostic::DrawDiagram()
  'uvSize'   in TGraphicsOGL::ConvertYUVToRGBA(void*, ch
  'vertexId'   in TFreetypeFont::RenderString_i(float, flo
  'vtp_savepic%d.webp#g#-01#00001#'   in TMSavegame::Draw()
  'wait for thread:'   in TPictureIO::RefreshSprite(bool)
  'warning: character %s / %d not in %s'   in TFreetypeFont::CreateChar(unsigned int, 
  'waysystem %d %s on scene %s is not regular and cannot be triangulated: %s'   in TGWaySystem::SetWaySystem(TVisObjRef con
  'yuv.bin'   in TGraphicsOGL::ConvertYUVToRGBA(void*, ch
numbers >= 0x30 in T* code: first 867, second 1030; only first 38, only second 201
ONLY IN THE FIRST: 3b0(TSoundFFMPEG) 3c0(TDiagnostic) 3d4(TDiagnostic) b20(TGraphicsOGL) b24(TGraphicsOGL) bc0(TGraphicsOGL) cf5(TAlphaTexture/TFreetypeF) 100a(TFFMPEGSoundItem/TSoundF) 1020(TSoundFFMPEG) 1021(TSoundFFMPEG) 1101(TSoundFFMPEG) 1103(TSoundFFMPEG) 1403(TFreetypeFont/TGraphicsO) 1f03(TGraphicsOGL) 1fff(TFreetypeFont) 2000(TFreetypeFont/TSoundFFMP) 2150(TSoundFFMPEG) 2200(TGraphicsOGL) 2300(TGraphicsOGL) 2538(TVisionaireGame) 2711(TGAction) 4004(TPictureGIF) 5622(TSoundFFMPEG) 7fff(TParticleEmitter) 8078(TGraphicsOGL) 84c1(TGraphicsOGL) 8571(TGraphicsOGL) 8572(TGraphicsOGL) 8580(TGraphicsOGL) 8581(TGraphicsOGL) 8588(TGraphicsOGL) 8589(TGraphicsOGL) 88e0(TGraphicsOGL) 88e4(TFreetypeFont) 8d41(TGraphicsOGL) bb80(TSoundFFMPEG) 343fd(TParticleEmitter) fffff(TDiagnostic)
ONLY IN THE SECOND: 4c(TDataGroup/TGAction/TVis) 59(TGameControl) 31b(TGAnimation/TTCharacter/) 31c(TGAnimation/THButton/TMB) 31d(TGAnimation/THButton/TMB) 31e(TGAnimation/THButton/TMB) 320(TGAnimation/THButton/TMB) 321(TGCharacter/TTCharacter/) 322(TGInterface/TTInterface/) 323(TGameControl/TTGame/TVis) 324(TTGame/TVisionaireGame) 325(TDataGroup/TTValue/TVisi) 326(TGObject/TGScene/TTParti) 327(TCAnimation/TTAnimation/) 328(TCAnimation/TTAnimation/) 329(TCAnimation/TTAnimation/) 32a(TGCharacter/TTOutfit/TVi) 32b(TGCharacter/TTAnimation/) 32c(TGAnimation/TGObject/THI) 32d(TGAnimation/TGObject/THI) 32e(TGAnimation/THButton/TMB) 32f(TGAnimation/THButton/TMB) 330(TVisionaireClipboard/TVi) 331(TVisionaireGame) 332(TTGame/TVisionaireGame) 333(TTGame/TVisionaireGame) 334(TTGame/TVisionaireGame) 335(TTShader/TVisionaireGame) 336(TTShader/TVisionaireGame) 337(TTShader/TVisionaireGame) 339(TTDialog/TVisionaireGame) 33a(TMasterControl/TTGame/TV) 33b(TVisionaire/TVisionaireG) 33c(TGraphicsSubSystemGL/TVi) 33d(TTAudioBus/TVisionaireGa) 33e(TTAudioBus/TVisionaireGa) 33f(TTAudioBus/TVisionaireGa) 341(TTable/TVisionaire/TVisi) 342(TTGame/TVisionaireGame) 343(TTGame/TVisionaireGame) 344(TTAudioBus/TVisionaireGa) 345(TTAudioBus/TVisionaireGa) 346(TSceneControl/TTScene/TV) 347(TGCharacter/TTCharacter/) 348(TTable/TVisionaireGame) 352(TVisionaire) 360(TTable) 374(TGraphicsOGL) 37e(TTable) 380(TSceneControl) 38e(TTable/TVisionaire) 392(TPictureIO) 3a0(TGameControl) 3c6(TTable) 3d5(TXMLProjectReader) 3ed(TSteamSDK) 3f0(TXMLProjectReader) 3f7(TVisionaire) 3f9(TXMLProjectReader) 401(TMasterControl) 402(TMasterControl) 404(TFramebufferGL) 425(TXMLProjectReader) 438(TFramebufferGL) 439(TXMLProjectReader) 43b(TFramebufferGL) 440(TFramebufferGL) 444(TFramebufferGL/TGAction) 449(TFramebufferGL) 44c(TFramebufferGL/TVisionai) 451(TFramebufferGL) 453(TPictureIO) 454(TFramebufferGL) 459(TTable/TVisionaire) 45a(TFramebufferGL) 45c(TTable) 45f(TFramebufferGL) 460(TGraphicsInterface) 467(TFramebufferGL) 474(TTextureGL) 477(TTable) 47e(TVisionaireObject) 47f(TTextureGL) 48b(TVisionaire) 48c(TTextureGL) 491(TVisionaire) 49d(TVisionaireObject) 49e(TVisionaire) 4a3(TTextureGL) 4b1(TVisionaire) 4b6(TTextureGL) 4b7(TVisionaireGame) 4c0(TTextureGL) 4c5(TTable) 4c7(TTextureGL) 4c8(TTable) 4ce(TTextureGL) 4d2(TXMLProjectReader) 4d3(TXMLProjectReader) 4d6(TTextureGL) 4e7(TTextureGL) 4f3(TGraphicsOGL/TTextureGL) 4fa(TXMLProjectReader) 504(TGraphicsOGL/TVisionaire) 508(TVisionaire) 509(TTable) 50f(TGraphicsOGL) 514(TTable) 520(TTable) 525(TGraphicsOGL) 53a(TVisionaire) 53e(TVisionaire) 546(TXMLProjectReader) 550(TConsole) 557(TTable) 558(TXMLProjectReader) 568(TGraphicsOGL) 5a6(TGraphicsOGL) 5ad(TVisionaire/TXMLProjectR) 5ec(TVisionaire) 5fc(TGraphicsOGL) 619(TGraphicsOGL) 632(TGraphicsOGL) 650(TGraphicsOGL) 651(TMasterControl) 652(TMasterControl) 653(TMasterControl) 654(TMasterControl) 655(TMasterControl) 65e(TVisionaire) 670(TVisionaire) 681(TGraphicsOGL) 6bb(TGraphicsOGL) 6cc(TGraphicsOGL) 700(TMasterControl) 701(TMasterControl) 818(TVisionaireGame) 8dd(TVisionaireGame) 8fe(TVisionaireGame) 900(TGraphicsSubSystemGL) 91d(TVisionaireGame) 93b(TVisionaireGame) 94e(TGraphicsOGL) 94f(TVisionaireGame) 960(TGAction) 967(TVisionaireGame) 976(TGAction) 97b(TVisionaireGame) 99b(TVisionaireGame) 9bb(TVisionaireGame) 9e1(TVisionaireGame) 9f0(TGraphicsOGL) a55(TGraphicsOGL) a8b(TGraphicsOGL) ba5(TGraphicsOGL) bb7(TVisionaireObject) c11(TGraphicsSubSystemGL) c4d(TGameControl) 12c0(TDiagnostic) 1388(TGraphicsOGL) 1f00(TGraphicsSubSystemGL) 1f01(TGraphicsSubSystemGL) 1f02(TGraphicsSubSystemGL) 2078(TCPDebuggerClient/TCPDeb) 2328(TGraphicsOGL) 2760(TVisionaireGame) 3fff(TPictureWebP) 4100(TGraphicsSubSystemGL) 4e1f(TPictureWebP) 4e20(TFreetypeFont) 8006(TFreetypeFont) 821a(TFramebufferGL) 84f9(TFramebufferGL) 84fa(TFramebufferGL) 84fe(TTextureGL) 84ff(TTextureGL) 88e8(TBufferGL) 88f0(TFramebufferGL) 8b8c(TGraphicsSubSystemGL) 8ca9(TFramebufferGL) 8cd6(TFramebufferGL) 8cd7(TFramebufferGL) 9100(TFramebufferGL) 9c40(TGraphicsInterface) 9c41(TCPDebuggerClient) 15f8f(TFreetypeFont) 30d40(TFreetypeFont/TGraphicsO) f4240(TGameControl) f4242(TGameControl) f4243(TGameControl) f4244(TGameControl) f4245(TGameControl) f4246(TGameControl) f4247(TGameControl) f4248(TGameControl) f4249(TGameControl) f424a(TGameControl) f424b(TGameControl) f424c(TGameControl) f424d(TGameControl) f424e(TGameControl)
```
