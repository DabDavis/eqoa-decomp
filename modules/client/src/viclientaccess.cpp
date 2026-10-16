// VIClient's accessors at 0x00BE3900..0x00BE3A70, right after VIClient.cpp's own template
// copies: probably the next TU, whose name is not known.  Names provisional; most of the
// meanings are FrontiersForge's (claude-notes/frontiersforge-viclient-xref-20261015.md),
// and the objects they return are the constructor's roll-call.
#include "game.h"

int VIClient::GetP2BAEC() { return m_p2BAEC; }
char *VIClient::GetP4F098() { return m_p4F098; }
int VIClient::GetWorldId() { return m_worldId; }
unsigned short *VIClient::GetReplyName() { return m_p4F0B8; }
VIUI *VIClient::GetUI() { return &m_ui; }
VIRaster *VIClient::GetRaster() { return &m_raster; }
VIScene *VIClient::GetScene() { return &m_scene; }
VICamera *VIClient::GetP00190() { return &m_camera; }
VIWalkCamera *VIClient::GetWalkCamera() { return &m_walkCam; }
VIDictionary *VIClient::GetDictionary() { return &m_dict; }
VIClientPart06D50 *VIClient::GetP06D50() { return &m_p06D50; }
void *VIClient::GetActiveWindow() { return m_p2BD40; }
long VIClient::GetP07568() { return m_p07568; }
int VIClient::GetP06DC0() { return m_p06DC0; }
int VIClient::GetP06DC4() { return m_p06DC4; }
unsigned int VIClient::GetSelfId() { return m_selfId; }
VIPlayer *VIClient::GetPlayer() { return &m_player; }
int VIClient::IsUnderLiquid() { return m_walkCam.IsUnderLiquid(); }
unsigned int VIClient::GetTargetId() { return m_target; }
int VIClient::GetEffectCount() { return m_effectCount; }
unsigned int VIClient::GetEffectIcon(int i) { return m_effectIcon[i]; }
unsigned short *VIClient::GetEffectName(int i) { return m_effectName[i]; }
int VIClient::GetTargetInfoValid() { return m_targetInfoValid; }
int VIClient::GetTargetDisposition() { return m_targetDisposition; }
int VIClient::GetP2BB70() { return m_p2BB70; }
