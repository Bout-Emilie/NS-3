#ifndef INTERFERENCE_HELPER_H
 #define INTERFERENCE_HELPER_H
 
 #include "ns3/nstime.h"
 #include "wifi-tx-vector.h"
 #include "wifi-ppdu.h"
 #include <map>
 #include <stdint.h>
#include <vector>
#include <list>

 
 namespace ns3 {
 
 class WifiPpdu;
 class WifiPsdu;
 class ErrorRateModel;
 class Packet;
 
 class Event : public SimpleRefCount<Event>
 {
     public:
        Event (Ptr<const Packet> packet, WifiTxVector txVector, Time duration, double rxPower);
        ~Event ();

        //Ptr<const WifiPsdu> GetPsdu (void) const;
        //Ptr<const WifiPpdu> GetPpdu (void) const;
        Ptr<const Packet> GetPacket(void) const;
        Time GetStartTime (void) const;
        Time GetEndTime (void) const;
        Time GetDuration (void) const;
        double GetRxPowerW (void) const;
        WifiTxVector GetTxVector (void) const;
        enum WifiPreamble GetPreambleType (void) const;
        WifiMode GetPayloadMode (void) const;

    private:
        //Ptr<const WifiPpdu> m_ppdu;
        Ptr<const Packet> m_packet; 
        WifiTxVector m_txVector; 
        enum WifiPreamble m_preamble;
        Time m_startTime; 
        Time m_endTime; 
        double m_rxPowerW; 
};
 
 //std::ostream& operator<< (std::ostream& os, const Event &event);

 class InterferenceHelper
 {
 public:
   struct SnrPer
   {
     double snr; 
     double per; 
     double packetRss;
   };
 
   InterferenceHelper ();
   ~InterferenceHelper ();
 
   void SetNoiseFigure (double value);
   void SetErrorRateModel (const Ptr<ErrorRateModel> rate);
   double GetNoiseFigure (void) const;
 
   Ptr<ErrorRateModel> GetErrorRateModel (void) const;
   void SetNumberOfReceiveAntennas (uint8_t rx);
 
   Time GetEnergyDuration (double energyW);

   struct InterferenceHelper::SnrPer CalculateSnrPer (Ptr<Event> event);
 
   Ptr<Event> Add (Ptr<const Packet> ppdu, WifiTxVector txVector, Time duration, double rxPower);
 
   void AddForeignSignal (Time duration, double rxPower);
   struct InterferenceHelper::SnrPer CalculatePayloadSnrPer (Ptr<Event> event, std::pair<Time, Time> relativeMpduStartStop) const;
   double CalculateSnr (Ptr<Event> event) const;
   struct InterferenceHelper::SnrPer CalculateNonHtPhyHeaderSnrPer (Ptr<Event> event) const;
   struct InterferenceHelper::SnrPer CalculateHtPhyHeaderSnrPer (Ptr<Event> event) const;
   struct InterferenceHelper::SnrPer CalculateNonLegacyPhyHeaderSnrPer (Ptr<Event> event) const;
   struct InterferenceHelper::SnrPer CalculateLegacyPhyHeaderSnrPer (Ptr<Event> event) const;
   struct InterferenceHelper::SnrPer CalculatePlcpHeaderSnrPer (Ptr<Event> event);
   struct InterferenceHelper::SnrPer CalculatePlcpPayloadSnrPer (Ptr<Event> event);

 
   void NotifyRxStart ();
   void NotifyRxEnd ();
   void EraseEvents (void);

    /**
   * \brief Get current node Received Signal Strength.
   * \param mode WifiMode.
   * \returns current RSS value (W).
   *
   * This function calculates the current RSS value at the node. Current RSS at
   * node is given by accumulating all signal present at node and noise floor.
   */
  double CurrentNodeRss (WifiTxVector mode);
 
 
 private:
   class NiChange
   {
 public:
     NiChange (Ptr<Event>, double power);
     //Time GetTime (void) const;
     //double GetDelta (void) const;
     bool operator < (const InterferenceHelper::NiChange& o) const;
     //void AddDelta (double delta);
     double GetPower (void) const;
     void AddPower (double power);
     Ptr<Event> GetEvent (void) const;
 
 
 private:
     //double m_power; 
     Ptr<Event> m_event; 
       //Time m_time;
       //double m_delta;
       double m_power;
   };
 
   typedef std::multimap<Time, NiChange> NiChanges;
   typedef std::list<Ptr<Event> > Events;

   
 
   void AppendEvent (Ptr<Event> event);
   double CalculateNoiseInterferenceW (Ptr<Event> event, NiChanges *ni) const;
   double CalculateSnr (double signal, double noiseInterference, uint16_t channelWidth) const;
   double CalculateChunkSuccessRate (double snir, Time duration, WifiMode mode, WifiTxVector txVector) const;
   double CalculateNoiseFloor (WifiTxVector mode) const;
   double CalculatePayloadChunkSuccessRate (double snir, Time duration, WifiTxVector txVector) const;
   double CalculatePayloadPer (Ptr<const Event> event, NiChanges *ni, std::pair<Time, Time> window) const;
   double CalculateNonHtPhyHeaderPer (Ptr<const Event> event, NiChanges *ni) const;
   double CalculateHtPhyHeaderPer (Ptr<const Event> event, NiChanges *ni) const;
   double CalculateLegacyPhyHeaderPer (Ptr<const Event> event, NiChanges *ni) const;
   double CalculateNonLegacyPhyHeaderPer (Ptr<const Event> event, NiChanges *ni) const;
   double CalculatePer (Ptr<const Event> event, NiChanges *ni) const;
   double CalculatePacketRss (Ptr<const Event> event, NiChanges *ni) const;
  double CalculatePlcpHeaderPer (Ptr<const Event> event, NiChanges *ni) const;
  double CalculatePlcpPayloadPer (Ptr<const Event> event, NiChanges *ni) const;
 
   double m_noiseFigure; 
   Ptr<ErrorRateModel> m_errorRateModel; 
   uint8_t m_numRxAntennas; 
   NiChanges m_niChanges;
   double m_firstPower; 
   bool m_rxing; 
 
   NiChanges::const_iterator GetNextPosition (Time moment) const;
   NiChanges::const_iterator GetPosition (Time moment) const;
   //NiChanges::iterator GetNextPosition (Time moment);
   NiChanges::const_iterator GetPreviousPosition (Time moment) const;
 
   NiChanges::iterator AddNiChangeEvent (Time moment,NiChange change);
 };
 
 } //namespace ns3
 
 #endif /* INTERFERENCE_HELPER_H */