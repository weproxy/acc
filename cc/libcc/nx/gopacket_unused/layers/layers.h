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
constexpr EthernetType EthernetTypeLLC = 0;
constexpr EthernetType EthernetTypeIPv4 = 0x0800;
constexpr EthernetType EthernetTypeARP = 0x0806;
constexpr EthernetType EthernetTypeIPv6 = 0x86DD;
constexpr EthernetType EthernetTypeCiscoDiscovery = 0x2000;
constexpr EthernetType EthernetTypeNortelDiscovery = 0x01a2;
constexpr EthernetType EthernetTypeTransparentEthernetBridging = 0x6558;
constexpr EthernetType EthernetTypeDot1Q = 0x8100;
constexpr EthernetType EthernetTypePPP = 0x880b;
constexpr EthernetType EthernetTypePPPoEDiscovery = 0x8863;
constexpr EthernetType EthernetTypePPPoESession = 0x8864;
constexpr EthernetType EthernetTypeMPLSUnicast = 0x8847;
constexpr EthernetType EthernetTypeMPLSMulticast = 0x8848;
constexpr EthernetType EthernetTypeEAPOL = 0x888e;
constexpr EthernetType EthernetTypeERSPAN = 0x88be;
constexpr EthernetType EthernetTypeQinQ = 0x88a8;
constexpr EthernetType EthernetTypeLinkLayerDiscovery = 0x88cc;
constexpr EthernetType EthernetTypeEthernetCTP = 0x9000;

// IPProtocol is an enumeration of IP protocol values, and acts as a decoder
// for any type it supports.
using IPProtocol = uint8;

constexpr IPProtocol IPProtocolIPv6HopByHop = 0;
constexpr IPProtocol IPProtocolICMPv4 = 1;
constexpr IPProtocol IPProtocolIGMP = 2;
constexpr IPProtocol IPProtocolIPv4 = 4;
constexpr IPProtocol IPProtocolTCP = 6;
constexpr IPProtocol IPProtocolUDP = 17;
constexpr IPProtocol IPProtocolRUDP = 27;
constexpr IPProtocol IPProtocolIPv6 = 41;
constexpr IPProtocol IPProtocolIPv6Routing = 43;
constexpr IPProtocol IPProtocolIPv6Fragment = 44;
constexpr IPProtocol IPProtocolGRE = 47;
constexpr IPProtocol IPProtocolESP = 50;
constexpr IPProtocol IPProtocolAH = 51;
constexpr IPProtocol IPProtocolICMPv6 = 58;
constexpr IPProtocol IPProtocolNoNextHeader = 59;
constexpr IPProtocol IPProtocolIPv6Destination = 60;
constexpr IPProtocol IPProtocolOSPF = 89;
constexpr IPProtocol IPProtocolIPIP = 94;
constexpr IPProtocol IPProtocolEtherIP = 97;
constexpr IPProtocol IPProtocolVRRP = 112;
constexpr IPProtocol IPProtocolSCTP = 132;
constexpr IPProtocol IPProtocolUDPLite = 136;
constexpr IPProtocol IPProtocolMPLSInIP = 137;

// LinkType is an enumeration of link types, and acts as a decoder for any
// link type it supports.
using LinkType = uint8;

// According to pcap-linktype(7) and http://www.tcpdump.org/linktypes.html
constexpr LinkType LinkTypeNull = 0;
constexpr LinkType LinkTypeEthernet = 1;
constexpr LinkType LinkTypeAX25 = 3;
constexpr LinkType LinkTypeTokenRing = 6;
constexpr LinkType LinkTypeArcNet = 7;
constexpr LinkType LinkTypeSLIP = 8;
constexpr LinkType LinkTypePPP = 9;
constexpr LinkType LinkTypeFDDI = 10;
constexpr LinkType LinkTypePPP_HDLC = 50;
constexpr LinkType LinkTypePPPEthernet = 51;
constexpr LinkType LinkTypeATM_RFC1483 = 100;
constexpr LinkType LinkTypeRaw = 101;
constexpr LinkType LinkTypeC_HDLC = 104;
constexpr LinkType LinkTypeIEEE802_11 = 105;
constexpr LinkType LinkTypeFRelay = 107;
constexpr LinkType LinkTypeLoop = 108;
constexpr LinkType LinkTypeLinuxSLL = 113;
constexpr LinkType LinkTypeLTalk = 114;
constexpr LinkType LinkTypePFLog = 117;
constexpr LinkType LinkTypePrismHeader = 119;
constexpr LinkType LinkTypeIPOverFC = 122;
constexpr LinkType LinkTypeSunATM = 123;
constexpr LinkType LinkTypeIEEE80211Radio = 127;
constexpr LinkType LinkTypeARCNetLinux = 129;
constexpr LinkType LinkTypeIPOver1394 = 138;
constexpr LinkType LinkTypeMTP2Phdr = 139;
constexpr LinkType LinkTypeMTP2 = 140;
constexpr LinkType LinkTypeMTP3 = 141;
constexpr LinkType LinkTypeSCCP = 142;
constexpr LinkType LinkTypeDOCSIS = 143;
constexpr LinkType LinkTypeLinuxIRDA = 144;
constexpr LinkType LinkTypeLinuxLAPD = 177;
constexpr LinkType LinkTypeLinuxUSB = 220;
constexpr LinkType LinkTypeFC2 = 224;
constexpr LinkType LinkTypeFC2Framed = 225;
constexpr LinkType LinkTypeIPv4 = 228;
constexpr LinkType LinkTypeIPv6 = 229;

// PPPoECode is the PPPoE code enum, taken from http://tools.ietf.org/html/rfc2516
using PPPoECode = uint8;

constexpr PPPoECode PPPoECodePADI = 0x09;
constexpr PPPoECode PPPoECodePADO = 0x07;
constexpr PPPoECode PPPoECodePADR = 0x19;
constexpr PPPoECode PPPoECodePADS = 0x65;
constexpr PPPoECode PPPoECodePADT = 0xA7;
constexpr PPPoECode PPPoECodeSession = 0x00;

// PPPType is an enumeration of PPP type values, and acts as a decoder for any
// type it supports.
using PPPType = uint16;

constexpr PPPType PPPTypeIPv4 = 0x0021;
constexpr PPPType PPPTypeIPv6 = 0x0057;
constexpr PPPType PPPTypeMPLSUnicast = 0x0281;
constexpr PPPType PPPTypeMPLSMulticast = 0x0283;

// SCTPChunkType is an enumeration of chunk types inside SCTP packets.
using SCTPChunkType = uint8;

constexpr SCTPChunkType SCTPChunkTypeData = 0;
constexpr SCTPChunkType SCTPChunkTypeInit = 1;
constexpr SCTPChunkType SCTPChunkTypeInitAck = 2;
constexpr SCTPChunkType SCTPChunkTypeSack = 3;
constexpr SCTPChunkType SCTPChunkTypeHeartbeat = 4;
constexpr SCTPChunkType SCTPChunkTypeHeartbeatAck = 5;
constexpr SCTPChunkType SCTPChunkTypeAbort = 6;
constexpr SCTPChunkType SCTPChunkTypeShutdown = 7;
constexpr SCTPChunkType SCTPChunkTypeShutdownAck = 8;
constexpr SCTPChunkType SCTPChunkTypeError = 9;
constexpr SCTPChunkType SCTPChunkTypeCookieEcho = 10;
constexpr SCTPChunkType SCTPChunkTypeCookieAck = 11;
constexpr SCTPChunkType SCTPChunkTypeShutdownComplete = 14;

// FDDIFrameControl is an enumeration of FDDI frame control bytes.
using FDDIFrameControl = uint8;

constexpr FDDIFrameControl FDDIFrameControlLLC = 0x50;

// EAPOLType is an enumeration of EAPOL packet types.
using EAPOLType = uint8;

constexpr EAPOLType EAPOLTypeEAP = 0;
constexpr EAPOLType EAPOLTypeStart = 1;
constexpr EAPOLType EAPOLTypeLogOff = 2;
constexpr EAPOLType EAPOLTypeKey = 3;
constexpr EAPOLType EAPOLTypeASFAlert = 4;

// ProtocolFamily is the set of values defined as PF_* in sys/socket.h
using ProtocolFamily = uint8;

constexpr ProtocolFamily ProtocolFamilyIPv4 = 2;
// BSDs use different values for INET6... glory be.  These values taken from
// tcpdump 4.3.0.
constexpr ProtocolFamily ProtocolFamilyIPv6BSD = 24;
constexpr ProtocolFamily ProtocolFamilyIPv6FreeBSD = 28;
constexpr ProtocolFamily ProtocolFamilyIPv6Darwin = 30;
constexpr ProtocolFamily ProtocolFamilyIPv6Linux = 10;

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

constexpr Dot11Type Dot11TypeMgmt = 0x00;
constexpr Dot11Type Dot11TypeCtrl = 0x01;
constexpr Dot11Type Dot11TypeData = 0x02;
constexpr Dot11Type Dot11TypeReserved = 0x03;
constexpr Dot11Type dot11TypeMask = 0x03;
constexpr Dot11Type dot11QOSMask = 0x23;

// The following are type/subtype conglomerations.

// Management
constexpr Dot11Type Dot11TypeMgmtAssociationReq = 0x00;
constexpr Dot11Type Dot11TypeMgmtAssociationResp = 0x04;
constexpr Dot11Type Dot11TypeMgmtReassociationReq = 0x08;
constexpr Dot11Type Dot11TypeMgmtReassociationResp = 0x0c;
constexpr Dot11Type Dot11TypeMgmtProbeReq = 0x10;
constexpr Dot11Type Dot11TypeMgmtProbeResp = 0x14;
constexpr Dot11Type Dot11TypeMgmtMeasurementPilot = 0x18;
constexpr Dot11Type Dot11TypeMgmtBeacon = 0x20;
constexpr Dot11Type Dot11TypeMgmtATIM = 0x24;
constexpr Dot11Type Dot11TypeMgmtDisassociation = 0x28;
constexpr Dot11Type Dot11TypeMgmtAuthentication = 0x2c;
constexpr Dot11Type Dot11TypeMgmtDeauthentication = 0x30;
constexpr Dot11Type Dot11TypeMgmtAction = 0x34;
constexpr Dot11Type Dot11TypeMgmtActionNoAck = 0x38;

// Control
constexpr Dot11Type Dot11TypeCtrlWrapper = 0x1d;
constexpr Dot11Type Dot11TypeCtrlBlockAckReq = 0x21;
constexpr Dot11Type Dot11TypeCtrlBlockAck = 0x25;
constexpr Dot11Type Dot11TypeCtrlPowersavePoll = 0x29;
constexpr Dot11Type Dot11TypeCtrlRTS = 0x2d;
constexpr Dot11Type Dot11TypeCtrlCTS = 0x31;
constexpr Dot11Type Dot11TypeCtrlAck = 0x35;
constexpr Dot11Type Dot11TypeCtrlCFEnd = 0x39;
constexpr Dot11Type Dot11TypeCtrlCFEndAck = 0x3d;

// Data
constexpr Dot11Type Dot11TypeDataCFAck = 0x06;
constexpr Dot11Type Dot11TypeDataCFPoll = 0x0a;
constexpr Dot11Type Dot11TypeDataCFAckPoll = 0x0e;
constexpr Dot11Type Dot11TypeDataNull = 0x12;
constexpr Dot11Type Dot11TypeDataCFAckNoData = 0x16;
constexpr Dot11Type Dot11TypeDataCFPollNoData = 0x1a;
constexpr Dot11Type Dot11TypeDataCFAckPollNoData = 0x1e;
constexpr Dot11Type Dot11TypeDataQOSData = 0x22;
constexpr Dot11Type Dot11TypeDataQOSDataCFAck = 0x26;
constexpr Dot11Type Dot11TypeDataQOSDataCFPoll = 0x2a;
constexpr Dot11Type Dot11TypeDataQOSDataCFAckPoll = 0x2e;
constexpr Dot11Type Dot11TypeDataQOSNull = 0x32;
constexpr Dot11Type Dot11TypeDataQOSCFPollNoData = 0x3a;
constexpr Dot11Type Dot11TypeDataQOSCFAckPollNoData = 0x3e;

}  // namespace layers
}  // namespace nx
