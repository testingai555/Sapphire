#include "StateCombat.h"
#include "Actor/BNpc.h"
#include "Logging/Logger.h"
#include <Service.h>

#include <Manager/TerritoryMgr.h>
#include <Territory/Territory.h>
#include <Navi/NaviProvider.h>

using namespace Sapphire::World;

void AI::Fsm::StateCombat::onUpdate(Entity::BNpc& bnpc, uint64_t tickCount)
{
  auto& teriMgr = Common::Service<World::Manager::TerritoryMgr>::ref();
  auto pZone = teriMgr.getTerritoryByGuId(bnpc.getTerritoryId());
  auto pNaviProvider = pZone->getNaviProvider();

  // 🔥 Add player to hate list if valid
  auto pPlayer = pZone->getPlayer(); // Replace this if your system uses a different method
  if (pPlayer && pPlayer->isAlive() && bnpc.getTerritoryId() == pPlayer->getTerritoryId())
  {
    if (!bnpc.hateListContains(pPlayer))
    {
      constexpr int hateValue = 100; // Choose an appropriate value for your hate system
      bnpc.hateListAdd(pPlayer, hateValue);
      Sapphire::Logging::Logger::info("AI", "Added player {} to hate list of NPC {}", pPlayer->getName(), bnpc.getName());
    }
  }

  auto pHatedActor = bnpc.hateListGetHighest();
  if (!pHatedActor)
    return;

  pNaviProvider->updateAgentParameters(bnpc);

  auto distanceOrig = Common::Util::distance(bnpc.getPos(), bnpc.getSpawnPos());

  if (!pHatedActor->isAlive() || bnpc.getTerritoryId() != pHatedActor->getTerritoryId())
  {
    bnpc.hateListRemove(pHatedActor);
    pHatedActor = bnpc.hateListGetHighest();
  }

  if (!pHatedActor)
    return;

  auto distance = Common::Util::distance(bnpc.getPos(), pHatedActor->getPos());

  if (!bnpc.hasFlag(Entity::NoDeaggro))
  {
    // TODO: Insert deaggro logic if required
  }

  if (!bnpc.hasFlag(Entity::Immobile) &&
      distance > (bnpc.getNaviTargetReachedDistance() + pHatedActor->getRadius()))
  {
    if (pNaviProvider)
      pNaviProvider->setMoveTarget(bnpc, pHatedActor->getPos());

    bnpc.moveTo(*pHatedActor);
  }

  if (pNaviProvider->syncPosToChara(bnpc))
    bnpc.sendPositionUpdate();

  if (distance < (bnpc.getNaviTargetReachedDistance() + pHatedActor->getRadius()))
  {
    if (!bnpc.hasFlag(Entity::TurningDisabled) && bnpc.face(pHatedActor->getPos()))
      bnpc.sendPositionUpdate();

    if (!bnpc.checkAction())
      bnpc.processGambits(tickCount);

    // In combat range — attack
    bnpc.autoAttack(pHatedActor);
  }
}

void AI::Fsm::StateCombat::onEnter(Entity::BNpc& bnpc)
{
  // Optional: Initialize combat state
}

void AI::Fsm::StateCombat::onExit(Entity::BNpc& bnpc)
{
  bnpc.hateListClear();
  bnpc.changeTarget(Common::INVALID_GAME_OBJECT_ID64);
  bnpc.setStance(Common::Stance::Passive);
  bnpc.setOwner(nullptr);
}
