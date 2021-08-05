#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include "main.h"
#include "actors.h"
#include "buildings.h"
#include "objects.h"
#include <future>
#include <mutex>
#include "gametext.h"
#include "projectile.h"
#include "orderCursor.h"
#include "player.h"
#include "simpleAI.h"

gameState currentGame;

void routeHelper(int i)
{
    if (listOfActors[i].isInitialized())
    {
        listOfActors[i].calculateRoute();
    }
}

void updateActorHelper(int i)
{
    if(listOfActors[i].isInitialized())
    {
        listOfActors[i].update();
    }
}


void updateBuildingsHelper(int i)
{
    listOfBuildings[i].update();
}

void projectileHelper(int i) {
    listOfProjectiles[i].updatePosition();
}

void clearOldCommandCursors()
{
    if (!listOfOrderCursors.empty()) {
        auto iter = std::find_if(listOfOrderCursors.begin(), listOfOrderCursors.end(),
            [&](orderCursor& p) {return p.isFinished(); });
        if (iter != listOfOrderCursors.end())
            listOfOrderCursors.erase(iter);
    }
}

void updateGameState(int& lastActor, int& lastBuilding, int& lastPath, int& lastProjectile){
    for (int i = 0; i < currentGame.getPlayerCount(); i++) {
        listOfPlayers[i].clearLists();

    }
    for (int n = 0; n < listOfActors.size(); n++) {
        if (listOfActors[n].isAlive()) {
            if (listOfActors[n].getType() == 0) {
                listOfPlayers[listOfActors[n].getTeam()].insertVillagerList(n);
                if (listOfActors[n].idle()) {
                    listOfPlayers[listOfActors[n].getTeam()].insertIdIntoIdleVillagerList(n);
                }
                else if (listOfActors[n].getIsBuilding()) {
                    listOfPlayers[listOfActors[n].getTeam()].insertBuilding(n);
                }
                else if (listOfActors[n].isGathering()) {
                    switch (listOfActors[n].getResourceGathered()) {
                    case resourceTypes::resourceWood: //wood
                        listOfPlayers[listOfActors[n].getTeam()].insertGatheringWood(n);
                        break;
                    case resourceTypes::resourceFood: //food
                        listOfPlayers[listOfActors[n].getTeam()].insertGatheringFood(n);
                        break;
                    case resourceTypes::resourceStone: //stone
                        listOfPlayers[listOfActors[n].getTeam()].insertGatheringStone(n);
                        break;
                    case resourceTypes::resourceGold: //gold
                        listOfPlayers[listOfActors[n].getTeam()].insertGatheringGold(n);
                        break;
                    }
                }
            }
            else if (listOfActors[n].getType() == 0) {
                listOfPlayers[listOfActors[n].getTeam()].insertSwordsman(n);
            }
        }
    }    
    if (!listOfAI.empty()) {
        for (int i = 0; i < listOfAI.size(); i++) {
            listOfAI[i].update();
        }
    }
    if (!listOfProjectiles.empty()) {
        int endProjectile = lastProjectile + 100;
        if (endProjectile > listOfProjectiles.size()) {
            endProjectile = static_cast<int>(listOfProjectiles.size());
        }
        //update porjectile positions
        for (int i = lastProjectile; i < endProjectile; i++) {
            auto projectileCalculations = std::async(std::launch::async, projectileHelper, i);
        }
        //Erase old projectiles (older then 30 sec)
        auto iter = std::find_if(listOfProjectiles.begin(), listOfProjectiles.end(),
            [&](projectile& p) {return p.getTimeLastUpdate() + 30.0f < currentGame.getTime(); });
        if (iter != listOfProjectiles.end())
            listOfProjectiles.erase(iter);
        if (endProjectile == static_cast<int>(listOfProjectiles.size())) {
            lastProjectile = 0;
        }
        else {
            lastProjectile = endProjectile;
        }
    }
    if (!listOfActors.empty())
    {
        if (!listOfActorsWhoNeedAPath.empty())
        {
            int begin = static_cast<int>(listOfActorsWhoNeedAPath.size());
            int endPath = lastPath + 5;
            if (endPath > static_cast<int>(listOfActorsWhoNeedAPath.size())) {
                endPath = static_cast<int>(listOfActorsWhoNeedAPath.size());
            }
            for (int i = lastPath; i < endPath; i++){
                auto routing = std::async(std::launch::async, routeHelper, listOfActorsWhoNeedAPath[i]);
            }
            int after = static_cast<int>(listOfActorsWhoNeedAPath.size());
            if (endPath == listOfActorsWhoNeedAPath.size()) {
                lastPath = 0;
                listOfActorsWhoNeedAPath.clear();
            }
            else {
               lastPath = endPath;
            }
        }

        int endActor = lastActor + 25;
        if (endActor > static_cast<int>(listOfActors.size())) {
            endActor = static_cast<int>(listOfActors.size());
        }
        for (int i = lastActor; i < endActor; i++)
        {
            auto actorUpdater = std::async(std::launch::async, updateActorHelper, i);
        }
        if (endActor == static_cast<int>(listOfActors.size())) {
            lastActor = 0;
        }
        else {
            lastActor = endActor;
        }
    }
    if (!listOfBuildings.empty())
    {
        int endBuilding = lastBuilding + 10;
        if (endBuilding > static_cast<int>(listOfBuildings.size())) {
            endBuilding = static_cast<int>(listOfBuildings.size());
        }
        for (int i = lastBuilding; i < endBuilding; i++)
        {
            auto buildingUpdater = std::async(std::launch::async, updateBuildingsHelper, i);
        }
        if (endBuilding == static_cast<int>(listOfBuildings.size())) {
            lastBuilding = 0;
        }
        else {
            lastBuilding = endBuilding;
        }
    }
  
}

int main()
{
    sf::Clock clockMain;
    int lastActor =0;
    int lastBuilding=0;
    int lastPath=0;
    int lastProjectile=0;
    currentGame.loadGame();
    for (int i = 0; i < currentGame.getPlayerCount() - 1; i++) {
        simpleAI newAIPlayer(i+1, 0);
        listOfAI.push_back(newAIPlayer);
    }
    while(window.isOpen())
    {
        sf::Time elapsedMain = clockMain.getElapsedTime();
        currentGame.elapsedTime = elapsedMain.asSeconds();
        gameText.throwOutOldMessages();
        clearOldCommandCursors();
        currentGame.interact();
        updateGameState(lastActor, lastBuilding, lastPath, lastProjectile);
        currentGame.drawGame();
    }
    return 0;
}
