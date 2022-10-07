#include "app.h"
#include <string>
#include "Event.h"
#include "timer.h"
#include <commdlg.h>
#include "renderer.h"


void debug_lua_stack(lua_State* L) {
	std::string str;
	for (auto index = lua_gettop(L); index > 0; index--) {
		int type = lua_type(L, index);
		str += "[" + std::to_string(index) + "]";
		switch (type)
		{
		case LUA_TBOOLEAN:
		{
			str += "bool: ";
			str += (lua_toboolean(L, index) ? "true" : "false");
			break;
		}
		break;
		case LUA_TNUMBER:
		{
			str += "number: ";
			str += std::to_string(lua_tonumber(L, index));
			break;
		}
		break;
		case LUA_TSTRING:
		{
			str += "string: ";
			const char* k = lua_tostring(L, index);
			const char* k2 = _strdup(lua_tolstring(L, index, 0));
			str += k2;
			delete[] k2;
			break;
		}
		break;
		case LUA_TTABLE:
		{
			str += "table: ";
			lua_Integer length = lua_rawlen(L, index);
			str += "length: " + std::to_string(length);
			break;
		}
		default:
		{
			str += "DEFAULT: ";
			str += lua_typename(L, type);
			break;
		}
		break;
		}
		str += "\n";
	}
	std::cout << str << std::endl;
}

static Engine::ColorParams ReadColorParams(lua_State* L) {
	//assume the color table is at the top of the stack
	Engine::ColorParams c{};

	lua_pushinteger(L, 1);
	lua_rawget(L, -2);
	c.r = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pushinteger(L, 2);
	lua_rawget(L, -2);
	c.g = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pushinteger(L, 3);
	lua_rawget(L, -2);
	c.b = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pushinteger(L, 4);
	lua_rawget(L, -2);
	c.a = lua_tonumber(L, -1);
	lua_pop(L, 1);

	return c;
}

static Engine::RectParams ReadRectParams(lua_State* L) {
	Engine::RectParams c{};

	//assume a table of form {int,int,int,int,int} is on the top of the stack
	lua_pushinteger(L, 1);
	lua_rawget(L, -2);
	c.x = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushinteger(L, 2);
	lua_rawget(L, -2);
	c.y = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushinteger(L, 3);
	lua_rawget(L, -2);
	c.width = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushinteger(L, 4);
	lua_rawget(L, -2);
	c.height = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushinteger(L, 5);
	lua_rawget(L, -2);
	Engine::ColorParams col = ReadColorParams(L);
	c.color = glm::vec4(col.r, col.g, col.b, col.a);
	lua_pop(L, 1);

	lua_pushinteger(L, 6);
	lua_rawget(L, -2);
	c.z = (uint32_t)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushinteger(L, 7);
	lua_rawget(L, -2);
	c.border = lua_tointeger(L, -1);
	lua_pop(L, 1);

	return c;
	//restores the stack
}

static Engine::FilledRectParams ReadFilledRectParams(lua_State* L) {
	Engine::FilledRectParams c{};

	lua_pushinteger(L, 1);
	lua_rawget(L, -2);
	c.x = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushinteger(L, 2);
	lua_rawget(L, -2);
	c.y = lua_tointeger(L, -1);
	lua_pop(L, 1);//y

	lua_pushinteger(L, 3);
	lua_rawget(L, -2);
	c.z = lua_tointeger(L, -1);
	lua_pop(L, 1);//z

	lua_pushinteger(L, 4);
	lua_rawget(L, -2);
	c.width = lua_tointeger(L, -1);
	lua_pop(L, 1);//width

	lua_pushinteger(L, 5);
	lua_rawget(L, -2);
	c.height = lua_tointeger(L, -1);
	lua_pop(L, 1);//height

	lua_pushinteger(L, 6);
	lua_rawget(L, -2);
	Engine::ColorParams col = ReadColorParams(L);
	c.color = glm::vec4(col.r, col.g, col.b, col.a);
	lua_pop(L, 1);//color

	return c;
}

struct Engine::TextParams ReadTextParams(lua_State* L) {
	Engine::TextParams c{};

	lua_pushinteger(L, 1);
	lua_rawget(L, -2);
	const char* _str = lua_tostring(L, -1);
	c.s = std::string(_str);
	lua_pop(L, 1);

	lua_pushinteger(L, 2);
	lua_rawget(L, -2);
	c.x = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushinteger(L, 3);
	lua_rawget(L, -2);
	c.y = lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_pushinteger(L, 4);
	lua_rawget(L, -2);
	c.z = lua_tointeger(L, -1);
	lua_pop(L, 1);//z

	lua_pushinteger(L, 5);
	lua_rawget(L, -2);
	c.width = lua_tointeger(L, -1);
	lua_pop(L, 1);//width

	lua_pushinteger(L, 6);
	lua_rawget(L, -2);
	c.height = lua_tointeger(L, -1);
	lua_pop(L, 1);//height

	lua_pushinteger(L, 7);
	lua_rawget(L, -2);
	c.size = lua_tointeger(L, -1);
	lua_pop(L, 1);//height

	lua_pushinteger(L, 8);
	lua_rawget(L, -2);
	Engine::ColorParams col = ReadColorParams(L);
	c.color = glm::vec4(col.r, col.g, col.b, col.a);
	lua_pop(L, 1);//color

	lua_pushinteger(L, 9);
	lua_rawget(L, -2);
	c.wrap = lua_toboolean(L, -1);
	lua_pop(L, 1);

	return c;
}

namespace app_events {
	DissertationProject::app* app;
	std::shared_ptr<Engine::renderer> app_renderer;
	static void OnResize(const Engine::EventData& e) {
		glViewport(0, 0, e.eventinfo.wri.width, e.eventinfo.wri.height);
		if (app_renderer != nullptr) {
			app_renderer->set_viewport_size(e.eventinfo.wri.width, e.eventinfo.wri.height);
		}
	}

	static void OnExit(const Engine::EventData& e) {
		app_renderer.reset(); //decrements ref count
		app->exit();
	}

	static void OnScroll(const Engine::EventData& e) {
		lua_State* L = app->get_lua_state();
		lua_getglobal(L, "cpp_onscroll"); //call cpp_on_scroll
		lua_pushnumber(L, e.eventinfo.msi.dX);
		lua_pushnumber(L, e.eventinfo.msi.dY);
		lua_call(L, 2, 0);
	}

	static void OnMouseMove(const Engine::EventData& e) {
		//WARN("wow mouse moved by " << e.eventinfo.mmi.xpos << " " << e.eventinfo.mmi.ypos);
	}

	static void OnClick(const Engine::EventData& e) {
		glm::vec2 mousepos = app->mouse_pos();

		lua_State* L = app->get_lua_state();
		lua_getglobal(L, "cpp_onclick");
		lua_pushnumber(L, mousepos.x);
		lua_pushnumber(L, mousepos.y);
		lua_pushnumber(L, e.eventinfo.mbi.button);
		lua_pushnumber(L, e.eventinfo.mbi.action);
		lua_call(L, 4, 0);
	}

	static void OnChar(const Engine::EventData& e) {
		lua_State* L = app->get_lua_state();
		lua_getglobal(L, "cpp_onchar");
		lua_pushstring(L, (char*)&e.eventinfo.ci.character);
		lua_call(L, 1, 0);
	}

	static void OnKeyDown(const Engine::EventData& e) {
		lua_State* L = app->get_lua_state();
		lua_getglobal(L, "cpp_onkeydown");
		lua_pushnumber(L, e.eventinfo.ki.key);
		lua_call(L, 1, 0);
	}

	static void OnKeyUp(const Engine::EventData& e) {

	}
}

namespace DissertationProject {
	static int lua_Error(lua_State* L) {
		lua_settop(L, 1); //take first arg 
		const char* _str = lua_tostring(L, 1);
		ERROR(_str);
		lua_pop(L, -1);
		return 0;
	}

	static int lua_getTextRect(lua_State* L) {
		/*
		called from lua GetTextRect(string, width, fontsize) 
		size
		width
		string
		*/

		lua_settop(L, 3); //take first 3 args, discard the rest 

		int32_t fontsize = lua_tointeger(L, -1);
		lua_pop(L, 1);

		int32_t width = lua_tointeger(L, -1);
		lua_pop(L, 1);

		const char* _str = lua_tostring(L, -1);
		std::string text(_str);
		lua_pop(L, 1);

		glm::vec2 size = app_events::app_renderer->get_text_bounds(text, width, fontsize);

		lua_pushnumber(L, size.x);
		lua_pushnumber(L, size.y);

		return 2;
	}

	static int lua_PromptOpenData(lua_State* L) {
		/*
		* PromptOpenData()
		* 
		*/

		lua_settop(L, 1);

		std::string filename = app::open_file_name("CSV Files(.csv)\0*.csv\0Any File(.*)\0*.*\0");
		if (filename.size() > 0) {
			app::s_App->ReadData(filename);
		}

		lua_pop(L, 1);

		lua_pushstring(L, filename.c_str());

		return 1;
	}

	static int lua_PromptOpenScript(lua_State* L) {
		lua_settop(L, 0);

		std::string filename = app::open_file_name("Lua Files(.lua)\0*.lua\0");
		
		lua_pushstring(L, filename.c_str());
		return 1;
	}

	static int lua_SetChartRect(lua_State* L) {
		lua_settop(L, 4);
		int32_t x = lua_tointeger(L, 1);
		int32_t y = lua_tointeger(L, 2);
		int32_t w = lua_tointeger(L, 3);
		int32_t h = lua_tointeger(L, 4);
		lua_pop(L, 4);

		glm::vec2 bounds = app_events::app->GetWindowSize();

		app::s_App->m_ChartPos = glm::vec2(x, y);
		app::s_App->m_ChartSize = glm::vec2(w, h);

		//WARN(w << " " << h);

		return 0;
	}

	static int lua_SetChartCam(lua_State* L) {
		lua_settop(L, 4);
		float x = lua_tonumber(L, 1);
		float y = lua_tonumber(L, 2);
		float w = lua_tonumber(L, 3);
		float h = lua_tonumber(L, 4);
		lua_pop(L, 4);

		app::s_App->SetChartViewSize(glm::vec2(w, h));
		app::s_App->SetChartViewPos(glm::vec2(x, y));

		return 0;
	}

	static int lua_SetPlaybackCandle(lua_State* L) {
		lua_settop(L, 1);

		int s = lua_tointeger(L, 1);
		lua_pop(L, 1);

		app::s_App->SetPlaybackCandle(s);
		return 0;
	}

	static int lua_GetCandleData(lua_State* L) {
		//expect single arg INT candleID
		lua_settop(L, 1);
		int i = lua_tointeger(L, 1);
		lua_pop(L, 1);

		Candlestick c = app::s_App->GetCandleData(i);

		lua_pushnumber(L, c.open);
		lua_pushnumber(L, c.high);
		lua_pushnumber(L, c.low);
		lua_pushnumber(L, c.close);
		lua_pushnumber(L, c.volume);

		return 5;
	}

	app* app::s_App = nullptr;

	app::app(int width, int height) : m_toexit(false), Engine::application(width, height) {
		s_App = this;
		Engine::EventHandler& e = m_window->get_event_handler();

		app_events::app = this;
		app_events::app_renderer = m_renderer;
		e.GetEvent(EVENT_WINDOWRESIZE)->Connect(std::function<void(const Engine::EventData&)>(app_events::OnResize));
		e.GetEvent(EVENT_WINDOWCLOSE)->Connect(std::function<void(const Engine::EventData&)>(app_events::OnExit));
		e.GetEvent(EVENT_MOUSESCROLL)->Connect(std::function<void(const Engine::EventData&)>(app_events::OnScroll));
		e.GetEvent(EVENT_MOUSEMOVE)->Connect(std::function<void(const Engine::EventData&)>(app_events::OnMouseMove));
		e.GetEvent(EVENT_MOUSEBUTTON)->Connect(std::function<void(const Engine::EventData&)>(app_events::OnClick));
		e.GetEvent(EVENT_CHAR)->Connect(std::function<void(const Engine::EventData&)>(app_events::OnChar));
		e.GetEvent(EVENT_KEYDOWN)->Connect(std::function<void(const Engine::EventData&)>(app_events::OnKeyDown));
		e.GetEvent(EVENT_KEYUP)->Connect(std::function<void(const Engine::EventData&)>(app_events::OnKeyUp));

		Engine::CheckGLError("After events");

		CONSOLECOLOR(FOREGROUND_WHITE); //set console color at start of execution (only matters for debugging with same console over multiple sessions)

		m_Lua = luaL_newstate();
		luaL_openlibs(m_Lua);

		lua_register(m_Lua, "SubmitRenderCommands", lua_SubmitRenderCommands);
		lua_register(m_Lua, "error", lua_Error);
		lua_register(m_Lua, "GetTextRect", lua_getTextRect);
		lua_register(m_Lua, "PromptOpenData", lua_PromptOpenData);
		lua_register(m_Lua, "SetChartRect", lua_SetChartRect);
		lua_register(m_Lua, "SetChartCam", lua_SetChartCam);
		lua_register(m_Lua, "SetPlaybackCandle", lua_SetPlaybackCandle);
		lua_register(m_Lua, "GetCandleData", lua_GetCandleData);
		lua_register(m_Lua, "PromptOpenScript", lua_PromptOpenScript);

		int error = luaL_dofile(m_Lua, "src/app.lua");
		if (lua_status(m_Lua) != 0 || error != 0) {
			ERROR("app.cpp::Lua: " << lua_tostring(m_Lua, -1) << error);
		}

		Engine::CheckGLError("After lua");

		Engine::gl_shader::create_shader("rect", "src/Shaders/rect/vert.glsl", "src/Shaders/rect/frag.glsl");
		Engine::gl_shader::create_shader("text", "src/Shaders/text/vert.glsl", "src/Shaders/text/frag.glsl");
		Engine::gl_shader::create_shader("candlebody", "src/Shaders/candlebody/vert.glsl", "src/Shaders/candlebody/frag.glsl");
		Engine::gl_shader::create_shader("candlevolume", "src/Shaders/volume/vert.glsl", "src/Shaders/volume/frag.glsl");
		Engine::gl_shader::create_shader("candlewick", "src/Shaders/candlewick/vert.glsl", "src/Shaders/candlewick/frag.glsl");
		Engine::gl_shader::create_shader("imgrect", "src/Shaders/imgrect/vert.glsl", "src/Shaders/imgrect/frag.glsl");

		Engine::CheckGLError("After shaders");

		//open_file_name("Lua Files(.lua)\0*.lua\0Any File(.*)\0*.*\0");

		m_Chart = std::shared_ptr<CTexture>(new CTexture());
		m_Chart->CreateFromDataExt(0, 1920, 1080, GL_RGBA, GL_RGBA, GL_FLOAT, false); //1920x1080 RGB texture for our chart

		Engine::CheckGLError("After m_Chart");

		FramebufferDesc d;
		d.Width = 1920;
		d.Height = 1080;
		d.DepthAndStencil = false;
		d.WindowBuffer = false;
		d.InternalFormat = GL_RGBA;
		d.PixelFormat = GL_RGBA;
		d.NoColorAttachment = false;
		d.ChannelType = GL_FLOAT;
		m_ChartBuffer = Framebuffer::Create(d);

		Engine::CheckGLError("After m_ChartBuffer");

		m_ChartBuffer->AttachTexture(GL_COLOR_ATTACHMENT0, m_Chart);

		Engine::CheckGLError("After attach texture");

		m_Camera = Camera::Create(glm::vec2(0, 100), glm::vec2(1200, 100));

		m_renderer->set_viewport_size(width, height);

	}

	Candlestick app::GetCandleData(int i) {
		return m_Candles.GetCandle(i);
	}

	void app::SetChartViewSize(const glm::vec2& size) {
		m_Camera->SetDimensions(size);
	}

	void app::SetChartViewPos(const glm::vec2& pos) {
		m_Camera->SetPosition(pos);
	}

	app::~app() {
		WARN("app::~app()");
		lua_close(m_Lua);
		Engine::gl_shader::exit();//static exit method to clean up shader resources
		//Engine::renderer::exit();//static exit method to clean up renderer resources
	}

	void app::exit() {
		m_toexit = true;
	}

	int app::lua_SubmitRenderCommands(lua_State* L) {
		/*
		local batch = {
			rects = {
				{x, y, width, height, {r,g,b,a}, z},
				{},
				{}
			},
			filled_rects = {
			},
			text = {
			}
		}
		*/
		std::shared_ptr<Engine::renderer> renderer = Engine::renderer::get_instance();

		lua_settop(L, 1);
		//on call func args pushed to stack, stack should be clear before the call, set the top to the 1st arg and discard the rest 
		if (!lua_istable(L, 1)) {
			ERROR("app.cpp::lua_SubmitRenderCommands: invalid argument[1] expect table, got " << lua_typename(L, 1));
			lua_pop(L, -1);
		}

		lua_pushstring(L, "Rects");
		lua_rawget(L, -2); //search the 2nd from top of stack (command batch table) for key Rects 
		if (!lua_istable(L, -1)) {
			ERROR("app.cpp::lua_SubmitRenderCommands: expected buffer.Rects to be a table, got " << lua_typename(L, -1));
			lua_pop(L, -1);
		} //sanity check
		size_t length = lua_rawlen(L, -1);
		for (lua_Integer i = 0; i < length; i++) {
			lua_pushinteger(L, i + 1);
			lua_rawget(L, -2);
			//read table key (i+1)

			if (lua_status(L) != 0) {
				ERROR("app.cpp::Lua: " << lua_error(L));
			}

			Engine::RectParams c = ReadRectParams(L);
			renderer->push_rect(c);
			//std::cout << "rect{c.x = " << c.x << " c.y = " << c.y << " c.z = " << c.z << " c.width = " << c.width << " c.height = " << c.height << " c.border = " << c.border << " c.color = " << glm::to_string(c.color) << "}" << std::endl << std::endl;
			lua_pop(L, 1);//pop single rect entity table off the stack
		}
		lua_pop(L, 1);
		//pop Rects table

		lua_pushstring(L, "FilledRects");
		lua_rawget(L, -2);
		if (!lua_istable(L, -1)) {
			ERROR("app.cpp::lua_SubmitRenderCommands: expected buffer.FilledRects to be a table, got " << lua_typename(L, -1));
			lua_pop(L, -1);
		}
		length = lua_rawlen(L, -1);
		for (lua_Integer i = 0; i < length; i++) {
			lua_pushinteger(L, i + 1);
			lua_rawget(L, -2);

			Engine::FilledRectParams c = ReadFilledRectParams(L);
			renderer->push_filled_rect(c);
			//std::cout << "filled_rect{c.x = " << c.x << " c.y = " << c.y << " c.z = " << c.z << " c.width = " << c.width << " c.height = " << c.height << " c.color = " << glm::to_string(c.color) << "}" << std::endl;
			lua_pop(L, 1); //pop single filledrect entity
		}
		lua_pop(L, 1);
		//pop FilledRects table

		lua_pushstring(L, "Texts");
		lua_rawget(L, -2);
		if (!lua_istable(L, -1)) {
			ERROR("app.cpp::lua_SubmitRenderCommands: expected buffer.Texts to be a table, got " << lua_typename(L, -1));
			lua_pop(L, -1);
		}
		length = lua_rawlen(L, -1);
		for (lua_Integer i = 0; i < length; i++) {
			lua_pushinteger(L, i + 1);
			lua_rawget(L, -2);

			Engine::TextParams c = ReadTextParams(L);
			renderer->push_text(c);
			//std::cout << "filled_rect{c.x = " << c.x << " c.y = " << c.y << " c.z = " << c.z << " c.width = " << c.width << " c.height = " << c.height << " c.color = " << glm::to_string(c.color) << "}" << std::endl;
			lua_pop(L, 1); //pop single filledrect entity
		}
		lua_pop(L, 1);
		//pop FilledRects table

		lua_pop(L, 1); //pop RenderCommandBatch

		return 0;
	}

	void app::on_update() {
		//WARN("app::on_update");
		{
			//Engine::timer t;
			m_window->on_update();
		}

		{
			//Engine::timer t;
			Engine::CheckGLError("pre depth buffer config");
			//set up render state (just to be sure it is in the right initial configuration)
			/*glEnable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDepthMask(GL_FALSE); //set depth testing to read-only
			glDepthFunc(GL_LEQUAL); //closer elements render on top, equal depth obeys order of execution
			*/
			glClearColor(.3f, .3f, .3f, 1.f);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(GL_TRUE); //set depth testing to read+write
			glDepthFunc(GL_LEQUAL); //closer elements render on top, equal depth obeys order of execution
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			Engine::CheckGLError("post depth buffer config");
		}

		{
			//Engine::timer t;
			//on_update logic
			lua_getglobal(m_Lua, "cpp_ondraw");
			if (lua_isfunction(m_Lua, -1)) {
				glm::vec2 viewsize = m_renderer->get_viewport_size();
				glm::vec2 mousepos = m_window->mouse_position();
				lua_pushnumber(m_Lua, viewsize.x);
				lua_pushnumber(m_Lua, viewsize.y);
				lua_pushnumber(m_Lua, mousepos.x);
				lua_pushnumber(m_Lua, mousepos.y);
				//debug_lua_stack(m_Lua);
				lua_call(m_Lua, 4, 0);
			}
			else
			{
				ERROR("app.cpp::Lua: global 'cpp_ondraw' expected function found " << lua_type(m_Lua, -1));
				lua_pop(m_Lua, -1); //pop with -1 clears the stack, this may caused unexpected behaviors. 
			}
		}
		//lua draw calls etc.

		//finally draw
		{
			//Engine::timer t;
			m_renderer->draw_rects();
			m_renderer->draw_filled_rects();
			m_renderer->draw_text();
		}
		//m_renderer->draw_text("This is a bit of a text that is the first paragraph in the total text block I'm sending in a single draw call.\n\nThis is the second paragraph following after two consecutive line breaks.", 0, 0, 1, 800, 100, 12, glm::vec4(0.f, 1.f, 1.f, 1.f));
		//m_renderer->draw_text("Hello, world!", 0, 0, 7, 300, 200, 12, glm::vec4(1.000000, 1.000000, 1.000000, 1.000000), 1, 1);

		//draw the chart
		{
			m_Candles.PushToGPU();

			
			m_ChartBuffer->Bind();

			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);

			glm::mat4 proj = m_Camera->GetProjection();
			m_renderer->draw_candles(m_Candles.GetCandleCount(), proj);

			m_ChartBuffer->Unbind();
			glm::vec2 s = m_renderer->get_viewport_size();
			glViewport(0, 0, s.x, s.y);
			std::shared_ptr<Engine::gl_shader> shader = Engine::gl_shader::get_shader("imgrect");
			shader->bind();
			shader->set_uniform2f("screenDim", m_renderer->get_viewport_size());

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_Chart->GetHandle());
			
			//WARN("Draw chart rect: " << m_ChartPos.x << ", " << m_ChartPos.y << " | " << m_ChartSize.x << ", " << m_ChartSize.y);
			m_renderer->draw_chart_rect(m_ChartPos, m_ChartSize);

			glBindTexture(GL_TEXTURE_2D, 0);
		}


		//std::cout << std::endl << "End of frame" << std::endl << std::endl;
	}

	lua_State* app::get_lua_state() {
		return m_Lua;
	}

	std::string app::open_file_name(const char* filter) {
		char filename[256];
		ZeroMemory(&filename, sizeof(filename));

		OPENFILENAMEA of{};
		ZeroMemory(&of, sizeof(OPENFILENAMEA));
		of.lStructSize = sizeof(of);
		of.lpstrFilter = filter;
		of.lpstrFile = filename;
		of.nMaxFile = 256;
		of.lpstrTitle = "Select a file";
		of.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&of)) {
			//std::cout << of.lpstrFile << std::endl;
			return std::string(filename);
		}
		else
		{
			WARN("OpenFileNameA error");
			WARN(GetLastError());
		}

		return "";
	}

	void app::ReadData(const std::string& path) {
		using namespace aria::csv;

		std::ifstream f(path);
		CsvParser parser(f);

		m_Candles.Clear();

		for (auto& row : parser) {
			int i = 0;
			Candlestick c{0.f};
			ZeroMemory(&c, sizeof(Candlestick));
			for (auto& field : row) {
				switch (i) {
				case 0:
					c.open = std::stof(field);
					break;
				case 1:
					c.high = std::stof(field);
					break;
				case 2:
					c.low = std::stof(field);
					break;
				case 3:
					c.close = std::stof(field);
					break;
				case 4:
					c.volume = std::stof(field);
				}
				i += 1;
			}
			m_Candles.AddCandle(c);
		}

		m_Candles.PushToGPU();
	}

	glm::vec2 app::GetWindowSize() {
		return glm::vec2(m_window->width(), m_window->height());
	}

	void app::SetPlaybackCandle(int candle) {
		m_Candles.SetPlaybackCandle(candle);
	}
}