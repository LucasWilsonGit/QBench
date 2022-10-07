#include "EventHandler.h"
#include <iostream>
#include "core.h"

Engine::EventHandler::EventHandler() : m_events(new Event*[sizeof(EVENTFLAGSTYPE)*8]) {
	for (int i = 0; i < sizeof(EVENTFLAGSTYPE)*8; i++) {
		std::cout << i;
		m_events[i] = new Event(); //I think the copy-assignment is causing memory issues with the pointers inside the Event list object
	}
}
Engine::EventHandler::~EventHandler() {
	for (auto i = 0; i < sizeof(EVENTFLAGSTYPE)*8; i++) { 
		delete m_events[i];
	}
}

void Engine::EventHandler::Fire(const EventData& e) {
	for (unsigned int i = 0; i < m_eventoffs; i++) {
		if (e.eventflags == i) {
			m_events[i]->Fire(e);
			//std::cout << "EVENT: " << i << std::endl;
		}
	}
}

uint8_t Engine::EventHandler::AddEvent(Event* e) {
	if (m_eventoffs >= sizeof(EVENTFLAGSTYPE)*8) {
		std::cout << "Event flags overflow" << std::endl;
		return -1;
	}
	m_events[m_eventoffs] = e;
	m_eventoffs++;
	return m_eventoffs-1;
}

Engine::Event* Engine::EventHandler::GetEvent(int idx) {
	if (idx >= sizeof(EVENTFLAGSTYPE) * 8) {
		ERROR("Attempt to index EventHandler event at " << idx);
	}
	return m_events[idx]; 
}