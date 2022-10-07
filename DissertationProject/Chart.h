#pragma once
#include "core.h"
#include "src/CandlestickContainer.h"

namespace DissertationProject {
	class Chart
	{
	private:
		static uint8_t s_ChartID;

		uint8_t m_ChartID;
		std::shared_ptr<CandlestickContainer> m_Candles;

		Chart();
	public:
		static std::shared_ptr<Chart> Chart::Create();

		uint8_t Chart::GetID();
	};

}