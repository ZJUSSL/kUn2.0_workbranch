#include "ZCirclePass.h"
#include "GDebugEngine.h"
#include <VisionModule.h>
#include "RobotSensor.h"
#include "skill/Factory.h"
#include "ControlModel.h"
#include <utils.h>
#include <DribbleStatus.h>
#include <ControlModel.h>
#include "GDebugEngine.h"
#include "TaskMediator.h"
#include "SkillUtils.h"

namespace {
    const int verbose = true;
    double MAX_DRIBBLE_DIST = 50; // dribbling ball cannot move more than 80 cm
    double SHOOT_ACCURACY = 3 * PARAM::Math::PI / 180;
    double MAX_ACC = 200;
    double MAX_ROT_ACC = 5;
    double MAX_ROT_SPEED = 5;
    double MAX_TURN_DIR = 100 * PARAM::Math::PI/180;
    int MAX_BLOCK_CNT = 100;
    const double DEBUG_TEXT_HIGH = 30;
    enum {
        TURN = 1,
        SIDEMOVE = 2,
        DRIBBLE = 3,
    };
    int grabMode = DRIBBLE;
    int last_mode = DRIBBLE;
    int fraredOn;
    int fraredOff;
    CGeoPoint dribblePoint;
    bool isDribble = false;
    int maxFrared = 180;
    bool isTurning[PARAM::Field::MAX_PLAYER];
    int blockCnt[PARAM::Field::MAX_PLAYER] = {0};
    bool shepi[PARAM::Field::MAX_PLAYER];
    int turnWay = 2;
    int movingWay = 1;
    bool returnPoint = false;
}

void CZCirclePass::plan(const CVisionModule* pVision) {
    const MobileVisionT& ball = pVision->ball();
    const CGeoPoint passTarget = task().player.pos;
    int vecNumber = task().executor;
    const PlayerVisionT& me = pVision->ourPlayer(vecNumber);
    oppNum = ZSkillUtils::instance()->getTheirBestPlayer();
    const PlayerVisionT& enemy = pVision->theirPlayer(oppNum);
    const double power = task().player.kickpower;
    const bool isChip = (task().player.kick_flag & PlayerStatus::CHIP);
    const bool setKick = (task().player.kick_flag & PlayerStatus::KICK);

    CVector me2Ball = ball.Pos() - me.Pos();
    CVector me2enemy = enemy.Pos() - me.Pos();
    CVector me2target = passTarget - me.Pos();
    bool canMoveOn = !(fabs(Utils::Normalize(me2target.dir() - me2enemy.dir())) < PARAM::Math::PI/4 && me2enemy.mod() < 50);//???寡膠?臬?血?冽??????
    double delta = fabs(Utils::Normalize(me2enemy.dir() - me.Dir()));
    //撠??刻??詨??
    double finalDir;
    bool canShoot = true;
    if(Utils::InTheirPenaltyArea(passTarget, 0)){
        KickDirection::Instance()->GenerateShootDir(vecNumber, me.Pos());
        finalDir = KickDirection::Instance()->getRealKickDir();//霈∠?撠??刻?
        canShoot = KickDirection::Instance()->getKickValid();//?斗?剛?賢?西腺??嚗??唾楝蝥踵?臬?西◤摰??典???
    }
    else{//銝?撠??典???湔?乩???
        finalDir = me2target.dir();
        canShoot = true;
    }
//    cout <<"canShoot: "<<canShoot<< "  "<< 180*fabs(Utils::Normalize(me.Dir() - finalDir))/3.1415926<<endl;
    //?貊??頧砍???斗??
    if (pVision->getCycle() - _lastCycle > 6) {
        isTurning[vecNumber] = false;//餈?6撣折??蝞?
    }
    //sidemove?嗅???斗?剖笆?寞?臬?虫??湔?∟膠
    if(!canMoveOn) blockCnt[oppNum]++;
    else blockCnt[oppNum] = 0;
    //??蝏剛◤撖寞?寞??MAX_BLOCK_CNT 撣批?????Ｚ粥雿?
    if(blockCnt[oppNum] >MAX_BLOCK_CNT){
        blockCnt[oppNum] = 0;//clear bit
        shepi[vecNumber] = ! shepi[vecNumber];
    }
    //30撣扯??券?Ｗ??撠勗??虫?銝芣?孵????
    if (isTurning[vecNumber] == true) {
        if(! Utils::InTheirPenaltyArea(passTarget, 0)) {
            isTurning[vecNumber] = false;//銝?撠??券??蝞?
        }
    }
    bool frared = RobotSensor::Instance()->IsInfraredOn(vecNumber);
    //撣行??撠?蝻?摮????賜??蝥Ｗ?
    if (frared) {
        fraredOn = fraredOn >= maxFrared ? maxFrared : fraredOn + 1;
        fraredOff = 0;
        if(fraredOn > 30 && !isDribble){
            dribblePoint = me.Pos(); // when dribble 5 frames, start to mark start point
            isDribble = true;   // not mark my point next time
        }
    }
    else {
        fraredOn = 0;
        fraredOff = fraredOff >= maxFrared ? maxFrared : fraredOff + 1;
        if(fraredOff > 30){
            dribblePoint = me.Pos();//if lose ball  frames, dribblePoint become my position
            isDribble = false; //mark my point next time
        }
    }
//    cout << "frared:  "<<frared<<endl;
//    TaskT grabTask(task());
    /*********************** judge mode ********************/
    if(fraredOn > 30){
//        if(delta < MAX_TURN_DIR && me.Dir() < PARAM::Math::PI/2) grabMode = TURN;
        if(delta < MAX_TURN_DIR) grabMode = TURN;
        else grabMode = SIDEMOVE;
    }
    else if(fraredOff > 5){
        grabMode = DRIBBLE;
    }
//    if(frared && last_mode == SIDEMOVE) grabMode = SIDEMOVE;
    last_mode = grabMode;

    /*********************** set subTask ********************/
    if(grabMode == TURN){
        if (verbose) GDebugEngine::Instance()->gui_debug_msg(me.Pos()+ Utils::Polar2Vector(DEBUG_TEXT_HIGH, -PARAM::Math::PI/1.5), "Turn", COLOR_CYAN);
        if(isTurning[vecNumber] == false){
//            turnWay = Utils::Normalize(me2Ball.dir() - me2target.dir()) > 0 ? 4 : 2;//2嚗?撌血??嚗?4:?喳??
            turnWay = me2target.dir() > 0 ? 2 : 4;//2嚗?撌血??嚗?4:?喳??
            isTurning[vecNumber] = true;
        }
        setSubTask(PlayerRole::makeItOpenSpeedCircle(vecNumber, 5, turnWay, finalDir, 1, PARAM::Math::PI/2));
    }
    if(grabMode == SIDEMOVE){
        if (verbose) GDebugEngine::Instance()->gui_debug_msg(me.Pos()+ Utils::Polar2Vector(DEBUG_TEXT_HIGH, -PARAM::Math::PI/1.5), "Side Move", COLOR_CYAN);
        turnWay = me2target.dir() > 0 ? 2 : 4;
        if(Utils::InTheirPenaltyArea(me.Pos(), PARAM::Vehicle::V2::PLAYER_SIZE+2*PARAM::Field::BALL_SIZE))
            setSubTask(PlayerRole::makeItGoto(vecNumber, Utils::MakeOutOfTheirPenaltyArea(me.Pos(), PARAM::Vehicle::V2::PLAYER_SIZE+2*PARAM::Field::BALL_SIZE), finalDir, PlayerStatus::DRIBBLING));
        else
            setSubTask(PlayerRole::makeItOpenSpeedCircle(vecNumber, 27, turnWay, finalDir, 1, PARAM::Math::PI/2));
    }
    if(grabMode == DRIBBLE){
        if (verbose) GDebugEngine::Instance()->gui_debug_msg(me.Pos()+ Utils::Polar2Vector(DEBUG_TEXT_HIGH, -PARAM::Math::PI/1.5), "Dribble", COLOR_CYAN);
        CGeoPoint target;
        double dir;
        if(ball.Valid()) target = ball.Pos() + Utils::Polar2Vector(5, enemy.Dir());
        else target = enemy.Pos() + Utils::Polar2Vector(5, enemy.Dir());
        if(ball.Valid()) dir = me2Ball.dir();
        else dir = me2enemy.dir();
        if(me2Ball.mod() > 50 || ball.Vel().mod() > 50)
            setSubTask(PlayerRole::makeItGetBallV4(vecNumber, PlayerStatus::DRIBBLE, target, CGeoPoint(999, 999), 0));
        else{
            if(!Utils::IsInField(target,PARAM::Vehicle::V2::PLAYER_SIZE+PARAM::Field::BALL_SIZE*2))
                target = Utils::MakeInField(target,PARAM::Vehicle::V2::PLAYER_SIZE+PARAM::Field::BALL_SIZE*2);
            setSubTask(PlayerRole::makeItGoto(vecNumber, target, dir, PlayerStatus::NOT_AVOID_THEIR_VEHICLE));
        }
    }
    /****************** other ***************/
    bool meInFrontOfBall = me2Ball.mod() < 30;//ball.Valid() ? (me2Ball.mod() < 20 && Utils::Normalize(me2BallDir - me.Dir()) < PARAM::Math::PI / 3) : RobotSensor::Instance()->IsInfraredOn(task().executor);
    if (meInFrontOfBall) {
        DribbleStatus::Instance()->setDribbleCommand(vecNumber, 3);
    }
    //?臬?血???
    if (setKick && canShoot && fabs(Utils::Normalize(me.Dir() - finalDir)) < SHOOT_ACCURACY) {
        if(!isChip) KickStatus::Instance()->setKick(vecNumber, power);
        else KickStatus::Instance()->setChipKick(vecNumber, power);
    }
    _lastCycle = pVision->getCycle();
    return CStatedTask::plan(pVision);
}

CPlayerCommand* CZCirclePass::execute(const CVisionModule* pVision) {
    if (subTask()) {
        return subTask()->execute(pVision);
    }
    return nullptr;
}
