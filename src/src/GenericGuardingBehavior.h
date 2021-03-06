#pragma once

const float GUARD_IDLE_RANGE = 20;
const float GUARD_SUSPECT_RANGE = 15;
const float GUARD_COMBAT_RANGE = 6;
const float GUARD_SEARCH_DURATION_SECS = 10;
const float GUARD_SEEING_RANGE = 28;
const float GUARD_HEARING_RANGE = 35;

const int SCOUTING_SCENARIOS_NUM = 3;
const char* const SCOUTING_SCENARIO_NAMES[SCOUTING_SCENARIOS_NUM] = { "WORLD_HUMAN_GUARD_LAZY", "WORLD_HUMAN_SMOKE", "WORLD_HUMAN_GUARD_SCOUT" };

class GenericGuardingBehavior : public PedBehavior
{
private:
	Vector3 defensePosition;
	float radius;
	TensionMode mode;
	bool _shouldTolerate;
	IdlingModifier idlingModifier;
	RoutineParams routineParams;
	set<Ped>* bodiesFound;
	GameStopwatch stopwatch;

public:
	GenericGuardingBehavior(Ped ped, Vector3 defensePosition, float radius, bool shouldTolerate = true, IdlingModifier idlingModifier = IdlingModifier::Scout);
	GenericGuardingBehavior(Ped ped, Vector3 defensePosition, float radius, IdlingModifier idlingModifier, RoutineParams routineParams, set<Ped>* bodiesFound = NULL, bool shouldTolerate = true);

	TensionMode getMode();
	virtual void start(bool withBlip = false);
	virtual void update();
	virtual void stop();
	void routine();
	void scout();
	void scout(Vector3 scoutPosition, float heading);
	void patrol();
	void patrol(vector<Vector3> nodes);
	void alert();
	void search();
	void search(Vector3 aroundWhere, float radius = GUARD_SUSPECT_RANGE);
	void combat();
	void addEnemyBlip();
	void removeBlip();

protected:
	void setMode(TensionMode mode);
	Vector3 getDefensePosition();
	float getRadius();
	bool shouldTolerate();
	void setShouldTolerate(bool flag);
	IdlingModifier getIdlingModifier();
	char* generateScoutingScenario();
	bool isPlayerWithinLos();
	void detectHighProfileEventAround();
	bool shouldIgnoreBody(Ped body);

	virtual void enterIdleMode();
	virtual void enterSuspectionMode();
	virtual void enterAlertedMode();
	virtual void enterWarningMode();
	virtual void enterSearchMode(Vector3 aroundWhere, float radius = GUARD_SUSPECT_RANGE);
	virtual void enterCombatMode();

private:
	void setIdlingModifier(IdlingModifier idlingModifier);
};