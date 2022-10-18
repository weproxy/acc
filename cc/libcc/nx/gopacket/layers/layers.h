//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "nx/gopacket/gopacket.h"

namespace nx {
namespace layers {
using namespace gx;

// EnumMetadata keeps track of a set of metadata for each enumeration value
// for protocol enumerations.
struct EnumMetadata {
    // DecodeWith is the decoder to use to decode this protocol's data.
    // DecodeWith gopacket.Decoder
    // Name is the name of the enumeration value.
    string Name;
    // LayerType is the layer type implied by the given enum.
    gopacket::LayerType LayerType;
};

// EthernetType is an enumeration of ethernet type values, and acts as a decoder
// for any type it supports.
using EthernetType = uint16;

// EthernetTypeLLC is not an actual ethernet type.  It is instead a
// placeholder we use in Ethernet frames that use the 802.3 standard of
// srcmac|dstmac|length|LLC instead of srcmac|dstmac|ethertype.
const EthernetType EthernetTypeLLC = 0;
const EthernetType EthernetTypeIPv4 = 0x0800;
const EthernetType EthernetTypeARP = 0x0806;
const EthernetType EthernetTypeIPv6 = 0x86DD;
const EthernetType EthernetTypeCiscoDiscovery = 0x2000;
const EthernetType EthernetTypeNortelDiscovery = 0x01a2;
const EthernetType EthernetTypeTransparentEthernetBridging = 0x6558;
const EthernetType EthernetTypeDot1Q = 0x8100;
const EthernetType EthernetTypePPP = 0x880b;
const EthernetType EthernetTypePPPoEDiscovery = 0x8863;
const EthernetType EthernetTypePPPoESession = 0x8864;
const EthernetType EthernetTypeMPLSUnicast = 0x8847;
const EthernetType EthernetTypeMPLSMulticast = 0x8848;
const EthernetType EthernetTypeEAPOL = 0x888e;
const EthernetType EthernetTypeERSPAN = 0x88be;
const EthernetType EthernetTypeQinQ = 0x88a8;
const EthernetType EthernetTypeLinkLayerDiscovery = 0x88cc;
const EthernetType EthernetTypeEthernetCTP = 0x9000;

// IPProtocol is an enumeration of IP protocol values, and acts as a decoder
// for any type it supports.
using IPProtocol = uint8;

const IPProtocol IPProtocolIPv6HopByHop = 0;
const IPProtocol IPProtocolICMPv4 = 1;
const IPProtocol IPProtocolIGMP = 2;
const IPProtocol IPProtocolIPv4 = 4;
const IPProtocol IPProtocolTCP = 6;
const IPProtocol IPProtocolUDP = 17;
const IPProtocol IPProtocolRUDP = 27;
const IPProtocol IPProtocolIPv6 = 41;
const IPProtocol IPProtocolIPv6Routing = 43;
const IPProtocol IPProtocolIPv6Fragment = 44;
const IPProtocol IPProtocolGRE = 47;
const IPProtocol IPProtocolESP = 50;
const IPProtocol IPProtocolAH = 51;
const IPProtocol IPProtocolICMPv6 = 58;
const IPProtocol IPProtocolNoNextHeader = 59;
const IPProtocol IPProtocolIPv6Destination = 60;
const IPProtocol IPProtocolOSPF = 89;
const IPProtocol IPProtocolIPIP = 94;
const IPProtocol IPProtocolEtherIP = 97;
const IPProtocol IPProtocolVRRP = 112;
const IPProtocol IPProtocolSCTP = 132;
const IPProtocol IPProtocolUDPLite = 136;
const IPProtocol IPProtocolMPLSInIP = 137;

// LinkType is an enumeration of link types, and acts as a decoder for any
// link type it supports.
using LinkType = uint8;

// According to pcap-linktype(7) and http://www.tcpdump.org/linktypes.html
const LinkType LinkTypeNull = 0;
const LinkType LinkTypeEthernet = 1;
const LinkType LinkTypeAX25 = 3;
const LinkType LinkTypeTokenRing = 6;
const LinkType LinkTypeArcNet = 7;
const LinkType LinkTypeSLIP = 8;
const LinkType LinkTypePPP = 9;
const LinkType LinkTypeFDDI = 10;
const LinkType LinkTypePPP_HDLC = 50;
const LinkType LinkTypePPPEthernet = 51;
const LinkType LinkTypeATM_RFC1483 = 100;
const LinkType LinkTypeRaw = 101;
const LinkType LinkTypeC_HDLC = 104;
const LinkType LinkTypeIEEE802_11 = 105;
const LinkType LinkTypeFRelay = 107;
const LinkType LinkTypeLoop = 108;
const LinkType LinkTypeLinuxSLL = 113;
const LinkType LinkTypeLTalk = 114;
const LinkType LinkTypePFLog = 117;
const LinkType LinkTypePrismHeader = 119;
const LinkType LinkTypeIPOverFC = 122;
const LinkType LinkTypeSunATM = 123;
const LinkType LinkTypeIEEE80211Radio = 127;
const LinkType LinkTypeARCNetLinux = 129;
const LinkType LinkTypeIPOver1394 = 138;
const LinkType LinkTypeMTP2Phdr = 139;
const LinkType LinkTypeMTP2 = 140;
const LinkType LinkTypeMTP3 = 141;
const LinkType LinkTypeSCCP = 142;
const LinkType LinkTypeDOCSIS = 143;
const LinkType LinkTypeLinuxIRDA = 144;
const LinkType LinkTypeLinuxLAPD = 177;
const LinkType LinkTypeLinuxUSB = 220;
const LinkType LinkTypeFC2 = 224;
const LinkType LinkTypeFC2Framed = 225;
const LinkType LinkTypeIPv4 = 228;
const LinkType LinkTypeIPv6 = 229;

// PPPoECode is the PPPoE code enum, taken from http://tools.ietf.org/html/rfc2516
using PPPoECode = uint8;

const PPPoECode PPPoECodePADI = 0x09;
const PPPoECode PPPoECodePADO = 0x07;
const PPPoECode PPPoECodePADR = 0x19;
const PPPoECode PPPoECodePADS = 0x65;
const PPPoECode PPPoECodePADT = 0xA7;
const PPPoECode PPPoECodeSession = 0x00;

// PPPType is an enumeration of PPP type values, and acts as a decoder for any
// type it supports.
using PPPType = uint16;

const PPPType PPPTypeIPv4 = 0x0021;
const PPPType PPPTypeIPv6 = 0x0057;
const PPPType PPPTypeMPLSUnicast = 0x0281;
const PPPType PPPTypeMPLSMulticast = 0x0283;

// SCTPChunkType is an enumeration of chunk types inside SCTP packets.
using SCTPChunkType = uint8;

const SCTPChunkType SCTPChunkTypeData = 0;
const SCTPChunkType SCTPChunkTypeInit = 1;
const SCTPChunkType SCTPChunkTypeInitAck = 2;
const SCTPChunkType SCTPChunkTypeSack = 3;
const SCTPChunkType SCTPChunkTypeHeartbeat = 4;
const SCTPChunkType SCTPChunkTypeHeartbeatAck = 5;
const SCTPChunkType SCTPChunkTypeAbort = 6;
const SCTPChunkType SCTPChunkTypeShutdown = 7;
const SCTPChunkType SCTPChunkTypeShutdownAck = 8;
const SCTPChunkType SCTPChunkTypeError = 9;
const SCTPChunkType SCTPChunkTypeCookieEcho = 10;
const SCTPChunkType SCTPChunkTypeCookieAck = 11;
const SCTPChunkType SCTPChunkTypeShutdownComplete = 14;

// FDDIFrameControl is an enumeration of FDDI frame control bytes.
using FDDIFrameControl = uint8;

const FDDIFrameControl FDDIFrameControlLLC = 0x50;

// EAPOLType is an enumeration of EAPOL packet types.
using EAPOLType = uint8;

const EAPOLType EAPOLTypeEAP = 0;
const EAPOLType EAPOLTypeStart = 1;
const EAPOLType EAPOLTypeLogOff = 2;
const EAPOLType EAPOLTypeKey = 3;
const EAPOLType EAPOLTypeASFAlert = 4;

// ProtocolFamily is the set of values defined as PF_* in sys/socket.h
using ProtocolFamily = uint8;

const ProtocolFamily ProtocolFamilyIPv4 = 2;
// BSDs use different values for INET6... glory be.  These values taken from
// tcpdump 4.3.0.
const ProtocolFamily ProtocolFamilyIPv6BSD = 24;
const ProtocolFamily ProtocolFamilyIPv6FreeBSD = 28;
const ProtocolFamily ProtocolFamilyIPv6Darwin = 30;
const ProtocolFamily ProtocolFamilyIPv6Linux = 10;

// Dot11Type is a combination of IEEE 802.11 frame's Type and Subtype fields.
// By combining these two fields together into a single type, we're able to
// provide a String function that correctly displays the subtype given the
// top-level type.
//
// If you just care about the top-level type, use the MainType function.
using Dot11Type = uint8;

// // MainType strips the subtype information from the given type,
// // returning just the overarching type (Mgmt, Ctrl, Data, Reserved).
// func (d Dot11Type) MainType() Dot11Type {
// 	return d & dot11TypeMask
// }

// func (d Dot11Type) QOS() bool {
// 	return d&dot11QOSMask == Dot11TypeDataQOSData
// }

const Dot11Type Dot11TypeMgmt = 0x00;
const Dot11Type Dot11TypeCtrl = 0x01;
const Dot11Type Dot11TypeData = 0x02;
const Dot11Type Dot11TypeReserved = 0x03;
const Dot11Type dot11TypeMask = 0x03;
const Dot11Type dot11QOSMask = 0x23;

// The following are type/subtype conglomerations.

// Management
const Dot11Type Dot11TypeMgmtAssociationReq = 0x00;
const Dot11Type Dot11TypeMgmtAssociationResp = 0x04;
const Dot11Type Dot11TypeMgmtReassociationReq = 0x08;
const Dot11Type Dot11TypeMgmtReassociationResp = 0x0c;
const Dot11Type Dot11TypeMgmtProbeReq = 0x10;
const Dot11Type Dot11TypeMgmtProbeResp = 0x14;
const Dot11Type Dot11TypeMgmtMeasurementPilot = 0x18;
const Dot11Type Dot11TypeMgmtBeacon = 0x20;
const Dot11Type Dot11TypeMgmtATIM = 0x24;
const Dot11Type Dot11TypeMgmtDisassociation = 0x28;
const Dot11Type Dot11TypeMgmtAuthentication = 0x2c;
const Dot11Type Dot11TypeMgmtDeauthentication = 0x30;
const Dot11Type Dot11TypeMgmtAction = 0x34;
const Dot11Type Dot11TypeMgmtActionNoAck = 0x38;

// Control
const Dot11Type Dot11TypeCtrlWrapper = 0x1d;
const Dot11Type Dot11TypeCtrlBlockAckReq = 0x21;
const Dot11Type Dot11TypeCtrlBlockAck = 0x25;
const Dot11Type Dot11TypeCtrlPowersavePoll = 0x29;
const Dot11Type Dot11TypeCtrlRTS = 0x2d;
const Dot11Type Dot11TypeCtrlCTS = 0x31;
const Dot11Type Dot11TypeCtrlAck = 0x35;
const Dot11Type Dot11TypeCtrlCFEnd = 0x39;
const Dot11Type Dot11TypeCtrlCFEndAck = 0x3d;

// Data
const Dot11Type Dot11TypeDataCFAck = 0x06;
const Dot11Type Dot11TypeDataCFPoll = 0x0a;
const Dot11Type Dot11TypeDataCFAckPoll = 0x0e;
const Dot11Type Dot11TypeDataNull = 0x12;
const Dot11Type Dot11TypeDataCFAckNoData = 0x16;
const Dot11Type Dot11TypeDataCFPollNoData = 0x1a;
const Dot11Type Dot11TypeDataCFAckPollNoData = 0x1e;
const Dot11Type Dot11TypeDataQOSData = 0x22;
const Dot11Type Dot11TypeDataQOSDataCFAck = 0x26;
const Dot11Type Dot11TypeDataQOSDataCFPoll = 0x2a;
const Dot11Type Dot11TypeDataQOSDataCFAckPoll = 0x2e;
const Dot11Type Dot11TypeDataQOSNull = 0x32;
const Dot11Type Dot11TypeDataQOSCFPollNoData = 0x3a;
const Dot11Type Dot11TypeDataQOSCFAckPollNoData = 0x3e;

}  // namespace layers
}  // namespace nx
