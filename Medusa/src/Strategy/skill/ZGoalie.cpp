#include "ZGoalie.h"
#include "VisionModule.h"
#include "parammanager.h"
#include "geometry.h"
#include "Factory.h"
#include "staticparams.h"
#include "SkillUtils.h"
#include "BallSpeedModel.h"
#include "BestPlayer.h"
#include "DribbleStatus.h"
#include "GDebugEngine.h"
#include "RobotSensor.h"
#include <QString>
namespace{
    const static int maxFrared = 200;
    const static int MIN_X = -PARAM::Field::PITCH_LENGTH/2;
    const static int MAX_X = -PARAM::Field::PITCH_LENGTH/2 + PARAM::Field::PENALTY_AREA_DEPTH;
    const static int MIN_Y = -PARAM::Field::PENALTY_AREA_WIDTH/2;
    const static int MAX_Y = PARAM::Field::PENALTY_AREA_WIDTH/2;

    const CGeoPoint OUR_GOAL(-PARAM::Field::PITCH_LENGTH/2,0);
    const CGeoPoint LEFT_GOAL_POST(-PARAM::Field::PITCH_LENGTH/2,-PARAM::Field::GOAL_WIDTH/2 - 2);
    const CGeoPoint RIGHT_GOAL_POST(-PARAM::Field::PITCH_LENGTH/2,PARAM::Field::GOAL_WIDTH/2 + 2);
    const CGeoSegment GOAL_LINE(LEFT_GOAL_POST,RIGHT_GOAL_POST);
    const double DRIBBLE_TARGET_X = -PARAM::Field::PITCH_LENGTH/2+PARAM::Field::PENALTY_AREA_DEPTH/2;
    const CGeoPoint LEFT_PENALTY_CENTER(DRIBBLE_TARGET_X,-1.3*PARAM::Field::GOAL_WIDTH/2);
    const CGeoPoint RIGHT_PENALTY_CENTER(DRIBBLE_TARGET_X,1.3*PARAM::Field::GOAL_WIDTH/2);

    const double PENALTY_PADDING = 0;//2*PARAM::Vehicle::V2::PLAYER_CENTER_TO_BALL_CENTER;
    const CGeoPoint PENALTY_LEFT_UP(MAX_X-PENALTY_PADDING,MIN_Y+PENALTY_PADDING);
    const CGeoPoint PENALTY_RIGHT_DOWN(MIN_X+PENALTY_PADDING,MAX_Y-PENALTY_PADDING);
    const CGeoRectangle PENALTY(PENALTY_LEFT_UP,PENALTY_RIGHT_DOWN);

    const double TOP_PADDING = PARAM::Vehicle::V2::PLAYER_SIZE*1.5;// after calculation
    const double Y_PADDING = PARAM::Vehicle::V2::PLAYER_SIZE*2.5;//after calculation
    const double BOTTOM_PADDING = PARAM::Vehicle::V2::PLAYER_SIZE;
    const CGeoPoint ATTACK_LEFT_UP(MAX_X-TOP_PADDING,MIN_Y+Y_PADDING);
    const CGeoPoint ATTACK_RIGHT_DOWN(MIN_X + BOTTOM_PADDING,MAX_Y-Y_PADDING);
    const CGeoRectangle ATTACK_RECTANGLE(ATTACK_LEFT_UP,ATTACK_RIGHT_DOWN);

    //bool in_back_of_penalty(const CGeoPoint& pos);
    bool in_our_penalty(const CGeoPoint& pos,double padding = 0);
//    CGeoPoint make_into_penalty(const CGeoPoint& pos,double distance);
    double get_defence_direction(const CGeoPoint&);
    bool checkCollision(const CVisionModule*,int,const CGeoSegment&);

    bool DEBUG = false;

    const double SAFE_ARRIVED_TIME = 0.01;
    const double ARRIVED_TIME = 0.05;

    PlayerCapabilityT GOALIE_CAPABILITY;

}
CZGoalie::CZGoalie():_state(NOTHING){
    ZSS::ZParamManager::instance()->loadParam(DEBUG,"Debug/ZGoalie",false);
    ZSS::ZParamManager::instance()->loadParam(GOALIE_CAPABILITY.maxAccel        ,"CGotoPositionV2/MGoalieAcc"   ,600);
    ZSS::ZParamManager::instance()->loadParam(GOALIE_CAPABILITY.maxSpeed        ,"CGotoPositionV2/MGoalieSpeed" ,300);
    ZSS::ZParamManager::instance()->loadParam(GOALIE_CAPABILITY.maxAngularAccel ,"CGotoPositionV2/RotationAcc"  ,15 );
    ZSS::ZParamManager::instance()->loadParam(GOALIE_CAPABILITY.maxAngularSpeed ,"CGotoPositionV2/RotationSpeed",15 );
    ZSS::ZParamManager::instance()->loadParam(GOALIE_CAPABILITY.maxDec          ,"CGotoPositionV2/MGoalieDec"   ,600);
    ZSS::ZParamManager::instance()->loadParam(GOALIE_CAPABILITY.maxAngularDec   ,"CGotoPositionV2/RotationAcc"  ,15 );
}
void CZGoalie::plan(const CVisionModule* pVision){
    if (pVision->getCycle() - _lastCycle > PARAM::Vision::FRAME_RATE * 0.1) {
        _state = CZGoalie::NOTHING;
    }
    int player = task().executor;
    double power = task().player.kickpower;
    int kick_flag = task().player.kick_flag;
    auto& pos = task().player.pos;

    bool frared = RobotSensor::Instance()->IsInfraredOn(player);
    if (frared) {
        fraredOn = fraredOn >= maxFrared ? maxFrared : fraredOn + 1;
        fraredOff = 0;
    } else {
        fraredOn = 0;
        fraredOff = fraredOff >= maxFrared ? maxFrared : fraredOff + 1;
    }

    auto ball_valid = pVision->ball().Valid();
    auto ball = ball_valid ? pVision->ball().RawPos() : pVision->ball().Pos();

    auto ball_vel = pVision->ball().Vel().mod();
    auto ball_vel_dir = pVision->ball().Vel().dir();
    CGeoSegment ball_line(ball,BallSpeedModel::Instance()->posForTime(9999,pVision));
    if(DEBUG) GDebugEngine::Instance()->gui_debug_line(ball_line.start(),ball_line.end(),COLOR_GREEN);
    auto& self = pVision->ourPlayer(player);
    auto& self_pos = self.Valid() ? self.RawPos() : self.Pos();

    // fix ball missing in clear_ball
    if(!ball_valid && in_our_penalty(self_pos,14*10) && fraredOn>4 && _state == CLEAR){
        ball = self_pos + Utils::Polar2Vector(PARAM::Vehicle::V2::PLAYER_CENTER_TO_BALL_CENTER,self.Dir());
    }

    CGeoEllipse stand_ellipse(OUR_GOAL,PARAM::Field::PENALTY_AREA_DEPTH/2,PARAM::Field::GOAL_WIDTH/2);
    CGeoLineEllipseIntersection stand_intersection(CGeoLine(ball,get_defence_direction(ball)),stand_ellipse);
    CGeoPoint stand_pos;
    if(stand_intersection.intersectant()){
        if(ball.dist2(stand_intersection.point1())<ball.dist2(stand_intersection.point2()))
            stand_pos = stand_intersection.point1();
        else
            stand_pos = stand_intersection.point2();
        if(stand_pos.x() < MIN_X + PARAM::Vehicle::V2::PLAYER_SIZE)
            stand_pos.setX(MIN_X + PARAM::Vehicle::V2::PLAYER_SIZE);
    }else{
        stand_pos = CGeoPoint(-PARAM::Field::PITCH_LENGTH/2+PARAM::Vehicle::V2::PLAYER_SIZE,0);
    }
    double stand_dir = (ball-stand_pos).dir();

    CGeoLineRectangleIntersection intersection(ball_line,PENALTY);
    auto& intersection_point1 = intersection.point1();
    auto& intersection_point2 = intersection.point2();
    CGeoPoint projection_pos = ball_line.projection(self_pos);
    bool projection_inside_penalty = in_our_penalty(projection_pos) && (ball_vel_dir - (projection_pos - ball).dir()) < PARAM::Math::PI/18;
    CGeoPoint target_pos = stand_pos;
    double target_dir = 0;
    if(projection_inside_penalty){
        target_pos = projection_pos;
    }else if(intersection.intersectant()){
        if(projection_pos.dist2(intersection_point1) < projection_pos.dist2(intersection_point2))
            target_pos = (ball_vel_dir - (intersection_point1 - ball).dir() < PARAM::Math::PI/18) ? intersection_point1 : intersection_point2;
        else
            target_pos = (ball_vel_dir - (intersection_point2 - ball).dir() < PARAM::Math::PI/18) ? intersection_point2 : intersection_point1;
    }else{
        target_pos = stand_pos;
    }
    if(DEBUG) GDebugEngine::Instance()->gui_debug_x(target_pos,0);
    bool target_on_ball_line = ball_line.IsPointOnLineOnSegment(target_pos);
    double time_ball_point = BallSpeedModel::Instance()->timeForDist(ball.dist(target_pos),pVision);
    if(time_ball_point < 0) time_ball_point = 9999;
    double time_player_point = predictedTime(self, target_pos);
    bool can_inter = intersection.intersectant() && target_on_ball_line && (time_ball_point - time_player_point > SAFE_ARRIVED_TIME || time_player_point < ARRIVED_TIME);

    CVector target_vel;
    predictRushSpeed(self,target_pos,time_ball_point-SAFE_ARRIVED_TIME,GOALIE_CAPABILITY,target_vel);

    CGeoLineLineIntersection danger_intersection(ball_line,GOAL_LINE);
    bool danger_to_our_goal = danger_intersection.Intersectant() && ball_line.IsPointOnLineOnSegment(danger_intersection.IntersectPoint()) && GOAL_LINE.IsPointOnLineOnSegment(danger_intersection.IntersectPoint()) && (ball_vel_dir - (danger_intersection.IntersectPoint() - ball).dir()) < PARAM::Math::PI/18;
    bool need_clear = in_our_penalty(ball,-16*10) && (ball_vel < 50*10 || _state == CLEAR);
    bool need_dribble_back;
    if(in_our_penalty(ball,15*10)&&!in_our_penalty(ball,25*10)){
        need_dribble_back = lastNeedDribbleBack;
    }else {
        need_dribble_back = in_our_penalty(ball,-16*10) && !in_our_penalty(ball,20*10);
    }
    lastNeedDribbleBack = need_dribble_back;
    const auto& enemy = pVision->theirPlayer(ZSkillUtils::instance()->getTheirBestPlayer());
    const auto& enemy_projection = ball_line.projection(enemy.Pos());
    const auto& enemy_dist_to_ball = (enemy_projection - enemy.Pos()).mod();
    bool enemy_danger = ball_line.IsPointOnLineOnSegment(enemy_projection);
    bool need_defense = enemy_danger && in_our_penalty(enemy.Pos(),-100*10) && !danger_to_our_goal && ball_vel > 50*10;
    bool need_attack = in_our_penalty(enemy.Pos(),-100*10) && ball_vel <= 50*10 && (ball - enemy.Pos()).mod() < 50*10;
    bool need_stand_closer = need_defense || need_attack;
//    const auto& enemy_inter_point = ZSkillUtils::instance()->predictedTheirInterPoint(enemy.Pos(), ball);
//    need_stand_closer = in_our_penalty(enemy_inter_point,-100);//test
    const auto& enemy_inter_point =ZSkillUtils::instance()->getTheirInterPoint(ZSkillUtils::instance()->getTheirBestPlayer());
    CGeoLineRectangleIntersection attack_intersection(CGeoLine(in_our_penalty(enemy_inter_point,-100*10) ? enemy_inter_point : enemy.Pos(),get_defence_direction(in_our_penalty(enemy_inter_point,-100*10) ? enemy_inter_point : enemy.Pos())),ATTACK_RECTANGLE);
    CGeoPoint attack_pos;
//    GDebugEngine::Instance()->gui_debug_arc(ZSkillUtils::instance()->getTheirInterPoint(ZSkillUtils::instance()->getTheirBestPlayer()), 20 ,0, 360);
    if(attack_intersection.intersectant()){
        if(enemy.Pos().dist2(attack_intersection.point1())<enemy.Pos().dist2(attack_intersection.point2()))
            attack_pos = attack_intersection.point1();
        else
            attack_pos = attack_intersection.point2();
    }else{
        attack_pos = stand_pos;
    }
    double attack_dir = (ball-attack_pos).dir();

    const CVector ball2self = (self_pos - ball);
    const CVector ball2point = ((ball.y()>0 ? RIGHT_PENALTY_CENTER : LEFT_PENALTY_CENTER)-ball);
    const CGeoPoint dribble_pos(ball+ball2self.unit()*(PARAM::Vehicle::V2::PLAYER_FRONT_TO_CENTER-2*10));
    const CGeoPoint dribble_back_pos(ball+ball2point.unit()*30);

    const double dribble_back_dir = ball.x()<DRIBBLE_TARGET_X ? ball2point.dir() : Utils::Normalize(ball2point.dir()+PARAM::Math::PI);

    bool collision = false;
    if(can_inter){
        collision = checkCollision(pVision,player,ball_line);
    }

    int flag = PlayerStatus::NOT_AVOID_PENALTY | PlayerStatus::NOT_AVOID_THEIR_VEHICLE | PlayerStatus::NOT_AVOID_OUR_VEHICLE;
    if(pVision->gameState().ballPlacement()){
        flag |= PlayerStatus::AVOID_STOP_BALL_CIRCLE;
    }

    // clear ball
    if(need_clear){
        _state = CZGoalie::CLEAR;
        if(DEBUG) GDebugEngine::Instance()->gui_debug_msg(CGeoPoint(-500*10,-160*10),"CLEAR_BALL",0);
        if(need_dribble_back){
            if(DEBUG){
                GDebugEngine::Instance()->gui_debug_msg(CGeoPoint(-500*10,-140*10),QString("DRIBBLE_BACK : %1").arg(frared).toLatin1(),0);
            }
            DribbleStatus::Instance()->setDribbleCommand(player, 3);
            if(fraredOn>4){DribbleStatus::Instance()->setDribbleCommand(player, 3);
                setSubTask(PlayerRole::makeItGoto(player,dribble_back_pos,dribble_back_dir,CVector(0,0),0,200*10,15,50*10,5,flag));
            }else{
                setSubTask(PlayerRole::makeItGoto(player,dribble_pos,Utils::Normalize(ball2self.dir()+PARAM::Math::PI),CVector(0,0),0,300*10,50,200*10,30,flag));
            }
        }else{
            setSubTask(PlayerRole::makeItGetBallV4(player,kick_flag,pos,CGeoPoint(-9999*10,-9999*10),int(power)));
        }
    }
    // zero velocity to inter the ball
    else if(can_inter && !collision){
        _state = CZGoalie::INTER;
        if(DEBUG) GDebugEngine::Instance()->gui_debug_msg(CGeoPoint(-500*10,-160*10),"ZERO_INTER",0);
        target_dir = ball_vel_dir + PARAM::Math::PI;
        setSubTask(PlayerRole::makeItGoto(player,target_pos,target_dir,flag));
    }
    // none zero velocity to save
    else if(danger_to_our_goal){
        _state = CZGoalie::DANDER;
        if(DEBUG) {
            GDebugEngine::Instance()->gui_debug_x(danger_intersection.IntersectPoint(),3);
            GDebugEngine::Instance()->gui_debug_msg(CGeoPoint(-500*10,-160*10),"NONE_ZERO_SAVE",0);
        }
        setSubTask(PlayerRole::makeItGoto(player,target_pos,target_dir,target_vel,0,flag));
    }
    // attack stand
    else if(need_stand_closer){
        _state = CZGoalie::CLOSER;
        if(DEBUG) GDebugEngine::Instance()->gui_debug_msg(CGeoPoint(-500*10,-160*10),"ATTACK_STAND",0);
        setSubTask(PlayerRole::makeItGoto(player,attack_pos,attack_dir,flag));
    }
    // normal stand
    else{
        _state = CZGoalie::NORMAL;
        if(DEBUG) GDebugEngine::Instance()->gui_debug_msg(CGeoPoint(-500*10,-160*10),"NORMAL_STAND",0);
        setSubTask(PlayerRole::makeItGoto(player,stand_pos,stand_dir,flag));
    }

    if((kick_flag & PlayerStatus::DRIBBLE) && _state==CZGoalie::CLEAR){
        DribbleStatus::Instance()->setDribbleCommand(player, 3);
    }

    _lastCycle = pVision->getCycle();
    CStatedTask::plan(pVision);
}
CPlayerCommand* CZGoalie::execute(const CVisionModule* pVision){
    if (subTask()) return subTask()->execute(pVision);
    return nullptr;
}
namespace{
// padding positive means smaller area, padding negative means larger area
bool in_our_penalty(const CGeoPoint& pos,double padding){
    if(pos.x() > MIN_X + padding && pos.x() < MAX_X - padding && pos.y() > MIN_Y + padding && pos.y() < MAX_Y - padding)
        return true;
    return false;
}
double get_defence_direction(const CGeoPoint & pos){
    double leftPostToBallDir = (pos - LEFT_GOAL_POST).dir();
    double rightPostToBallDir = (pos - RIGHT_GOAL_POST).dir();
    if(DEBUG){
        GDebugEngine::Instance()->gui_debug_line(pos,LEFT_GOAL_POST,6);
        GDebugEngine::Instance()->gui_debug_line(pos,RIGHT_GOAL_POST,6);
    }
    return Utils::Normalize((leftPostToBallDir + rightPostToBallDir) / 2 + PARAM::Math::PI);
}
//CGeoPoint make_into_penalty(const CGeoPoint& pos,double distance){

//}
// avoid reflict from our defender's ass
bool checkOneCollision(const PlayerVisionT& test_robot,const CGeoSegment& ballLine){
    if(test_robot.Valid() && in_our_penalty(test_robot.Pos(),-100*10)){
        if(DEBUG) GDebugEngine::Instance()->gui_debug_robot(test_robot.RawPos(),test_robot.RawDir(),COLOR_CYAN);
        auto projection = ballLine.projection(test_robot.Pos());
        auto distance = (projection-test_robot.Pos()).mod();
        if(distance < PARAM::Vehicle::V2::PLAYER_SIZE+2*10){
            if(DEBUG){
                GDebugEngine::Instance()->gui_debug_msg(CGeoPoint(-500*10,-120*10),"CAN_INTER but COLLISION",COLOR_RED);
                GDebugEngine::Instance()->gui_debug_robot(test_robot.RawPos(),test_robot.RawDir(),COLOR_GREEN);
                GDebugEngine::Instance()->gui_debug_line(test_robot.RawPos(),projection,COLOR_GREEN);
            }
            return true;
        }
    }
    return false;
}
bool checkCollision(const CVisionModule* pVision,int goalie,const CGeoSegment& ballLine){
    for(int i = 0; i < PARAM::Field::MAX_PLAYER; i++){
        if(checkOneCollision(pVision->theirPlayer(i),ballLine)){
            return true;
        }
        if(i!= goalie && checkOneCollision(pVision->ourPlayer(i),ballLine)){
            return true;
        }
    }
    return false;
}
}
