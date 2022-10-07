#pragma once
#include "Engine/Src/Engine.h"
#include "CandlestickContainer.h"
#include "CsvParser.h"
#include "Texture.h"
#include "Camera.h"
#include "Framebuffer.h"

namespace DissertationProject {

	class app : public Engine::application
	{
	private:
		CandlestickContainer m_Candles;
		std::shared_ptr<CTexture> m_Chart;
		std::shared_ptr<Camera> m_Camera;
		std::shared_ptr<Framebuffer> m_ChartBuffer;

	public:
		app(int width = 1200, int height = 900);
		~app();

		bool m_toexit;
		glm::vec2 m_ChartPos;
		glm::vec2 m_ChartSize;

		void on_update() override;
		static int lua_SubmitRenderCommands(lua_State* L);
		void exit();
		lua_State* get_lua_state();
		static std::string open_file_name(const char* filter);
		void ReadData(const std::string& path);
		
		static app* s_App;

		glm::vec2 GetWindowSize();
		void SetPlaybackCandle(int candle);
		void SetChartViewSize(const glm::vec2& size);
		void SetChartViewPos(const glm::vec2& pos);
		Candlestick GetCandleData(int i);
	};

}

