/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
 /*
  * Copyright (c) 2019 Orange Labs
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
  * Author: Rediet <getachew.redieteab@orange.com>
  */
 
 #ifndef WIFI_PPDU_H
 #define WIFI_PPDU_H
 
 #include <list>
 #include "ns3/nstime.h"
 #include "wifi-tx-vector.h"
 #include "wifi-phy-header.h"
 
 namespace ns3 {
 
 class WifiPsdu;
 
 class WifiPpdu : public SimpleRefCount<WifiPpdu>
 {
 public:
   WifiPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration, uint16_t frequency);
 
   virtual ~WifiPpdu ();
 
   WifiTxVector GetTxVector (void) const;
   Ptr<const WifiPsdu> GetPsdu (void) const;
   bool IsTruncatedTx (void) const;
   void SetTruncatedTx (void);
   Time GetTxDuration () const;
   
 
   void Print (std::ostream &os) const;
 
 private:
   DsssSigHeader m_dsssSig;          
   LSigHeader m_lSig;                
   HtSigHeader m_htSig;              
   VhtSigHeader m_vhtSig;            
   HeSigHeader m_heSig;              
   WifiPreamble m_preamble;          
   WifiModulationClass m_modulation; 
   Ptr<const WifiPsdu> m_psdu;       
   bool m_truncatedTx;               
   uint16_t m_frequency;             
   uint16_t m_channelWidth;          
   uint8_t m_txPowerLevel;  

 };
 
 std::ostream& operator<< (std::ostream& os, const WifiPpdu &ppdu);
 
 } //namespace ns3
 
 #endif /* WIFI_PPDU_H */