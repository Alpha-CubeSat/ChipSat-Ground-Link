#ifndef Globals_h
#define Globals_h

#include "Modes/module_state_enums.h"
#include "Modes/packet_type_enums.h"

extern volatile bool receivedFlag;
extern volatile bool enableInterrupt;
extern unsigned long currentTime;
extern unsigned long transmittingStartTime;
extern ModuleState currentState;
extern PacketType currentPacketType;
extern uint16_t newDownlinkPeriod;

#endif
