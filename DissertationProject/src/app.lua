if _G.apploaded then return else _G.apploaded = true end 

local unpack = table.unpack
local floor, max, min = math.floor, math.max, math.min

local core = require("src/lua_common/core")
Color = core.Color
Event = core.Event
vec2 = core.vec2
UITransform = core.UITransform

--creating colors in loops inside of per frame hooks can be expensive, better to have some commonly used colors premade
local white = Color(1,1,1,1)
local black = Color(0,0,0,1)

local UI = require("src/UI")

local startTime = os.clock()

local function tointeger(x)
	return math.floor(x)
end

local function round(x)
	local raw = floor(x)
	local frac = x - floor(x)
	if frac >= .5 then
		return raw + 1
	else
		return raw
	end
end

local _oldtype = type
function type(x)
	return (x and _oldtype(x) == "table" and x.Type) or _oldtype(x)
end
function clamp(x, min, max)
	return math.max(math.min(x, max), min)
end

local function s2dp(s)
	return string.format("%.2f", s)
end

local function _truncate(p)
	local str = ""
	if p >= 1000000 then
		str = s2dp(tostring( tointeger(p / 1000000 * 100) / 100 )) .. "m"
	elseif p >= 100000 then
		str = s2dp(tostring( p / 1000 )) .. "k"
	else
		str = tostring( round(p*100)/100 )
	end
	return str
end

function cpp_onscroll(dX, dY)
	UI.OnScroll:Fire(dX, dY)
end

function cpp_onclick(x, y, button, action)
	UI.mousepos = vec2(x, y) 
	UI.OnClick:Fire(button, action)

	if action == 1 then
		UI.OnMouseDown:Fire(x, y)
	else
		UI.OnMouseUp:Fire(x, y)
	end
end

function cpp_onchar(s)
	UI.OnChar:Fire(s)
end

function cpp_onkeydown(vk)
	UI.OnKeyDown:Fire(vk)
end

function cpp_onkeyup(vk)
	UI.OnKeyUp:Fire(vk)
end 

function cpp_ondraw(width, height, mouseX, mouseY)
	if width ~= UI.width or height ~= UI.height then
		UI.width = width
		UI.height = height
		--print("resize")
	end

	UI.RenderCommandBuffer = {
		Rects = {
		},
		FilledRects = {
		},
		Texts = {
		}
	} --clean render command buffer for frame
	UI.DrawCount = 0
	UI.width = width
	UI.height = height
	local p = UI.mousepos
	UI.mousepos = vec2(mouseX, mouseY)
	local delta = UI.mousepos - p
	UI.OnMouseMove:Fire(delta)

	UI.OnDraw:Fire() --evaluate hooked draw calls etc after setting pre state (clened render command buffer)

	collectgarbage()

	SubmitRenderCommands(UI.RenderCommandBuffer);--submit draw calls
end

local leftpane = UI.GUIBox()
leftpane.Size = UITransform(0.2, 1, 0, -50)
leftpane.Position = UITransform(0,0,0,50)
leftpane.Color = Color(.15, .15, .15, 1)
leftpane.BorderSize = 1
leftpane.BorderColor = Color(.5, .5, .5, 1) --white border

local tradelog = UI.ScrollBox()
tradelog.Parent = leftpane
tradelog.Size = UITransform(1, 1, 0, 0)

--App params
local startTime = os.clock()
local lastcandle = 0 --used for mouse hover candlestick, not for playback
local playbackcandle = 0
local paused = false
local bot = nil

local Session



--forward declare UI elements
local bottompane = UI.GUIBox()
local toppane = UI.GUIBox()
local rightpane = UI.GUIBox()
local liquiditylabel = UI.Label()
local liquidityvaluelabel = UI.Label()
local balancelabel = UI.Label()
local PLlabel = UI.Label()
local PLvaluelabel = UI.Label()
local playbackspeedlabel = UI.Label()
local playbackspeedslider = UI.Slider()
local datasetlabel = UI.Label()
local datasetbutton = UI.TextButton()
local steplabel = UI.Label()
local stepslider = UI.Slider()
local forwardstepbutton = UI.TextButton()
local backwardstepbutton = UI.TextButton()
local resetbutton = UI.TextButton()
local pausebutton = UI.TextButton()
local stopbutton = UI.TextButton()
local initiallabel = UI.Label()
local initialbox = UI.TextBox()
local console = UI.ScrollBox()
local scriptpathlabel = UI.Label()
local scriptpathbutton = UI.TextButton()

function Log(str, col)
	local elem = console:AddText(str)
	if (col) then
		elem.TextColor = col
	else
		elem.TextColor = Color(1,1,1,1)
	end
end

function ResetPlayback() --Rewind the candles
	startTime = os.clock()
	playbackcandle = 0
	SetPlaybackCandle(0)

	Session.NetPosition = 0

	if bot then
		bot:ClearCandles()
	end
end

function RestartSession()
	ResetPlayback()
	--paused = true

	console:Clear()
	tradelog:Clear()

	Session.StartLiquidity = tonumber(initialbox.Text)
	Session.Liquidity = Session.StartLiquidity
	balancelabel.Text = "Balance: $" .. _truncate(Session.Liquidity)

	if bot then
		bot:ResetSession()
	end
end

function Pause()
	paused = true
	pausebutton.Text = "Play"

	Log("Paused playback", Color(1,1,0,1))

	if bot then
		bot:Pause()
	end
end

function Play()
	paused = false
	pausebutton.Text = "Pause"

	if bot then
		bot:Resume()
	end
end

function FakeExcept(str)
	Log(str, Color(1,0,0,1))
	Pause()
end
function Buy(lotsize)
	if Session.Liquidity < lotsize * Session.ActivePrice then
		FakeExcept("Attempt to spend beyond buying power, available liquidity: " .. tostring(Session.Liquidity) .. " attempted to buy " .. tostring(lotsize) .. "@$" .. tostring(round(Session.ActivePrice * 100) / 100))
		return
	end

	Session.NetPosition = Session.NetPosition + lotsize
	Session.Liquidity = Session.Liquidity - Session.ActivePrice * lotsize

	local t = tradelog:AddText("Buy " .. tostring(lotsize) .. "@$" .. tostring(Session.ActivePrice))
	t.TextColor = Color(0,1,0,1)
end
function Sell(lotsize)
	Session.NetPosition = Session.NetPosition - lotsize
	Session.Liquidity = Session.Liquidity + lotsize * Session.ActivePrice

	if Session.NetPosition < 0 then
		Session.ShortPrice = Session.ActivePrice
	end

	local t = tradelog:AddText( (Session.NetPosition < 0 and "Sell short" or "Sell long")  .. " " .. tostring(lotsize) .. "@$" .. tostring(round(Session.ActivePrice*100)/100 ))
	t.TextColor = Color(1,0,0,1)
end

local root = UI.GUIBox()
root.Color = Color(0,0,0,0)
root.Position = UITransform(0,0,0,400)
root.Size = UITransform(1,0,1,-400)
root.Parent = rightpane
Session = {
	Log = Log,
	Buy = Buy,
	Sell = Sell,
	Error = FakeExcept,
	
	NetPosition = 0,
	
	Trades = 0,
	WinTrades = 0,
	LoseTrades = 0,

	Elements = {},
	Root = root,
	Liquidity = tonumber(#initialbox.Text > 0 and initialbox.Text or 10000),
	Balance = tonumber(#initialbox.Text > 0 and initialbox.Text or 10000),
	StartLiquidity = tonumber(#initialbox.Text > 0 and initialbox.Text or 10000),
	ActivePrice = 0,
	ShortPrice = 0
}
function Session:AddLabel(str)
	local e = UI.Label()
	e.Color = Color(0,0,0,0)
	e.TextColor = white
	e.TextSize = 10
	e.Text = str
	e.Parent = root
	e.Position = UITransform(0,0, 10, 20 * #self.Elements)
	e.Size = UITransform(1,0,0,20)
	table.insert(self.Elements, e)
	return e
end

--UI elements

bottompane.Size = UITransform(.6, .2, 0, 0)
bottompane.Position = UITransform(.2, .8, 0, 0)
bottompane.Color = Color(.04, .07, .025, 1)
bottompane.BorderSize = 1
bottompane.BorderColor = Color(.5, .5, .5, 1)

toppane.Size = UITransform(1,0,0,50)
toppane.Position = UITransform(0,0,0,0)
toppane.Color = Color(.15,.15,.15,.15)
toppane.BorderSize = 1
toppane.BorderColor = Color(.5,.5,.5,1)

rightpane.Size = UITransform(.2, 1, 0, -50)
rightpane.Position = UITransform(.8, 0, 0, 50)
rightpane.Color = Color(.15, .15, .15, 1)
rightpane.BorderSize = 1
rightpane.BorderColor = Color(.5, .5, .5, 1)

local w,h = GetTextRect("Liquidity:", 200, 14)
w = tointeger(w)
liquiditylabel.Size = UITransform(0,0,w,16)
liquiditylabel.Position = UITransform(0,.5,10,-10)
liquiditylabel.Color = Color(0,0,0,0)
liquiditylabel.BorderSize = 0
liquiditylabel.TextColor = white
liquiditylabel.TextSize = 14
liquiditylabel.Text = "Liquidity:"
liquiditylabel.Parent = toppane

liquidityvaluelabel.Size = UITransform(0,0,85,16)
liquidityvaluelabel.Position = UITransform(0,.5, 12 + w, -10)
liquidityvaluelabel.Color = Color(0,0,0,0)
liquidityvaluelabel.BorderSize = 0
liquidityvaluelabel.TextColor = white
liquidityvaluelabel.TextSize = 14
liquidityvaluelabel.Text = "$" .. _truncate(Session.Liquidity)
liquidityvaluelabel.Parent = toppane

balancelabel.Size = UITransform(0,0,110,16)
balancelabel.Position = UITransform(0,.5,12 + w + 85,-10)
balancelabel.Color = Color(0,0,0,0)
balancelabel.BorderSize = 0
balancelabel.TextColor = white
balancelabel.TextSize = 14
balancelabel.Text = "Balance: $" .. _truncate(Session.StartLiquidity)
balancelabel.Parent = toppane

PLlabel.Size = UITransform(0,0,30,16)
PLlabel.Position = UITransform(0,.5,12 + w + 90 + 110,-10)
PLlabel.Color = Color(0,0,0,0)
PLlabel.BorderSize = 0
PLlabel.TextColor = white
PLlabel.TextSize = 14
PLlabel.Text = "P/L:"
PLlabel.Parent = toppane

PLvaluelabel.Size = UITransform(0,0,100,16)
PLvaluelabel.Position = UITransform(0,.5,12 + w + 90 + 150,-10)
PLvaluelabel.Color = Color(0,0,0,0)
PLvaluelabel.BorderSize = 0
PLvaluelabel.TextColor = white
PLvaluelabel.TextSize = 14
PLvaluelabel.Text = "$".._truncate(Session.Liquidity - Session.StartLiquidity)
PLvaluelabel.Parent = toppane

playbackspeedlabel.Color = Color(0,0,0,0)
playbackspeedlabel.BorderSize = 0
playbackspeedlabel.TextColor = white
playbackspeedlabel.Text = "Playback speed: 1/s"
playbackspeedlabel.TextSize = 10
playbackspeedlabel.Parent = rightpane
playbackspeedlabel.Size = UITransform(1,-20,0,20)
playbackspeedlabel.Position = UITransform(0,0,10,20)

playbackspeedslider.Min = 0.5
playbackspeedslider.Max = 10
playbackspeedslider.Step = 0.5
playbackspeedslider.Value = 1
playbackspeedslider.Size = UITransform(1,0,-20,20)
playbackspeedslider.Position = UITransform(0,0,10,40)
playbackspeedslider.Parent = rightpane

local working_path = os.getenv("PWD") or io.popen("cd"):read()
print(working_path)
io.popen("cd " .. working_path .. "/../")
print(io.popen("cd"):read())
--local path = PromptOpenData()
local path = ""

datasetlabel.Color = Color(0,0,0,0)
datasetlabel.BorderSize = 0
datasetlabel.Text = "Dataset: " .. path:gsub(working_path, "")
datasetlabel.TextColor = white
datasetlabel.TextSize = 10
datasetlabel.Parent = rightpane
datasetlabel.Size = UITransform(1,0,-20,12)
datasetlabel.Position = UITransform(0, 0, 10, 70)

datasetbutton.Color = Color(.3,.3,.3,1)
datasetbutton.BorderSize = 0
datasetbutton.Text = "Select Dataset"
datasetbutton.TextColor = white
datasetbutton.TextSize = 10
datasetbutton.Parent = rightpane
datasetbutton.Size = UITransform(0,0,150,20)
datasetbutton.Position = UITransform(0, 0, 10, 90)
datasetbutton:Hook(datasetbutton.Pressed, function()
	datasetbutton.Color = Color(.1,.1,.1,1)
	path = PromptOpenData()
	datasetlabel.Text = "Dataset: " .. path:gsub(working_path, "")
	
	ResetPlayback()
end)

steplabel.Color = Color(0,0,0,0)
steplabel.BorderSize = 0
steplabel.Text = "Step Candles: 1"
steplabel.TextColor = white
steplabel.TextSize = 10
steplabel.Parent = rightpane
steplabel.Size = UITransform(1,0,-20,20)
steplabel.Position = UITransform(0,0,10,120)

stepslider.Min = 1
stepslider.Max = 20
stepslider.Step = 1
stepslider.Value = 1
stepslider.Size = UITransform(1,0,-20,20)
stepslider.Position = UITransform(0,0,10,140)
stepslider.Parent = rightpane

forwardstepbutton.Color = Color(.3,.3,.3,1)
forwardstepbutton.BorderSize = 0
forwardstepbutton.Text = "Step Forwards"
forwardstepbutton.TextColor = white
forwardstepbutton.TextSize = 10
forwardstepbutton.Parent = rightpane
forwardstepbutton.Size = UITransform(0,0,150,20)
forwardstepbutton.Position = UITransform(0, 0, 10, 165)
forwardstepbutton:Hook(forwardstepbutton.Pressed, function()
	forwardstepbutton.Color = Color(.1,.1,.1,1)

	if bot then
		print(playbackcandle, playbackcandle+stepslider.Value)
		for i = playbackcandle+1, playbackcandle + stepslider.Value do
			local o,h,l,c,v = GetCandleData(tointeger(i))
			Session.ActivePrice = c
			bot:CandleAdded({Open = o, High = h, Low = l, Close = c, Volume = v})
		end
	end
	
	playbackcandle = playbackcandle + stepslider.Value

	SetPlaybackCandle(playbackcandle)
end)

backwardstepbutton.Color = Color(.3,.3,.3,1)
backwardstepbutton.BorderSize = 0
backwardstepbutton.Text = "Step Backwards"
backwardstepbutton.TextColor = white
backwardstepbutton.TextSize = 10
backwardstepbutton.Parent = rightpane
backwardstepbutton.Size = UITransform(0,0,150,20)
backwardstepbutton.Position = UITransform(0, 0, 10, 190)
backwardstepbutton:Hook(backwardstepbutton.Pressed, function()
	backwardstepbutton.Color = Color(.1,.1,.1,1)
	
	playbackcandle = max(playbackcandle - stepslider.Value, 0)
	SetPlaybackCandle(playbackcandle)

	if bot then
		bot:RemoveCandles(stepslider.Value)
	end
end)
--important that I should store candles CandleID (int) when making trades so I can rewind trades 

resetbutton.Color = Color(.3,.3,.3,1)
resetbutton.BorderSize = 0
resetbutton.Text = "Reset"
resetbutton.TextColor = white
resetbutton.TextSize = 14
resetbutton.Parent = toppane
resetbutton.Size = UITransform(0,0,70,20)
local o = -230
resetbutton.Position = UITransform(1, .5, o, -10)
resetbutton:Hook(resetbutton.Pressed, function()
	resetbutton.Color = Color(.1,.1,.1,1)

	RestartSession()
end)

pausebutton.Color = Color(.3,.3,.3,1)
pausebutton.BorderSize = 0
pausebutton.Text = "Pause"
pausebutton.TextColor = white
pausebutton.TextSize = 14
pausebutton.Parent = toppane
pausebutton.Size = UITransform(0,0,70,20)
o = o + resetbutton.RealSize.x + 5
pausebutton.Position = UITransform(1, .5, o, -10)
pausebutton:Hook(pausebutton.Pressed, function()
	pausebutton.Color = Color(.1,.1,.1,1)
	
	if paused then Play() else Pause() end
end)

stopbutton.Color = Color(.3,.3,.3,1)
stopbutton.BorderSize = 0
stopbutton.Text = "Stop"
stopbutton.TextColor = white
stopbutton.TextSize = 14
stopbutton.Parent = toppane
stopbutton.Size = UITransform(0,0,70,20)
o = o + pausebutton.RealSize.x + 5
stopbutton.Position = UITransform(1, .5, o, -10)
stopbutton:Hook(stopbutton.Pressed, function()
	stopbutton.Color = Color(.1,.1,.1,1)
	
	RestartSession()
	Pause()
end)

initiallabel.Color = Color(0,0,0,0)
initiallabel.TextColor = white
initiallabel.BorderSize = 0
initiallabel.Text = "Initial liquidity: "
initiallabel.TextSize = 10
initiallabel.Parent = rightpane
initiallabel.Size = UITransform(0,0,200,20)
initiallabel.Position = UITransform(0,0,10,215)

initialbox.Color = Color(.7,.7,.7,1)
initialbox.BorderSize = 1
initialbox.BorderColor = Color(0,0,0,1)
initialbox.Text = tostring(Session.StartLiquidity)
initialbox.TextColor = Color(0,0,0,1)
initialbox.TextSize = 10
initialbox.Parent = rightpane
initialbox.Size = UITransform(0,0,200,20)
initialbox.Position = UITransform(0,0,10,240)

scriptpathlabel.Color = Color(0,0,0,0)
scriptpathlabel.BorderSize = 0
scriptpathlabel.Text = "Script: "
scriptpathlabel.TextSize = 10
scriptpathlabel.TextColor = white
scriptpathlabel.Parent = rightpane
scriptpathlabel.Size = UITransform(0,0,200,20)
scriptpathlabel.Position = UITransform(0,0,10,275)

scriptpathbutton.Color = Color(.3,.3,.3,1)
scriptpathbutton.BorderSize = 0
scriptpathbutton.Text = "Select"
scriptpathbutton.TextSize = 10
scriptpathbutton.TextColor = white
scriptpathbutton.Parent = rightpane
scriptpathbutton.Size = UITransform(0,0,200,20)
scriptpathbutton.Position = UITransform(0,0,10,290)



scriptpathbutton:Hook(scriptpathbutton.Pressed, function()
	local cwd = os.getenv("PWD") or io.popen("cd"):read() .. "\\"
	local p = PromptOpenScript()
	
	if #p > 0 then
		bot = require(p:gsub(cwd, ""):gsub(".lua", ""))
	else
		bot = nil
	end
	scriptpathlabel.Text = "Script: " .. p:gsub(cwd, "")

	bot:Init(Session)

	for i=1, playbackcandle do
		local o,h,l,c,v = GetCandleData(tointeger(i))
		Session.ActivePrice = c
		bot:CandleAdded({Open = o, High = h, Low = l, Close = c, Volume = v})
	end
end)

UI.OnMouseUp:Connect(function()
	datasetbutton.Color = Color(.3,.3,.3,1)
	forwardstepbutton.Color = Color(.3,.3,.3,1)
	backwardstepbutton.Color = Color(.3,.3,.3,1)
	resetbutton.Color = Color(.3,.3,.3,1)
	pausebutton.Color = Color(.3,.3,.3,1)
	stopbutton.Color = Color(.3,.3,.3,1)
	scriptpathbutton.Color = Color(.3,.3,.3,1)
end)

console.Parent = bottompane
console.Size = UITransform(1,1,0,0)

local dragyscale = false
local dragchart = false
local ChartBack = UI.TextButton()
ChartBack.Size = UITransform(.6, .8, 0, -50)
ChartBack.Position = UITransform(.2, 0, 0, 50)
ChartBack.Color = Color(.05, .05, .05, 1)
ChartBack.Text = " "
ChartBack.Pressed:Connect(function(pX, pY)
	if (ChartBack.RealSize.x - pX < 30) then
		dragyscale = true
	else
		dragchart = true
	end
end)
UI.OnMouseUp:Connect(function()
	dragchart = false
	dragyscale = false
end)
local chartcampos = vec2(0,100)
local basesize = vec2(1200, 70)
local chartcamsize = basesize + vec2(0,0)
UI.OnMouseMove:Connect(function(delta)
	if dragchart then
		delta = delta * chartcamsize / basesize * vec2(1,-.1)
		chartcampos = chartcampos - delta
		SetChartCam(chartcampos.x, chartcampos.y, chartcamsize.x, chartcamsize.y)
	elseif dragyscale then
		chartcamsize = chartcamsize * vec2(1, 1 + delta.y * 2 / ChartBack.RealSize.y) 
		SetChartCam(chartcampos.x, chartcampos.y, chartcamsize.x, chartcamsize.y)
	end
end)
UI.OnScroll:Connect(function(dX, dY)
	if not ChartBack:Contains(UI.mousepos) then return end

	chartcamsize = chartcamsize - vec2(dY, 0) * 10 * chartcamsize.x / 800
	SetChartCam(chartcampos.x, chartcampos.y, chartcamsize.x, chartcamsize.y)
end)

UI.OnDraw:Connect(function()

	if tointeger((os.clock() - startTime) * playbackspeedslider.Value ) > 1 and not paused then
		playbackcandle = playbackcandle + 1
		SetPlaybackCandle(playbackcandle)
		if bot then
			local o,h,l,c,v = GetCandleData(tointeger(playbackcandle))
			Session.ActivePrice = c
			bot:CandleAdded({Open = o, High = h, Low = l, Close = c, Volume = v})
		end
		startTime = os.clock()
	elseif paused then
		startTime = os.clock()
	end

	local candle = tointeger(((UI.mousepos.x - ChartBack.RealPosition.x) / ChartBack.RealSize.x * chartcamsize.x + chartcampos.x + 10) / 20)
	UI.Candle = candle
	UI.ActiveCandle = {}
	if (candle ~= lastcandle) then
		local o,h,l,c,v = GetCandleData(candle)
		lastcandle = Candle
		UI.ActiveCandle = {Open = o, High = h, Low = l, Close = c, Volume = v}
		if bot then bot:MouseHoverCandle(candle) end
	end
	
	local relY = (ChartBack.RealPosition.y - UI.mousepos.y) / ChartBack.RealSize.y * chartcamsize.y + chartcampos.y * 2
	UI.MousePrice = round((relY * 100)/ 100)

	--based on the chart zoom, change the price label step
	local stepsize = 10
	if chartcamsize.y < 180 then
		stepsize = 10
	elseif chartcamsize.y < 1000 then
		stepsize = 50
	elseif chartcamsize.y < 2000 then
		stepsize = 100
	elseif chartcamsize.y < 5000 then
		stepsize = 250
	elseif chartcamsize.y < 7500 then
		stepsize = 500
	else
		stepsize = 2000
	end

	--draw the chart price labels
	for i = 0, 20 do
		local p = round((floor(chartcampos.y / stepsize) - 20 / stepsize + i) * stepsize + 20)

		local sY = ChartBack.RealSize.y
		local x = ChartBack.RealPosition.x + ChartBack.RealSize.x
		local y = ChartBack.RealPosition.y + sY 

		local Y =  (p - chartcampos.y) / chartcamsize.y

		local eY = y - Y * sY

		if eY > ChartBack.RealPosition.y and eY < ChartBack.RealPosition.y + ChartBack.RealSize.y - 10 then
			local t  = tostring(p)
			local w,h = GetTextRect(t, 200, 10)
			UI:DrawText(p, x - w, eY, 12, 200, 10, white, false, false)
		end
	end

	local vol = tointeger(UI.ActiveCandle.Volume) --removes the nasty .0 from the float
	local volstr = _truncate(vol)
		
	UI:DrawText("Volume: " .. volstr, ChartBack.RealPosition.x + 20, ChartBack.RealPosition.y, 200, 12, 10, white, false, false)

	local mY = clamp(UI.mousepos.y, ChartBack.RealPosition.y, ChartBack.RealPosition.y + ChartBack.RealSize.y)
	local cY = 1 - (mY - ChartBack.RealPosition.y) / ChartBack.RealSize.y
	local vY = tointeger((chartcampos.y + chartcamsize.y * cY) * 100) / 100
	local vStr = string.format("%.2f", tostring(vY))
	local w,h = GetTextRect(vStr, 100, 10)
	UI:DrawText(tostring(vY), ChartBack.RealPosition.x + ChartBack.RealSize.x - 40, mY, 200, 12, 10, white, false, false)

	SetChartRect(ChartBack.RealPosition.x, ChartBack.RealPosition.y, tointeger(ChartBack.RealSize.x), tointeger(ChartBack.RealSize.y))

	--rightpane:Draw()
	--liquiditylabel:Draw()
	Session.Balance = Session.Liquidity + Session.ActivePrice * Session.NetPosition
	liquidityvaluelabel.Text = "$" .. _truncate(Session.Liquidity + math.min(0, Session.NetPosition) * Session.ActivePrice)
	local plcolor = Session.Balance > Session.StartLiquidity and Color(0,1,0,1) or Color(1,0,0,1)
	liquidityvaluelabel.TextColor = plcolor
	PLvaluelabel.TextColor = plcolor

	playbackspeedlabel.Text = "Playback speed: " .. tostring(playbackspeedslider.Value) .. "/s" .. (paused and " (PAUSED)" or "")
	steplabel.Text = "Step Candles: " .. tostring(stepslider.Value)
	PLvaluelabel.Text = "$".._truncate(Session.Balance - Session.StartLiquidity)
	balancelabel.Text = "$" .. _truncate(Session.Balance)
end)

