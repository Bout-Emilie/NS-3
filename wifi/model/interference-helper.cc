 #include <algorithm>

#include "ns3/simulator.h"
 #include "ns3/log.h"
 #include "ns3/packet.h"
 #include "interference-helper.h"
 #include "wifi-phy.h"
 #include "error-rate-model.h"
 #include "wifi-utils.h"
 #include "wifi-ppdu.h"
 #include "wifi-psdu.h"
 
 namespace ns3 {
 
 NS_LOG_COMPONENT_DEFINE ("InterferenceHelper");
 
 /****************************************************************
  *       PHY event class
  ****************************************************************/
 
    Event::Event (Ptr<const Packet> packet, WifiTxVector txVector, Time duration, double rxPower)
    :   m_packet(packet),
        m_txVector (txVector),
        m_startTime (Simulator::Now ()),
        m_endTime (m_startTime + duration),
        m_rxPowerW (rxPower)
    {
      NS_LOG_FUNCTION(this );
    }
 
    Event::~Event ()
    {
    }
 
   /* Ptr<const WifiPsdu>
    Event::GetPsdu (void) const
    {
    return m_ppdu->GetPsdu ();
    }
    
    Ptr<const WifiPpdu>
    Event::GetPpdu (void) const
    {
    return m_ppdu;
    }*/
    
    Ptr<const Packet>
    Event::GetPacket(void) const{
        return m_packet;
    }
    Time
    Event::GetStartTime (void) const
    {
    return m_startTime;
    }
    
    Time
    Event::GetEndTime (void) const
    {
    return m_endTime;
    }
    
    Time
    Event::GetDuration (void) const
    {
    return m_endTime - m_startTime;
    }
    
    double
    Event::GetRxPowerW (void) const
    {
    return m_rxPowerW;
    }
    
    WifiTxVector
    Event::GetTxVector (void) const
    {
    return m_txVector;
    }

    enum WifiPreamble
    Event::GetPreambleType (void) const
    {
    return m_preamble;
    }


    WifiMode
    Event::GetPayloadMode (void) const
    {
    return m_txVector.GetMode ();
    }
    
   /* std::ostream & operator << (std::ostream &os, const Event &event)
    {
    os << "start=" << event.GetStartTime () << ", end=" << event.GetEndTime ()
        << ", TXVECTOR=" << event.GetTxVector ()
        << ", power=" << event.GetRxPowerW () << "W";
       // << ", PPDU=" << event.GetPpdu ();
    return os;
    }
*/

/****************************************************************
  *       Class which records SNIR change events for a
  *       short period of time.
  ****************************************************************/
 
InterferenceHelper::NiChange::NiChange (Ptr<Event> event, double power)
  : //m_time(time),
    m_event (event),
    //m_delta (delta)
    m_power (power)
    
{
}
/*Time
InterferenceHelper::NiChange::GetTime (void) const
{
  return m_time;
}*/



/*double
InterferenceHelper::NiChange::GetDelta (void) const
{
  return m_delta;
}*/

double
InterferenceHelper::NiChange::GetPower (void) const
{
  return m_power;
}

/*void
InterferenceHelper::NiChange::AddDelta (double delta) 
{
  m_delta+= delta;
}*/

void
InterferenceHelper::NiChange::AddPower(double power) 
{
  m_power+= power;
}

/*bool
InterferenceHelper::NiChange::operator < (const InterferenceHelper::NiChange& o) const
{
  return (m_time < o.m_time);
}*/

std::ostream & operator << (std::ostream &os, const Event &event)
 {
   os << "start=" << event.GetStartTime () << ", end=" << event.GetEndTime ()
      << ", TXVECTOR=" << event.GetTxVector ()
      << ", power=" << event.GetRxPowerW () << "W"
      ;
   return os;
 }

InterferenceHelper::NiChanges::const_iterator
InterferenceHelper::GetPosition ( Time moment) const
{
  return m_niChanges.upper_bound (moment);

}
/*InterferenceHelper::NiChanges::const_iterator
 InterferenceHelper::GetPosition (Time moment) const
 {
   NS_LOG_INFO(moment);
   return m_niChanges.upper_bound (moment);
 }*/
 
 /*double
 InterferenceHelper::NiChange::GetPower (void) const
 {
   return m_power;
 }
 
 void
 InterferenceHelper::NiChange::AddPower (double power)
 {
   m_power += power;
 }
 */
 Ptr<Event>
 InterferenceHelper::NiChange::GetEvent (void) const
 {
   return m_event;
 }

 /****************************************************************
  *       The actual InterferenceHelper
  ****************************************************************/
 
 InterferenceHelper::InterferenceHelper ()
   :m_errorRateModel (0), 
    m_numRxAntennas (1),
     m_firstPower (0),
     m_rxing (false)
 {
   // Always have a zero power noise event in the list
   AddNiChangeEvent ( Time(0),NiChange (0, 0.0));
   NS_LOG_FUNCTION(this<< m_niChanges.size());
 }
 
 InterferenceHelper::~InterferenceHelper ()
 {
   EraseEvents ();
   m_errorRateModel = 0;
   
 }
 
 Ptr<Event>
 InterferenceHelper::Add (Ptr<const Packet> packet, WifiTxVector txVector, Time duration, double rxPowerW)
 {
   NS_LOG_FUNCTION(this << rxPowerW);
   NS_LOG_FUNCTION(this << duration);
   NS_LOG_FUNCTION(this<< packet << txVector << duration << rxPowerW);
   Ptr<Event> event = Create<Event> (packet, txVector, duration, rxPowerW);
    NS_LOG_FUNCTION(this<<event);
   AppendEvent (event);
   return event;
 }
 
 void
 InterferenceHelper::AddForeignSignal (Time duration, double rxPowerW)
 {
   // Parameters other than duration and rxPowerW are unused for this type
   // of signal, so we provide dummy versions
   NS_LOG_FUNCTION(this);
   WifiMacHeader hdr;
   hdr.SetType (WIFI_MAC_QOSDATA);
   //Ptr<WifiPpdu> fakePpdu = Create<WifiPpdu> (Create<WifiPsdu> (Create<Packet> (0), hdr),WifiTxVector (), duration, 0);
   Ptr<Packet> p = Create<Packet>();
   Add (p, WifiTxVector (), duration, rxPowerW);
 }
 
 void
 InterferenceHelper::SetNoiseFigure (double value)
 {
   NS_LOG_FUNCTION(this);
   m_noiseFigure = value;
 }
 double
InterferenceHelper::GetNoiseFigure (void) const
{
  NS_LOG_FUNCTION(this);
  return m_noiseFigure;
}

 
 void
 InterferenceHelper::SetErrorRateModel (const Ptr<ErrorRateModel> rate)
 {
   m_errorRateModel = rate;
 }
 
 Ptr<ErrorRateModel>
 InterferenceHelper::GetErrorRateModel (void) const
 {
   return m_errorRateModel;
 }
 
 void
 InterferenceHelper::SetNumberOfReceiveAntennas (uint8_t rx)
 {
   m_numRxAntennas = rx;
 }
 
 Time
 InterferenceHelper::GetEnergyDuration (double energyW) 
 {
   NS_LOG_FUNCTION(this);
   Time now = Simulator::Now ();
   auto i = GetPreviousPosition (now);
   Time end = i->first;
   for (; i != m_niChanges.end (); ++i)
     {
       double noiseInterferenceW = i->second.GetPower ();
       end = i->first;
       if (noiseInterferenceW < energyW)
         {
           break;
         }
     }
   return end > now ? end - now : MicroSeconds (0);

   /*Time now = Simulator::Now ();
  double noiseInterferenceW = 0.0;
  Time end = now;
  NS_LOG_INFO("test size" << m_niChanges.size());
  NS_LOG_INFO("test firstPower" << m_firstPower);
  auto i = GetPreviousPosition (now);
  for (; i != m_niChanges.end ();  ++i)
    {
      NS_LOG_INFO(i->second.GetPower());
      NS_LOG_INFO(i->second.GetEvent().GetStartTime());
      noiseInterferenceW = i->second.GetPower();
      end = i->second.GetTime ();
      if (noiseInterferenceW < energyW)
        {
            NS_LOG_INFO("essais2");
          break;
        }
    }
  return end > now ? end - now : MicroSeconds (0);*/
 }
 

 void
 InterferenceHelper::AppendEvent (Ptr<Event> event)
 {
   /*NS_LOG_FUNCTION(this << m_rxing);
   NS_LOG_FUNCTION (this);
   double previousPowerStart = 0;
   double previousPowerEnd = 0;
   previousPowerStart = GetPreviousPosition (event->GetStartTime ());
   previousPowerEnd = GetPreviousPosition (event->GetEndTime ());
 
   if (!m_rxing)
     {
        = previousPowerStart;
       // Always leave the first zero power noise event in the list
       m_niChanges.erase (++(m_niChanges.begin ()),
                          GetNextPosition (event->GetStartTime ()));
     }
   auto first = AddNiChangeEvent (NiChange (previousPowerStart, event));
   auto last = AddNiChangeEvent ( NiChange (previousPowerEnd, event));
   for (auto i = first; i != last; ++i)
     {
       i->second.AddDelta (event->GetRxPowerW ());
     }
*/
  NS_LOG_FUNCTION(this << event);
   
   double previousPowerStart = 0.0;
   double previousPowerEnd = 0.0;
   std::multimap<Time, InterferenceHelper::NiChange>::iterator it;
   NS_LOG_INFO(m_niChanges.size());
for (it = m_niChanges.begin(); it != m_niChanges.end(); ++it)
	   {
	       NiChange& d = (*it).second;
         NS_LOG_INFO(this << (*it).first << "\t::==\t" << d.GetPower());
	   }

   double e = GetPreviousPosition (event->GetStartTime ())->second.GetPower();
   NS_LOG_INFO(this << "test previous evenet"<< e);
   previousPowerStart = GetPreviousPosition (event->GetStartTime ())->second.GetPower ();
   previousPowerEnd = GetPreviousPosition (event->GetEndTime ())->second.GetPower ();
   
    NS_LOG_INFO("rentre");
  /*if (!m_rxing)
    {
      NiChanges::const_iterator nowIterator = GetPosition (now);
       
      for (NiChanges::const_iterator i = m_niChanges.begin (); i != nowIterator; ++i)
        {
          NS_LOG_FUNCTION("Append et oui je passe ici " <<i->second.GetDelta () );
          m_firstPower += i->second.GetDelta ();
        }
        NS_LOG_FUNCTION(m_firstPower);
      m_niChanges.erase (m_niChanges.begin (), nowIterator);
      NiChange test= NiChange(event->GetStartTime(),event->GetRxPowerW());
      m_niChanges.insert (m_niChanges.begin (), std::make_pair (event->GetStartTime(),test));
    }
  else
    {
      AddNiChangeEvent (NiChange (event->GetStartTime (), event->GetRxPowerW ()));
    }
  AddNiChangeEvent (NiChange (event->GetEndTime (), -event->GetRxPowerW ()));*/

if (!m_rxing)
    {
      NS_LOG_FUNCTION(this << event);
      NS_LOG_INFO("J EFFACE");
      m_niChanges.erase (++(m_niChanges.begin ()),GetNextPosition (event->GetStartTime()) );
    }
       NS_LOG_INFO("sorti");
       auto first = AddNiChangeEvent(event->GetStartTime (),NiChange(event, previousPowerStart));
      
      auto end = AddNiChangeEvent(event->GetEndTime (),NiChange(event, previousPowerEnd));
      for(auto i = first;i !=end;++i ){
        i->second.AddPower (event->GetRxPowerW ());
      } 
      NS_LOG_INFO("ma taille apres ajout" << m_niChanges.size());

 }


 
 double
 InterferenceHelper::CalculateSnr (double signal, double noiseInterference, uint16_t channelWidth) const
 {
   NS_LOG_FUNCTION(this);
   //thermal noise at 290K in J/s = W
   static const double BOLTZMANN = 1.33e-23;
   //Nt is the power of thermal noise in W
   double Nt = BOLTZMANN * 290 * channelWidth * 1e6;
   //receiver noise Floor (W) which accounts for thermal noise and non-idealities of the receiver
   double noiseFloor = m_noiseFigure * Nt;
   double noise = noiseFloor + noiseInterference;
   double snr = signal / noise; //linear scale
   NS_LOG_DEBUG ("bandwidth(MHz)=" << channelWidth << ", signal(W)= " << signal << ", noise(W)=" << noiseFloor << ", interference(W)=" << noiseInterference << ", snr=" << RatioToDb(snr) << "dB");
  /* if (m_numRxAntennas > txVector.GetNss ())
     {
       gain = static_cast<double>(m_numRxAntennas) / txVector.GetNss (); //compute gain offered by diversity for AWGN
     }
   NS_LOG_DEBUG ("SNR improvement thanks to diversity: " << 10 * std::log10 (gain) << "dB");
   snr *= gain;*/
   return snr;
 }
 
 double
 InterferenceHelper::CalculateNoiseInterferenceW (Ptr<Event> event, NiChanges *ni) const
 {
   NS_LOG_FUNCTION(this);
   /*double noiseInterferenceW = m_firstPower;
   auto it = m_niChanges.find (event->GetStartTime ());
   for (; it != m_niChanges.end () && it->first < Simulator::Now (); ++it)
     {
       noiseInterferenceW = it->second.GetPower () - event->GetRxPowerW ();
     }
   it = m_niChanges.find (event->GetStartTime ());
   for (; it != m_niChanges.end () && it->second.GetEvent () != event; ++it);
   ni->emplace (event->GetStartTime (), NiChange (0, event));
   while (++it != m_niChanges.end () && it->second.GetEvent () != event)
     {
       ni->insert (*it);
     }
   ni->emplace (event->GetEndTime (), NiChange (0, event));
   NS_ASSERT_MSG (noiseInterferenceW >= 0, "CalculateNoiseInterferenceW returns negative value " << noiseInterferenceW);
   return noiseInterferenceW;*/

   
   /*double noiseInterference = m_firstPower;
  //NS_ASSERT (m_rxing);
  for (NiChanges::const_iterator i = m_niChanges.begin (); i != m_niChanges.end (); ++i)
    {
      if ((event->GetEndTime () == i->second.GetTime ()) && event->GetRxPowerW () == -i->second.GetDelta ())
        {
          break;
        }
      ni->insert (*i);
    }
  ni->emplace (event->GetEndTime(), NiChange (event->GetStartTime (), noiseInterference));
  //ni->push_back (NiChange (event->GetEndTime (), 0));
  return noiseInterference;*/

  double noiseInterferenceW = m_firstPower;
   auto it = m_niChanges.find (event->GetStartTime ());
   for (; it != m_niChanges.end () && it->first < Simulator::Now (); ++it)
     {
       noiseInterferenceW = it->second.GetPower () - event->GetRxPowerW ();
     }
   it = m_niChanges.find (event->GetStartTime ());
   for (; it != m_niChanges.end () && it->second.GetEvent () != event; ++it);
   ni->emplace (event->GetStartTime (), NiChange (event, 0));
   while (++it != m_niChanges.end () && it->second.GetEvent () != event)
     {
       ni->insert (*it);
     }
   ni->emplace (event->GetEndTime (), NiChange ( event, 0));
   NS_ASSERT_MSG (noiseInterferenceW >= 0, "CalculateNoiseInterferenceW returns negative value " << noiseInterferenceW);
   NS_LOG_FUNCTION(this<<noiseInterferenceW);
   return noiseInterferenceW;
 }
 
 double
 InterferenceHelper::CalculateChunkSuccessRate (double snir, Time duration, WifiMode mode, WifiTxVector txVector) const
 {
   NS_LOG_FUNCTION(this);
   if (duration.IsZero ())
     {
       return 1.0;
     }
   uint64_t rate = mode.GetDataRate (txVector.GetChannelWidth ());
   uint64_t nbits = static_cast<uint32_t> (rate * duration.GetSeconds ());


   NS_LOG_INFO("mode in CalculateChunckSuccessRate"<< mode);
   NS_LOG_INFO("txVector in CalculateChunckSuccessRate"<< txVector);
   NS_LOG_INFO("snir in CalculateChunckSuccessRate"<< snir);
   NS_LOG_INFO("nbits in CalculateChunckSuccessRate"<< nbits);
   NS_LOG_INFO("error in CalculateChunckSuccessRate"<< m_errorRateModel);
   double csr = m_errorRateModel->GetChunkSuccessRate (mode,txVector, snir, nbits);
   NS_LOG_INFO( csr);
   return csr;
 }

 double
InterferenceHelper::CalculateNoiseFloor (WifiTxVector mode) const
{
  NS_LOG_FUNCTION(this);
  // thermal noise at 290K in J/s = W
  static const double BOLTZMANN = 1.33e-23;
  // Nt is the power of thermal noise in W
  double Nt = BOLTZMANN * 290.0 * mode.GetChannelWidth();
  /*
   * Receiver noise Floor (W) which accounts for thermal noise and non-
   * idealities of the receiver.
   */
  return m_noiseFigure * Nt;
}
 
 double
 InterferenceHelper::CalculatePayloadChunkSuccessRate (double snir, Time duration, WifiTxVector txVector) const
 {
   NS_LOG_FUNCTION(this);
   if (duration.IsZero ())
     {
       return 1.0;
     }
   WifiMode mode = txVector.GetMode ();
   uint32_t rate = mode.GetDataRate (txVector);
   uint32_t nbits = static_cast<uint32_t> (rate * duration.GetSeconds ());
   nbits /= txVector.GetNss (); //divide effective number of bits by NSS to achieve same chunk error rate as SISO for AWGN
   double csr = m_errorRateModel->GetChunkSuccessRate (mode, txVector, snir, nbits);
   return csr;
 }
 
 double
 InterferenceHelper::CalculatePayloadPer (Ptr<const Event> event, NiChanges *ni, std::pair<Time, Time> window) const
 {
   NS_LOG_FUNCTION (this << window.first << window.second);
   const WifiTxVector txVector = event->GetTxVector ();
   double psr = 1.0; /* Packet Success Rate */
   auto j = ni->begin ();
   Time previous = j->first;
   WifiMode payloadMode = event->GetTxVector ().GetMode ();
   WifiPreamble preamble = txVector.GetPreambleType ();
   Time phyHeaderStart = j->first + WifiPhy::GetPlcpPreambleDuration (txVector); //PPDU start time + preamble
   Time phyLSigHeaderEnd = phyHeaderStart + WifiPhy::GetPlcpHeaderDuration (txVector); //PPDU start time + preamble + L-SIG
   Time phyTrainingSymbolsStart = phyLSigHeaderEnd + WifiPhy::GetPlcpHtSigHeaderDuration (preamble) + WifiPhy::GetPlcpSigA1Duration (preamble) + WifiPhy::GetPlcpSigA2Duration (preamble); //PPDU start time + preamble + L-SIG + HT-SIG or SIG-A
   Time phyPayloadStart = phyTrainingSymbolsStart + WifiPhy::GetPlcpTrainingSymbolDuration (txVector) + WifiPhy::GetPlcpSigBDuration (preamble); //PPDU start time + preamble + L-SIG + HT-SIG or SIG-A + Training + SIG-B
   Time windowStart = phyPayloadStart + window.first;
   Time windowEnd = phyPayloadStart + window.second;
   double noiseInterferenceW = m_firstPower;
   double powerW = event->GetRxPowerW ();
   while (++j != ni->end ())
     {
       Time current = j->first;
       NS_LOG_DEBUG ("previous= " << previous << ", current=" << current);
       NS_ASSERT (current >= previous);
       double snr = CalculateSnr (powerW, noiseInterferenceW, txVector.GetChannelWidth ());
       //Case 1: Both previous and current point to the windowed payload
       if (previous >= windowStart)
         {
           psr *= CalculatePayloadChunkSuccessRate (snr, Min (windowEnd, current) - previous, txVector);
           NS_LOG_DEBUG ("Both previous and current point to the windowed payload: mode=" << payloadMode << ", psr=" << psr);
         }
       //Case 2: previous is before windowed payload and current is in the windowed payload
       else if (current >= windowStart)
         {
           psr *= CalculatePayloadChunkSuccessRate (snr, Min (windowEnd, current) - windowStart, txVector);
           NS_LOG_DEBUG ("previous is before windowed payload and current is in the windowed payload: mode=" << payloadMode << ", psr=" << psr);
         }
       noiseInterferenceW = j->second.GetPower() - powerW;
       previous = j->first;
       if (previous > windowEnd)
         {
           NS_LOG_DEBUG ("Stop: new previous=" << previous << " after time window end=" << windowEnd);
           break;
         }
     }
   double per = 1 - psr;
   return per;
 }
 
 double
 InterferenceHelper::CalculateNonHtPhyHeaderPer (Ptr<const Event> event, NiChanges *ni) const
 {
   NS_LOG_FUNCTION (this);
   const WifiTxVector txVector = event->GetTxVector ();
   double psr = 1.0; /* Packet Success Rate */
   auto j = ni->begin ();
   Time previous = j->first;
   WifiPreamble preamble = txVector.GetPreambleType ();
   WifiMode headerMode = WifiPhy::GetPlcpHeaderMode (txVector);
   Time phyHeaderStart = j->first + WifiPhy::GetPlcpPreambleDuration (txVector); //PPDU start time + preamble
   Time phyLSigHeaderEnd = phyHeaderStart + WifiPhy::GetPlcpHeaderDuration (txVector); //PPDU start time + preamble + L-SIG
   Time phyTrainingSymbolsStart = phyLSigHeaderEnd + WifiPhy::GetPlcpHtSigHeaderDuration (preamble) + WifiPhy::GetPlcpSigA1Duration (preamble) + WifiPhy::GetPlcpSigA2Duration (preamble); //PPDU start time + preamble + L-SIG + HT-SIG or SIG-A
   Time phyPayloadStart = phyTrainingSymbolsStart + WifiPhy::GetPlcpTrainingSymbolDuration (txVector) + WifiPhy::GetPlcpSigBDuration (preamble); //PPDU start time + preamble + L-SIG + HT-SIG or SIG-A + Training + SIG-B
   double noiseInterferenceW = m_firstPower;
   double powerW = event->GetRxPowerW ();
   while (++j != ni->end ())
     {
       Time current = j->first;
       NS_LOG_DEBUG ("previous= " << previous << ", current=" << current);
       NS_ASSERT (current >= previous);
       double snr = CalculateSnr (powerW, noiseInterferenceW, txVector.GetChannelWidth ());
       //Case 1: previous and current after payload start
       if (previous >= phyPayloadStart)
         {
           psr *= 1;
           NS_LOG_DEBUG ("Case 1 - previous and current after payload start: nothing to do");
         }
       //Case 2: previous is in training or in SIG-B: non-HT will not enter here since it didn't enter in the last two and they are all the same for non-HT
       else if (previous >= phyTrainingSymbolsStart)
         {
           NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
           psr *= 1;
           NS_LOG_DEBUG ("Case 2 - previous is in training or in SIG-B: nothing to do");
         }
       //Case 3: previous is in HT-SIG or SIG-A: non-HT will not enter here since it didn't enter in the last two and they are all the same for non-HT
       else if (previous >= phyLSigHeaderEnd)
         {
           NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
           psr *= 1;
           NS_LOG_DEBUG ("Case 3cii - previous is in HT-SIG or SIG-A: nothing to do");
         }
       //Case 4: previous in L-SIG: HT GF will not reach here because it will execute the previous if and exit
       else if (previous >= phyHeaderStart)
         {
           NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
           //Case 4a: current after payload start
           if (current >= phyPayloadStart)
             {
               psr *= CalculateChunkSuccessRate (snr, phyLSigHeaderEnd - previous, headerMode, txVector);
               NS_LOG_DEBUG ("Case 4a - previous in L-SIG and current after payload start: mode=" << headerMode << ", psr=" << psr);
             }
           //Case 4b: current is in training or in SIG-B. non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
           else if (current >= phyTrainingSymbolsStart)
             {
               NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
               psr *= CalculateChunkSuccessRate (snr, phyLSigHeaderEnd - previous, headerMode, txVector);
               NS_LOG_DEBUG ("Case 4a - previous in L-SIG and current is in training or in SIG-B: mode=" << headerMode << ", psr=" << psr);
             }
           //Case 4c: current in HT-SIG or in SIG-A. non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
           else if (current >= phyLSigHeaderEnd)
             {
               NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
               psr *= CalculateChunkSuccessRate (snr, phyLSigHeaderEnd - previous, headerMode, txVector);
               NS_LOG_DEBUG ("Case 4ci - previous is in L-SIG and current in HT-SIG or in SIG-A: mode=" << headerMode << ", psr=" << psr);
             }
           //Case 4d: current with previous in L-SIG
           else
             {
               psr *= CalculateChunkSuccessRate (snr, current - previous, headerMode, txVector);
               NS_LOG_DEBUG ("Case 4d - current with previous in L-SIG: mode=" << headerMode << ", psr=" << psr);
             }
         }
       //Case 5: previous is in the preamble works for all cases
       else
         {
           //Case 5a: current after payload start
           if (current >= phyPayloadStart)
             {
               psr *= CalculateChunkSuccessRate (snr, phyLSigHeaderEnd - phyHeaderStart, headerMode, txVector);
               NS_LOG_DEBUG ("Case 5aii - previous is in the preamble and current is after payload start: mode=" << headerMode << ", psr=" << psr);
             }
           //Case 5b: current is in training or in SIG-B. non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
           else if (current >= phyTrainingSymbolsStart)
             {
               NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
               psr *= CalculateChunkSuccessRate (snr, phyLSigHeaderEnd - phyHeaderStart, headerMode, txVector);
               NS_LOG_DEBUG ("Case 5b - previous is in the preamble and current is in training or in SIG-B: mode=" << headerMode << ", psr=" << psr);
             }
           //Case 5c: current in HT-SIG or in SIG-A. non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
           else if (current >= phyLSigHeaderEnd)
             {
               NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
               psr *= CalculateChunkSuccessRate (snr, phyLSigHeaderEnd - phyHeaderStart, headerMode, txVector);
               NS_LOG_DEBUG ("Case 5b - previous is in the preamble and current in HT-SIG or in SIG-A: mode=" << headerMode << ", psr=" << psr);
             }
           //Case 5d: current is in L-SIG.
           else if (current >= phyHeaderStart)
             {
               NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
               psr *= CalculateChunkSuccessRate (snr, current - phyHeaderStart, headerMode, txVector);
               NS_LOG_DEBUG ("Case 5d - previous is in the preamble and current is in L-SIG: mode=" << headerMode << ", psr=" << psr);
             }
         }
 
       noiseInterferenceW = j->second.GetPower () - powerW;
       previous = j->first;
     }
 
   double per = 1 - psr;
   return per;
 }
 
 double
 InterferenceHelper::CalculateHtPhyHeaderPer (Ptr<const Event> event, NiChanges *ni) const
 {
   NS_LOG_FUNCTION (this); 
   const WifiTxVector txVector = event->GetTxVector ();
   double psr = 1.0; /* Packet Success Rate */
   auto j = ni->begin ();
   Time previous = j->first;
   WifiPreamble preamble = txVector.GetPreambleType ();
   WifiMode mcsHeaderMode;
   if (preamble == WIFI_PREAMBLE_HT_MF || preamble == WIFI_PREAMBLE_HT_GF)
     {
       //mode for PHY header fields sent with HT modulation
       mcsHeaderMode = WifiPhy::GetHtPlcpHeaderMode ();
     }
   else if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_VHT_MU)
     {
       //mode for PHY header fields sent with VHT modulation
       mcsHeaderMode = WifiPhy::GetVhtPlcpHeaderMode ();
     }
   else if (preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_HE_MU)
     {
       //mode for PHY header fields sent with HE modulation
       mcsHeaderMode = WifiPhy::GetHePlcpHeaderMode ();
     }
   WifiMode headerMode = WifiPhy::GetPlcpHeaderMode (txVector);
   Time phyHeaderStart = j->first + WifiPhy::GetPlcpPreambleDuration (txVector); //PPDU start time + preamble
   Time phyLSigHeaderEnd = phyHeaderStart + WifiPhy::GetPlcpHeaderDuration (txVector); //PPDU start time + preamble + L-SIG
   Time phyTrainingSymbolsStart = phyLSigHeaderEnd + WifiPhy::GetPlcpHtSigHeaderDuration (preamble) + WifiPhy::GetPlcpSigA1Duration (preamble) + WifiPhy::GetPlcpSigA2Duration (preamble); //PPDU start time + preamble + L-SIG + HT-SIG or SIG-A
   Time phyPayloadStart = phyTrainingSymbolsStart + WifiPhy::GetPlcpTrainingSymbolDuration (txVector) + WifiPhy::GetPlcpSigBDuration (preamble); //PPDU start time + preamble + L-SIG + HT-SIG or SIG-A + Training + SIG-B
   double noiseInterferenceW = m_firstPower;
   double powerW = event->GetRxPowerW ();
   while (++j != ni->end ())
     {
       Time current = j->first;
       NS_LOG_DEBUG ("previous= " << previous << ", current=" << current);
       NS_ASSERT (current >= previous);
       double snr = CalculateSnr (powerW, noiseInterferenceW, txVector.GetChannelWidth ());
       //Case 1: previous and current after payload start: nothing to do
       if (previous >= phyPayloadStart)
         {
           psr *= 1;
           NS_LOG_DEBUG ("Case 1 - previous and current after payload start: nothing to do");
         }
       //Case 2: previous is in training or in SIG-B: non-HT will not enter here since it didn't enter in the last two and they are all the same for non-HT
       else if (previous >= phyTrainingSymbolsStart)
         {
           NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
           //Case 2a: current after payload start
           if (current >= phyPayloadStart)
             {
               psr *= CalculateChunkSuccessRate (snr, phyPayloadStart - previous, mcsHeaderMode, txVector);
               NS_LOG_DEBUG ("Case 2a - previous is in training or in SIG-B and current after payload start: mode=" << mcsHeaderMode << ", psr=" << psr);
             }
           //Case 2b: current is in training or in SIG-B
           else
             {
               psr *= CalculateChunkSuccessRate (snr, current - previous, mcsHeaderMode, txVector);
               NS_LOG_DEBUG ("Case 2b - previous is in training or in SIG-B and current is in training or in SIG-B: mode=" << mcsHeaderMode << ", psr=" << psr);
             }
         }
       //Case 3: previous is in HT-SIG or SIG-A: non-HT will not enter here since it didn't enter in the last two and they are all the same for non-HT
       else if (previous >= phyLSigHeaderEnd)
         {
           NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
           //Case 3a: current after payload start
           if (current >= phyPayloadStart)
             {
               psr *= CalculateChunkSuccessRate (snr, phyPayloadStart - phyTrainingSymbolsStart, mcsHeaderMode, txVector);
               //Case 3ai: VHT or HE format
               if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                 {
                   //SIG-A is sent using non-HT OFDM modulation
                   psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - previous, headerMode, txVector);
                   NS_LOG_DEBUG ("Case 3ai - previous is in SIG-A and current after payload start: mcs mode=" << mcsHeaderMode << ", non-HT mode=" << headerMode << ", psr=" << psr);
                 }
               //Case 3aii: HT formats
               else
                 {
                   psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - previous, mcsHeaderMode, txVector);
                   NS_LOG_DEBUG ("Case 3aii - previous is in HT-SIG and current after payload start: mode=" << mcsHeaderMode << ", psr=" << psr);
                 }
             }
           //Case 3b: current is in training or in SIG-B
           else if (current >= phyTrainingSymbolsStart)
             {
               psr *= CalculateChunkSuccessRate (snr, current - phyTrainingSymbolsStart, mcsHeaderMode, txVector);
               //Case 3bi: VHT or HE format
               if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                 {
                   //SIG-A is sent using non-HT OFDM modulation
                   psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - previous, headerMode, txVector);
                   NS_LOG_DEBUG ("Case 3bi - previous is in SIG-A and current is in training or in SIG-B: mode=" << headerMode << ", psr=" << psr);
                 }
               //Case 3bii: HT formats
               else
                 {
                   psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - previous, mcsHeaderMode, txVector);
                   NS_LOG_DEBUG ("Case 3bii - previous is in HT-SIG and current is in HT training: mode=" << mcsHeaderMode << ", psr=" << psr);
                 }
             }
           //Case 3c: current with previous in HT-SIG or SIG-A
           else
             {
               //Case 3ci: VHT or HE format
               if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                 {
                   //SIG-A is sent using non-HT OFDM modulation
                   psr *= CalculateChunkSuccessRate (snr, current - previous, headerMode, txVector);
                   NS_LOG_DEBUG ("Case 3ci - previous with current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                 }
               //Case 3cii: HT mixed format or HT greenfield
               else
                 {
                   psr *= CalculateChunkSuccessRate (snr, current - previous, mcsHeaderMode, txVector);
                   NS_LOG_DEBUG ("Case 3cii - previous with current in HT-SIG: mode=" << mcsHeaderMode << ", psr=" << psr);
                 }
             }
         }
       //Case 4: previous in L-SIG: HT GF will not reach here because it will execute the previous if and exit
       else if (previous >= phyHeaderStart)
         {
           NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
           //Case 4a: current after payload start
           if (current >= phyPayloadStart)
             {
               //Case 4ai: non-HT format
               if (preamble == WIFI_PREAMBLE_LONG || preamble == WIFI_PREAMBLE_SHORT)
                 {
                   psr *= 1;
                   NS_LOG_DEBUG ("Case 4ai - previous in L-SIG and current after payload start: nothing to do");
                 }
               //Case 4aii: VHT or HE format
               else if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                 {
                   psr *= CalculateChunkSuccessRate (snr, phyPayloadStart - phyTrainingSymbolsStart, mcsHeaderMode, txVector);
                   psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - phyLSigHeaderEnd, headerMode, txVector);
                   NS_LOG_DEBUG ("Case 4aii - previous is in L-SIG and current after payload start: mcs mode=" << mcsHeaderMode << ", non-HT mode=" << headerMode << ", psr=" << psr);
                 }
               //Case 4aiii: HT mixed format
               else
                 {
                   psr *= CalculateChunkSuccessRate (snr, phyPayloadStart - phyLSigHeaderEnd, mcsHeaderMode, txVector);
                   NS_LOG_DEBUG ("Case 4aiii - previous in L-SIG and current after payload start: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                 }
             }
           //Case 4b: current is in training or in SIG-B. non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
           else if (current >= phyTrainingSymbolsStart)
             {
               NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
               //Case 4bi: VHT or HE format
               if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                 {
                   psr *= CalculateChunkSuccessRate (snr, current - phyTrainingSymbolsStart, mcsHeaderMode, txVector);
                   psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - phyLSigHeaderEnd, headerMode, txVector);
                   NS_LOG_DEBUG ("Case 4bi - previous is in L-SIG and current in training or in SIG-B: mcs mode=" << mcsHeaderMode << ", non-HT mode=" << headerMode << ", psr=" << psr);
                 }
               //Case 4bii: HT mixed format
               else
                 {
                   psr *= CalculateChunkSuccessRate (snr, current - phyLSigHeaderEnd, mcsHeaderMode, txVector);
                   NS_LOG_DEBUG ("Case 4bii - previous in L-SIG and current in HT training: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                 }
             }
           //Case 4c: current in HT-SIG or in SIG-A. Non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
           else if (current >= phyLSigHeaderEnd)
             {
               NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
               //Case 4ci: VHT format
               if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                 {
                   psr *= CalculateChunkSuccessRate (snr, current - phyLSigHeaderEnd, headerMode, txVector);
                   NS_LOG_DEBUG ("Case 4ci - previous is in L-SIG and current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                 }
               //Case 4cii: HT mixed format
               else
                 {
                   psr *= CalculateChunkSuccessRate (snr, current - phyLSigHeaderEnd, mcsHeaderMode, txVector);
                   NS_LOG_DEBUG ("Case 4cii - previous in L-SIG and current in HT-SIG: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                 }
             }
           //Case 4d: current with previous in L-SIG
           else
             {
               psr *= 1;
               NS_LOG_DEBUG ("Case 4d - current with previous in L-SIG: nothing to do");
             }
         }
       //Case 5: previous is in the preamble works for all cases
       else
         {
           //Case 5a: current after payload start
           if (current >= phyPayloadStart)
             {
               //Case 5ai: non-HT format (No HT-SIG or Training Symbols)
               if (preamble == WIFI_PREAMBLE_LONG || preamble == WIFI_PREAMBLE_SHORT)
                 {
                   psr *= 1;
                   NS_LOG_DEBUG ("Case 5ai - previous is in the preamble and current is after payload start: nothing to do");
                 }
               //Case 5aii: VHT or HE format
               else if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                 {
                   psr *= CalculateChunkSuccessRate (snr, phyPayloadStart - phyTrainingSymbolsStart, mcsHeaderMode, txVector);
                   psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - phyLSigHeaderEnd, headerMode, txVector);
                   NS_LOG_DEBUG ("Case 5aii - previous is in the preamble and current is after payload start: mcs mode=" << mcsHeaderMode << ", non-HT mode=" << headerMode << ", psr=" << psr);
                 }
               //Case 5aiii: HT formats
               else
                 {
                   psr *= CalculateChunkSuccessRate (snr, phyPayloadStart - phyLSigHeaderEnd, mcsHeaderMode, txVector);
                   NS_LOG_DEBUG ("Case 5aiii - previous is in the preamble and current is after payload start: mcs mode=" << mcsHeaderMode << ", non-HT mode=" << headerMode << ", psr=" << psr);
                 }
             }
           //Case 5b: current is in training or in SIG-B. Non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
           else if (current >= phyTrainingSymbolsStart)
             {
               NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
               //Case 5bi: VHT or HE format
               if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                 {
                   psr *= CalculateChunkSuccessRate (snr, current - phyTrainingSymbolsStart, mcsHeaderMode, txVector);
                   psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - phyLSigHeaderEnd, headerMode, txVector);
                   NS_LOG_DEBUG ("Case 5bi - previous is in the preamble and current in training or in SIG-B: mcs mode=" << mcsHeaderMode << ", non-HT mode=" << headerMode << ", psr=" << psr);
                 }
               //Case 5bii: HT mixed format
               else
                 {
                   psr *= CalculateChunkSuccessRate (snr, current - phyLSigHeaderEnd, mcsHeaderMode, txVector);
                   NS_LOG_DEBUG ("Case 5bii - previous is in the preamble and current in HT training: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                 }
             }
           //Case 5c: current in HT-SIG or in SIG-A. Non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
           else if (current >= phyLSigHeaderEnd)
             {
               NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
               //Case 5ci: VHT or HE format
               if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                 {
                   psr *= CalculateChunkSuccessRate (snr, current - phyLSigHeaderEnd, headerMode, txVector);
                   NS_LOG_DEBUG ("Case 5ci - previous is in preamble and current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                 }
               //Case 5cii: HT formats
               else
                 {
                   psr *= CalculateChunkSuccessRate (snr, current - phyLSigHeaderEnd, mcsHeaderMode, txVector);
                   NS_LOG_DEBUG ("Case 5cii - previous in preamble and current in HT-SIG: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                 }
             }
           //Case 5d: current is in L-SIG. HT-GF will not come here
           else if (current >= phyHeaderStart)
             {
               NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
               psr *= 1;
               NS_LOG_DEBUG ("Case 5d - previous is in the preamble and current is in L-SIG: nothing to do");
             }
         }
 
       noiseInterferenceW = j->second.GetPower () - powerW;
       previous = j->first;
     }
 
   double per = 1 - psr;
   return per;
 }

 struct InterferenceHelper::SnrPer
InterferenceHelper::CalculateNonLegacyPhyHeaderSnrPer (Ptr<Event> event) const
{
  NS_LOG_FUNCTION(this);
  NiChanges ni;
  double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
  double snr = CalculateSnr (event->GetRxPowerW (),
                             noiseInterferenceW,
                             event->GetTxVector().GetChannelWidth ());
  
  /* calculate the SNIR at the start of the plcp header and accumulate
   * all SNIR changes in the snir vector.
   */
  double per = CalculateNonLegacyPhyHeaderPer (event, &ni);
  
  struct SnrPer snrPer;
  snrPer.snr = snr;
  snrPer.per = per;
  return snrPer;
}

struct InterferenceHelper::SnrPer
InterferenceHelper::CalculateLegacyPhyHeaderSnrPer (Ptr<Event> event) const
{
  NS_LOG_FUNCTION(this);
  NiChanges ni;
  double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
  double snr = CalculateSnr (event->GetRxPowerW (),
                             noiseInterferenceW,
                             event->GetTxVector ().GetChannelWidth ());

  /* calculate the SNIR at the start of the plcp header and accumulate
   * all SNIR changes in the snir vector.
   */
  double per = CalculateLegacyPhyHeaderPer (event, &ni);

  struct SnrPer snrPer;
  snrPer.snr = snr;
  snrPer.per = per;
  return snrPer;
}

double
InterferenceHelper::CalculateLegacyPhyHeaderPer (Ptr<const Event> event, NiChanges *ni) const
{
  NS_LOG_FUNCTION (this);
  const WifiTxVector txVector = event->GetTxVector ();
  double psr = 1.0; /* Packet Success Rate */
  auto j = ni->begin ();
  Time previous = j->first;
  WifiPreamble preamble = txVector.GetPreambleType ();
  WifiMode headerMode = WifiPhy::GetPlcpHeaderMode (txVector);
  Time plcpHeaderStart = j->first + WifiPhy::GetPlcpPreambleDuration (txVector); //packet start time + preamble
  Time plcpHsigHeaderStart = plcpHeaderStart + WifiPhy::GetPlcpHeaderDuration (txVector); //packet start time + preamble + L-SIG
  Time plcpTrainingSymbolsStart = plcpHsigHeaderStart + WifiPhy::GetPlcpHtSigHeaderDuration (preamble) + WifiPhy::GetPlcpSigA1Duration (preamble) + WifiPhy::GetPlcpSigA2Duration (preamble); //packet start time + preamble + L-SIG + HT-SIG or SIG-A
  Time plcpPayloadStart = plcpTrainingSymbolsStart + WifiPhy::GetPlcpTrainingSymbolDuration (txVector) + WifiPhy::GetPlcpSigBDuration (preamble); //packet start time + preamble + L-SIG + HT-SIG or SIG-A + Training + SIG-B
  double noiseInterferenceW = m_firstPower;
  double powerW = event->GetRxPowerW ();
  NS_LOG_INFO(preamble);
  NS_LOG_INFO(plcpPayloadStart);
  while (++j != ni->end ())
    {
      Time current = j->first;
      NS_LOG_DEBUG ("previous= " << previous << ", current=" << current);
      NS_ASSERT (current >= previous);
      //Case 1: previous and current after playload start
      if (previous >= plcpPayloadStart)
        {
          NS_LOG_INFO("if1");
          psr *= 1;
          NS_LOG_DEBUG ("Case 1 - previous and current after playload start: nothing to do");
        }
      //Case 2: previous is in training or in SIG-B: legacy will not enter here since it didn't enter in the last two and they are all the same for legacy
      else if (previous >= plcpTrainingSymbolsStart)
        {
          NS_LOG_INFO("if2");
          NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
          psr *= 1;
          NS_LOG_DEBUG ("Case 2 - previous is in training or in SIG-B: nothing to do");
        }
      //Case 3: previous is in HT-SIG or SIG-A: legacy will not enter here since it didn't enter in the last two and they are all the same for legacy
      else if (previous >= plcpHsigHeaderStart)
        {
          NS_LOG_INFO("if3");
          NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
          psr *= 1;
          NS_LOG_DEBUG ("Case 3cii - previous is in HT-SIG or SIG-A: nothing to do");
        }
      //Case 4: previous in L-SIG: HT GF will not reach here because it will execute the previous if and exit
      else if (previous >= plcpHeaderStart)
        {
          NS_LOG_INFO("if4a");
          NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
          //Case 4a: current after payload start
          if (current >= plcpPayloadStart)
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                plcpHsigHeaderStart - previous,
                                                headerMode, txVector);
              NS_LOG_DEBUG ("Case 4a - previous in L-SIG and current after payload start: mode=" << headerMode << ", psr=" << psr);
            }
          //Case 4b: current is in training or in SIG-B. legacy will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= plcpTrainingSymbolsStart)
            {
              NS_LOG_INFO("if4a1");
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                plcpHsigHeaderStart - previous,
                                                headerMode, txVector);
              NS_LOG_DEBUG ("Case 4a - previous in L-SIG and current is in training or in SIG-B: mode=" << headerMode << ", psr=" << psr);
            }
          //Case 4c: current in HT-SIG or in SIG-A. Legacy will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= plcpHsigHeaderStart)
            {
              NS_LOG_INFO("if4c");
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                plcpHsigHeaderStart - previous,
                                                headerMode, txVector);
              NS_LOG_DEBUG ("Case 4ci - previous is in L-SIG and current in HT-SIG or in SIG-A: mode=" << headerMode << ", psr=" << psr);
            }
          //Case 4d: current with previous in L-SIG
          else
            {
              NS_LOG_INFO("if4d");
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                current - previous,
                                                headerMode, txVector);
              NS_LOG_DEBUG ("Case 4d - current with previous in L-SIG: mode=" << headerMode << ", psr=" << psr);
            }
        }
      //Case 5: previous is in the preamble works for all cases
      else
        {
          //Case 5a: current after payload start
          if (current >= plcpPayloadStart)
            {
              NS_LOG_INFO("if5a");
              NS_LOG_INFO(txVector);
              NS_LOG_INFO(headerMode);
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                plcpHsigHeaderStart - plcpHeaderStart,
                                                headerMode, txVector);
              NS_LOG_DEBUG ("Case 5aii - previous is in the preamble and current is after payload start: mode=" << headerMode << ", psr=" << psr);
            }
          //Case 5b: current is in training or in SIG-B. Legacy will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= plcpTrainingSymbolsStart)
            {
              NS_LOG_INFO("if5b");
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                plcpHsigHeaderStart - plcpHeaderStart,
                                                headerMode, txVector);
              NS_LOG_DEBUG ("Case 5b - previous is in the preamble and current is in training or in SIG-B: mode=" << headerMode << ", psr=" << psr);
            }
          //Case 5c: current in HT-SIG or in SIG-A. Legacy will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= plcpHsigHeaderStart)
            {
              NS_LOG_INFO("if5c");
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                plcpHsigHeaderStart - plcpHeaderStart,
                                                headerMode, txVector);
              NS_LOG_DEBUG ("Case 5c - previous is in the preamble and current in HT-SIG or in SIG-A: mode=" << headerMode << ", psr=" << psr);
            }
          //Case 5d: current is in L-SIG.
          else if (current >= plcpHeaderStart)
            {
              NS_LOG_INFO("if5d");
              NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                current - plcpHeaderStart,
                                                headerMode, txVector);
              NS_LOG_DEBUG ("Case 5d - previous is in the preamble and current is in L-SIG: mode=" << headerMode << ", psr=" << psr);
            }
        }

      noiseInterferenceW = j->second.GetPower () - powerW;
      previous = j->first;
    }

  double per = 1 - psr;
  return per;
}

double
InterferenceHelper::CalculateNonLegacyPhyHeaderPer (Ptr<const Event> event, NiChanges *ni) const
{
  NS_LOG_FUNCTION (this);
  const WifiTxVector txVector = event->GetTxVector ();
  double psr = 1.0; /* Packet Success Rate */
  auto j = ni->begin ();
  Time previous = j->first;
  WifiPreamble preamble = txVector.GetPreambleType ();
  WifiMode mcsHeaderMode;
  if (preamble == WIFI_PREAMBLE_HT_MF || preamble == WIFI_PREAMBLE_HT_GF)
    {
      //mode for PLCP header fields sent with HT modulation
      mcsHeaderMode = WifiPhy::GetHtPlcpHeaderMode ();
    }
  else if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_VHT_MU)
    {
      //mode for PLCP header fields sent with VHT modulation
      mcsHeaderMode = WifiPhy::GetVhtPlcpHeaderMode ();
    }
  else if (preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_HE_MU)
    {
      //mode for PLCP header fields sent with HE modulation
      mcsHeaderMode = WifiPhy::GetHePlcpHeaderMode ();
    }
  WifiMode headerMode = WifiPhy::GetPlcpHeaderMode (txVector);
  Time plcpHeaderStart = j->first + WifiPhy::GetPlcpPreambleDuration (txVector); //packet start time + preamble
  Time plcpHsigHeaderStart = plcpHeaderStart + WifiPhy::GetPlcpHeaderDuration (txVector); //packet start time + preamble + L-SIG
  Time plcpTrainingSymbolsStart = plcpHsigHeaderStart + WifiPhy::GetPlcpHtSigHeaderDuration (preamble) + WifiPhy::GetPlcpSigA1Duration (preamble) + WifiPhy::GetPlcpSigA2Duration (preamble); //packet start time + preamble + L-SIG + HT-SIG or SIG-A
  Time plcpPayloadStart = plcpTrainingSymbolsStart + WifiPhy::GetPlcpTrainingSymbolDuration (txVector) + WifiPhy::GetPlcpSigBDuration (preamble); //packet start time + preamble + L-SIG + HT-SIG or SIG-A + Training + SIG-B
  double noiseInterferenceW = m_firstPower;
  double powerW = event->GetRxPowerW ();
  while (++j != ni->end ())
    {
      Time current = j->first;
      NS_LOG_DEBUG ("previous= " << previous << ", current=" << current);
      NS_ASSERT (current >= previous);
      //Case 1: previous and current after playload start: nothing to do
      if (previous >= plcpPayloadStart)
        {
          psr *= 1;
          NS_LOG_DEBUG ("Case 1 - previous and current after playload start: nothing to do");
        }
      //Case 2: previous is in training or in SIG-B: legacy will not enter here since it didn't enter in the last two and they are all the same for legacy
      else if (previous >= plcpTrainingSymbolsStart)
        {
          NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
          //Case 2a: current after payload start
          if (current >= plcpPayloadStart)
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                plcpPayloadStart - previous,
                                                mcsHeaderMode, txVector);
              NS_LOG_DEBUG ("Case 2a - previous is in training or in SIG-B and current after payload start: mode=" << mcsHeaderMode << ", psr=" << psr);
            }
          //Case 2b: current is in training or in SIG-B
          else
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                current - previous,
                                                mcsHeaderMode, txVector);
              NS_LOG_DEBUG ("Case 2b - previous is in training or in SIG-B and current is in training or in SIG-B: mode=" << mcsHeaderMode << ", psr=" << psr);
            }
        }
      //Case 3: previous is in HT-SIG or SIG-A: legacy will not enter here since it didn't enter in the last two and they are all the same for legacy
      else if (previous >= plcpHsigHeaderStart)
        {
          NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
          //Case 3a: current after payload start
          if (current >= plcpPayloadStart)
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                plcpPayloadStart - plcpTrainingSymbolsStart,
                                                mcsHeaderMode, txVector);
              //Case 3ai: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  //SIG-A is sent using legacy OFDM modulation
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - previous,
                                                    headerMode, txVector);
                  NS_LOG_DEBUG ("Case 3ai - previous is in SIG-A and current after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
              //Case 3aii: HT formats
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - previous,
                                                    mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 3aii - previous is in HT-SIG and current after payload start: mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 3b: current is in training or in SIG-B
          else if (current >= plcpTrainingSymbolsStart)
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                current - plcpTrainingSymbolsStart,
                                                mcsHeaderMode, txVector);
              //Case 3bi: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  //SIG-A is sent using legacy OFDM modulation
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - previous,
                                                    headerMode, txVector);
                  NS_LOG_DEBUG ("Case 3bi - previous is in SIG-A and current is in training or in SIG-B: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 3bii: HT formats
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - previous,
                                                    mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 3bii - previous is in HT-SIG and current is in HT training: mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 3c: current with previous in HT-SIG or SIG-A
          else
            {
              //Case 3ci: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  //SIG-A is sent using legacy OFDM modulation
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - previous,
                                                    headerMode, txVector);
                  NS_LOG_DEBUG ("Case 3ci - previous with current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 3cii: HT mixed format or HT greenfield
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - previous,
                                                    mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 3cii - previous with current in HT-SIG: mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
        }
      //Case 4: previous in L-SIG: HT GF will not reach here because it will execute the previous if and exit
      else if (previous >= plcpHeaderStart)
        {
          NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
          //Case 4a: current after payload start
          if (current >= plcpPayloadStart)
            {
              //Case 4ai: legacy format
              if (preamble == WIFI_PREAMBLE_LONG || preamble == WIFI_PREAMBLE_SHORT)
                {
                  psr *= 1;
                  NS_LOG_DEBUG ("Case 4ai - previous in L-SIG and current after payload start: nothing to do");
                }
              //Case 4aii: VHT or HE format
              else if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpPayloadStart - plcpTrainingSymbolsStart,
                                                    mcsHeaderMode, txVector);
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - plcpHsigHeaderStart,
                                                    headerMode, txVector);
                  NS_LOG_DEBUG ("Case 4aii - previous is in L-SIG and current after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
              //Case 4aiii: HT mixed format
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpPayloadStart - plcpHsigHeaderStart,
                                                    mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 4aiii - previous in L-SIG and current after payload start: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 4b: current is in training or in SIG-B. legacy will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= plcpTrainingSymbolsStart)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              //Case 4bi: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpTrainingSymbolsStart,
                                                    mcsHeaderMode, txVector);
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - plcpHsigHeaderStart,
                                                    headerMode, txVector);
                  NS_LOG_DEBUG ("Case 4bi - previous is in L-SIG and current in training or in SIG-B: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
              //Case 4bii: HT mixed format
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpHsigHeaderStart,
                                                    mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 4bii - previous in L-SIG and current in HT training: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 4c: current in HT-SIG or in SIG-A. Legacy will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= plcpHsigHeaderStart)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              //Case 4ci: VHT format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpHsigHeaderStart,
                                                    headerMode, txVector);
                  NS_LOG_DEBUG ("Case 4ci - previous is in L-SIG and current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 4cii: HT mixed format
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpHsigHeaderStart,
                                                    mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 4cii - previous in L-SIG and current in HT-SIG: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 4d: current with previous in L-SIG
          else
            {
              psr *= 1;
              NS_LOG_DEBUG ("Case 4d - current with previous in L-SIG: nothing to do");
            }
        }
      //Case 5: previous is in the preamble works for all cases
      else
        {
          //Case 5a: current after payload start
          if (current >= plcpPayloadStart)
            {
              //Case 5ai: legacy format (No HT-SIG or Training Symbols)
              if (preamble == WIFI_PREAMBLE_LONG || preamble == WIFI_PREAMBLE_SHORT)
                {
                  psr *= 1;
                  NS_LOG_DEBUG ("Case 5ai - previous is in the preamble and current is after payload start: nothing to do");
                }
              //Case 5aii: VHT or HE format
              else if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpPayloadStart - plcpTrainingSymbolsStart,
                                                    mcsHeaderMode, txVector);
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - plcpHsigHeaderStart,
                                                    headerMode, txVector);
                  NS_LOG_DEBUG ("Case 5aii - previous is in the preamble and current is after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
              //Case 5aiii: HT formats
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpPayloadStart - plcpHsigHeaderStart,
                                                    mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 5aiii - previous is in the preamble and current is after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
            }
          //Case 5b: current is in training or in SIG-B. Legacy will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= plcpTrainingSymbolsStart)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              //Case 5bi: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpTrainingSymbolsStart,
                                                    mcsHeaderMode, txVector);
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - plcpHsigHeaderStart,
                                                    headerMode, txVector);
                  NS_LOG_DEBUG ("Case 5bi - previous is in the preamble and current in training or in SIG-B: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
              //Case 5bii: HT mixed format
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpHsigHeaderStart,
                                                    mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 5bii - previous is in the preamble and current in HT training: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 5c: current in HT-SIG or in SIG-A. Legacy will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= plcpHsigHeaderStart)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              //Case 5ci: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpHsigHeaderStart,
                                                    headerMode, txVector);
                  NS_LOG_DEBUG ("Case 5ci - previous is in preamble and current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 5cii: HT formats
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpHsigHeaderStart,
                                                    mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 5cii - previous in preamble and current in HT-SIG: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 5d: current is in L-SIG. HT-GF will not come here
          else if (current >= plcpHeaderStart)
            {
              NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
              psr *= 1;
              NS_LOG_DEBUG ("Case 5d - previous is in the preamble and current is in L-SIG: nothing to do");
            }
        }

      noiseInterferenceW = j->second.GetPower() - powerW;
      previous = j->first;
    }

  double per = 1 - psr;
  return per;
}

 struct InterferenceHelper::SnrPer
 InterferenceHelper::CalculatePayloadSnrPer (Ptr<Event> event, std::pair<Time, Time> relativeMpduStartStop) const
 {
   NS_LOG_FUNCTION(this);
   NiChanges ni;
   double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
   double snr = CalculateSnr (event->GetRxPowerW (),
                              noiseInterferenceW,
                              event->GetTxVector ().GetChannelWidth ());
 
   /* calculate the SNIR at the start of the MPDU (located through windowing) and accumulate
    * all SNIR changes in the SNIR vector.
    */
   double per = CalculatePayloadPer (event, &ni, relativeMpduStartStop);
 
   struct SnrPer snrPer;
   snrPer.snr = snr;
   snrPer.per = per;
   return snrPer;
 }

 struct InterferenceHelper::SnrPer
  InterferenceHelper::CalculatePlcpHeaderSnrPer (Ptr<Event> event)
  {
    NS_LOG_FUNCTION(this);
   NiChanges ni;
   
     double snr = CalculateSnr (event);
   
     double per = CalculatePlcpHeaderPer (event, &ni);
   
     struct SnrPer snrPer;
     snrPer.snr = snr;
     snrPer.per = per;
     return snrPer;
 }
 
 double
 InterferenceHelper::CalculateSnr (Ptr<Event> event) const
 {
   NS_LOG_FUNCTION(this);
   NiChanges ni;
   double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
   double snr = CalculateSnr (event->GetRxPowerW (),
                              noiseInterferenceW,
                              event->GetTxVector().GetChannelWidth ());
  return snr;
 }

 double
  InterferenceHelper::CalculatePlcpHeaderPer (Ptr<const Event> event, NiChanges *ni) const
   {

     NS_LOG_FUNCTION (this);
     const WifiTxVector txVector = event->GetTxVector ();
    double psr = 1.0; /* Packet Success Rate */
     NiChanges::const_iterator j = ni->begin ();
    Time previous = (*j).second.GetEvent()->GetStartTime ();
    WifiPreamble preamble = txVector.GetPreambleType ();
     WifiMode mcsHeaderMode;
     if (preamble == WIFI_PREAMBLE_HT_MF || preamble == WIFI_PREAMBLE_HT_GF)
       {
        //mode for PLCP header fields sent with HT modulation
         mcsHeaderMode = WifiPhy::GetHtPlcpHeaderMode ();
       }
     else if (preamble == WIFI_PREAMBLE_HE_SU)
       {
        //mode for PLCP header fields sent with HE modulation
        mcsHeaderMode = WifiPhy::GetHePlcpHeaderMode ();
       }
    WifiMode headerMode = WifiPhy::GetPlcpHeaderMode (txVector);
   Time plcpHeaderStart = (*j).second.GetEvent()->GetStartTime () + WifiPhy::GetPlcpPreambleDuration (txVector); //packet start time + preamble
     Time plcpHsigHeaderStart = plcpHeaderStart + WifiPhy::GetPlcpHeaderDuration (txVector); //packet start time + preamble + L-SIG
    Time plcpTrainingSymbolsStart = plcpHsigHeaderStart + WifiPhy::GetPlcpHtSigHeaderDuration (preamble) + WifiPhy::GetPlcpSigA1Duration (preamble) + WifiPhy::GetPlcpSigA2Duration (preamble); //packet start time + preamble + L-SIG + HT-SIG or SIG-A
     Time plcpPayloadStart = plcpTrainingSymbolsStart + WifiPhy::GetPlcpTrainingSymbolDuration (txVector) + WifiPhy::GetPlcpSigBDuration (preamble); //packet start time + preamble + L-SIG + HT-SIG or SIG-A + Training + SIG-B
     double noiseInterferenceW = (*j).second.GetPower ();
     double powerW = event->GetRxPowerW ();
     j++;
     while (ni->end () != j)
      {
        Time current = (*j).second.GetEvent()->GetStartTime ();
        NS_LOG_DEBUG ("previous= " << previous << ", current=" << current);
        NS_ASSERT (current >= previous);
       //Case 1: previous and current after playload start: nothing to do
         if (previous >= plcpPayloadStart)
           {
             psr *= 1;
             NS_LOG_DEBUG ("Case 1 - previous and current after playload start: nothing to do");
           }
         //Case 2: previous is in training or in SIG-B: legacy will not enter here since it didn't enter in the last two and they are all the same for legacy
         else if (previous >= plcpTrainingSymbolsStart)
           {
             NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
             //Case 2a: current after payload start
             if (current >= plcpPayloadStart)
               {
                 psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                 noiseInterferenceW,
                                                                 txVector.GetChannelWidth ()),
                                                   plcpPayloadStart - previous,
                                                   mcsHeaderMode, txVector);
   
                 NS_LOG_DEBUG ("Case 2a - previous is in training or in SIG-B and current after payload start: mode=" << mcsHeaderMode << ", psr=" << psr);
               }
             //Case 2b: current is in training or in SIG-B
             else
               {
                 psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                 noiseInterferenceW,
                                                                 txVector.GetChannelWidth ()),
                                                   current - previous,
                                                   mcsHeaderMode, txVector);
   
                 NS_LOG_DEBUG ("Case 2b - previous is in training or in SIG-B and current is in training or in SIG-B: mode=" << mcsHeaderMode << ", psr=" << psr);
               }
           }
         //Case 3: previous is in HT-SIG or SIG-A: legacy will not enter here since it didn't enter in the last two and they are all the same for legacy
         else if (previous >= plcpHsigHeaderStart)
           {
             NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
             //Case 3a: current after payload start
             if (current >= plcpPayloadStart)
               {
                 psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                 noiseInterferenceW,
                                                                 txVector.GetChannelWidth ()),
                                                   plcpPayloadStart - plcpTrainingSymbolsStart,
                                                   mcsHeaderMode, txVector);
   
                 //Case 3ai: VHT or HE format
                 if ( preamble == WIFI_PREAMBLE_HE_SU)
                   {
                     //SIG-A is sent using legacy OFDM modulation
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpTrainingSymbolsStart - previous,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 3ai - previous is in SIG-A and current after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                   }
                 //Case 3aii: HT formats
                 else
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpTrainingSymbolsStart - previous,
                                                       mcsHeaderMode, txVector);
   
                     NS_LOG_DEBUG ("Case 3aii - previous is in HT-SIG and current after payload start: mode=" << mcsHeaderMode << ", psr=" << psr);
                   }
               }
             //Case 3b: current is in training or in SIG-B
             else if (current >= plcpTrainingSymbolsStart)
               {
                 psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                 noiseInterferenceW,
                                                                 txVector.GetChannelWidth ()),
                                                   current - plcpTrainingSymbolsStart,
                                                   mcsHeaderMode, txVector);
   
                 //Case 3bi: VHT or HE format
                 if ( preamble == WIFI_PREAMBLE_HE_SU)
                   {
                     //SIG-A is sent using legacy OFDM modulation
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpTrainingSymbolsStart - previous,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 3bi - previous is in SIG-A and current is in training or in SIG-B: mode=" << headerMode << ", psr=" << psr);
                   }
                 //Case 3bii: HT formats
                 else
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpTrainingSymbolsStart - previous,
                                                       mcsHeaderMode, txVector);
  
                     NS_LOG_DEBUG ("Case 3bii - previous is in HT-SIG and current is in HT training: mode=" << mcsHeaderMode << ", psr=" << psr);
                   }
               }
             //Case 3c: current with previous in HT-SIG or SIG-A
             else
               {
                 //Case 3bi: VHT or HE format
                 if ( preamble == WIFI_PREAMBLE_HE_SU)
                   {
                     //SIG-A is sent using legacy OFDM modulation
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       current - previous,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 3ci - previous with current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                   }
                 //Case 3bii: HT mixed format or HT greenfield
                 else
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       current - previous,
                                                       mcsHeaderMode, txVector);
   
                     NS_LOG_DEBUG ("Case 3cii - previous with current in HT-SIG: mode=" << mcsHeaderMode << ", psr=" << psr);
                   }
               }
           }
         //Case 4: previous in L-SIG: HT GF will not reach here because it will execute the previous if and exit
         else if (previous >= plcpHeaderStart)
           {
             NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
             //Case 4a: current after payload start
             if (current >= plcpPayloadStart)
               {
                 //Case 4ai: legacy format
                 if (preamble == WIFI_PREAMBLE_LONG || preamble == WIFI_PREAMBLE_SHORT)
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpPayloadStart - previous,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 4ai - previous in L-SIG and current after payload start: mode=" << headerMode << ", psr=" << psr);
                   }
                 //Case 4aii: VHT or HE format
                 else if ( preamble == WIFI_PREAMBLE_HE_SU)
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpPayloadStart - plcpTrainingSymbolsStart,
                                                       mcsHeaderMode, txVector);
   
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpTrainingSymbolsStart - previous,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 4aii - previous is in L-SIG and current after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                   }
                 //Case 4aiii: HT mixed format
                 else
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpPayloadStart - plcpHsigHeaderStart,
                                                       mcsHeaderMode, txVector);
   
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpHsigHeaderStart - previous,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 4aiii - previous in L-SIG and current after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                   }
               }
             //Case 4b: current is in training or in SIG-B. legacy will not come here since it went in previous if or if the previous if is not true this will be not true
             else if (current >= plcpTrainingSymbolsStart)
               {
                 NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
   
                 //Case 4bi: VHT or HE format
                 if ( preamble == WIFI_PREAMBLE_HE_SU)
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       current - plcpTrainingSymbolsStart,
                                                       mcsHeaderMode, txVector);
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpTrainingSymbolsStart - previous,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 4bi - previous is in L-SIG and current in training or in SIG-B: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                   }
                 //Case 4bii: HT mixed format
                 else
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                       current - plcpHsigHeaderStart,
                                                       mcsHeaderMode, txVector);
   
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpHsigHeaderStart - previous,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 4bii - previous in L-SIG and current in HT training: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                   }
               }
             //Case 4c: current in HT-SIG or in SIG-A. Legacy will not come here since it went in previous if or if the previous if is not true this will be not true
             else if (current >= plcpHsigHeaderStart)
               {
                 NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
   
                 //Case 4ci: VHT format
                 if ( preamble == WIFI_PREAMBLE_HE_SU)
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       current - previous,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 4ci - previous is in L-SIG and current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                   }
                 //Case 4cii: HT mixed format
                 else
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       current - plcpHsigHeaderStart,
                                                       mcsHeaderMode, txVector);
   
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpHsigHeaderStart - previous,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 4cii - previous in L-SIG and current in HT-SIG: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                   }
               }
             //Case 4d: current with previous in L-SIG
             else
               {
                 psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                 noiseInterferenceW,
                                                                 txVector.GetChannelWidth ()),
                                                   current - previous,
                                                   headerMode, txVector);
   
                 NS_LOG_DEBUG ("Case 3c - current with previous in L-SIG: mode=" << headerMode << ", psr=" << psr);
               }
           }
         //Case 5: previous is in the preamble works for all cases
        else
           {
             //Case 5a: current after payload start
             if (current >= plcpPayloadStart)
               {
                 //Case 5ai: legacy format (No HT-SIG or Training Symbols)
                 if (preamble == WIFI_PREAMBLE_LONG || preamble == WIFI_PREAMBLE_SHORT)
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                      plcpPayloadStart - plcpHeaderStart,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 5a - previous is in the preamble and current is after payload start: mode=" << headerMode << ", psr=" << psr);
                   }
                 //Case 5aii: VHT or HE format
                 else if ( preamble == WIFI_PREAMBLE_HE_SU)
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpPayloadStart - plcpTrainingSymbolsStart,
                                                       mcsHeaderMode, txVector);
   
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpTrainingSymbolsStart - plcpHeaderStart,
                                                      headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 5aii - previous is in the preamble and current is after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                   }
   
                 //Case 5aiii: HT formats
                else
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpPayloadStart - plcpHsigHeaderStart,
                                                       mcsHeaderMode, txVector);
   
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpHsigHeaderStart - plcpHeaderStart, //HT GF: plcpHsigHeaderStart - plcpHeaderStart = 0
                                                      headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 5aiii - previous is in the preamble and current is after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                   }
              }
             //Case 5b: current is in training or in SIG-B. Legacy will not come here since it went in previous if or if the previous if is not true this will be not true
             else if (current >= plcpTrainingSymbolsStart)
               {
                 NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
   
                 //Case 5bi: VHT or HE format
                 if ( preamble == WIFI_PREAMBLE_HE_SU)
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       current - plcpTrainingSymbolsStart,
                                                       mcsHeaderMode, txVector);
   
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpTrainingSymbolsStart - plcpHeaderStart,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 5bi - previous is in the preamble and current in training or in SIG-B: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                   }
                 //Case 5bii: HT mixed format
                 else
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       current - plcpHsigHeaderStart,
                                                       mcsHeaderMode, txVector);
   
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpHsigHeaderStart - plcpHeaderStart,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 5bii - previous is in the preamble and current in HT training: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                   }
               }
             //Case 5c: current in HT-SIG or in SIG-A. Legacy will not come here since it went in previous if or if the previous if is not true this will be not true
             else if (current >= plcpHsigHeaderStart)
               {
                 NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
   
                 //Case 5ci: VHT or HE format
                 if ( preamble == WIFI_PREAMBLE_HE_SU)
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       current - plcpHeaderStart,
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 5ci - previous is in preamble and current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                   }
                 //Case 5cii: HT formats
                 else
                   {
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       current - plcpHsigHeaderStart,
                                                       mcsHeaderMode, txVector);
   
                     psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                     noiseInterferenceW,
                                                                     txVector.GetChannelWidth ()),
                                                       plcpHsigHeaderStart - plcpHeaderStart, //HT-GF: plcpHsigHeaderStart - plcpHeaderStart = 0
                                                       headerMode, txVector);
   
                     NS_LOG_DEBUG ("Case 5cii - previous in preamble and current in HT-SIG: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                   }
               }
             //Case 5d: current is in L-SIG. HT-GF will not come here
             else if (current >= plcpHeaderStart)
               {
                 NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
   
                 psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                 noiseInterferenceW,
                                                                 txVector.GetChannelWidth ()),
                                                   current - plcpHeaderStart,
                                                  headerMode, txVector);
   
                 NS_LOG_DEBUG ("Case 5d - previous is in the preamble and current is in L-SIG: mode=" << headerMode << ", psr=" << psr);
               }
           }
   
         noiseInterferenceW += (*j).second.GetPower ();
         previous = (*j).second.GetEvent()->GetStartTime ();
         j++;
       }
   
     double per = 1 - psr;
     return per;
   }
 
 struct InterferenceHelper::SnrPer
 InterferenceHelper::CalculatePlcpPayloadSnrPer (Ptr<Event> event)
   {
     NS_LOG_FUNCTION(this);
     NiChanges ni;
     double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
     double snr = CalculateSnr (event->GetRxPowerW (),
                               noiseInterferenceW,
                                event->GetTxVector ().GetChannelWidth ());
   
 
     double per = CalculatePlcpPayloadPer (event, &ni);
   
     struct SnrPer snrPer;
     snrPer.snr = snr;
     snrPer.per = per;
     return snrPer;
     }

double
   InterferenceHelper::CalculatePlcpPayloadPer (Ptr<const Event> event, NiChanges *ni) const
   {
     NS_LOG_FUNCTION (this);
     const WifiTxVector txVector = event->GetTxVector ();
     double psr = 1.0; /* Packet Success Rate */
     NiChanges::const_iterator j = ni->begin ();
     Time previous = (*j).second.GetEvent()->GetStartTime ();
     WifiMode payloadMode = event->GetPayloadMode ();
     WifiPreamble preamble = txVector.GetPreambleType ();
     Time plcpHeaderStart = (*j).second.GetEvent()->GetStartTime () + WifiPhy::GetPlcpPreambleDuration (txVector); //packet start time + preamble
     Time plcpHsigHeaderStart = plcpHeaderStart + WifiPhy::GetPlcpHeaderDuration (txVector); //packet start time + preamble + L-SIG
   Time plcpTrainingSymbolsStart = plcpHsigHeaderStart + WifiPhy::GetPlcpHtSigHeaderDuration (preamble) + WifiPhy::GetPlcpSigA1Duration (preamble) + WifiPhy::GetPlcpSigA2Duration (preamble); //packet start time + preamble + L-SIG + HT-SIG or SIG-A
   Time plcpPayloadStart = plcpTrainingSymbolsStart + WifiPhy::GetPlcpTrainingSymbolDuration (txVector) + WifiPhy::GetPlcpSigBDuration (preamble); //packet start time + preamble + L-SIG + HT-SIG or SIG-A + Training + SIG-B
     double noiseInterferenceW = (*j).second.GetPower ();
     double powerW = event->GetRxPowerW ();
     j++;
     while (ni->end () != j)
       {
         Time current = (*j).second.GetEvent()->GetStartTime ();
         NS_LOG_DEBUG ("previous= " << previous << ", current=" << current);
         NS_ASSERT (current >= previous);
         //Case 1: Both previous and current point to the payload
        if (previous >= plcpPayloadStart)
           {
             psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                             noiseInterferenceW,
                                                             txVector.GetChannelWidth ()),
                                               current - previous,
                                               payloadMode, txVector);
   
             NS_LOG_DEBUG ("Both previous and current point to the payload: mode=" << payloadMode << ", psr=" << psr);
           }
         //Case 2: previous is before payload and current is in the payload
         else if (current >= plcpPayloadStart)
           {
             psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                             noiseInterferenceW,
                                                            txVector.GetChannelWidth ()),
                                               current - plcpPayloadStart,
                                               payloadMode, txVector);
             NS_LOG_DEBUG ("previous is before payload and current is in the payload: mode=" << payloadMode << ", psr=" << psr);
           }
   
         noiseInterferenceW += (*j).second.GetPower ();
         previous = (*j).second.GetEvent()->GetStartTime ();
         j++;
       }
   
     double per = 1 - psr;
     return per;
   }   

 struct InterferenceHelper::SnrPer
 InterferenceHelper::CalculateNonHtPhyHeaderSnrPer (Ptr<Event> event) const
 {
   NiChanges ni;
   double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
   double snr = CalculateSnr (event->GetRxPowerW (),
                              noiseInterferenceW,
                              event->GetTxVector ().GetChannelWidth ());
 
   /* calculate the SNIR at the start of the PHY header and accumulate
    * all SNIR changes in the SNIR vector.
    */
   double per = CalculateNonHtPhyHeaderPer (event, &ni);
 
   struct SnrPer snrPer;
   snrPer.snr = snr;
   snrPer.per = per;
   return snrPer;
 }
 struct InterferenceHelper::SnrPer
InterferenceHelper::CalculateSnrPer (Ptr<Event> event)
{
  NS_LOG_FUNCTION(this);
  NiChanges ni;
  double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
  NS_LOG_FUNCTION(noiseInterferenceW);
  double snr = CalculateSnr (event->GetRxPowerW (),
                             noiseInterferenceW,
                             event->GetTxVector().GetChannelWidth());


  NS_LOG_INFO("snr"<<snr);
  /*
   * calculate the SNIR at the start of the packet and accumulate
   * all SNIR changes in the snir vector.
   */
  double per = CalculatePer (event, &ni);
  NS_LOG_INFO("per"<<per);

  struct SnrPer snrPer;
  snrPer.snr = snr;
  snrPer.per = per;
  snrPer.packetRss = CalculatePacketRss (event, &ni);

  return snrPer;
}

double
InterferenceHelper::CalculatePacketRss (Ptr<const Event> event,
                                        NiChanges *ni) const
{
  NS_LOG_FUNCTION(this);
  double rss = 0;
  double packetRss = 0;



  NiChanges::iterator j = ni->begin ();
  Time previous = (*j).first;
  NS_LOG_FUNCTION(previous);
  const WifiTxVector txVector = event->GetTxVector ();
 // WifiMode payloadMode = txVector.GetMode ();
  //WifiPreamble preamble = txVector.GetPreambleType ();
  
  //WifiMode headerMode = WifiPhy::GetPlcpHeaderMode (txVector);
  Time plcpHeaderStart = (*j).first + MicroSeconds (WifiPhy::GetPlcpPreambleDuration (txVector));
  Time plcpPayloadStart = plcpHeaderStart + MicroSeconds (WifiPhy::GetPlcpHeaderDuration (txVector));
  double powerW = event->GetRxPowerW ();  // power of packet at receiver
  double noiseInterferenceW = (*j).second.GetPower ();
  NS_LOG_INFO(noiseInterferenceW);

  j++;
  NS_LOG_FUNCTION(this << "test");
  while (ni->end () != j)
    {
      Time current = (*j).first;
      NS_ASSERT (current >= previous);
      NS_LOG_FUNCTION(this << "test");
      NS_LOG_FUNCTION(this << previous);
      NS_LOG_FUNCTION(this << plcpPayloadStart);
      NS_LOG_FUNCTION(this << plcpHeaderStart);
       NS_LOG_FUNCTION(this << current);
      // payload only
      if (previous >= plcpPayloadStart)
        {
          NS_LOG_FUNCTION(this << "if1");
          rss = powerW + noiseInterferenceW + CalculateNoiseFloor (txVector);
          packetRss += rss * (current - previous).GetSeconds ();
        }
      else if (previous >= plcpHeaderStart)
        {
          NS_LOG_FUNCTION(this << "if2");
          // half header half payload
          if (current >= plcpPayloadStart)
            {
              NS_LOG_FUNCTION(this << "if3");
              // header chunk
              rss = powerW + noiseInterferenceW + CalculateNoiseFloor (txVector);
              packetRss += rss * (plcpPayloadStart - previous).GetSeconds ();
              // payload chunk
              rss = powerW + noiseInterferenceW + CalculateNoiseFloor (txVector);
              packetRss += rss * (current - plcpPayloadStart).GetSeconds ();
            }
          // header only
          else
            {
              NS_LOG_FUNCTION(this << "if4");
              NS_ASSERT (current >= plcpHeaderStart);
              rss = powerW + noiseInterferenceW + CalculateNoiseFloor (txVector);
              packetRss += rss * (current - previous).GetSeconds ();
            }
        }
      else
        {
          NS_LOG_FUNCTION(this << "if5a");
          // half header half payload
          if (current >= plcpPayloadStart)
            {
              NS_LOG_FUNCTION(this << "if5");
              // header chunk
              rss = powerW + noiseInterferenceW + CalculateNoiseFloor (txVector);
              packetRss += rss * (plcpPayloadStart - plcpHeaderStart).GetSeconds ();
              // payload chunk
              rss = powerW + noiseInterferenceW + CalculateNoiseFloor (txVector);
              packetRss += rss * (current - plcpPayloadStart).GetSeconds ();
            }
          // header only
          else if (current >= plcpHeaderStart)
            {
              NS_LOG_FUNCTION(this << "if5b");
              rss = powerW + noiseInterferenceW + CalculateNoiseFloor (txVector);
              packetRss += rss * (current - plcpHeaderStart).GetSeconds ();
            }
        }

      NS_LOG_FUNCTION(this << "end");
      noiseInterferenceW = (*j).second.GetPower ();
      previous = current;
      j++;
    }

  NS_ASSERT (event->GetDuration ().GetSeconds () != 0);
  NS_LOG_FUNCTION(this << "test");
  // real duration = time stamp of (last ni change - start of header)
  NS_LOG_INFO(ni->end()->second.GetEvent());
  Time duration = (ni->end()->first) - plcpHeaderStart;

  NS_LOG_FUNCTION(this << "test");
  packetRss /= duration.GetSeconds ();
  return packetRss;
}

double
InterferenceHelper::CalculatePer (Ptr<const Event> event, NiChanges *ni) const
{
  NS_LOG_FUNCTION(this);
  double psr = 1.0; /* Packet Success Rate */
  auto j = ni->begin ();
  Time previous = j->first;
  const WifiTxVector txVector = event->GetTxVector ();
  WifiMode payloadMode = txVector.GetMode ();
  //WifiPreamble preamble = txVector.GetPreambleType ();
  //WifiMode headerMode = WifiPhy::GetPlcpHeaderMode (txVector);
  Time plcpHeaderStart = j->first + WifiPhy::GetPlcpPreambleDuration (txVector);
  Time plcpPayloadStart = plcpHeaderStart + WifiPhy::GetPlcpHeaderDuration (txVector);
  double noiseInterferenceW = m_firstPower;
  double powerW = event->GetRxPowerW ();
  NS_LOG_INFO("plcpPayloadStart" << plcpPayloadStart);
  NS_LOG_INFO("previous" << previous);
  NS_LOG_INFO("plcpHeaderStart" <<plcpHeaderStart );

  
  while (++j != ni->end () )
    {
      Time current = j->first;
      NS_LOG_INFO("current" <<current );
      NS_ASSERT (current >= previous);

      if (previous >= plcpPayloadStart)
        {
          NS_LOG_INFO("i1");
          psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                          noiseInterferenceW,
                                                          txVector.GetChannelWidth()),
                                            current - previous,payloadMode,
                                            txVector);
        }
      else if (previous >= plcpHeaderStart)
        {
          NS_LOG_INFO("i2");
          if (current >= plcpPayloadStart)
            {
              NS_LOG_INFO("i2");
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth()),
                                                plcpPayloadStart - previous, payloadMode,
                                                txVector);
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth()),
                                                current - plcpPayloadStart,payloadMode,
                                                txVector);
            }
          else
            {
              NS_LOG_INFO("i3");
              NS_ASSERT (current >= plcpHeaderStart);
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth()),
                                                current - previous,payloadMode,
                                                txVector);
            }
        }
      else
        {
          NS_LOG_INFO("i4a");
          if (current >= plcpPayloadStart)
            {
              NS_LOG_INFO("i4");
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth()),
                                                plcpPayloadStart - plcpHeaderStart,payloadMode,
                                                txVector);
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth()),
                                                current - plcpPayloadStart,payloadMode,
                                                txVector);
            }
          else if (current >= plcpHeaderStart)
            {
              NS_LOG_INFO("i5");
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth()),
                                                current - plcpHeaderStart,payloadMode,
                                                txVector);
            }
        }

      noiseInterferenceW += j->second.GetPower () -powerW ;
      previous = j->first;
    }

NS_LOG_INFO("psr"<<psr);
  double per = 1 - psr;
  return per;
}

 
 struct InterferenceHelper::SnrPer
 InterferenceHelper::CalculateHtPhyHeaderSnrPer (Ptr<Event> event) const
 {
   NS_LOG_FUNCTION(this);
   NiChanges ni;
   double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
   double snr = CalculateSnr (event->GetRxPowerW (),
                              noiseInterferenceW,
                              event->GetTxVector ().GetChannelWidth ());
   
   /* calculate the SNIR at the start of the PHY header and accumulate
    * all SNIR changes in the SNIR vector.
    */
   double per = CalculateHtPhyHeaderPer (event, &ni);
   
   struct SnrPer snrPer;
   snrPer.snr = snr;
   snrPer.per = per;
   return snrPer;
 }
 

 void
InterferenceHelper::EraseEvents (void)
{
  m_niChanges.clear ();
  AddNiChangeEvent (Time (0), NiChange (0, 0.0));
  
  NS_LOG_FUNCTION(this<< m_niChanges.size());
  m_rxing = false;
  m_firstPower = 0.0;
}

double
InterferenceHelper::CurrentNodeRss (WifiTxVector mode)
{
  NS_LOG_FUNCTION(this);
  double rss = CalculateNoiseFloor (mode);
  Time now = Simulator::Now ();
  NiChanges::const_iterator nowIterator = GetPosition (now);
  for (NiChanges::iterator i = m_niChanges.begin (); i != nowIterator; i++)
    {
      rss += i->second.GetPower ();
    }
  return rss;

}
 
 InterferenceHelper::NiChanges::const_iterator
 InterferenceHelper::GetNextPosition (Time moment) const
 { NS_LOG_FUNCTION(this<<moment);
   return m_niChanges.upper_bound (moment);
 }
 
 InterferenceHelper::NiChanges::const_iterator
 InterferenceHelper::GetPreviousPosition (Time moment) const
 {
   NS_LOG_FUNCTION(this << moment);
   auto it = GetNextPosition (moment);
   // This is safe since there is always an NiChange at time 0,
   // before moment.
   --it;
   return it;
 }
 
InterferenceHelper::NiChanges::iterator
InterferenceHelper::AddNiChangeEvent (Time moment,NiChange change)
{
  
 NS_LOG_FUNCTION(this<< m_niChanges.size());
 return m_niChanges.insert (GetNextPosition(moment),std::make_pair(moment,change));
}
 
 void
 InterferenceHelper::NotifyRxStart ()
 {
   NS_LOG_FUNCTION (this);
   m_rxing = true;
 }
 
 void
 InterferenceHelper::NotifyRxEnd ()
 {
   m_rxing = false;
   auto it = GetPreviousPosition (Simulator::Now ());
   it--;
   m_firstPower = it->second.GetPower ();
 }
 
 } //namespace ns3