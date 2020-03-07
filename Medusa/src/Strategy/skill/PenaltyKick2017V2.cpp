#include "PenaltyKick2017V2.h"
#include "skill/Factory.h"
#include <CommandFactory.h>
#include <utils.h>
#include <VisionModule.h>
#include <KickStatus.h>
#include <RobotCapability.h>
#include <WorldModel.h>
#include "GDebugEngine.h"
#include "WorldDefine.h"
#include "staticparams.h"
#include "RobotSensor.h"

namespace {
    bool VERBOSE_MODE = false;

    double goalBuffer = 5;
    CGeoPoint ourBase;
	CGeoPoint ourGoal;
	CGeoPoint leftGoal;
	CGeoPoint rightGoal;
	CGeoPoint leftKickPoint;
	CGeoPoint rightKickPoint;
	double ROTATE_ANGLE = PARAM::Math::PI / 3;
	enum PenaltyKickState {
		KICK = 1
	};

	int _timeCnt;
	double _frameTime = 30 / PARAM::Vision::FRAME_RATE;
	double _enemyMaxVel = 350;
	double _meMaxVel = 350;
	double _enemyMaxAcc = 500;
	double _meMaxAcc = 500;
	int infraredOn;
	int infraredOff;
	int maxFrared = 100;
	int hasBallMaxDist = 50;

	CVector _enemyLastVel;
	CVector _enemyPreVel;
	CVector _mePreVel;
}

CPenaltyKick2017V2::CPenaltyKick2017V2()
{
    ourBase = CGeoPoint(-PARAM::Field::PITCH_LENGTH / 2, 0);
	ourGoal = CGeoPoint(PARAM::Field::PITCH_LENGTH / 2, 0); // (-600, 0)
	leftGoal = CGeoPoint(PARAM::Field::PITCH_LENGTH / 2, -PARAM::Field::GOAL_WIDTH / 2 + goalBuffer); // (-600, -55) 最后射门的目标点
	rightGoal = CGeoPoint(PARAM::Field::PITCH_LENGTH / 2, PARAM::Field::GOAL_WIDTH / 2 - goalBuffer); // (-600, 55)
	leftKickPoint = CGeoPoint(PARAM::Field::PITCH_LENGTH / 2, -PARAM::Field::GOAL_WIDTH / 2); // (-600, -60) 轻踢时的目标点
	rightKickPoint = CGeoPoint(PARAM::Field::PITCH_LENGTH / 2, PARAM::Field::GOAL_WIDTH / 2); // (-600, 60)
	_timeCnt = 0;
	_lastCycle = 0;
	_theirGoalie = 0;
	_targetside = 0;
	infraredOff = 0;
	infraredOn = 0;
}

void CPenaltyKick2017V2::plan(const CVisionModule* pVision)
{
	if (state() == BEGINNING || pVision->getCycle() - _lastCycle > 6) {
		setState(KICK);
		_timeCnt = 0;
		_theirGoalie = 0;
        _circleShootFlag = false;
		for (int i = 0; i < PARAM::Field::MAX_PLAYER; i++) {
			if (Utils::InTheirPenaltyArea(pVision->theirPlayer(i).Pos(), 0)) {
				_theirGoalie = i;
			}
		}
		int tempnum = rand();
		if (tempnum % 2 == 0) {
			_targetPoint = leftGoal;
			_targetside = 0;
			cout << "left kick" << endl;
		}
		else {
			_targetPoint = rightGoal;
			_targetside = 1;
			cout << "right kick" << endl;
		}
	}
	//updateMsg(pVision);

	int rolenum = task().executor;
	const MobileVisionT& ball = pVision->ball();
	const PlayerVisionT& me = pVision->ourPlayer(rolenum);
	const PlayerVisionT& enemy = pVision->theirPlayer(_theirGoalie);

	TaskT myTask(task());

	bool frared = RobotSensor::Instance()->IsInfraredOn(rolenum);
	if (frared) {
		infraredOn = infraredOn >= maxFrared ? maxFrared : infraredOn + 1;
		infraredOff = 0;
	}
	else {
		infraredOn = 0;
		infraredOff = infraredOff >= maxFrared ? maxFrared : infraredOff + 1;
	}

	CGeoPoint bestPoint = getBestPoint(pVision);
    GDebugEngine::Instance()->gui_debug_x(bestPoint);
	CVector ball2Best(bestPoint.x() - ball.Pos().x(), bestPoint.y() - ball.Pos().y());
	if (_shootFlag) { // 当计算得到射门不会被守门员截到时射门
		if (VERBOSE_MODE) cout << "shoot" << bestPoint << endl;
		_shootFlag = 0;
		double kickpower = 650;
		myTask.player.angle = ball2Best.dir();
		myTask.player.pos = ball.Pos();
		KickStatus::Instance()->setKick(rolenum, kickpower);
		setSubTask(TaskFactoryV2::Instance()->GotoPosition(myTask));
	}
    else if (_chipFlag) {
        /// change chip to circleshoot gjy 2018.6.8
        if (!_circleShootFlag) {
            if (VERBOSE_MODE) cout << "preCircle" << endl;
            myTask.player.pos = ball.Pos(); // enemy.Pos() + Utils::Polar2Vector(50, (ball.Pos() - enemy.Pos()).dir());
            if (_targetside == 0) { // left side
                myTask.player.angle = (rightGoal - me.Pos()).dir();
            }
            else { // right side
                myTask.player.angle = (leftGoal - me.Pos()).dir();
            }
            if (infraredOn > 20) _circleShootFlag = true;
            setSubTask(TaskFactoryV2::Instance()->SlowGetBall(myTask));
        }
        else {
            if (VERBOSE_MODE) cout << "circle"<< endl;
            myTask.player.speed_x = 27;
            myTask.player.speed_y = PARAM::Math::PI / 4;
            if (_targetside == 0) { // left side
                myTask.player.rotdir = 2;
                myTask.player.angle = (leftGoal - me.Pos()).dir();
            }
            else { // right side
                myTask.player.rotdir = 4;
                myTask.player.angle = (rightGoal - me.Pos()).dir();
            }
            setSubTask(TaskFactoryV2::Instance()->OpenSpeedCircle(myTask));
        }
    }
	else { // 轻踢带球往前，需要根据不同场地调整力度!!!
		static int tmpCnt = 0;
		if (tmpCnt > 5) {
			if (VERBOSE_MODE) cout << "kick" << endl;
            double kickPower = ball2Best.mod() * 4;
            if (kickPower > 200) kickPower = 200;
            KickStatus::Instance()->setKick(rolenum, kickPower);
			myTask.player.pos = ball.Pos();
			myTask.player.angle = ball2Best.dir();
			setSubTask(TaskFactoryV2::Instance()->GotoPosition(myTask));
		}
		else {
			if (VERBOSE_MODE) cout << "prepare for kick" << endl;
			KickStatus::Instance()->clearAll();
			myTask.player.pos = ball.Pos() + Utils::Polar2Vector(-8, ball2Best.dir() / 2);
			myTask.player.angle = ball2Best.dir();
			setSubTask(TaskFactoryV2::Instance()->GotoPosition(myTask));
			tmpCnt++;
		}
	}

    if (me.Pos().x() > 350) { // 如果很接近球门还没有机会强行射门
		if (VERBOSE_MODE) cout << "force kick" << endl;
		KickStatus::Instance()->setKick(rolenum, 600);
		myTask.player.pos = ball.Pos();
		myTask.player.angle = _targetside == 0 ? (leftGoal - ball.Pos()).dir() : (rightGoal - ball.Pos()).dir();
		setSubTask(TaskFactoryV2::Instance()->ChaseKickV1(myTask));
	}

	bool isMeHasBall = isVisionHasBall(pVision) || (infraredOn > 1 && !ball.Valid());

	if ((enemy.Pos() - ball.Pos()).mod() < (me.Pos() - ball.Pos()).mod()) {
		if (!isMeHasBall) {
			if (VERBOSE_MODE) cout << "give up" << endl;
			KickStatus::Instance()->clearAll();
			myTask.player.pos = CGeoPoint(0, 0);
			setSubTask(TaskFactoryV2::Instance()->GotoPosition(myTask));
		}
	}

	CStatedTask::plan(pVision);
	_lastCycle = pVision->getCycle();
}

void CPenaltyKick2017V2::updateMsg(const CVisionModule * pVision)
{
	int rolenum = task().executor;
//	const MobileVisionT& ball = pVision->Ball();
	const PlayerVisionT& me = pVision->ourPlayer(rolenum);
	const PlayerVisionT& enemy = pVision->theirPlayer(_theirGoalie);
	_enemyPreVel = enemy.Vel();
	_mePreVel = me.Vel();

	if (_enemyPreVel.mod() > _enemyMaxVel) _enemyMaxVel = _enemyPreVel.mod(); // update maxvel
	double tmpAcc = (_enemyPreVel - _enemyLastVel).mod() / _frameTime; // calc opp's acc this frame
	if (tmpAcc > _enemyMaxAcc) _enemyMaxAcc = tmpAcc; // update maxacc

	// calc the circle area me and opp can reach next frame
	_enemyCircle = CGeoCirlce(enemy.Pos() + Utils::Polar2Vector(_enemyPreVel.mod() * _frameTime, _enemyPreVel.mod()), 0.5 * _enemyMaxAcc * pow(_frameTime, 2));
	_meCircle = CGeoCirlce(me.Pos() + Utils::Polar2Vector(_mePreVel.mod() * _frameTime, _mePreVel.mod()), 0.5 * _meMaxAcc * pow(_frameTime, 2));
	_me2EnemyVec = CVector(me.Pos().x() - enemy.Pos().x(), me.Pos().y() - enemy.Pos().y());

	_hasInter = _me2EnemyVec.mod() < _enemyCircle.Radius() + _meCircle.Radius() ? 1 : 0; // whether there is intersection between two circles
	_isClose = _me2EnemyVec.mod() < _enemyCircle.Radius() + _meCircle.Radius() * 3 / 2 ? 1 : 0; // dist between two circles is more than r
	_isFar = _me2EnemyVec.mod() < _enemyCircle.Radius() + _meCircle.Radius() * 2 ? 1 : 0; // dist between two circles is more than 2 * r

	_enemyLastVel = _enemyPreVel;
}

CGeoPoint CPenaltyKick2017V2::getBestPoint(const CVisionModule * pVision)
{
	int rolenum = task().executor;
	const MobileVisionT& ball = pVision->ball();
	const PlayerVisionT& me = pVision->ourPlayer(rolenum);
	const PlayerVisionT& enemy = pVision->theirPlayer(_theirGoalie);
	CGeoPoint finalPoint;

	CGeoPoint goalPoint;
	if (_targetside == 0) goalPoint = leftGoal;
	else goalPoint = rightGoal; // goal point
	CGeoLine tmpLine(ball.Pos(), goalPoint);
	CGeoLine enemyLine(enemy.Pos(), (goalPoint - ball.Pos()).dir() + PARAM::Math::PI / 2);
	CGeoLineLineIntersection enemyInter(tmpLine, enemyLine);
	double ball2InterDist = (ball.Pos() - enemyInter.IntersectPoint()).mod();
	double ball2InterTime = ball2InterDist / 700;
	double enemyBallRadius = 0.5 * _enemyMaxAcc * pow(ball2InterTime, 2);
	enemyBallRadius = enemyBallRadius > PARAM::Vehicle::V2::PLAYER_SIZE ? enemyBallRadius : PARAM::Vehicle::V2::PLAYER_SIZE;
	_enemyBallCircle = CGeoCirlce(enemy.Pos() + Utils::Polar2Vector(_enemyPreVel.mod() * _frameTime, _enemyPreVel.mod()), enemyBallRadius);
	double enemy2InterDist = (enemy.Pos() - enemyInter.IntersectPoint()).mod(); 

	CVector me2Ball(ball.Pos().x() - me.Pos().x(), ball.Pos().y() - me.Pos().y());
	CGeoPoint chipPoint = enemy.Pos() + me2Ball;  // chip point

	CGeoPoint kickPoint;
	if (_targetside == 0)
		if ((enemy.Pos() - rightGoal).mod() > (enemy.Pos() - leftGoal).mod()) {
			if (VERBOSE_MODE) cout << "turn left" << endl;
			_targetside = 1;
			kickPoint = (ball.Pos() + Utils::Polar2Vector(80, (leftKickPoint - ball.Pos()).dir()));
		}
		else kickPoint = (ball.Pos() + Utils::Polar2Vector(80, (rightKickPoint - ball.Pos()).dir()));
	else
		if ((enemy.Pos() - rightGoal).mod() < (enemy.Pos() - leftGoal).mod()) {
			if (VERBOSE_MODE) cout << "turn right" << endl;
			_targetside = 0;
			kickPoint = (ball.Pos() + Utils::Polar2Vector(40, (rightKickPoint - ball.Pos()).dir()));
		}
		else kickPoint = (ball.Pos() + Utils::Polar2Vector(40, (leftKickPoint - ball.Pos()).dir()));

    if (_enemyBallCircle.Radius() < enemy2InterDist - 10) {
		_shootFlag = 1;
		finalPoint = goalPoint;
	}
    else if ((enemy.Pos() - me.Pos()).mod() < 200) {
		_chipFlag = 1;
		finalPoint = chipPoint;
	}
	else {
		_shootFlag = 0;
		_chipFlag = 0;
		finalPoint = kickPoint;
	}

	return finalPoint;
}

bool CPenaltyKick2017V2::isVisionHasBall(const CVisionModule * pVision)
{
	int rolenum = task().executor;
	const MobileVisionT& ball = pVision->ball();
	const PlayerVisionT& me = pVision->ourPlayer(rolenum);
//	const PlayerVisionT& enemy = pVision->theirPlayer(_theirGoalie);
//	int distBuffer = 10;

	CVector me2Ball = ball.Pos() - me.Pos();
	bool isDistHasBall = me2Ball.mod() < hasBallMaxDist;
	bool isDirHasBall = fabs(Utils::Normalize(me2Ball.dir() - ball.Vel().dir())) < 0.5;

//	cout << isDistHasBall << " " << isDirHasBall << endl;
	return isDistHasBall && isDirHasBall;
}
