/****************************************************************************/
/// @file    ApplV_09_Dissolve.cc
/// @author  Mani Amoozadeh <maniam@ucdavis.edu>
/// @author  second author name
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

#include "ApplV_02_PlatoonMg.h"

namespace VENTOS {

void ApplVPlatoonMg::dissolve_handleSelfMsg(omnetpp::cMessage* msg)
{
    if(msg == plnTIMER12)
    {
        if(vehicleState == state_waitForDissolveAck)
        {
            vehicleState = state_sendDissolve;
            reportStateToStat();

            dissolve_DataFSM();
        }
    }


}


void ApplVPlatoonMg::dissolve_BeaconFSM(BeaconVehicle* wsm)
{


}


void ApplVPlatoonMg::dissolve_DataFSM(PlatoonMsg* wsm)
{
    if(vehicleState == state_sendDissolve)
    {
        // get the last vehicle
        std::deque<std::string>::iterator it = plnMembersList.end() - 1;
        lastVeh = *it;

        // send a unicast DISSOLVE message my follower
        PlatoonMsg* dataMsg = prepareData(lastVeh, DISSOLVE, plnID);
        EV << "### " << SUMOID << ": sent DISSOLVE to followers." << std::endl;
        sendDelayed(dataMsg, individualOffset, lowerLayerOut);
        reportCommandToStat(dataMsg);

        vehicleState = state_waitForDissolveAck;
        reportStateToStat();

        scheduleAt(omnetpp::simTime() + 1., plnTIMER12);
    }
    else if(vehicleState == state_waitForDissolveAck)
    {
        if (wsm->getType() == ACK && wsm->getSender() == lastVeh)
        {
            cancelEvent(plnTIMER12);

            plnMembersList.pop_back();

            if(plnMembersList.size() == 1)
            {
                vehicleState = state_platoonLeader;
                reportStateToStat();
            }

            vehicleState = state_sendDissolve;
            reportStateToStat();

            dissolve_DataFSM();
        }
    }
    else if(vehicleState == state_platoonFollower)
    {
        if ( wsm->getType() == DISSOLVE && wsm->getSender() == plnID && wsm->getRecipient() == SUMOID )
        {
            // send ACK
            PlatoonMsg* dataMsg = prepareData(wsm->getSender(), ACK, wsm->getSendingPlatoonID());
            EV << "### " << SUMOID << ": sent ACK." << std::endl;
            sendDelayed(dataMsg, individualOffset, lowerLayerOut);
            reportCommandToStat(dataMsg);

            // make it a free agent
            plnID = SUMOID;
            myPlnDepth = 0;
            plnSize = 1;
            plnMembersList.push_back(SUMOID);
            TraCI->vehicleSetTimeGap(SUMOID, TP);

            busy = false;

            // change color to red!
            RGB newColor = Color::colorNameToRGB("red");
            TraCI->vehicleSetColor(SUMOID, newColor);

            vehicleState = state_platoonLeader;
            reportStateToStat();
        }
    }
}

}

