/****************************************************************************/
/// @file    TL_LongestQueueNoStarv.cc
/// @author  Mani Amoozadeh <maniam@ucdavis.edu>
/// @date    August 2013
///
/****************************************************************************/
// VENTOS, Vehicular Network Open Simulator; see http:?
// Copyright (C) 2013-2015
/****************************************************************************/
//
// This file is part of VENTOS.
// VENTOS is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <08_TL_LongestQueue_NoStarv.h>
#include <queue>

namespace VENTOS {

Define_Module(VENTOS::TrafficLightLongestQueueNoStarv);

class sortedEntryQ
{
public:
    int oneCount;
    int totalQueue;
    int maxVehCount;
    std::vector<int> batchMovements;

    sortedEntryQ(int i1, int i2, int i3, std::vector<int> bm)
    {
        this->oneCount = i1;
        this->totalQueue = i2;
        this->maxVehCount = i3;
        batchMovements.swap(bm);
    }
};


class sortCompareQ
{
public:
    bool operator()(sortedEntryQ p1, sortedEntryQ p2)
    {
        if(p1.totalQueue < p2.totalQueue)
            return true;
        else if(p1.totalQueue == p2.totalQueue && p1.oneCount < p2.oneCount)
            return true;
        else
            return false;
    }
};


// using a 'functor' rather than a 'function'
// Reason: to be able to pass an additional argument (bestMovement) to predicate
struct servedLQ
{
public:
    servedLQ(std::vector<int> best)
{
        bestMovement.swap(best);
}

    bool operator () (const sortedEntryQ v)
    {
        for (unsigned int linkNumber = 0; linkNumber < v.batchMovements.size(); linkNumber++)
        {
            // Ignore all permissive right turns since these are always green:
            int rightTurns[8] = {0, 2, 5, 7, 10, 12, 15, 17};
            bool rightTurn = std::find(std::begin(rightTurns), std::end(rightTurns), linkNumber) != std::end(rightTurns);

            // Want to remove movements that have already been done:
            if (!rightTurn && v.batchMovements[linkNumber] == 1 && bestMovement[linkNumber] == 1)
                return true;
        }

        return false;
    }

private:
    std::vector<int> bestMovement;
};


TrafficLightLongestQueueNoStarv::~TrafficLightLongestQueueNoStarv()
{

}


void TrafficLightLongestQueueNoStarv::initialize(int stage)
{
    TrafficLightActuated::initialize(stage);

    if(TLControlMode != TL_Adaptive_Time_Queue)
        return;

    if(stage == 0)
    {
        maxQueueSize = par("maxQueueSize").longValue();

        if(maxQueueSize <= 0 && maxQueueSize != -1)
            error("maxQueueSize value is set incorrectly!");

        nextGreenIsNewCycle = false;
        ChangeEvt = new cMessage("ChangeEvt", 1);

        // collect queue information
        measureIntersectionQueue = true;
    }
}


void TrafficLightLongestQueueNoStarv::finish()
{
    TrafficLightActuated::finish();
}


void TrafficLightLongestQueueNoStarv::handleMessage(cMessage *msg)
{
    TrafficLightActuated::handleMessage(msg);

    if(TLControlMode != TL_Adaptive_Time_Queue)
        return;

    if (msg == ChangeEvt)
    {
        if(greenInterval.empty())
            calculatePhases("C");

        chooseNextInterval();

        if(intervalOffSet <= 0)
            error("intervalOffSet is <= 0");

        // Schedule next light change event:
        scheduleAt(simTime().dbl() + intervalOffSet, ChangeEvt);
    }
}


void TrafficLightLongestQueueNoStarv::executeFirstTimeStep()
{
    TrafficLightActuated::executeFirstTimeStep();

    if(TLControlMode != TL_Adaptive_Time_Queue)
        return;

    std::cout << "Longest Queue traffic signal control ..." << endl << endl;

    // get all non-conflicting movements in allMovements vector
    TrafficLightAllowedMoves::getMovements("C");

    // calculate phases at the beginning of the cycle
    calculatePhases("C");

    // set initial settings:
    currentInterval = greenInterval.front().greenString;
    intervalOffSet = greenInterval.front().greenTime;
    intervalElapseTime = 0;

    scheduleAt(simTime().dbl() + intervalOffSet, ChangeEvt);

    for (auto &TL :TLList)
    {
        TraCI->TLSetProgram(TL, "adaptive-time");
        TraCI->TLSetState(TL, currentInterval);

        // initialize TL status
        updateTLstate(TL, "init", currentInterval);
    }

    if(ev.isGUI() && debugLevel > 0)
    {
        char buff[300];
        sprintf(buff, "SimTime: %4.2f | Planned interval: %s | Start time: %4.2f | End time: %4.2f", simTime().dbl(), currentInterval.c_str(), simTime().dbl(), simTime().dbl() + intervalOffSet);
        std::cout << buff << endl << endl;
        std::cout.flush();
    }
}


void TrafficLightLongestQueueNoStarv::executeEachTimeStep()
{
    TrafficLightActuated::executeEachTimeStep();

    if(TLControlMode != TL_Adaptive_Time_Queue)
        return;

    intervalElapseTime += updateInterval;
}


void TrafficLightLongestQueueNoStarv::chooseNextInterval()
{
    if (currentInterval == "yellow")
    {
        currentInterval = "red";

        // change all 'y' to 'r'
        std::string str = TraCI->TLGetState("C");
        std::string nextInterval = "";
        for(char& c : str) {
            if (c == 'y')
                nextInterval += 'r';
            else
                nextInterval += c;
        }

        // set the new state
        TraCI->TLSetState("C", nextInterval);
        intervalElapseTime = 0.0;
        intervalOffSet = redTime;

        // update TL status for this phase
        updateTLstate("C", "red");
    }
    else if (currentInterval == "red")
    {
        if(nextGreenIsNewCycle)
        {
            updateTLstate("C", "phaseEnd", nextGreenInterval, true);  // a new cycle
            nextGreenIsNewCycle = false;
        }
        else
            updateTLstate("C", "phaseEnd", nextGreenInterval);

        currentInterval = nextGreenInterval;

        // set the new state
        TraCI->TLSetState("C", nextGreenInterval);
        intervalElapseTime = 0.0;
        intervalOffSet = greenInterval.front().greenTime;
    }
    else
        chooseNextGreenInterval();

    if(ev.isGUI() && debugLevel > 0)
    {
        char buff[300];
        sprintf(buff, "SimTime: %4.2f | Planned interval: %s | Start time: %4.2f | End time: %4.2f", simTime().dbl(), currentInterval.c_str(), simTime().dbl(), simTime().dbl() + intervalOffSet);
        std::cout << buff << endl << endl;
        std::cout.flush();
    }
}


void TrafficLightLongestQueueNoStarv::chooseNextGreenInterval()
{
    // Remove current old phase:
    greenInterval.erase(greenInterval.begin());

    // Assign new green:
    if (greenInterval.empty())
    {
        calculatePhases("C");
        nextGreenIsNewCycle = true;
    }

    nextGreenInterval = greenInterval.front().greenString;

    // calculate 'next interval'
    std::string nextInterval = "";
    bool needYellowInterval = false;  // if we have at least one yellow interval
    for(unsigned int linkNumber = 0; linkNumber < currentInterval.size(); ++linkNumber)
    {
        if( (currentInterval[linkNumber] == 'G' || currentInterval[linkNumber] == 'g') && nextGreenInterval[linkNumber] == 'r')
        {
            nextInterval += 'y';
            needYellowInterval = true;
        }
        else
            nextInterval += currentInterval[linkNumber];
    }

    if(needYellowInterval)
    {
        currentInterval = "yellow";
        TraCI->TLSetState("C", nextInterval);

        intervalElapseTime = 0.0;
        intervalOffSet =  yellowTime;

        // update TL status for this phase
        updateTLstate("C", "yellow");
    }
    // extend the current green interval
    else
    {
        intervalOffSet = greenInterval.front().greenTime;

        if(ev.isGUI() && debugLevel > 0)
        {
            std::cout << ">>> Continue the last green interval." << endl << endl;
            std::cout.flush();
        }
    }
}


// calculate all phases (up to 4)
void TrafficLightLongestQueueNoStarv::calculatePhases(std::string TLid)
{
    if(ev.isGUI() && debugLevel > 1)
    {
        std::cout << "Queue size per lane: ";
        for(auto& entry : laneQueueSize)
        {
            std::string lane = entry.first;
            int qSize = entry.second.second;
            if(qSize != 0)
                std::cout << lane << " (" << qSize << ") | ";
        }
        std::cout << endl << endl;
    }

    // batch of all non-conflicting movements, sorted by total queue size per batch
    std::priority_queue< sortedEntryQ /*type of each element*/, std::vector<sortedEntryQ> /*container*/, sortCompareQ > sortedMovements;

    // clear the priority queue
    sortedMovements = std::priority_queue < sortedEntryQ, std::vector<sortedEntryQ>, sortCompareQ >();

    for(unsigned int i = 0; i < allMovements.size(); ++i)  // row
    {
        int totalQueueRow = 0;  // total queue size for this batch of movements
        int oneCount = 0;

        // for each batch of movement get
        // - 'total queue size' : total queue size
        // - 'one count'        : # of allowed movements (except right turns)
        // - 'maxVehCount'      : max queue size
        int maxVehCount = 0;
        for(unsigned int j = 0; j < allMovements[i].size(); ++j)  // column (link number)
        {
            // if a right turn
            bool rightTurn = std::find(std::begin(rightTurns), std::end(rightTurns), j) != std::end(rightTurns);

            if(allMovements[i][j] == 1 && !rightTurn)
            {
                int queueSize = linkQueueSize[std::make_pair(TLid,j)];
                int queueSizeBounded = (maxQueueSize == -1) ? queueSize : std::min(maxQueueSize,queueSize);

                totalQueueRow = totalQueueRow + queueSizeBounded;
                oneCount++;
                maxVehCount = std::max(maxVehCount, queueSizeBounded);
            }
        }

        // add this batch of movements to priority_queue
        sortedEntryQ *entry = new sortedEntryQ(oneCount, totalQueueRow, maxVehCount, allMovements[i]);
        sortedMovements.push(*entry);
    }

    // copy sortedMovements to a vector for iteration:
    std::vector<sortedEntryQ> batchMovementVector;
    while(!sortedMovements.empty())
    {
        batchMovementVector.push_back(sortedMovements.top());
        sortedMovements.pop();
    }

    // Select only the necessary phases for the new cycle:
    while(!batchMovementVector.empty())
    {
        // Always select the first movement because it will be the best(?):
        std::vector<int> bestMovement = batchMovementVector.front().batchMovements;

        // calculate the next green interval.
        // right-turns are all permissive and are given 'g'
        std::string nextInterval = "";
        for(unsigned linkNumber = 0; linkNumber < bestMovement.size(); ++linkNumber)
        {
            // if a right turn
            bool rightTurn = std::find(std::begin(rightTurns), std::end(rightTurns), linkNumber) != std::end(rightTurns);

            if(bestMovement[linkNumber] == 0)
                nextInterval += 'r';
            else if(bestMovement[linkNumber] == 1 && rightTurn)
                nextInterval += 'g';
            else if(bestMovement[linkNumber] == 1 && !rightTurn)
                nextInterval += 'G';
        }

        // todo: is this necessary?
        // Avoid pushing "only permissive right turns" phase:
        if (nextInterval == "grgrrgrgrrgrgrrgrgrrrrrr")
        {
            batchMovementVector.erase(batchMovementVector.begin());
            continue;
        }

        greenIntervalInfo_Maxqueue *entry = new greenIntervalInfo_Maxqueue(batchMovementVector.front().maxVehCount, 0.0, nextInterval);
        greenInterval.push_back(*entry);

        // Now delete these movements because they should never occur again:
        batchMovementVector.erase( std::remove_if(batchMovementVector.begin(), batchMovementVector.end(), servedLQ(bestMovement)), batchMovementVector.end() );
    }

    // calculate number of vehicles in the intersection
    int vehCountIntersection = 0;
    for (auto &i : greenInterval)
        vehCountIntersection += i.maxVehCount;

    // If intersection is empty, then run each green interval with minGreenTime:
    if (vehCountIntersection == 0)
    {
        for (auto &i : greenInterval)
            i.greenTime = minGreenTime;
    }
    else
    {
        for (auto &i : greenInterval)
            i.greenTime = (double)i.maxVehCount * (minGreenTime / 5.);
    }

    // If no green time (0s) is given to a phase, then this queue is empty and useless:
    int oldSize = greenInterval.size();
    auto rme = std::remove_if(greenInterval.begin(), greenInterval.end(),
            [](const greenIntervalInfo_Maxqueue v)
            {
        if (v.greenTime == 0.0)
            return true;
        else
            return false;
            }
    );
    greenInterval.erase( rme, greenInterval.end() );

    // throw error if cycle contains more than 4 phases:
    if (greenInterval.size() > 4)
        error("cycle contains %d phases which is more than 4!", greenInterval.size());

    int newSize = greenInterval.size();
    if(ev.isGUI() && debugLevel > 1 && oldSize != newSize)
    {
        std::cout << ">>> " << oldSize - newSize << " phase(s) removed due to zero queue size!" << endl << endl;
        std::cout.flush();
    }

    // make sure the green splits are bounded
    for (auto &i : greenInterval)
        i.greenTime = std::min(std::max(i.greenTime, minGreenTime), maxGreenTime);

    if(ev.isGUI() && debugLevel > 1)
    {
        std::cout << "Selected green intervals for this cycle: " << endl;
        for (auto &i : greenInterval)
            std::cout << "Movement " << i.greenString
            << " with maxVehCount of " << i.maxVehCount
            << " for " << i.greenTime << "s" << endl;

        std::cout << endl;
        std::cout.flush();
    }
}

}