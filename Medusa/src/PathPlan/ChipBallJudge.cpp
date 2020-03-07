//#include "ChipBallJudge.h"
//#include <VisionModule.h>
//#include <WorldModel.h>
//#include <BestPlayer.h>
//#include <utils.h>
//#include "GDebugEngine.h"
//namespace
//{
//	bool DEBUG = false;
//}
//CChipBallJudge::CChipBallJudge()
//{
//	_cycle = 99999;
//	_lastCycle = 99999;
//	isFirstGetRefereeMsg = true;
//	ballInialPos = CGeoPoint(0,0);
//	count = 0;
//	goKickAlrealdy= false;
//}

//CChipBallJudge::~CChipBallJudge()
//{

//}

//bool CChipBallJudge::doJudge(const CVisionModule* pVision,double & ballActualMovingDir, CGeoPoint& ballStartPos)
//{
//	bool isChipKick = false;
//	const MobileVisionT& ball = pVision->Ball();
//	checkKickCarNum(pVision);

//	ballActualMovingDir = ball.Vel().dir();
//	ballStartPos = ball.Pos();
//	//获取裁判盒信息
//	const string refMsg = WorldModel::Instance()->CurrentRefereeMsg();
//	//获取球的位置
//	if (DEBUG)
//	{
//		//cout << "isFirstGetRefereeMsg:" <<isFirstGetRefereeMsg<< endl;
//		//cout << "theirIndirectKick == refMsg:" <<("TheirIndirectKick" == refMsg)<< endl;
//	}
	
//	if (("TheirIndirectKick" == refMsg || "TheirDirectKick" == refMsg || "TheirKickOff" == refMsg ) )
//	{
//		if (isFirstGetRefereeMsg)
//		{
//			if (DEBUG)
//			{
//				cout << "Fuck ball Initial Pos!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" <<endl;
//			}

//			ballInialPos = ball.Pos();
//			ballStartPos = ballInialPos;
//			isFirstGetRefereeMsg = false;
//		}
		
//	}
//	//获取开球车开球时候的朝向。
//	if (checkKickerGoKick(pVision) && !goKickAlrealdy && ("TheirIndirectKick" == refMsg || "TheirDirectKick" == refMsg || "TheirKickOff" == refMsg ) )
//	{
//		initialKickerDir = pVision->theirPlayer(theirKickerID).Dir();
//		goKickAlrealdy = true;
//	}
//	//如果开球车去开球了，开始判断
//	if (goKickAlrealdy)
//	{
//	//	cout << "Kicker Go Kick"<<endl;
////		double currentBallMovingDir =ball.Vel().dir();
//		/*if ( ((ball.Pos() - ballInialPos).mod() > 5))
//		{*/
//	//		cout <<"start Judge" <<endl;
//			if (isChip(pVision))
//			{
//				if (DEBUG)
//				{
//					cout << "ChipChip" << endl;
//				}
			
//				ballActualMovingDir = initialKickerDir;
//				isChipKick = true;
//			}else
//			{
//				if (DEBUG)
//				{
//					//cout << "Not Chip"<<endl;
//				}
				
//				ballActualMovingDir = ball.Vel().dir();
//				isChipKick =false;
//			}
//		//}
//	}
	
//	//if ((ball.Pos() - ballInialPos).mod()>100)
//	//{
//	//	//cout << "reset" <<endl;
//	//	isFirstGetRefereeMsg = true;
//	////	ballInialPos = CGeoPoint(0,0);
//	////	count = 0;
//	//	goKickAlrealdy= false;
//	//}
//	GDebugEngine::Instance()->gui_debug_x(ballInialPos,COLOR_BLACK);
//	return isChipKick;
//}

//void CChipBallJudge::checkKickCarNum(const CVisionModule* pVision)
//{
//	const CBestPlayer::PlayerList& oppList = BestPlayer::Instance()->theirFastestPlayerToBallList();
//	if ( oppList.size() < 1)
//		theirKickerID = 0;
//	else
//		theirKickerID = oppList[0].num;
//}

//bool CChipBallJudge::checkKickerGoKick(const CVisionModule* pVision)
//{
//	const PlayerVisionT& theirKicker = pVision->theirPlayer(theirKickerID);
//	const MobileVisionT& ball = pVision->Ball();
//	double distTheirKicker2Ball = (theirKicker.Pos() - ball.Pos()).mod();
//	bool isTheirKickerFaceBall  = abs(Utils::Normalize(theirKicker.Dir() - (ball.Pos() - theirKicker.Pos()).dir()) )< PARAM::Math::PI / 6;
//	if (isTheirKickerFaceBall && (distTheirKicker2Ball < (PARAM::Vehicle::V2::PLAYER_FRONT_TO_CENTER + PARAM::Field::BALL_SIZE + 3)))
//	{
//		return true;
//	}
//	else
//	{
//		return false;
//	}
//}

//bool CChipBallJudge::isChip(const CVisionModule* pVision)
//{
//	bool dirJudgeChip = false;
//	bool distJudgeChip = false;

//	const MobileVisionT& ball = pVision->Ball();
//	double InitialBallDir2currentBall =(ball.Pos() - ballInialPos ).dir();
//	double ballMovingDir = ball.Vel().dir();
//	//////////////////////////////////////
//	//距离判断
//	/////////////////
	
//	CGeoLine ballLine = CGeoLine(ballInialPos,initialKickerDir);
////	GDebugEngine::Instance()->gui_debug_line(ballInialPos,ballInialPos + Utils::Polar2Vector(100,initialKickerDir));
//	double flyingBall2LineDist = (ballLine.projection(ball.Pos()) - ball.Pos()).mod();
//	if ((ball.Pos() - ballInialPos).mod() > 10)
//	{
//		distJudgeChip = flyingBall2LineDist > 8;
//	}
//	//cout << "ball.Pos() - ballInialPos:" << (ball.Pos() - ballInialPos).mod() << endl;
//	////////////////////////
//	//角度判断
//	///////////////
	
//	//cout << abs(Utils::Normalize(ballMovingDir - InitialBallDir2currentBall)) << endl;
//	if (abs(ball.Vel().mod() > 10)&&abs(Utils::Normalize(ballMovingDir - InitialBallDir2currentBall)) > PARAM::Math::PI / 40)
//	{
//		//cout  << "DIRjudeg CHIP" << endl;
//		dirJudgeChip = true;
//	}
//	else
//	{
//		//cout  << "DIRjudeg FLAT" << endl;
//		dirJudgeChip = false;
//	}
////cout << "dirJudge:"  << dirJudgeChip <<" "<<"distJudge:"<<distJudgeChip<<endl;
////	cout << "flyingBall2LineDist:" << flyingBall2LineDist << "  " <<"dir:" << abs(Utils::Normalize(ballMovingDir - InitialBallDir2currentBall)) * 180 / PARAM::Math::PI;
//	double finalJudge = distJudgeChip || dirJudgeChip;
//	return finalJudge;
//}
