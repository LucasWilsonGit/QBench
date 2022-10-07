#pragma once
#include <vector>
#include "renderer.h"

namespace DissertationProject {

	struct Candlestick {
		float open;
		float high;
		float low;
		float close;
		float volume;
	};

	class CandlestickContainer
	{
	public:
		CandlestickContainer();
		~CandlestickContainer();

		void AddCandle();
		void AddCandle(Candlestick c);
		void AddCandle(float o, float h, float l, float c, float v);
		void UpdateCandle(float price);
		void RemoveCandle();
		void Clear();
		void PushToGPU();
		void SetPlaybackCandle(int candle);
		int GetCandleCount();
		Candlestick GetCandle(int i);

	private:
		std::vector<Candlestick> m_candlesticks;
		float m_lastprice;
		int m_playbackCandle;
	};

}