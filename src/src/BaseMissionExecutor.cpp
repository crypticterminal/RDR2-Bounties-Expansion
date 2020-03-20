#include "Main.h"

BaseMissionExecutor::BaseMissionExecutor(BountyMissionData missionData)
{
	this->missionData = new BountyMissionData(missionData);
	this->stage = MissionInitialization;
	this->status = Pending;
}

BountyMissionData* BaseMissionExecutor::getMissionData()
{
	return missionData;
}

BountyMissionStage BaseMissionExecutor::getMissionStage()
{
	return stage;
}

BountyMissionStatus BaseMissionExecutor::getMissionStatus()
{
	return status;
}

void BaseMissionExecutor::update()
{
	Ped player = PLAYER::PLAYER_PED_ID();
	Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(player, true, false);

	if (stage == BountyMissionStage::MissionInitialization)
	{
		initialize();
		nextStage();
	}
	else if (stage == BountyMissionStage::CollectPoster)
	{
		if (distanceBetween(playerPos, *getArea()->bountyPostersCoords) <= 2.5f)
		{
			inspectPosterPrompt->show();

			if (inspectPosterPrompt->isActivatedByPlayer())
			{
				nextStage();
			}
		}
		else
		{
			inspectPosterPrompt->hide();
		}
	}
	else if (stage == BountyMissionStage::GoToArea)
	{
		if (distanceBetween(playerPos, missionData->startPosition) <= AREA_RADIUS)
		{
			nextStage();
		}
	}
	else if (stage == BountyMissionStage::LocateTarget)
	{
		if (distanceBetweenEntities(player, target) <= 25)
		{
			nextStage();
		}
	}
	else if (stage == BountyMissionStage::CaptureTarget)
	{
		if (isPedHogtied(target) || 
			(missionData->requiredTargetCondition == TargetCondition::DeadOrAlive && ENTITY::IS_ENTITY_DEAD(target)))
		{
			nextStage();
		}
	}
	else if (stage == BountyMissionStage::ArriveToPoliceStation)
	{
		Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(target, true, false);
		if (distanceBetween(targetPos, *getArea()->policeDeptCoords) < 20)
		{
			nextStage();
		}
	}
	else if (stage == BountyMissionStage::HandOverTarget)
	{
		Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(target, true, false);
		if (distanceBetween(targetPos, *getArea()->cellCoords) < 3 &&
			ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(target) <= 1)
		{
			nextStage();
		}
	}
	else if (stage == BountyMissionStage::CollectReward)
	{
		nextStage();
	}

	if (stage >= BountyMissionStage::LocateTarget)
	{
		if (missionData->requiredTargetCondition == TargetCondition::Alive && 
			ENTITY::IS_ENTITY_DEAD(target))
		{
			fail("Bounty failed. The target was wanted alive!");
		}
	}

	if (stage > BountyMissionStage::CaptureTarget && stage < BountyMissionStage::CollectReward)
	{
		if (!ENTITY::IS_ENTITY_DEAD(target) && !isPedHogtied(target))
		{
			stage = BountyMissionStage::CaptureTarget;

			std::stringstream text;
			text << "Capture ~COLOR_RED~" << missionData->targetName;
			showSubtitle(text.str().c_str());
		}
	}
}

MapArea* BaseMissionExecutor::getArea()
{
	return getMapArea(missionData->area);
}

Ped BaseMissionExecutor::getTarget()
{
	return target;
}

void BaseMissionExecutor::nextStage()
{
	switch (stage)
	{
		case BountyMissionStage::MissionInitialization:	
			stage = BountyMissionStage::CollectPoster; 
			break;

		case BountyMissionStage::CollectPoster:			
			onPosterCollected();
			stage = BountyMissionStage::GoToArea; 
			break;

		case BountyMissionStage::GoToArea:				
			onArrivalToArea();
			stage = BountyMissionStage::LocateTarget; 
			break;

		case BountyMissionStage::LocateTarget:			
			onTargetLocated();
			stage = BountyMissionStage::CaptureTarget; 
			break;

		case BountyMissionStage::CaptureTarget:			
			onTargetCaptured();
			stage = BountyMissionStage::ArriveToPoliceStation; 
			break;

		case BountyMissionStage::ArriveToPoliceStation:	
			onArrivalToPoliceStation();
			stage = BountyMissionStage::HandOverTarget; 
			break;

		case BountyMissionStage::HandOverTarget:		
			onTargetHandedOver();
			stage = BountyMissionStage::CollectReward; 
			break;

		case BountyMissionStage::CollectReward:			
			onRewardCollected();
			stage = BountyMissionStage::Finished; 
			break;
	}
}

void BaseMissionExecutor::fail(const char* reason)
{
	status = BountyMissionStatus::Failed;

	if (reason)
	{
		showSubtitle(reason);
	}
}

void BaseMissionExecutor::initialize()
{
	status = BountyMissionStatus::Pending;

	MapArea* area = getMapArea(missionData->area);
	Vector3* posterPos = area->bountyPostersCoords;

	poster = createProp("p_cs_newspaper_01x", *posterPos, true);
	posterBlip = createBlip(poster, 0xEC972124, 0x9E6FEC8F);
}

void BaseMissionExecutor::onPosterCollected()
{
	status = BountyMissionStatus::CollectedPoster;

	ENTITY::DELETE_ENTITY(&poster);
	inspectPosterPrompt->hide();
	targetAreaBlip = createBlip(missionData->startPosition, AREA_RADIUS, 0xB04092F8, 0x7EAB2A55 /* Bounty sprite */);

	const char* condition = missionData->requiredTargetCondition == TargetCondition::Alive ? "Alive" : "Dead or Alive";
	std::stringstream label;
	label << "Bounty Hunting: "
		<< missionData->targetName << ", "
		<< missionData->reward << "$, "
		<< condition;
	setBlipLabel(targetAreaBlip, label.str().c_str());
}

void BaseMissionExecutor::onArrivalToArea()
{
	status = BountyMissionStatus::InProgress;

	RADAR::REMOVE_BLIP(&targetAreaBlip);
	targetAreaBlip = createBlip(missionData->startPosition, AREA_RADIUS, 0xC19DA63, 0);

	target = spawnTarget();
	prepareSet();

	const char* gender = missionData->isTargetMale ? "He" : "She";
	const char* condition = missionData->requiredTargetCondition == TargetCondition::Alive ? "Alive" : "Dead or Alive";
	std::stringstream text;
	text << "Locate and capture ~COLOR_RED~" << missionData->targetName << "~COLOR_WHITE~. " << gender << " is wanted " << condition << ".";
	showSubtitle(text.str().c_str());
}

void BaseMissionExecutor::onTargetLocated()
{
	decorateTarget();
	RADAR::REMOVE_BLIP(&targetAreaBlip);
}

void BaseMissionExecutor::onTargetCaptured()
{
	cellBlip = createBlip(*getArea()->policeDeptCoords, 0x1857A152);

	std::stringstream text;
	text << "Take ~COLOR_RED~" << missionData->targetName << "~COLOR_WHITE~ to the ~COLOR_YELLOW~Police Department";
	showSubtitle(text.str().c_str());
}

void BaseMissionExecutor::onArrivalToPoliceStation()
{
	RADAR::REMOVE_BLIP(&cellBlip);
	cellBlip = createBlip(*getArea()->cellCoords, 0xC19DA63);
	
	std::stringstream text;
	text << "Drop ~COLOR_RED~" << missionData->targetName << "~COLOR-WHITE~ in the ~COLOR_YELLOW~Cell";
	showSubtitle(text.str().c_str());
}

void BaseMissionExecutor::onTargetHandedOver()
{
	Blip targetBlip = RADAR::GET_BLIP_FROM_ENTITY(target);
	RADAR::REMOVE_BLIP(&targetBlip);
}

void BaseMissionExecutor::onRewardCollected()
{
	CASH::PLAYER_ADD_CASH(missionData->reward * 100, 0);
}

void BaseMissionExecutor::onFinished()
{
	status = BountyMissionStatus::Completed;
	cleanup();
}

void BaseMissionExecutor::cleanup()
{

}

void BaseMissionExecutor::decorateTarget()
{
	PED::_0x4A48B6E03BABB4AC(target, (Any*)missionData->targetName); // Set ped name
	createBlip(target, BLIP_TYPE_BOUNTY_TARGET, BLIP_SPRITE_BOUNTY_TARGET);
}