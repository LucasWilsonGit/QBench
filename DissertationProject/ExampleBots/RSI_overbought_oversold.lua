local core = require("./src/lua_common/core")
Color = core.Color
Event = core.Event
vec2 = core.vec2
UITransform = core.UITransform

local max, min, floor, ceil, abs = math.max, math.min, math.floor, math.ceil, math.abs

function round(x)
	local w = math.floor(x)
	local f = x - w
	return f >= .5 and w + 1 or w
end

local tointeger = math.floor

function Truncate(x,y)
	local mult = math.pow(10, y+1)
	return round(x * mult) / mult
end

local bot = {
	Candles = {},
	State = ""
}
local Log = function() end

function bot:Init(Session)
	Log = Session.Log
	
	self.Session = Session
	self.labelRSI = self.Session:AddLabel("RSI: 0")
	self.labelNet = self.Session:AddLabel("NetPosition: 0")

	Log("Loading RSI14 bot by Lucas Wilson", Color(1,1,1,1))
end

function bot:ClearCandles()
	self.Candles = {}
end

function bot:ResetSession()
	self.Candles = {}
	self.State = ""
end

function bot:Pause()

end

function bot:Resume()

end

function bot:CandleAdded(candle)
	if candle.Volume == 0 then return end 
	--ignore bugged candles (data feeds can misreport etc, unsterilized data sets, so on so forth)
	local last = #self.Candles
	--print("added candle ", last+1)

	local avggain = 0
	local avgloss = 0

	local idx = last + 1
	self.Candles[idx] = candle
	
	if idx == 1 then return end

	local prev = self.Candles[idx - 1]
	candle.Delta = candle.Close - prev.Close
	candle.Gain = max(candle.Delta, 0)
	candle.Loss = math.abs(math.min(0, candle.Delta))
	candle.AvgGain = 0
	candle.AvgLoss = 0

	if idx == 15 then
		local sumgain = 0
		local sumloss = 0
		for i = 0,13 do
			local curr = self.Candles[idx - i] or candle

			sumgain = sumgain + curr.Gain
			sumloss = sumloss + curr.Loss
		end
		candle.AvgGain = sumgain / 14
		candle.AvgLoss = sumloss / 14
		candle.RS = candle.AvgGain / candle.AvgLoss
		candle.RSI = 100 - 100 / (1 + candle.RS)
	elseif idx > 15 then
		local pgain = prev.AvgGain
		local ploss = prev.AvgLoss
		candle.AvgGain = (13 * pgain + candle.Gain) / 14
		candle.AvgLoss = (13 * ploss + candle.Loss) / 14
		candle.RS = candle.AvgGain / candle.AvgLoss
		candle.RSI = 100 - 100 / (1 + candle.RS)
	end

	if candle.RSI then
		if candle.RSI <= 35 and self.State ~= "BUY" then
			local size = math.floor((.1 * self.Session.Liquidity) / self.Session.ActivePrice)
			Log("Try buy " .. tostring(size))
			if size > 0 then
				self.Session.Buy(size)
				self.State = self.Session.NetPosition == 0 and "NULL" or (self.Session.NetPosition > 0 and "BUY" or "SELL")
			end

		elseif candle.RSI >= 65 and self.State == "BUY" then
			local size = max(self.Session.NetPosition, math.floor((.1 * self.Session.Liquidity) / self.Session.ActivePrice))
			Log("long sell " .. tostring(size))
			if size > 0 then
				self.Session.Sell(size)
				self.State = self.Session.NetPosition == 0 and "NULL" or (self.Session.NetPosition > 0 and "BUY" or "SELL")
			end
		elseif candle.RSI >= 75 and self.State ~= "SELL" then
			local size = max(self.Session.NetPosition, math.floor((.03 * self.Session.Liquidity) / self.Session.ActivePrice))
			Log("short sell " .. tostring(size))
			if size > 0 then
				self.Session.Sell(size)
				self.State = self.Session.NetPosition == 0 and "NULL" or (self.Session.NetPosition > 0 and "BUY" or "SELL")
			end
		end
	end

	self.labelNet.Text = "NetPosition: " .. tostring(self.Session.NetPosition)
end

function bot:RemoveCandles(count)
	for i = 1, count do
		table.remove(self.Candles, 1)
		print(#self.Candles)
	end
end

--fires every frame, it's hacky but I can force a UI draw here
function bot:MouseHoverCandle( candle )
	candle = max(candle, 1)

	candle = self.Candles[candle] 
	if candle then
		self.labelRSI.Text = "RSI: " .. Truncate(candle.RSI or 0, 3)
	end
end

return bot