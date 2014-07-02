/* $Id: ipx_drv.h,v 1.7 2003/10/12 09:17:47 btb Exp $ */

/*
 *
 * IPX driver interface
 *
 * parts from:
 * ipx[HA] header file for IPX for the DOS emulator
 * 		Tim Bird, tbird@novell.com
 *
 */

#ifndef _IPX_DRV_H
#define _IPX_DRV_H

#define IPX_MANUAL_ADDRESS

#ifdef _WIN32
//#	include <winsock.h>
#else
#	include <sys/socket.h>
#endif

#include "pstypes.h"

#ifdef _WIN32
#pragma pack (push, 1)
#endif

typedef struct IPXAddressStruct {
	uint8_t Network[4]; // __pack__;
	uint8_t Node[6]; // __pack__;
	uint8_t Socket[2]; // __pack__;
} IPXAddress_t;

typedef struct IPXPacketStructure {
	uint16_t Checksum; // __pack__;
	uint16_t Length; // __pack__;
	uint8_t TransportControl; // __pack__;
	uint8_t PacketType; // __pack__;
	IPXAddress_t Destination; // __pack__;
	IPXAddress_t Source; // __pack__;
} IPXPacket_t;

#ifdef _WIN32
#pragma pack (pop)
#endif

#ifndef _WIN32
#	define UINT_PTR	uint32_t
#	define INT_PTR		int32_t
#endif

typedef struct ipx_socket_struct {
	uint16_t socket;
	UINT_PTR	fd;
} ipx_socket_t;

class CPacketOrigin : public CNetworkNode {
	/* all network order */
	private:
		//tNetworkNode	src_addr;
		int32_t				packetType;

	public:
		uint16_t			src_socket;
		uint16_t			dst_socket;

	public:
		inline void SetType (int32_t nType) { packetType = nType; }
#if 0 // inherited from CNetworkNode
		inline uint32_t Server (void) { return src_addr.address.portAddress.ip.a; }
		inline uint16_t Port (void) { return src_addr.address.portAddress.port.s; }
		inline uint8_t* Network (void) { return src_addr.network.b; }
		inline uint8_t* Node (void) { return src_addr.address.node; }
		inline int32_t Type (void) { return packetType; }

		inline void SetServer (void* server) { memcpy (src_addr.address.portAddress.ip.octets, (uint8_t*) server, sizeof (src_addr.address.portAddress.ip.octets)); }
		inline void SetNetwork (void* network) { memcpy (src_addr.network.b, (uint8_t*) network, sizeof (src_addr.network)); }
		inline void SetNode (void* node) { memcpy (src_addr.address.node, (uint8_t*) node, sizeof (src_addr.address.node)); }
		inline void SetPort (void* port) { memcpy (src_addr.address.portAddress.port.b, (uint8_t*) port, sizeof (src_addr.address.portAddress.port)); }

		inline void ResetServer (uint8_t filler = 0) { memset (src_addr.address.portAddress.ip.octets, filler, sizeof (src_addr.address.portAddress.ip.octets)); }
		inline void ResetNetwork (uint8_t filler = 0) { memset (src_addr.network.b, filler, sizeof (src_addr.network)); }
		inline void ResetNode (uint8_t filler = 0) { memset (src_addr.address.node, filler, sizeof (src_addr.address.node)); }
		inline void ResetPort (uint8_t filler = 0) { memset (src_addr.address.portAddress.port.b, filler, sizeof (src_addr.address.portAddress.port)); }
#endif
		inline tNetworkNode& Address (void) { return m_address; }

		inline bool operator== (CPacketOrigin& other) { return !memcmp (&Address (), &other.Address (), sizeof (tNetworkNode)); }
		inline CPacketOrigin& operator= (CPacketOrigin& other) { 
			memcpy (this, &other, sizeof (*this));
			return *this;
			}
};

struct ipx_driver {
	int32_t (*GetMyAddress)();
	int32_t (*OpenSocket)(ipx_socket_t *, int32_t);
	void (*CloseSocket)(ipx_socket_t *);
	int32_t (*SendPacket)(ipx_socket_t *, IPXPacket_t *, uint8_t *, int32_t);
	int32_t (*ReceivePacket)(ipx_socket_t *, uint8_t *, int32_t, CPacketOrigin *);
	int32_t (*PacketReady)(ipx_socket_t *s);
	void (*InitNetGameAuxData)(ipx_socket_t *, uint8_t []);
	int32_t (*HandleNetGameAuxData)(ipx_socket_t *, const uint8_t []);
	void (*HandleLeaveGame)(ipx_socket_t *);
	int32_t (*SendGamePacket)(ipx_socket_t *s, uint8_t *, int32_t);
};

int32_t IPXGeneralPacketReady (ipx_socket_t *s);

extern uint8_t ipx_MyAddress[10];

#endif /* _IPX_DRV_H */
