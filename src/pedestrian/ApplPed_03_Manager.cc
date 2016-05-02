/****************************************************************************/
/// @file    ApplPed_03_Manager.cc
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

#include "ApplPed_03_Manager.h"

namespace VENTOS {

const simsignalwrap_t ApplPedManager::mobilityStateChangedSignal = simsignalwrap_t(MIXIM_SIGNAL_MOBILITY_CHANGE_NAME);

Define_Module(VENTOS::ApplPedManager);

ApplPedManager::~ApplPedManager()
{

}


void ApplPedManager::initialize(int stage)
{
    super::initialize(stage);

	if (stage == 0)
	{

        // findHost()->subscribe(mobilityStateChangedSignal, this);
	}
}


void ApplPedManager::finish()
{
    super::finish();

    // findHost()->unsubscribe(mobilityStateChangedSignal, this);
}


void ApplPedManager::receiveSignal(omnetpp::cComponent* source, omnetpp::simsignal_t signalID, cObject* obj, cObject* details)
{
    Enter_Method_Silent();

    if (signalID == mobilityStateChangedSignal)
    {
        handlePositionUpdate(obj);
    }
}


void ApplPedManager::handleSelfMsg(omnetpp::cMessage* msg)
{
    super::handleSelfMsg(msg);
}


void ApplPedManager::handleLowerMsg(omnetpp::cMessage* msg)
{
    // make sure msg is of type WaveShortMessage
    Veins::WaveShortMessage* wsm = dynamic_cast<Veins::WaveShortMessage*>(msg);
    ASSERT(wsm);

    if (std::string(wsm->getName()) == "beaconVehicle")
    {
        BeaconVehicle* wsm = dynamic_cast<BeaconVehicle*>(msg);
        ASSERT(wsm);
    }
    else if (std::string(wsm->getName()) == "beaconRSU")
    {
        BeaconRSU* wsm = dynamic_cast<BeaconRSU*>(msg);
        ASSERT(wsm);
    }
    else if(std::string(wsm->getName()) == "platoonMsg")
    {
        PlatoonMsg* wsm = dynamic_cast<PlatoonMsg*>(msg);
        ASSERT(wsm);

        ApplPedManager::onData(wsm);
    }

    delete msg;
}


// is called, every time the position of vehicle changes
void ApplPedManager::handlePositionUpdate(cObject* obj)
{
    // pass it down
    super::handlePositionUpdate(obj);

    ChannelMobilityPtrType const mobility = omnetpp::check_and_cast<ChannelMobilityPtrType>(obj);
    curPosition = mobility->getCurrentPosition();
}


void ApplPedManager::onBeaconVehicle(BeaconVehicle* wsm)
{
    // pass it down
    //super::onBeaconVehicle(wsm);
}


void ApplPedManager::onBeaconRSU(BeaconRSU* wsm)
{
    // pass it down
    //super::onBeaconRSU(wsm);
}


void ApplPedManager::onData(PlatoonMsg* wsm)
{
    // pass it down
    //super::onData(wsm);
}


}

