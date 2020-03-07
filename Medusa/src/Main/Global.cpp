#include "Global.h"

CVisionModule*   vision;
CKickStatus*    kickStatus;
CDribbleStatus* dribbleStatus;
CGDebugEngine*  debugEngine;
CWorldModel*	world;
CKickDirection* kickDirection;
CDefPos2015* defPos2015;
CGuardPos* guardpos;
CIndirectDefender* indirectDefender;
CTandemPos* tandemPos;
SkillUtils* skillUtils;
//CBestPlayer* bestPlayer;
CDefenceInfo* defenceInfo;
//CChipBallJudge* chipBallJudge;
//CSupportPos* supportPos;
//CAssistPoint* assistPoint;
CMessiDecision* messi;
//CGetLuaData* getluadata;
CDefenceSquence* defenceSquence;
void initializeSingleton()
{
	vision        = VisionModule::Instance();
	kickStatus    = KickStatus::Instance();
	dribbleStatus = DribbleStatus::Instance();
	debugEngine   = GDebugEngine::Instance();
	world		  = WorldModel::Instance();
	kickDirection = KickDirection::Instance();
//	bestAlg		  = GPUBestAlgThread::Instance();
//	supportPos = SupportPos::Instance();
	defPos2015 = DefPos2015::Instance();
    skillUtils = ZSkillUtils::instance();
//	bestPlayer    = BestPlayer::Instance();
	defenceInfo = DefenceInfo::Instance();
	tandemPos = TandemPos::Instance();
//    chipBallJudge = ChipBallJudge::Instance();
	indirectDefender = IndirectDefender::Instance();
//	assistPoint = AssistPoint::Instance();
    messi = MessiDecision::Instance();
//	getluadata = GetLuaData::Instance();
    messi = MessiDecision::Instance();
    guardpos = GuardPos::Instance();
    defenceSquence = DefenceSquence::Instance();
}
