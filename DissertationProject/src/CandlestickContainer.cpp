#include "CandlestickContainer.h"
#include "renderer.h"

int min(int a, int b) {
	return (a < b) ? a : b;
}

namespace DissertationProject {

	CandlestickContainer::CandlestickContainer() : m_candlesticks(std::vector<Candlestick>()), m_lastprice(0.f), m_playbackCandle(10000) {}
	CandlestickContainer::~CandlestickContainer() {}

	void CandlestickContainer::AddCandle() {
		m_candlesticks.push_back(Candlestick{ m_lastprice, m_lastprice + .01f, m_lastprice, m_lastprice + .01f });
	}

	void CandlestickContainer::AddCandle(Candlestick c) {
		m_lastprice = c.close;
		//Close should be the last tick of the candle but not sure if this is really an intuitive behavior / design decision.
		m_candlesticks.push_back(c);
	}

	void CandlestickContainer::AddCandle(float o, float h, float l, float c, float v) {
		m_lastprice = c;

		m_candlesticks.push_back({ o,h,l,c,v });
	}

	void CandlestickContainer::UpdateCandle(float price) {
		if (m_candlesticks.size() > 0) {
			Candlestick& c = *(m_candlesticks.end()--);
			c.close = price;
			if (price > c.high) {
				c.high = price;
			}
			else if (price < c.low) {
				c.low = price;
			}
		}
	}

	void CandlestickContainer::RemoveCandle() {
		if (m_candlesticks.size() > 0)
			m_candlesticks.pop_back();
	}

	void CandlestickContainer::Clear() {
		m_candlesticks.clear();
	}

	void CandlestickContainer::PushToGPU() {
		int count = GetCandleCount();
		if (m_candlesticks.size() > 0) {

			Engine::renderer::get_instance()->write_candle_gpu_data(&m_candlesticks[0], count * sizeof(Candlestick));
			//writes the data up to the playback position to the GPU buffer for the instanced draw, does not overflow the candlestick data (avoids reading garbage or exceeding gpu buffer)
		}
	}

	void CandlestickContainer::SetPlaybackCandle(int candle) {
		m_playbackCandle = std::max(0, candle); //causes exceptions if not filtered! 
	}

	//returns the least of the playback position candle, vs the total candles in the dataset
	int CandlestickContainer::GetCandleCount() {
		return min(m_playbackCandle, m_candlesticks.size());
	}

	Candlestick CandlestickContainer::GetCandle(int i) {
		
		if (m_candlesticks.size() > i) {
			Candlestick r = m_candlesticks[i];
			return r;
		}
		
		return Candlestick{ 0,0,0,0,0 };
	}
}