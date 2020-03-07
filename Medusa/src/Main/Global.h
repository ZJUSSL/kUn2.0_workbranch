#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "VisionModule.h"
#include "DribbleStatus.h"
#include "KickStatus.h"
#include "GDebugEngine.h"
#include "WorldModel.h"
#include "singleton.h"
#include "KickDirection.h"
#include "DefPos2015.h"
#include "guardpos.h"
#include "TandemPos.h"
#include "ChipBallJudge.h"
#include "BestPlayer.h"
#include "defence/DefenceInfo.h"
#include "IndirectDefender.h"
#include "SupportPos.h"
#include "AssistPoint.h"
#include "GetLuaData.h"
#include "SkillUtils.h"
#include "messidecision.h"
#include "defencesquence.h"
extern CVisionModule*  vision;
extern CKickStatus*    kickStatus;
extern CDribbleStatus* dribbleStatus;
extern CGDebugEngine*  debugEngine;
extern CWorldModel*	   world;
extern CKickDirection* kickDirection;
extern CDefPos2015* defPos2015;
extern CGuardPos* guardpos;
extern CTandemPos* tandemPos;
extern SkillUtils* skillUtils;
//extern CBestPlayer* bestPlayer;
extern CDefenceInfo* defenceInfo;
//extern CChipBallJudge* chipBallJudge;
//extern CHttpServer* httpServer;
extern CIndirectDefender* indirectDefender;
//extern CSupportPos* supportPos;
//extern CAssistPoint* assistPoint;
extern CMessiDecision* messi;
//extern CGetLuaData* getluadata;
extern CDefenceSquence* defenceSquence;
void initializeSingleton();
#endif
