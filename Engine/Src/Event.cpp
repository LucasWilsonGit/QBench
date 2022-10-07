#include "Event.h"
#include <iostream>

namespace Engine {
	Listener::Listener(Callback c) : m_callback(c) {}
	Listener::~Listener() {}

	void Listener::Fire(const EventData& e) {
		m_callback(e);
	}

	Event::Event() : m_hooks(std::vector<std::shared_ptr<Listener>>{}) {
		std::cout << "Event init" << std::endl;
		std::cout << "Event hooks: " << m_hooks.size() << std::endl;
	}
	Event::~Event() {
	}

	void Event::Fire(const EventData& e) {
		//std::cout << "Event::Fire " << e.eventflags << std::endl;
		if (m_hooks.size() > 0 && !m_hooks.empty()) {
			for (auto it = m_hooks.begin(); it != m_hooks.end(); it++) {
				(*it)->Fire(e);
			}
		}
	}

	std::shared_ptr<Listener> Event::Connect(Callback c) {
		std::shared_ptr<Listener> l = std::make_shared<Listener>(c);
		m_hooks.push_back(l);
		return l;
	}

	void Event::Disconnect(std::shared_ptr<Listener>& l) {
		for (auto it = m_hooks.begin(); it != m_hooks.end(); ) {
			if ((*it) == l) {
				m_hooks.erase(it);
				return;
			}
			else
			{
				it++;
			}
		}
		std::cout << "FAILED TO DISCONNECT!? Event.cpp" << std::endl;
	}
}