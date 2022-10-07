#pragma once

#include <functional>
#include <list>

#define BIT(x) 1 << x

#define EVENT_KEYDOWN 0
#define EVENT_KEYUP 1
#define EVENT_MOUSEBUTTON 2
#define EVENT_MOUSEMOVE 3
#define EVENT_MOUSESCROLL 4
#define EVENT_WINDOWRESIZE 5
#define EVENT_WINDOWCLOSE 6
#define EVENT_CHAR 7
//..

#define EVENT_UNDEFINED 8//The first free flag for custom events to be defined

#define EVENTFLAGSTYPE uint32_t

//not threadsafe
namespace Engine {
	
	struct KeyEventInfo {
		int32_t key;
		int32_t scancode;
		int32_t action;
		int32_t mods;
	};

	struct MouseButtonEventInfo {
		int32_t button;
		int32_t action;
		int32_t mods;
	};

	struct MouseMoveEventInfo {
		double xpos;
		double ypos;
	};

	struct MouseScrollEventInfo {
		double dX;
		double dY;
	};

	struct WindowResizeEventInfo {
		uint32_t width;
		uint32_t height;
	};

	struct CharEventInfo {
		uint16_t character; //holds 1 ascii char and then a null terminator aka 2 char string
	};

	struct EventData {
		EVENTFLAGSTYPE eventflags; //overkill but allows sufficient space for expansion of event implementation
		union {
			KeyEventInfo ki;
			MouseButtonEventInfo mbi;
			MouseMoveEventInfo mmi;
			MouseScrollEventInfo msi;
			WindowResizeEventInfo wri;
			CharEventInfo ci;
		} eventinfo;
		std::function<void(const EventData&)> nexthook;
	};
	//Maybe unions are a bit cursed.

	using Callback = std::function<void(const EventData&)>;

	class Listener
	{
	public:
		Listener(Callback);
		~Listener();
		
		void Fire(const EventData&);
	private:
		Callback m_callback;
	};

	class Event
	{
	public:
		Event();
		~Event();

		void Fire(const EventData&);
		std::shared_ptr<Listener> Connect(Callback);
		void Disconnect(std::shared_ptr<Listener>&);

	private:
		std::vector<std::shared_ptr<Listener>> m_hooks;
	};
}
