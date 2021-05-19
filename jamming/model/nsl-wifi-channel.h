/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#ifndef NSL_WIFI_CHANNEL_H
#define NSL_WIFI_CHANNEL_H

#include <vector>
#include <stdint.h>
#include "ns3/packet.h"
#include "ns3/wifi-mode.h"
#include "ns3/wifi-preamble.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/channel.h"
#include "ns3/mpdu-aggregator.h"

namespace ns3 {

class NetDevice;
class PropagationLossModel;
class PropagationDelayModel;
class NslWifiPhy;
class Packet;
class Time;

/**
 * This class is almost identical to YansWifiChannel. It is introduced together
 * with NslWifiPhy.
 */
class NslWifiChannel : public Channel 
{
public:
  static TypeId GetTypeId (void);
  NslWifiChannel ();
  virtual ~NslWifiChannel ();

  // inherited from Channel.
  virtual std::size_t GetNDevices (void) const;
  virtual Ptr<NetDevice> GetDevice (std::size_t i) const;

  void Add (Ptr<NslWifiPhy> phy);

  /**
   * \param loss the new propagation loss model.
   */
  void SetPropagationLossModel (Ptr<PropagationLossModel> loss);

  /**
   * \param delay the new propagation delay model.
   */
  void SetPropagationDelayModel (Ptr<PropagationDelayModel> delay);

  /**
   * \param sender the device from which the packet is originating.
   * \param packet the packet to send
   * \param txPowerDbm the tx power associated to the packet
   * \param duration
   *
   * This method should not be invoked by normal users. It is currently invoked
   * only from WifiPhy::Send. NslWifiChannel delivers packets only between
   * PHYs with the same m_channelNumber, e.g. PHYs that are operating on the
   * same channel.
   */
  void Send (Ptr<NslWifiPhy> sender, Ptr<const Packet> packet,
             double txPowerDbm, WifiMode wifiMode, WifiPreamble preamble,WifiTxVector txVector) const;

 // void Send (Ptr<NslWifiPhy> sender, Ptr<const Packet> packet, double txPowerDbm, Time duration) const;

  int64_t AssignStreams (int64_t stream);

private:
  typedef std::vector<Ptr<NslWifiPhy> > PhyList;
  void Receive (Ptr<NslWifiPhy> receiver, Ptr<Packet> packet, double rxPowerDbm,
                WifiMode wifiMode, WifiPreamble preamble,WifiTxVector txVector) const;

  // static void Receive (Ptr<YansWifiPhy> receiver, Ptr<Packet> packet, double txPowerDbm, Time duration);

  PhyList m_phyList;
  Ptr<PropagationLossModel> m_loss;
  Ptr<PropagationDelayModel> m_delay;
};

} // namespace ns3


#endif /* NSL_WIFI_CHANNEL_H */
