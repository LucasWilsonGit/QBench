#pragma once
#include <vector>
#include <functional>

#include "Event.h"

namespace Engine {
	class EventHandler
	{
	public:
		EventHandler();
		~EventHandler();

		void Fire(const EventData&);
		uint8_t AddEvent(Event* e);
		Event* GetEvent(int idx); //gets event from the internal event ptr array
	private:
		uint32_t m_eventoffs = EVENT_UNDEFINED;
		Event** m_events;
	};
}