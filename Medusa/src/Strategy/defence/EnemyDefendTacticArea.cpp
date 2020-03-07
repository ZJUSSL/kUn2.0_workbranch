#include "EnemyDefendTacticArea.h"
#include <VisionModule.h>
#include "DefendUtils.h"

namespace{
	CGeoPoint theirGoal =CGeoPoint(PARAM::Field::PITCH_LENGTH /2.0 ,0); 

}

CEnemyDefendTacticArea::CEnemyDefendTacticArea()
{
	ourCarNum = 0;
	mask = 0;
	areaShape = 0;
	circleCenter = CGeoPoint(0,0);
	jointLength = 0;
	interRadius = 0;
	outerRadius = 0;
}

CEnemyDefendTacticArea::~CEnemyDefendTacticArea()
{

}

bool CEnemyDefendTacticArea::isInCircleArea(CGeoPoint & enemyCarPos)
{
	////circle 为以我们的车为中心的一个圆 ，传入有效半径为outerRadius
	double enemy2meDist = CVector(circleCenter - enemyCarPos).mod();
	if (enemy2meDist <outerRadius)
	{
		return true;
	}else
	{
		return false;
	}
}

bool CEnemyDefendTacticArea::isInLongCircle(CGeoPoint & enemyCarPos)
{
	////LongCirclr为禁区，传入有效量表示：  circleCenter：球门中心；outerRadius：两个1/4圆的半径;jointLength:两个半圆连接长度
	const CGeoPoint leftC = CGeoPoint(PARAM::Field::PITCH_LENGTH/2, -PARAM::Field::PENALTY_AREA_L/2);
	const CGeoPoint rightC = CGeoPoint(PARAM::Field::PITCH_LENGTH/2, PARAM::Field::PENALTY_AREA_L/2);
	if (Utils::InTheirPenaltyArea(enemyCarPos, 10)) return true;
	return false;

	/*if (abs(enemyCarPos.y()) < PARAM::Field::PENALTY_AREA_DEPTH / 2.0)// 如果y在禁区范围内
	{
		if ((enemyCarPos - theirGoal ).mod() <  ( PARAM::Field::PENALTY_AREA_DEPTH - 10 ))
		{
			return true;
		}else
		{
			return false;
		}
	}else if (abs(enemyCarPos.y()) > (PARAM::Field::PENALTY_AREA_WIDTH / 2.0 ))
	{
		return false;
	}else if(enemyCarPos.y() > 0)
	{
		if ( CVector(enemyCarPos - rightC).mod() < PARAM::Field::PENALTY_AREA_R - 10)
		{
			return true;
		}else{return false;}
	}else if (enemyCarPos.y() < 0)
	{
		if (CVector(enemyCarPos - leftC).mod() < PARAM::Field::PENALTY_AREA_R - 10)
		{
			return true;
		}else return false;
	}  */
}

bool CEnemyDefendTacticArea::isInAnnulus(CGeoPoint & enemyCarPos)
{
	///Annulus为禁区外一圈，传入量表示：  circleCenter：球门中心；outerRadius：两个1/4外圆的半径；interRadius:两个1/4内圆的半径；;jointLength:两个半圆连接长度
//	double buffer = outerRadius - interRadius;
	
//	bool inInterCircle = false;
//	bool inOuterCicle =false;

//	const CGeoPoint leftC = CGeoPoint(PARAM::Field::PITCH_LENGTH/2, -PARAM::Field::PENALTY_AREA_L/2);
//	const CGeoPoint rightC = CGeoPoint(PARAM::Field::PITCH_LENGTH/2, PARAM::Field::PENALTY_AREA_L/2);
	if (Utils::InTheirPenaltyArea(enemyCarPos, 10)) return true;
	return false;
 ////内圆判断
	/*
	if (abs(enemyCarPos.y()) < PARAM::Field::PENALTY_AREA_L / 2.0)
	{
		if ((enemyCarPos - theirGoal ).mod() <  ( PARAM::Field::PENALTY_AREA_DEPTH - 10 ))
		{
			inInterCircle = true;
		}else
		{
			inInterCircle = false;
		}
	}else if (abs(enemyCarPos.y()) > (PARAM::Field::PENALTY_AREA_L / 2.0 + PARAM::Field::PENALTY_AREA_R))
	{
		inInterCircle = false;
	}else if(enemyCarPos.y() > 0)
	{
		if (CVector(enemyCarPos - rightC).mod() < PARAM::Field::PENALTY_AREA_R - 10)
		{
			inInterCircle = true;
		}else{inInterCircle = false;}
	}else if (enemyCarPos.y() < 0)
	{
		if (CVector(enemyCarPos - leftC).mod() < PARAM::Field::PENALTY_AREA_R - 10)
		{
			inInterCircle = true;
		}else inInterCircle = false;
	}
	////外圆判断
	if (abs(enemyCarPos.y()) < PARAM::Field::PENALTY_AREA_L / 2.0)
	{
		if ((enemyCarPos - theirGoal ).mod() <  ( PARAM::Field::PENALTY_AREA_DEPTH + buffer ))
		{
			inOuterCicle = true;
		}else
		{
			inOuterCicle = false;
		}
	}else if (abs(enemyCarPos.y()) > (PARAM::Field::PENALTY_AREA_L / 2.0 + PARAM::Field::PENALTY_AREA_R + buffer))
	{
		inOuterCicle = false;
	}else if(enemyCarPos.y() > 0)
	{
		if (CVector(enemyCarPos - rightC).mod() < PARAM::Field::PENALTY_AREA_R +buffer)
		{
			inOuterCicle = true;
		}else{inOuterCicle = false;}
	}else if (enemyCarPos.y() < 0)
	{
		if (CVector(enemyCarPos - leftC).mod() < PARAM::Field::PENALTY_AREA_R + buffer)
		{
			inOuterCicle = true;
		}else inOuterCicle = false;
	}
	if (inInterCircle == false && inOuterCicle == true)
	{
		return true;
	}else
		return false;
	*/
}

bool CEnemyDefendTacticArea::isMarkingOurCar(int enemyCarNum,const CVisionModule* pVision)
{
	const PlayerVisionT& theirCar = pVision->theirPlayer(enemyCarNum);
	const PlayerVisionT& ourCar =pVision->ourPlayer(ourCarNum);
	const MobileVisionT& ball =pVision->ball();
	double me2oppDir =Utils::Normalize( (theirCar.Pos() - ourCar.Pos()).dir());
	double me2Goal =Utils::Normalize((theirGoal - ourCar.Pos()).dir());
	double me2Ball = Utils::Normalize((ball.Pos() - ourCar.Pos()).dir());

	bool oppBetweenMeAndGoal = abs(Utils::Normalize(me2Goal - me2oppDir)) < PARAM::Math::PI / 6;
	bool oppBetweenMeAndBall = abs(Utils::Normalize(me2oppDir - me2Ball)) < PARAM::Math::PI / 6;
	return (oppBetweenMeAndBall || oppBetweenMeAndGoal);
}

bool CEnemyDefendTacticArea::isOnBallHalf(int enemyCarNum,const CVisionModule* pVision)
{
	const MobileVisionT& ball = pVision->ball();
	const PlayerVisionT & theirCar =pVision->theirPlayer(enemyCarNum);
	if (ball.Pos().x() > 0)
	{
		return theirCar.Pos().x() >0;
	}else
	{
		return theirCar.Pos().x() <= 0;
		}
}
