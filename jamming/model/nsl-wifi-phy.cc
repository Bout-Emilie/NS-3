/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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

#include "nsl-wifi-phy.h"
#include "wireless-module-utility.h"
#include "nsl-wifi-channel.h"
#include "ns3/wifi-mode.h"
#include "ns3/wifi-preamble.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy-state-helper.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/error-rate-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/energy-source.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/pointer.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/wifi-phy-standard.h"
#include "ns3/wifi-tx-vector.h"
#include "ns3/ampdu-tag.h"
#include "ns3/wifi-phy-tag.h"
#include "ns3/ampdu-tag.h"
#include "ns3/frame-capture-model.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NslWifiPhy");
NS_OBJECT_ENSURE_REGISTERED (NslWifiPhy);



TypeId
NslWifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NslWifiPhy")
    .SetParent<WifiPhy> ()
    .SetGroupName ("Wifi")
    .AddConstructor<NslWifiPhy> ()
    .AddAttribute ("NslEnergyDetectionThreshold",
                   "The energy of a received signal should be higher than "
                   "this threshold (dBm) to allow the PHY layer to detect the signal.",
                   DoubleValue (-140.0),
                   MakeDoubleAccessor (&NslWifiPhy::SetEdThreshold,
                                       &NslWifiPhy::GetEdThreshold),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NslCcaMode1Threshold",
                   "The energy of a received signal should be higher than "
                   "this threshold (dBm) to allow the PHY layer to declare CCA BUSY state",
                   DoubleValue (-140.0),
                   MakeDoubleAccessor (&NslWifiPhy::SetCcaMode1Threshold,
                                       &NslWifiPhy::GetCcaMode1Threshold),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NslTxGain",
                   "Transmission gain (dB).",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&NslWifiPhy::SetTxGain,
                                       &NslWifiPhy::GetTxGain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NslRxGain",
                   "Reception gain (dB).",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&NslWifiPhy::SetRxGain,
                                       &NslWifiPhy::GetRxGain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NslTxPowerLevels",
                   "Number of transmission power levels available between "
                   "TxPowerBase and TxPowerEnd included.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&NslWifiPhy::m_nTxPower),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NslTxPowerEnd",
                   "Maximum available transmission level (dBm).",
                   DoubleValue (16.0206),
                   MakeDoubleAccessor (&NslWifiPhy::SetTxPowerEnd,
                                       &NslWifiPhy::GetTxPowerEnd),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NslTxPowerStart",
                   "Minimum available transmission level (dBm).",
                   DoubleValue (16.0206),
                   MakeDoubleAccessor (&NslWifiPhy::SetTxPowerStart,
                                       &NslWifiPhy::GetTxPowerStart),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NslRxNoiseFigure",
                   "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                   " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                   "\"the difference in decibels (dB) between"
                   " the noise output of the actual receiver to the noise output of an "
                   " ideal receiver with the same overall gain and bandwidth when the receivers "
                   " are connected to sources at the standard noise temperature T0 (usually 290 K)\"."
                   " For",
                   DoubleValue (7),
                   MakeDoubleAccessor (&NslWifiPhy::SetRxNoiseFigure,
                                       &NslWifiPhy::GetRxNoiseFigure),
                   MakeDoubleChecker<double> ())
    /*.AddAttribute ("NslState", "The state of the PHY layer",
                   PointerValue (),
                   MakePointerAccessor (&NslWifiPhy::m_state),
                   MakePointerChecker<WifiPhyStateHelper> ())*/
    .AddAttribute ("NslChannelSwitchDelay",
                   "Delay between two short frames transmitted on different frequencies. NOTE: Unused now.",
                   TimeValue (MicroSeconds (250)),
                   MakeTimeAccessor (&NslWifiPhy::m_channelSwitchDelay),
                   MakeTimeChecker ())
    .AddAttribute ("NslChannelNumber",
                   "Channel center frequency = Channel starting frequency + 5 MHz * (nch - 1)",
                   UintegerValue (1),
                   MakeUintegerAccessor (&NslWifiPhy::SetChannelNumber,
                                         &NslWifiPhy::GetChannelNumber),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("NslFrequency", "The operating frequency.",
                     UintegerValue (2407),
                     MakeUintegerAccessor (&NslWifiPhy::GetFrequency,
                                          &NslWifiPhy::SetFrequency),
                     MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NslTransmitters", "The number of transmitters.",
                      UintegerValue (1),
                      MakeUintegerAccessor (&NslWifiPhy::GetNumberOfTransmitAntennas,
                                          &NslWifiPhy::SetNumberOfTransmitAntennas),
                      MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("NslReceivers", "The number of receivers.",
                      UintegerValue (1),
                      MakeUintegerAccessor (&NslWifiPhy::GetNumberOfReceiveAntennas,
                                           &NslWifiPhy::SetNumberOfReceiveAntennas),
                      MakeUintegerChecker<uint32_t> ())
      .AddAttribute ("NslShortGuardEnabled", "Whether or not short guard interval is enabled.",
                      BooleanValue (false),
                      MakeBooleanAccessor (&NslWifiPhy::GetGuardInterval,
                                           &NslWifiPhy::SetGuardInterval),
                      MakeBooleanChecker ())
       .AddAttribute ("NslLdpcEnabled", "Whether or not LDPC is enabled.",
                      BooleanValue (false),
                      MakeBooleanAccessor (&NslWifiPhy::GetLdpc,
                                           &NslWifiPhy::SetLdpc),
                      MakeBooleanChecker ())
       .AddAttribute ("NslSTBCEnabled", "Whether or not STBC is enabled.",
                      BooleanValue (false),
                      MakeBooleanAccessor (&NslWifiPhy::GetStbc,
                                           &NslWifiPhy::SetStbc),
                      MakeBooleanChecker ())
       .AddAttribute ("NslGreenfieldEnabled", "Whether or not STBC is enabled.",
                      BooleanValue (false),
                      MakeBooleanAccessor (&NslWifiPhy::GetGreenfield,
                                           &NslWifiPhy::SetGreenfield),
                      MakeBooleanChecker ())
       .AddAttribute ("NslChannelBonding", "Whether 20MHz or 40MHz.",
                      BooleanValue (false),
                      MakeBooleanAccessor (&NslWifiPhy::GetChannelBonding,
                                           &NslWifiPhy::SetChannelBonding),
                      MakeBooleanChecker ())
    ;
  return tid;
}

NslWifiPhy::NslWifiPhy ()
  :  m_channelNumber (1),
     m_endRxEvent (),
     m_channelStartingFrequency (0),
     m_mpdusNum(0),
     m_plcpSuccess(false),
     m_isDriverInitialized(false)
{
  NS_LOG_FUNCTION (this);
 // m_state = CreateObject<WifiPhyStateHelper> ();
  //m_random = CreateObject<UniformRandomVariable>();
  m_txVector = WifiTxVector();
  ResetDriver ();
}

NslWifiPhy::~NslWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
NslWifiPhy::DoStart (void)
{
  NS_LOG_FUNCTION (this);
  InitDriver ();  // initialize driver at beginning of simulation
}

void
NslWifiPhy::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
 // m_device = 0;
 // m_mobility = 0;
  //m_state = 0;
  m_node =0;
  ResetDriver (); // driver not initialized
  NS_LOG_FUNCTION ("je m installe");
}

void
NslWifiPhy::DoInitialize(){
    NS_LOG_FUNCTION (this);
    InitDriver();
    //m_initialized=true;
}

/*void
NslWifiPhy::ConfigureStandard (enum WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  switch (standard) {
  case WIFI_PHY_STANDARD_80211a:
   Configure80211a ();
    break;
  case WIFI_PHY_STANDARD_80211b:
    Configure80211b ();
    break;
  case WIFI_PHY_STANDARD_80211_10MHZ:
    Configure80211_10Mhz ();
    break;
  case WIFI_PHY_STANDARD_80211_5MHZ:
    Configure80211_5Mhz ();
    break;
  case WIFI_PHY_STANDARD_holland:
    ConfigureHolland ();
    break;
  case WIFI_PHY_STANDARD_80211n_2_4GHZ:
    m_channelStartingFrequency=2407;
    Configure80211n ();
    break;
  case WIFI_PHY_STANDARD_80211n_5GHZ:
    m_channelStartingFrequency=5e3;
    Configure80211n ();
    break;
  default:
    NS_ASSERT (false);
    break;
  }
  NS_LOG_INFO("hello");
}*/

void
NslWifiPhy::SetRxNoiseFigure (double noiseFigureDb)
{
  NS_LOG_FUNCTION (this << noiseFigureDb);
  m_interference.SetNoiseFigure (DbToRatio (noiseFigureDb));
}
void
NslWifiPhy::SetTxPowerStart (double start)
{
  NS_LOG_FUNCTION (this << start);
  m_txPowerBaseDbm = start;
}
void
NslWifiPhy::SetTxPowerEnd (double end)
{
  NS_LOG_FUNCTION (this << end);
  m_txPowerEndDbm = end;
}
void
NslWifiPhy::SetNTxPower (uint32_t n)
{
  NS_LOG_FUNCTION (this << n);
  m_nTxPower = n;
}
void
NslWifiPhy::SetTxGain (double gain)
{
  NS_LOG_FUNCTION (this << gain);
  m_txGainDb = gain;
}
void
NslWifiPhy::SetRxGain (double gain)
{
  NS_LOG_FUNCTION (this << gain);
  m_rxGainDb = gain;
}
void
NslWifiPhy::SetEdThreshold (double threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_edThresholdW = DbmToW (threshold);
}
void
NslWifiPhy::SetCcaMode1Threshold (double threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_ccaMode1ThresholdW = DbmToW (threshold);
}
/*void
NslWifiPhy::SetErrorRateModel (Ptr<ErrorRateModel> rate)
{
  m_interference.SetErrorRateModel (rate);
}*/
/*void
NslWifiPhy::SetDevice (Ptr<NetDevice> device)
{
  m_device = device;
}*/
/*void
NslWifiPhy::SetMobility (Ptr<Object> mobility)
{
  m_mobility = mobility;
}*/

double
NslWifiPhy::GetRxNoiseFigure (void) const
{
  return RatioToDb (m_interference.GetNoiseFigure ());
}
double
NslWifiPhy::GetTxPowerStart (void) const
{
  return m_txPowerBaseDbm;
}
double
NslWifiPhy::GetTxPowerEnd (void) const
{
  return m_txPowerEndDbm;
}
double
NslWifiPhy::GetTxGain (void) const
{
  return m_txGainDb;
}
double
NslWifiPhy::GetRxGain (void) const
{
  return m_rxGainDb;
}

double
NslWifiPhy::GetEdThreshold (void) const
{
  return WToDbm (m_edThresholdW);
}

double
NslWifiPhy::GetCcaMode1Threshold (void) const
{
  return WToDbm (m_ccaMode1ThresholdW);
}

/*Ptr<ErrorRateModel>
NslWifiPhy::GetErrorRateModel (void) const
{
  return m_interference.GetErrorRateModel ();
}*/

/*Ptr<NetDevice>
NslWifiPhy::GetDevice (void) const
{
  return m_device;
}*/

/*Ptr<Object>
NslWifiPhy::GetMobility (void)
{
  return m_mobility;
}*/

double
NslWifiPhy::CalculateSnr (WifiTxVector txMode, double ber) const
{
  return m_interference.GetErrorRateModel()->CalculateSnr(txMode, ber);
}

Ptr<Channel>
NslWifiPhy::GetChannel (void) const
{
  return m_channel;
}
void
NslWifiPhy::SetChannel (Ptr<NslWifiChannel> channel)
{
  m_channel = channel;
  m_channel->Add (this);
  m_channelNumber = 1;      // always start on channel starting frequency (channel 1)
}

void
NslWifiPhy::SetChannelNumber (uint16_t nch)
{
  if (Simulator::Now () == Seconds (0))
    {
      // this is not channel switch, this is initialization
      NS_LOG_DEBUG("start at channel " << nch);
      m_channelNumber = nch;
      return;
    }
    NS_LOG_DEBUG (m_state->GetState ());

  NS_ASSERT(!IsStateSwitching ());
  NS_LOG_DEBUG ("NslWifiPhy:Attempting to set channel number to " << nch);
  switch (m_state->GetState ()) {
  case WifiPhyState::RX:
    NS_LOG_DEBUG ("drop packet because of channel switching while reception");
    m_endRxEvent.Cancel();
    goto switchChannel;
    break;
  case WifiPhyState::TX:
    NS_LOG_DEBUG ("channel switching postponed until end of current transmission");
    Simulator::Schedule (GetDelayUntilIdle(), &NslWifiPhy::SetChannelNumber, this, nch);
    break;
  case WifiPhyState::CCA_BUSY:
  case WifiPhyState::IDLE:
    goto switchChannel;
    break;
  case WifiPhyState::SLEEP:
    NS_LOG_DEBUG ("channel switching ignored in sleep mode");
    break;
  default:
    NS_ASSERT (false);
    break;
  }

  return;

  switchChannel:

  NS_LOG_DEBUG("NslWifiPhy:switching channel " << m_channelNumber << " -> "
      << nch << ", At time = " << Simulator::Now ().GetSeconds () << "s");

  NS_LOG_DEBUG("NslWifiPhy:switching channel " << m_channelSwitchDelay);
      
  m_state->SwitchToChannelSwitching(m_channelSwitchDelay);
  m_interference.EraseEvents();
  /*
   * Needed here to be able to correctly sensed the medium for the first
   * time after the switching. The actual switching is not performed until
   * after m_channelSwitchDelay. Packets received during the switching
   * state are added to the event list and are employed later to figure
   * out the state of the medium after the switching.
   */
  m_channelNumber = nch;

  /*
   * Driver interface.
   */
  UpdatePhyLayerInfo ();
}

uint16_t
NslWifiPhy::GetChannelNumber() const
{
  return m_channelNumber;
}

double
NslWifiPhy::GetChannelFrequencyMhz() const
{
  return m_channelStartingFrequency + 5 * (GetChannelNumber() - 1);
}

void
 NslWifiPhy::SetSleepMode (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state->GetState ())
      {
      case WifiPhyState::TX:
        NS_LOG_DEBUG ("setting sleep mode postponed until end of current transmission");
        Simulator::Schedule (GetDelayUntilIdle (), &NslWifiPhy::SetSleepMode, this);
        break;
     case WifiPhyState::RX:
        NS_LOG_DEBUG ("setting sleep mode postponed until end of current reception");
        Simulator::Schedule (GetDelayUntilIdle (), &NslWifiPhy::SetSleepMode, this);
        break;
      case WifiPhyState::SWITCHING:
        NS_LOG_DEBUG ("setting sleep mode postponed until end of channel switching");
        Simulator::Schedule (GetDelayUntilIdle (), &NslWifiPhy::SetSleepMode, this);
        break;
      case WifiPhyState::CCA_BUSY:
      case WifiPhyState::IDLE:
       NS_LOG_DEBUG ("setting sleep mode");
        m_state->SwitchToSleep ();
        break;
      case WifiPhyState::SLEEP:
        NS_LOG_DEBUG ("already in sleep mode");
        break;
      default:
        NS_ASSERT (false);
        break;
      }
}   

void
NslWifiPhy::ResumeFromSleep (void)
{
   NS_LOG_FUNCTION (this);
    switch (m_state->GetState ())
      {
      case WifiPhyState::TX:
      case WifiPhyState::RX:
      case WifiPhyState::IDLE:
      case WifiPhyState::CCA_BUSY:
      case WifiPhyState::SWITCHING:
      {
       NS_LOG_DEBUG ("not in sleep mode, there is nothing to resume");
       break;
      }
     case WifiPhyState::SLEEP:
     {
        NS_LOG_DEBUG ("resuming from sleep mode");
        Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaMode1ThresholdW);
        m_state->SwitchFromSleep (delayUntilCcaEnd);
        break;
     }
     default:
     {
        NS_ASSERT (false);
        break;
     }
      }
 }
void
NslWifiPhy::SetReceiveOkCallback (RxOkCallback callback)
{
  m_state->SetReceiveOkCallback (callback);
}
void
NslWifiPhy::SetReceiveErrorCallback (RxErrorCallback callback)
{
  m_state->SetReceiveErrorCallback (callback);
}


/*void
NslWifiPhy::StartReceivePreamble (Ptr<Packet> packet, double rxPowerW, Time rxDuration)
{
  NS_LOG_FUNCTION (this << packet << rxPowerW << rxDuration);
  WifiPhyTag tag;
  bool found = packet->RemovePacketTag (tag);
  NS_LOG_INFO(packet);
  if (!found)
    {
      NS_LOG_INFO("test start");
      NS_FATAL_ERROR ("Received Wi-Fi Signal with no WifiPhyTag");
      return;
    }

  WifiPreamble preamble = tag.GetPreambleType ();
  WifiModulationClass modulation = tag.GetModulation ();
  WifiTxVector txVector;
  txVector.SetPreambleType (preamble);
  if ((modulation == WIFI_MOD_CLASS_DSSS) || (modulation == WIFI_MOD_CLASS_HR_DSSS))
    {
      DsssSigHeader dsssSigHdr;
      found = packet->RemoveHeader (dsssSigHdr);
      if (!found)
        {
          NS_FATAL_ERROR ("Received 802.11b signal with no SIG field");
          return;
        }
      txVector.SetChannelWidth (22);
      for (uint8_t i = 0; i < GetNModes (); i++)
        {
          WifiMode mode = GetMode (i);
          if (((mode.GetModulationClass() == WIFI_MOD_CLASS_DSSS) || (mode.GetModulationClass() == WIFI_MOD_CLASS_HR_DSSS))
              && (mode.GetDataRate (22) == dsssSigHdr.GetRate ()))
            {
              txVector.SetMode (mode);
              break;
            }
        }
    }
  else if ((modulation != WIFI_MOD_CLASS_HT) || (preamble != WIFI_PREAMBLE_HT_GF))
    {
      LSigHeader lSigHdr;
      found = packet->RemoveHeader (lSigHdr);
      if (!found)
        {
          NS_FATAL_ERROR ("Received OFDM 802.11 signal with no SIG field");
          return;
        }
      uint16_t channelWidth = GetChannelWidth ();
      txVector.SetChannelWidth (channelWidth > 20 ? 20 : channelWidth);
      for (uint8_t i = 0; i < GetNModes (); i++)
        {
          WifiMode mode = GetMode (i);
          if (mode.GetDataRate (GetChannelWidth ()) == lSigHdr.GetRate (GetChannelWidth ()))
            {
              txVector.SetMode (mode);
              break;
            }
        }
    }
  if (modulation == WIFI_MOD_CLASS_HT)
    {
      HtSigHeader htSigHdr;
      found = packet->RemoveHeader (htSigHdr);
      if (!found)
        {
          NS_FATAL_ERROR ("Received 802.11n signal with no HT-SIG field");
          return;
        }
      txVector.SetChannelWidth (htSigHdr.GetChannelWidth ());
      for (uint8_t i = 0; i < GetNMcs (); i++)
        {
          WifiMode mode = GetMcs (i);
          if ((mode.GetModulationClass () == WIFI_MOD_CLASS_HT) && (mode.GetMcsValue () == htSigHdr.GetMcs ()))
            {
              txVector.SetMode (mode);
              txVector.SetNss (1 + (txVector.GetMode ().GetMcsValue () / 8));
              break;
            }
        }
      txVector.SetGuardInterval(htSigHdr.GetShortGuardInterval () ? 400 : 800);
      txVector.SetAggregation (htSigHdr.GetAggregation ());
    }
  else if (modulation == WIFI_MOD_CLASS_VHT)
    {
      VhtSigHeader vhtSigHdr;
      vhtSigHdr.SetMuFlag (preamble == WIFI_PREAMBLE_VHT_MU);
      found = packet->RemoveHeader (vhtSigHdr);
      if (!found)
        {
          NS_FATAL_ERROR ("Received 802.11ac signal with no VHT-SIG field");
          return;
        }
      txVector.SetChannelWidth (vhtSigHdr.GetChannelWidth ());
      txVector.SetNss (vhtSigHdr.GetNStreams ());
      for (uint8_t i = 0; i < GetNMcs (); i++)
        {
          WifiMode mode = GetMcs (i);
          if ((mode.GetModulationClass () == WIFI_MOD_CLASS_VHT) && (mode.GetMcsValue () == vhtSigHdr.GetSuMcs ()))
            {
              txVector.SetMode (mode);
              break;
            }
        }
      txVector.SetGuardInterval (vhtSigHdr.GetShortGuardInterval () ? 400 : 800);
      if (IsAmpdu (packet))
        {
          txVector.SetAggregation (true);
        }
    }
  else if (modulation == WIFI_MOD_CLASS_HE)
    {
      HeSigHeader heSigHdr;
      heSigHdr.SetMuFlag (preamble == WIFI_PREAMBLE_HE_MU);
      found = packet->RemoveHeader (heSigHdr);
      if (!found)
        {
          NS_FATAL_ERROR ("Received 802.11ax signal with no HE-SIG field");
          return;
        }
      txVector.SetChannelWidth (heSigHdr.GetChannelWidth ());
      txVector.SetNss (heSigHdr.GetNStreams ());
      for (uint8_t i = 0; i < GetNMcs (); i++)
        {
          WifiMode mode = GetMcs (i);
          if ((mode.GetModulationClass () == WIFI_MOD_CLASS_HE) && (mode.GetMcsValue () == heSigHdr.GetMcs ()))
            {
              txVector.SetMode (mode);
              break;
            }
        }
      txVector.SetGuardInterval (heSigHdr.GetGuardInterval ());
      txVector.SetBssColor (heSigHdr.GetBssColor ());
      if (IsAmpdu (packet))
        {
          txVector.SetAggregation (true);
        }
    }

  Ptr<Event> event;
  event = m_interference.Add (packet,
                              txVector,
                              rxDuration,
                              rxPowerW);

  if (m_state->GetState () == WifiPhyState::OFF)
    {
      NS_LOG_DEBUG ("Cannot start RX because device is OFF");
      return;
    }

  if (tag.GetFrameComplete () == 0)
    {
      NS_LOG_DEBUG ("Packet reception stopped because transmitter has been switched off");
      return;
    }

  if (!txVector.GetModeInitialized ())
    {
      //If SetRate method was not called above when filling in txVector, this means the PHY does support the rate indicated in PHY SIG headers
      NS_LOG_DEBUG ("drop packet because of unsupported RX mode");
      NotifyRxDrop (packet, UNSUPPORTED_SETTINGS);
      return;
    }

  Time endRx = Simulator::Now () + rxDuration;
  switch (m_state->GetState ())
    {
    case WifiPhyState::SWITCHING:
      NS_LOG_DEBUG ("drop packet because of channel switching");
      NotifyRxDrop (packet, NOT_ALLOWED);
      
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the completion of the channel switching.
          MaybeCcaBusyDuration ();
          return;
        }
      break;
    case WifiPhyState::RX:
      NS_ASSERT (m_currentEvent != 0);
      if (m_frameCaptureModel != 0
          && m_frameCaptureModel->IsInCaptureWindow (m_timeLastPreambleDetected)
          && m_frameCaptureModel->CaptureNewFrame (m_currentEvent, event))
        {
          AbortCurrentReception (FRAME_CAPTURE_PACKET_SWITCH);
          NS_LOG_DEBUG ("Switch to new packet");
          StartRx (event, rxPowerW, rxDuration);
        }
      else
        {
          NS_LOG_DEBUG ("Drop packet because already in Rx (power=" <<
                        rxPowerW << "W)");
          NotifyRxDrop (packet, NOT_ALLOWED);
          if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
            {
              //that packet will be noise _after_ the reception of the currently-received packet.
              MaybeCcaBusyDuration ();
              return;
            }
        }
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("Drop packet because already in Tx (power=" <<
                    rxPowerW << "W)");
      NotifyRxDrop (packet, NOT_ALLOWED);
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the transmission of the currently-transmitted packet.
          MaybeCcaBusyDuration ();
          return;
        }
      break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
      StartRx (event, rxPowerW, rxDuration);
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("Drop packet because in sleep mode");
      NotifyRxDrop (packet, NOT_ALLOWED);
      break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
}*/

void
NslWifiPhy::StartReceivePlcp (Ptr<Packet> packet,
                              double rxPowerDbm,
                              WifiTxVector txVector,
                              enum WifiPreamble preamble,
                              uint8_t packetType, Time rxDuration)
 {
    // This function should be later split to check separately whether plcp preamble and plcp header can be successfully received.
    // Note: plcp preamble reception is not yet modeled.
    NS_LOG_FUNCTION (this << packet << rxPowerDbm << txVector.GetMode()<< preamble << (uint32_t)packetType);
    AmpduTag ampduTag;
   
     rxPowerDbm += m_rxGainDb;
     double rxPowerW = DbmToW (rxPowerDbm);
     Time endRx = Simulator::Now () + rxDuration;
     Time plcpDuration = CalculatePlcpPreambleAndHeaderDuration (txVector);
  
    Ptr<Event> event = m_interference.Add (packet, txVector,rxDuration,rxPowerW);

      
    switch (m_state->GetState ())
     {
     case WifiPhyState::SWITCHING:
       NS_LOG_DEBUG ("drop packet because of channel switching");
       NotifyRxDrop (packet,NOT_ALLOWED);
       m_plcpSuccess = false;
 
        if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
          {
           goto maybeCcaBusy;
          }
        break;
      case WifiPhyState::RX:
        NS_LOG_DEBUG ("drop packet because already in Rx (power=" <<
                      rxPowerW << "W)" << packet);
       NotifyRxDrop (packet,NOT_ALLOWED);
       if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
          {
           NS_LOG_INFO("maybe");
           goto maybeCcaBusy;
          }
        break;
      case WifiPhyState::TX:
        NS_LOG_DEBUG ("drop packet because already in Tx (power=" <<
                      rxPowerW << "W)" << packet);
        NotifyRxDrop (packet,NOT_ALLOWED);
        if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
            {
            NS_LOG_INFO("maybe");
            goto maybeCcaBusy;
          }
        break;
      case WifiPhyState::CCA_BUSY:
      case WifiPhyState::IDLE:
        if (rxPowerW > m_edThresholdW) //checked here, no need to check in the payload reception (current implementation assumes constant rx power over the packet duration)
          {
            if (preamble == WIFI_PREAMBLE_NONE && m_mpdusNum == 0)
              {
                NS_LOG_DEBUG ("drop packet because no preamble has been received");
                NotifyRxDrop (packet,NOT_ALLOWED);
                goto maybeCcaBusy;
              }
            else if (preamble == WIFI_PREAMBLE_NONE && m_plcpSuccess == false) // A-MPDU reception fails
              {
                NS_LOG_DEBUG ("Drop MPDU because no plcp has been received");
                NotifyRxDrop (packet,NOT_ALLOWED);
                goto maybeCcaBusy;
              }
             else if (preamble != WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum == 0)
              {
                //received the first MPDU in an MPDU
                m_mpdusNum = ampduTag.GetRemainingNbOfMpdus()-1;
              }
             else if (preamble == WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum > 0)
              {
                //received the other MPDUs that are part of the A-MPDU
                if (ampduTag.GetRemainingNbOfMpdus() < m_mpdusNum)
                  {
                      NS_LOG_DEBUG ("Missing MPDU from the A-MPDU " << m_mpdusNum - ampduTag.GetRemainingNbOfMpdus());
                      m_mpdusNum = ampduTag.GetRemainingNbOfMpdus();
                  }
                else
                  m_mpdusNum--;
              }
            else if (preamble != WIFI_PREAMBLE_NONE && m_mpdusNum > 0 )
              {
                NS_LOG_DEBUG ("Didn't receive the last MPDUs from an A-MPDU " << m_mpdusNum);
                m_mpdusNum = 0;
              }
              
           NS_LOG_DEBUG ("sync to signal (power=" << rxPowerW << "W)");
            // sync to signal
            m_state->SwitchToRx (rxDuration);
            NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
            NotifyRxBegin (packet);
            m_interference.NotifyRxStart ();
              
            if (preamble != WIFI_PREAMBLE_NONE)
            {
              NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
              m_endPlcpRxEvent = Simulator::Schedule (plcpDuration, &NslWifiPhy::StartReceivePacket, this,
                                                      packet, txVector, preamble, rxPowerDbm);
            }
              
            NS_ASSERT (m_endRxEvent.IsExpired ());
            m_endRxEvent = Simulator::Schedule (rxDuration, &NslWifiPhy::EndReceive, this,
                                                packet,event);
          }
        else
          {
           NS_LOG_DEBUG ("drop packet because signal power too Small (" <<
                          rxPowerW << "<" << m_edThresholdW << ")");
            NotifyRxDrop (packet,NOT_ALLOWED);
            m_plcpSuccess = false;
            goto maybeCcaBusy;
          }
        break;
      case WifiPhyState::SLEEP:{
        NS_LOG_DEBUG ("drop packet because in sleep mode");
        NotifyRxDrop (packet,NOT_ALLOWED);
        m_plcpSuccess = false;
        break;
      }
      default:
      {
        NS_ASSERT (false);
        break;
      }
      }
  
    return;
   
    maybeCcaBusy:
  
    Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaMode1ThresholdW);
    if (!delayUntilCcaEnd.IsZero ())
      {
        m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
      }
 }


void
NslWifiPhy::StartReceivePacket (Ptr<Packet> packet,
                                WifiTxVector txVector,
                             WifiPreamble preamble, double rxPowerDbm)
{
  NS_LOG_FUNCTION (this << packet << (uint32_t)txVector.GetTxPowerLevel() << txVector << preamble);
  rxPowerDbm += m_rxGainDb;
  double rxPowerW = DbmToW (rxPowerDbm);
  txVector.SetPreambleType(preamble);
  Time rxDuration = CalculateTxDuration (packet->GetSize (), txVector, preamble);

  Time endRx = Simulator::Now () + rxDuration;

  NS_LOG_FUNCTION(this << txVector.GetMode());
  
  NS_LOG_FUNCTION(this << txVector.GetPreambleType());
  Ptr<Event> event;
  event = m_interference.Add (packet,
                              txVector,
                              rxDuration,
                              rxPowerW);


    SetCurrentWifiMode (event->GetPayloadMode ());

  NS_LOG_INFO(m_state->GetState());


  switch (m_state->GetState ()) {
  case WifiPhyState::SWITCHING:
    NS_LOG_DEBUG ("NslWifiPhy:drop packet because of channel switching");
    NotifyRxDrop (packet,NOT_ALLOWED);
  
    if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
      {
        // that packet will be noise _after_ the completion of the
        // channel switching.
        goto maybeCcaBusy;
      }
    break;
  case WifiPhyState::RX:
    NS_LOG_DEBUG ("NslWifiPhy:drop packet because already in Rx (power = "<<
                  rxPowerW<<" W)" << packet);
    NotifyRxDrop (packet,NOT_ALLOWED);
       NS_LOG_INFO(endRx);
    NS_LOG_INFO(Simulator::Now () + m_state->GetDelayUntilIdle ());
    if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
      {
        NS_LOG_INFO("maybe"<<packet);
        // that packet will be noise _after_ the reception of the
        // currently-received packet.
        goto maybeCcaBusy;
      }
    break;
  case WifiPhyState::TX:
    NS_LOG_DEBUG ("NslWifiPhy:drop packet because already in Tx (power = "<<
                  rxPowerW<<" W)" << packet);
    NotifyRxDrop (packet,NOT_ALLOWED);
    NS_LOG_INFO(endRx);
    NS_LOG_INFO(Simulator::Now () + m_state->GetDelayUntilIdle ());
    if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
      {
        NS_LOG_INFO("maybe"<<packet);
        // that packet will be noise _after_ the transmission of the
        // currently-transmitted packet.
        goto maybeCcaBusy;
      }
    break;
  case WifiPhyState::CCA_BUSY:
  case WifiPhyState::IDLE:
    if (rxPowerW > m_edThresholdW)
      {
        NS_LOG_DEBUG ("NslWifiPhy:SYNC (power = " << rxPowerW << " W) at Node #" <<
                      m_node->GetId ());
        // drop noise/jamming packet
        NS_LOG_DEBUG (preamble);
        NS_LOG_DEBUG (WIFI_PREAMBLE_INVALID);
       if (preamble == WIFI_PREAMBLE_INVALID)
          {
            NS_LOG_DEBUG ("NslWifiPhy:drop **jamming** packet!" << packet);
            NotifyRxDrop (packet,UNSUPPORTED_SETTINGS);
            goto maybeCcaBusy;
          }
     
        if (!DriverStartRx (packet, MeasureRss ()))
          {
            NS_LOG_DEBUG ("NslWifiPhy:Ignoring RX! at Node #" << m_node->GetId ());
            NotifyRxDrop (packet,NOT_ALLOWED);
            return; // still in IDLE or CCA_BUSY
          }

        // sync to signal
    
        m_interference.NotifyRxStart ();
        NotifyRxBegin (event->GetPacket());
        m_state->SwitchToRx (rxDuration);
        NS_LOG_INFO("m state in start receive" << m_state);
        NS_ASSERT (m_endRxEvent.IsExpired ());
        
       // 
        m_endRxEvent = Simulator::Schedule (rxDuration, &NslWifiPhy::EndReceive,
                                            this, packet,event);
      }
      
    else  // drop because power too low
      {
        NS_LOG_DEBUG ("NslWifiPhy:drop packet because signal power too Small (" <<
                      rxPowerW << "<" << m_edThresholdW << ")");
        NotifyRxDrop (packet,UNSUPPORTED_SETTINGS);
        goto maybeCcaBusy;
      }
    break;
    case WifiPhyState::SLEEP:
        NS_LOG_DEBUG ("Drop packet because in sleep mode");
        NotifyRxDrop (packet, NOT_ALLOWED);
    break;
    case WifiPhyState::OFF:
        NS_LOG_DEBUG ("Drop packet because in sleep mode");
         if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
         {
           goto maybeCcaBusy;
         }
      break;
  }
  return;

 maybeCcaBusy:
  // We are here because we have received the first bit of a packet and we are
  // not going to be able to synchronize on it
  // In this model, CCA becomes busy when the aggregation of all signals as
  // tracked by the InterferenceHelper class is higher than the CcaBusyThreshold

 
  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaMode1ThresholdW);
  Time t1 = Seconds (0.8);
  NS_LOG_INFO("maybeCcaBusy" << delayUntilCcaEnd);
  if (!delayUntilCcaEnd.IsZero ())
    {
      m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
}

void
NslWifiPhy::SendPacket (Ptr<const Packet> packet, WifiTxVector txVector,
                        WifiPreamble preamble, uint8_t txPower)
{
  NS_LOG_FUNCTION (this << packet << txVector.GetMode() << preamble << (uint32_t)txVector.GetTxPowerLevel() << (uint32_t)txPower);
  /*
   * Transmission can happen if:
   *  - we are syncing on a packet. It is the responsibility of the
   *    MAC layer to avoid doing this but the PHY does nothing to
   *    prevent it.
   *  - we are idle
   */

  NS_ASSERT (!m_state->IsStateTx () && !m_state->IsStateSwitching ());

       /*WifiTxVector txMode;
       txMode.SetMode (mode);
       txMode.SetPreambleType (preamble);*/

    if(m_state->IsStateSleep()){
        NS_LOG_DEBUG ("Dropping packet because in sleep mode");
        NotifyTxDrop(packet);
        return;
    }

  Time txDuration = CalculateTxDuration (packet->GetSize (), txVector,GetFrequency ());
  if (m_state->IsStateRx ())
    {
      m_endPlcpRxEvent.Cancel ();
      m_endRxEvent.Cancel ();
      m_interference.NotifyRxEnd ();
    }
  NotifyTxBegin (packet,txVector.GetTxPowerLevel());
  //uint32_t dataRate500KbpsUnits;
  /*if (txVector.GetMode().GetModulationClass () == WIFI_MOD_CLASS_HT){
      dataRate500KbpsUnits = 128 + WifiModeToMcs (txVector);
  }
  else{
      dataRate500KbpsUnits = txVector.GetMode().GetDataRate (txVector) * txVector.GetNss() / 500000;
  }*/
  
  //bool isShortPreamble = (WIFI_PREAMBLE_SHORT == preamble);
  NotifyMonitorSniffTx (packet, (uint16_t)GetChannelFrequencyMhz (),  txVector);


  m_state->SwitchToTx (txDuration, packet, txPower, txVector);
  NS_LOG_DEBUG ("NslWifiPhy:Calling Send function .. txPower =  in Dbm = " << GetPowerDbm (txPower) <<
                " plus gain = " << m_txGainDb);
  m_channel->Send (this, packet, GetPowerDbm (txPower) + m_txGainDb,txVector.GetMode(),preamble,txVector);
  //m_channel->Send (this, packet, GetPowerDbm (txPower) + m_txGainDb,txDuration,txVector,preamble);

  SetCurrentWifiMode (txVector.GetMode());
  DriverStartTx (packet, DbmToW (GetPowerDbm (txPower) + m_txGainDb));
}

/*uint8_t
NslWifiPhy::GetNModes (void) const
{
  return m_deviceRateSet.size ();
}*/

/*WifiMode
NslWifiPhy::GetMode (uint32_t mode) const
{
  return m_deviceRateSet[mode];
}*/
bool
NslWifiPhy::IsModeSupported (WifiMode mode) const
 {
    for (uint32_t i = 0; i < GetNModes (); i++)
      {
        if (mode == GetMode (i))
         {
             return true;
          }
       }
     return false;
 }

/*bool
NslWifiPhy::IsMcsSupported (WifiMode mode)
 {
    for (uint8_t i = 0; i < GetNMcs (); i++)
      {
        if (mode == McsToWifiMode(GetMcs(i)))
          {
            return true;
          }
      }
    return false;
  }*/
  
uint32_t
NslWifiPhy::GetNTxPower (void) const
{
  return m_nTxPower;
}

void
NslWifiPhy::Configure80211a (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; // 5.000 GHz

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate48Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate54Mbps ());
}


/*void
NslWifiPhy::Configure80211b (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 2407; // 2.412 GHz

  m_deviceRateSet.push_back (WifiPhy::GetDsssRate1Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate2Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate5_5Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate11Mbps ());

  //uint8_t test = static_cast<uint8_t> (m_deviceRateSet.size ());
   NS_LOG_FUNCTION (this <<m_deviceRateSet[0]);
  
}**/

void
NslWifiPhy::Configure80211_10Mhz (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; // 5.000 GHz, suppose 802.11a

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate3MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate4_5MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate27MbpsBW10MHz ());
  
}

void
NslWifiPhy::Configure80211_5Mhz (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; // 5.000 GHz, suppose 802.11a

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate1_5MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate2_25MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate3MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate4_5MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate13_5MbpsBW5MHz ());
}

void
NslWifiPhy::ConfigureHolland (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; // 5.000 GHz
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate54Mbps ());
}

void
NslWifiPhy::Configure80211p_CCH (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; // 802.11p works over the 5Ghz freq range

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate3MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate4_5MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate27MbpsBW10MHz ());
}

void
NslWifiPhy::Configure80211p_SCH (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; // 802.11p works over the 5Ghz freq range

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate3MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate4_5MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate27MbpsBW10MHz ());
}

/*void 
NslWifiPhy::RegisterListener (WifiPhyListener *listener)
{
  NS_LOG_FUNCTION (this);
  m_state->RegisterListener (listener);
}

void 
NslWifiPhy::UnregisterListerner (WifiPhyListener *listener)
{
  NS_LOG_FUNCTION (this);
  m_state->UnregisterListener (listener);
}*/

/*bool
NslWifiPhy::IsStateCcaBusy (void)
{
  return m_state->IsStateCcaBusy ();
}

bool
NslWifiPhy::IsStateIdle (void)
{
  return m_state->IsStateIdle ();
}
bool
NslWifiPhy::IsStateBusy (void)
{
  return m_state->IsStateCcaBusy ();
}
bool
NslWifiPhy::IsStateRx (void)
{
  return m_state->IsStateRx ();
}
bool
NslWifiPhy::IsStateTx (void)
{
  return m_state->IsStateTx ();
}
bool
NslWifiPhy::IsStateSwitching (void)
{
  return m_state->IsStateSwitching ();
}

bool
NslWifiPhy::IsStateSleep (void)
{
  return m_state->IsStateSleep ();
}*/

/*Time
NslWifiPhy::GetStateDuration (void)
{
  return m_state->GetStateDuration ();
}*/

/*Time
NslWifiPhy::GetDelayUntilIdle (void)
{
  return m_state->GetDelayUntilIdle ();
}*/

/*Time
NslWifiPhy::GetLastRxStartTime (void) const
{
  return m_state->GetLastRxStartTime ();
}
*/
double
NslWifiPhy::DbToRatio (double dB) const
{
  double ratio = pow(10.0,dB/10.0);
  return ratio;
}

double
NslWifiPhy::DbmToW (double dBm) const
{
  double mW = pow(10.0,dBm/10.0);

  return mW / 1000.0;
}

double
NslWifiPhy::WToDbm (double w) const
{
  return 10.0 * log10(w * 1000.0);
}

double
NslWifiPhy::RatioToDb (double ratio) const
{
  return 10.0 * log10(ratio);
}

double
NslWifiPhy::GetEdThresholdW (void) const
{
  return m_edThresholdW;
}

double
NslWifiPhy::GetPowerDbm (uint8_t power) const
{
  NS_ASSERT (m_txPowerBaseDbm <= m_txPowerEndDbm);
  NS_ASSERT (m_nTxPower > 0);
  double dbm;
  if (m_nTxPower > 1)
    {
      dbm = m_txPowerBaseDbm + power * (m_txPowerEndDbm - m_txPowerBaseDbm) / (m_nTxPower - 1);
    }
  else
    {
      NS_ASSERT_MSG (m_txPowerBaseDbm == m_txPowerEndDbm,
                     "cannot have TxPowerEnd != TxPowerStart with TxPowerLevels == 1");
      dbm = m_txPowerBaseDbm;
    }
  return dbm;
}

void
NslWifiPhy::EndReceive (Ptr<Packet> packet,  Ptr<Event> event)
{
 NS_LOG_FUNCTION (this << packet << event );
 NS_LOG_INFO("m state in end receive" << m_state);

Ptr<const Packet> packet1 = event->GetPacket ();
Ptr<Packet> packet2 = packet1->Copy ();
  //NS_ASSERT (m_state->IsStateRx());
  NS_ASSERT (event->GetEndTime () == Simulator::Now ());

  Ptr<Packet> copy = packet1->Copy ();


  struct InterferenceHelper::SnrPer snrPer;
   snrPer = m_interference.CalculateSnrPer (event);
   std::vector<bool> statusPerMpdu;
   std::pair<bool, SignalNoiseDbm> rxInfo;
   m_interference.NotifyRxEnd ();
   Time psduDuration = event->GetEndTime () - event->GetStartTime ();
   Time relativeStart = NanoSeconds (0);

     NS_LOG_DEBUG ("mode=" << (event->GetPayloadMode ().GetDataRate (event->GetTxVector())) <<
                   ", snr=" << snrPer.snr << ", per=" << snrPer.per << ", size=" << packet->GetSize ());
  
    NS_LOG_DEBUG("m_random"<<m_random->GetValue ());
    NS_LOG_DEBUG("snrPer"<<snrPer.per);
     if (m_random->GetValue () > snrPer.per)
    {
      NS_LOG_INFO("notify");
      NotifyRxEnd (packet2);
        rxInfo = WifiPhy::GetReceptionStatus (packet2, event, relativeStart, psduDuration);
       //bool isShortPreamble = (WIFI_PREAMBLE_SHORT == event->GetPreambleType ());
        //double signalDbm = RatioToDb (event->GetRxPowerW ()) + 30;
       // double noiseDbm = RatioToDb (event->GetRxPowerW () / snrPer.snr) - GetRxNoiseFigure () + 30;
        SignalNoiseDbm noiseDbm2 = rxInfo.second;
        statusPerMpdu.push_back (rxInfo.first);
        NotifyMonitorSniffRx (packet2, (uint16_t)GetChannelFrequencyMhz (), event->GetTxVector(), noiseDbm2, statusPerMpdu);
        m_state->SwitchFromRxEndOk (packet2, snrPer.snr, event->GetTxVector(),statusPerMpdu);
        DriverEndRx (copy, snrPer.packetRss, true);
      }
     else
     {
       // failure. 
       NS_LOG_INFO("packet non recu");
        NotifyRxDrop (packet2,NOT_ALLOWED);
       m_state->SwitchFromRxEndError (packet2, snrPer.snr);
       DriverEndRx (copy, snrPer.packetRss, false);
     }
}

/*void
NslWifiPhy::EndReceive (Ptr<Packet> packet,Ptr<Event> event)
{
  Time psduDuration = event->GetEndTime () - event->GetStartTime ();
  NS_LOG_FUNCTION (this << event->GetPacket () << event->GetTxVector () << event << psduDuration);
  NS_ASSERT (event->GetEndTime () == Simulator::Now ());

  double snr = m_interference.CalculateSnr (event);
  std::vector<bool> statusPerMpdu;
  SignalNoiseDbm signalNoise;

  //Ptr<const Packet> packet = event->GetPacket ();
  Time relativeStart = NanoSeconds (0);
  bool receptionOkAtLeastForOneMpdu = true;
  std::pair<bool, SignalNoiseDbm> rxInfo;
  WifiTxVector txVector = event->GetTxVector ();
  if (txVector.IsAggregation ())
    {
      //Extract all MPDUs of the A-MPDU to compute per-MPDU PER stats
      //TODO remove PLCP header first
      std::list<Ptr<const Packet>> ampduSubframes = MpduAggregator::PeekAmpduSubframes (packet);
      size_t nbOfRemainingMpdus = ampduSubframes.size ();
      Time remainingAmpduDuration = event->GetEndTime () - event->GetStartTime ();
      MpduType mpdutype = (nbOfRemainingMpdus == 1) ? SINGLE_MPDU : FIRST_MPDU_IN_AGGREGATE;
      for (const auto & subframe : ampduSubframes)
        {
          Time mpduDuration = GetPayloadDuration (subframe->GetSize (), txVector, GetFrequency (), mpdutype, 1);
          remainingAmpduDuration -= mpduDuration;
          --nbOfRemainingMpdus;
          if (nbOfRemainingMpdus == 0 && !remainingAmpduDuration.IsZero ()) //no more MPDU coming
            {
              mpduDuration += remainingAmpduDuration; //apply a correction just in case rounding had induced slight shift
            }
          rxInfo = GetReceptionStatus (MpduAggregator::PeekMpduInAmpduSubframe (subframe), event, relativeStart, mpduDuration);
          NS_LOG_DEBUG ("Extracted MPDU #" << ampduSubframes.size () - nbOfRemainingMpdus - 1 << ": duration: " << mpduDuration.GetNanoSeconds () << "ns" <<
                        ", correct reception: " << rxInfo.first <<
                        ", Signal/Noise: " << rxInfo.second.signal << "/" << rxInfo.second.noise << "dBm");
          signalNoise = rxInfo.second; //same information for all MPDUs
          statusPerMpdu.push_back (rxInfo.first);
          receptionOkAtLeastForOneMpdu |= rxInfo.first;
          relativeStart += mpduDuration;
          mpdutype = (nbOfRemainingMpdus == 1) ? LAST_MPDU_IN_AGGREGATE : MIDDLE_MPDU_IN_AGGREGATE;
        }
    }
  else
    {
      //Simple MPDU
      rxInfo = GetReceptionStatus (packet, event, relativeStart, psduDuration);
      signalNoise = rxInfo.second; //same information for all MPDUs
      statusPerMpdu.push_back (rxInfo.first);
      receptionOkAtLeastForOneMpdu = rxInfo.first;
    }

  if (receptionOkAtLeastForOneMpdu)
    {
      NotifyMonitorSniffRx (packet, GetFrequency (), txVector, signalNoise, statusPerMpdu);
      m_state->SwitchFromRxEndOk (packet->Copy (), snr, txVector, statusPerMpdu);
    }
  else
    {
      m_state->SwitchFromRxEndError (packet->Copy (), snr);
    }

  m_interference.NotifyRxEnd ();
 // m_currentEvent = 0;
}*/

int64_t
NslWifiPhy::AssignStreams (int64_t stream){
    NS_LOG_FUNCTION (this << stream);
    m_random->SetStream(stream);
    return 1;
}

void
NslWifiPhy::SetFrequency(uint32_t freq){
    m_channelStartingFrequency = freq;
}

void
NslWifiPhy::SetNumberOfTransmitAntennas(uint32_t tx){
    m_numberOfTransmitters = tx;
}

void
NslWifiPhy::SetNumberOfReceiveAntennas(uint32_t tx){
    m_numberOfReceivers = tx;
}

void 
NslWifiPhy::SetLdpc(bool stbc){
    m_ldpc=stbc;
}

void 
NslWifiPhy::SetStbc(bool stbc){
    m_stbc=stbc;
}

void
NslWifiPhy::SetGreenfield(bool green){
    m_greenfield=green;
}

bool
NslWifiPhy::GetGuardInterval(void) const{
    return m_guardInterval;
}

void
NslWifiPhy::SetGuardInterval(bool guardInterval) {
     m_guardInterval = guardInterval ;
}

uint32_t
NslWifiPhy::GetFrequency(void) const{
    return m_channelStartingFrequency;
}



uint32_t
NslWifiPhy::GetNumberOfTransmitAntennas(void) const{
    return m_numberOfTransmitters ;
}


uint32_t
NslWifiPhy::GetNumberOfReceiveAntennas(void) const{
    return m_numberOfReceivers ;
}

bool
NslWifiPhy::GetLdpc(void) const{
    return m_ldpc;
}

bool
NslWifiPhy::GetStbc(void) const{
    return m_stbc;
}

bool
NslWifiPhy::GetGreenfield(void) const{
    return m_greenfield;
}

bool
NslWifiPhy::GetChannelBonding(void) const{
    return m_channelBonding;
}


void
NslWifiPhy::SetChannelBonding(bool cb){
     m_channelBonding= cb;
}

void
NslWifiPhy::Configure80211n (void)
 {
   NS_LOG_FUNCTION (this);
    if (m_channelStartingFrequency>=2400 && m_channelStartingFrequency<=2500) //@ 2.4 GHz
      {
       m_deviceRateSet.push_back (WifiPhy::GetDsssRate1Mbps ());
       m_deviceRateSet.push_back (WifiPhy::GetDsssRate2Mbps ());
       m_deviceRateSet.push_back (WifiPhy::GetDsssRate5_5Mbps ());
       m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate6Mbps ());
       m_deviceRateSet.push_back (WifiPhy::GetDsssRate11Mbps ());
       m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate12Mbps ());
       m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate24Mbps ());
     }
   if (m_channelStartingFrequency>=5000 && m_channelStartingFrequency<=6000) //@ 5 GHz
     {
       m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
       m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
       m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24Mbps ());
     }
   m_bssMembershipSelectorSet.push_back(HT_PHY);
   for (uint8_t i=0; i <8; i++)
     {
       m_deviceMcsSet.push_back(i);
     }
 }

uint32_t
NslWifiPhy::GetNBssMembershipSelectors (void) const
 {
   return  m_bssMembershipSelectorSet.size ();
 }

uint32_t
NslWifiPhy::GetBssMembershipSelector(uint32_t se) const
 {
   return  m_bssMembershipSelectorSet[se];
 }


 /*WifiModeList
 NslWifiPhy::GetMembershipSelectorModes(uint32_t selector)
 {
   uint32_t id=GetBssMembershipSelector(selector);
   WifiModeList supportedmodes;
   if (id == HT_PHY)
   {
     //mandatory MCS 0 to 7
      supportedmodes.push_back (WifiPhy::GetOfdmRate6Mbps ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate13MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate19_5MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate26MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate39MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate52MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate58_5MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate65MbpsBW20MHz ());
   }
   return supportedmodes;
 }*/

uint8_t
 NslWifiPhy::GetNMcs (void) const
  {
    return  m_deviceMcsSet.size ();
 }

 uint8_t
 NslWifiPhy::GetMcs (uint8_t m) const
  {
    return  m_deviceMcsSet[m];
 }

uint32_t 
NslWifiPhy::WifiModeToMcs (WifiTxVector txVector)
 {
    uint32_t mcs = 0;
     if ( txVector.GetMode().GetUniqueName() == "OfdmRate135MbpsBW40MHzShGi" || txVector.GetMode().GetUniqueName() == "OfdmRate65MbpsBW20MHzShGi" )
       {
               mcs=6;
       }
    else
      {
       switch (txVector.GetMode().GetDataRate(txVector))
         {
           case 6500000:
           case 7200000:
           case 13500000:
           case 15000000:
             mcs=0;
             break;
           case 13000000:
           case 14400000:
           case 27000000:
           case 30000000:
             mcs=1;
             break;
           case 19500000:
           case 21700000:
           case 40500000:
           case 45000000:
             mcs=2;
             break;
           case 26000000:
           case 28900000:
           case 54000000:
           case 60000000:
             mcs=3;
             break;
           case 39000000:
           case 43300000:
           case 81000000:
           case 90000000:        
             mcs=4;
             break;
           case 52000000:
           case 57800000:
           case 108000000:
           case 120000000:
             mcs=5;
             break; 
           case 58500000:
           case 121500000:
             mcs=6;
             break;
           case 65000000:
           case 72200000:
           case 135000000:
           case 150000000:
             mcs=7;
             break;     
         }
      }
    return mcs;
  }

/*WifiMode
 NslWifiPhy::McsToWifiMode (uint8_t mcs)
  {
     WifiMode mode;
     switch (mcs)
       { 
         case 7:
            if (!GetGuardInterval() && !GetChannelBonding())
             {
                mode =  WifiPhy::GetOfdmRate65MbpsBW20MHz ();
              }
           else if(GetGuardInterval() && !GetChannelBonding())
              {
                mode = WifiPhy::GetOfdmRate72_2MbpsBW20MHz ();
              }
            else if (!GetGuardInterval() && GetChannelBonding())
              {
                mode = WifiPhy::GetOfdmRate135MbpsBW40MHz ();
              }
            else
              {
                mode = WifiPhy::GetOfdmRate150MbpsBW40MHz ();
              }
            break;
         case 6:
            if (!GetGuardInterval() && !GetChannelBonding())
             {
                mode = WifiPhy::GetOfdmRate58_5MbpsBW20MHz ();
   
              }
           else if(GetGuardInterval() && !GetChannelBonding())
              {
                mode =  WifiPhy::GetOfdmRate65MbpsBW20MHzShGi ();
         
              }
            else if (!GetGuardInterval() && GetChannelBonding())
              {
                mode = WifiPhy::GetOfdmRate121_5MbpsBW40MHz ();
       
              }
           else
              {
                mode= WifiPhy::GetOfdmRate135MbpsBW40MHzShGi ();
            
              }
            break;
         case 5:
            if (!GetGuardInterval() && !GetChannelBonding())
             {
                mode = WifiPhy::GetOfdmRate52MbpsBW20MHz ();
    
              }
           else if(GetGuardInterval() && !GetChannelBonding())
              {
                mode = WifiPhy::GetOfdmRate57_8MbpsBW20MHz ();
              }
            else if (!GetGuardInterval() && GetChannelBonding())
              {
              mode = WifiPhy::GetOfdmRate108MbpsBW40MHz ();
       
              }
            else
              {
               mode = WifiPhy::GetOfdmRate120MbpsBW40MHz ();
         
              }
            break;
         case 4:
            if (!GetGuardInterval() && !GetChannelBonding())
             {
                mode = WifiPhy::GetOfdmRate39MbpsBW20MHz ();
              }
           else if(GetGuardInterval() && !GetChannelBonding())
              {
                mode = WifiPhy::GetOfdmRate43_3MbpsBW20MHz ();
             }
            else if (!GetGuardInterval() && GetChannelBonding())
              {
                mode = WifiPhy::GetOfdmRate81MbpsBW40MHz ();
    
              }
            else
              {
                mode = WifiPhy::GetOfdmRate90MbpsBW40MHz ();
          
              }
            break;
         case 3:
            if (!GetGuardInterval() && !GetChannelBonding())
             {
                mode =  WifiPhy::GetOfdmRate26MbpsBW20MHz ();
    
             }
         else if(GetGuardInterval() && !GetChannelBonding())
             {
                mode = WifiPhy::GetOfdmRate28_9MbpsBW20MHz ();
        
              }
            else if (!GetGuardInterval() && GetChannelBonding())
             {
                mode = WifiPhy::GetOfdmRate54MbpsBW40MHz ();
       
              }
            else
              {
                mode = WifiPhy::GetOfdmRate60MbpsBW40MHz ();
              }
            break;
         case 2:
            if (!GetGuardInterval() && !GetChannelBonding())
             {
                mode = WifiPhy::GetOfdmRate19_5MbpsBW20MHz ();
   
              }
           else if(GetGuardInterval() && !GetChannelBonding())
             {
                mode = WifiPhy::GetOfdmRate21_7MbpsBW20MHz ();
       
              }
           else if (!GetGuardInterval() && GetChannelBonding())
             {
               mode =  WifiPhy::GetOfdmRate40_5MbpsBW40MHz ();
   
             }
            else
              {
                mode = WifiPhy::GetOfdmRate45MbpsBW40MHz ();
             
             }
            break;
         case 1:
           if (!GetGuardInterval() && !GetChannelBonding())
            {
              mode = WifiPhy::GetOfdmRate13MbpsBW20MHz ();
    
              }
           else if(GetGuardInterval() && !GetChannelBonding())
              {
               mode =  WifiPhy::GetOfdmRate14_4MbpsBW20MHz ();
              }
            else if (!GetGuardInterval() && GetChannelBonding())
              {
                mode = WifiPhy::GetOfdmRate27MbpsBW40MHz ();
       
             }
            else
              {
                mode = WifiPhy::GetOfdmRate30MbpsBW40MHz ();
              }
            break;
        case 0:
        default:
           if (!GetGuardInterval() && !GetChannelBonding())
             {
               mode = WifiPhy::GetOfdmRate6_5MbpsBW20MHz ();
                
              }
           else if(GetGuardInterval() && !GetChannelBonding())
              {
                mode = WifiPhy::GetOfdmRate7_2MbpsBW20MHz ();
              }
            else if (!GetGuardInterval() && GetChannelBonding())
              {
                mode = WifiPhy::GetOfdmRate13_5MbpsBW40MHz ();
   
              }
            else
              {
                mode = WifiPhy::GetOfdmRate15MbpsBW40MHz ();
              }
           break;
          }
  return mode;
 }*/

void
NslWifiPhy::SetNode (Ptr<Node> nodePtr)
{
  NS_LOG_FUNCTION (this << nodePtr);
  m_node = nodePtr;
}

void
NslWifiPhy::ResetDriver (void)
{
  NS_LOG_FUNCTION (this);
  m_isDriverInitialized = false;
  //m_utility = NULL;
}

void
NslWifiPhy::InitDriver (void)
{
  NS_LOG_FUNCTION (this);
 
  if (!m_isDriverInitialized)
    {
      NS_LOG_DEBUG ("NslWifiPhy:Driver being initialized at Node #" << m_node->GetId ());
      // setting default wifi mode
      SetCurrentWifiMode (WifiPhy::GetMode(0));
      m_utility = m_node->GetObject<WirelessModuleUtility> ();
     
   
      if (m_utility != NULL)
        {
          NS_LOG_FUNCTION ("ok je vais envoyer");
          m_utility->SetRssMeasurementCallback (MakeCallback (&NslWifiPhy::MeasureRss, this));
          m_utility->SetSendPacketCallback (MakeCallback (&NslWifiPhy::UtilitySendPacket, this));
          m_utility->SetChannelSwitchCallback (MakeCallback (&NslWifiPhy::SetChannelNumber, this));
          UpdatePhyLayerInfo ();
        }
      m_isDriverInitialized = true;
    }
  else
    {
      NS_LOG_DEBUG ("NslWifiPhy:Driver already initialized at Node #" << m_node->GetId ());
    }
  // show some debug messages
  if (m_utility == NULL)
    {
      NS_LOG_DEBUG ("NslWifiPhy:Utility module is *not* installed on Node #" << m_node->GetId ());
    }
}

void
NslWifiPhy::DriverStartTx (Ptr<const Packet> packet, double txPower)
{
  NS_LOG_FUNCTION (this << packet << txPower);
  NS_LOG_FUNCTION ("start de message" << Simulator::Now().GetNanoSeconds());
  // notify utility for start of TX
  if (m_utility != NULL)
    {
      m_utility->StartTxHandler (packet, txPower);
    }
  else
    {
      NS_LOG_DEBUG ("NslWifiPhy:Utility module is *not* installed on Node #" << m_node->GetId ());
    }
  // schedule DriverEndTx
  Simulator::Schedule (m_state->GetDelayUntilIdle (), &NslWifiPhy::DriverEndTx, this, packet, txPower);
}

void
NslWifiPhy::DriverEndTx (Ptr<const Packet> packet, double txPower)
{
  NS_LOG_FUNCTION (this << packet << txPower);

  /*
   * TX can not be interrupted. Hence we do not have to check state within this
   * function.
   */

  // notify utility for end of TX
  if (m_utility != NULL)
    {
      m_utility->EndTxHandler (packet, txPower);
    }
  else
    {
      NS_LOG_DEBUG ("NslWifiPhy:Utility module is *not* installed on Node #" << m_node->GetId ());
    }
}

bool
NslWifiPhy::DriverStartRx (Ptr<Packet> packet, double startRssW)
{
  
  NS_LOG_FUNCTION (this << packet << startRssW);

  bool isPacketToBeReceived = true;

  // notify utility for start of RX
  if (m_utility != NULL)
    {
      isPacketToBeReceived =  m_utility->StartRxHandler (packet, startRssW);
    }
  else
    {
      NS_LOG_DEBUG ("NslWifiPhy:Utility module is *not* installed on Node #" << m_node->GetId ());
    }

    Simulator::Schedule (m_state->GetDelayUntilIdle (), &NslWifiPhy::DriverEndTx, this, packet, startRssW);

  return isPacketToBeReceived;
}

void
NslWifiPhy::DriverEndRx (Ptr<Packet> packet, double averageRssW,
                         const bool isSuccessfullyReceived)
{
  NS_LOG_FUNCTION ("fin de message" << Simulator::Now().GetNanoSeconds());
  NS_LOG_FUNCTION (this << packet << averageRssW << isSuccessfullyReceived);

  // notify utility for end of RX
  if (m_utility != 0)
    {
      m_utility->EndRxHandler (packet, averageRssW, isSuccessfullyReceived);
    }
  else
    {
      NS_LOG_DEBUG ("NslWifiPhy:Utility module is *not* installed on Node #" << m_node->GetId ());
    }
}


void
NslWifiPhy::SetCurrentWifiMode (WifiMode mode)
{
  NS_LOG_FUNCTION (this);
  m_currentWifiMode = mode;
  m_txVector.SetMode(mode);
  // notify utility
  if (m_utility)
    {
      UpdatePhyLayerInfo ();
    }
  else
    {
      NS_LOG_DEBUG ("NslWifiPhy:CurrentWifiModeUtility is *not* installed on Node #" << m_node->GetId ());
    }
}

double
NslWifiPhy::MeasureRss (void)
{
  NS_LOG_FUNCTION (this);
  return m_interference.CurrentNodeRss (m_txVector);
}

void
NslWifiPhy::UtilitySendPacket(Ptr<Packet> packet, double &powerW, int utilitySendMode)
{
  NS_LOG_FUNCTION (this << packet << powerW << utilitySendMode);

  // Convert power in Watts to a power level
  uint8_t powerLevel;
  powerLevel = (uint8_t)(m_nTxPower * (WToDbm(powerW) - m_txPowerBaseDbm) /
               (m_txPowerEndDbm - m_txPowerBaseDbm));

  if (powerLevel >= 0 && powerLevel < m_nTxPower)
    {
      NS_LOG_DEBUG ("NslWifiPhy:Inside send packet callback at node #" <<
                    m_node->GetId() << ". Sending packet.");
      switch (utilitySendMode)
        {
        case WirelessModuleUtility::SEND_AS_JAMMER:
          NS_LOG_DEBUG ("NslWifiPhy: Sending packet as jammer." << packet);
          NS_LOG_INFO ("NslWifiPhy: powerLevel"<< powerLevel);
          m_txVector.SetPreambleType(WIFI_PREAMBLE_INVALID);
          SendPacket (packet, m_txVector, WIFI_PREAMBLE_INVALID, powerLevel);
          break;
        case WirelessModuleUtility::SEND_AS_HONEST:
          {
            WifiMacHeader hdr;
            hdr.SetType (WIFI_MAC_DATA);
            packet->AddHeader (hdr);
            NS_LOG_DEBUG ("NslWifiPhy: Sending packet as honest node.");
            m_txVector.SetPreambleType(WIFI_PREAMBLE_LONG);
            SendPacket (packet, m_txVector, WIFI_PREAMBLE_LONG, powerLevel);
          }
          break;
        case WirelessModuleUtility::SEND_AS_OTHERS:
          NS_FATAL_ERROR ("NslWifiPhy:Undefined utility send packet mode!");
          break;
        default:
          break;
        }
      // update the actual TX power
      powerW = DbmToW (m_txPowerBaseDbm);
      powerW += powerLevel * DbmToW ((m_txPowerEndDbm - m_txPowerBaseDbm) / m_nTxPower);
    }
  else
    {
      NS_LOG_DEBUG ("NslWifiPhy: Node # "<< m_node->GetId () <<
                    "Error in send packet callback. Incorrect power level.");
      // set sent power to 0 to indicate error
      powerW = 0;
    }
}

void
NslWifiPhy::UpdatePhyLayerInfo (void)
{
  NS_LOG_FUNCTION (this);
  m_phyLayerInfo.currentChannel = m_channelNumber;
  m_phyLayerInfo.maxTxPowerW = DbmToW (m_txPowerEndDbm);
  m_phyLayerInfo.minTxPowerW = DbmToW (m_txPowerBaseDbm);
  m_phyLayerInfo.TxGainDb = m_txGainDb;
  m_phyLayerInfo.RxGainDb = m_rxGainDb;
  m_phyLayerInfo.numOfChannels = 11;  // XXX assuming US standard
  m_phyLayerInfo.phyRate = m_currentWifiMode.GetDataRate(m_txVector);
  m_phyLayerInfo.channelSwitchDelay = m_channelSwitchDelay;
  // notify utility
  if (m_utility != NULL)
    {
      m_utility->SetPhyLayerInfo (m_phyLayerInfo);
    }
  else
    {
      NS_LOG_DEBUG ("NslWifiPhy:Utility is *not* installed on Node #" <<
                    m_node->GetId ());
    }
}

void
NslWifiPhy::StartTx (Ptr<Packet> packet, WifiTxVector txVector,Time txDuration)
{
  NS_LOG_DEBUG ("Start transmission: signal power before antenna gain=" << GetPowerDbm (txVector.GetTxPowerLevel ()) << "dBm");

  m_channel->Send (this, packet, GetPowerDbm (txVector.GetTxPowerLevel ()), txVector.GetMode(),txVector.GetPreambleType (),txVector);
}

/*void
NslWifiPhy::StartTx (Ptr<Packet> packet, WifiTxVector txVector, Time txDuration)
{
  NS_LOG_DEBUG ("Start transmission: signal power before antenna gain=" << GetPowerDbm (txVector.GetTxPowerLevel ()) << "dBm");
  m_channel->Send (this, packet, GetTxPowerForTransmission (txVector) + GetTxGain (), txDuration,txVector,txVector.GetPreambleType ());
}*/




} // namespace ns3
