local unpack = table.unpack
local floor, max, min = math.floor, math.max, math.min

local core = require("src/lua_common/core")
Color = core.Color
Event = core.Event
vec2 = core.vec2
UITransform = core.UITransform

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

local UI = {
	RenderCommandBuffer = {
		Rects = {
			{0, 0, 100, 100, 0xff0000ff, 1},
			{50, 50, 100, 100, 0x0000ffff, 2}
		},
		FilledRects = {
		},
		Texts = {
		}
	},
	DrawCount = 0,
	
	--from cpp events
	OnDraw = Event(),
	OnScroll = Event(),
	OnClick = Event(),
	OnMouseMove = Event(),
	OnChar = Event(),

	--tier 2 events aka lua passed and defined
	OnMouseDown = Event(),
	OnMouseUp = Event(),
	OnKeyDown = Event(),
	OnKeyUp = Event(),

	width = 800,
	height = 600,
	mousepos = vec2(0,0)
}

local function tointeger(x)
	return math.floor(x)
end

function UI:DrawRect(x,y,width,height,color,border)
	x = tointeger(x)
	y = tointeger(y)
	width = tointeger(width)
	height = tointeger(height)
	border = tointeger(border)
	

	if type(color) ~= "Color" then
		error("UI:DrawRect:: Expected arg 4 to be 'Color' object got " .. type(color))
	end

	local rects = self.RenderCommandBuffer.Rects
	rects[#rects+1] = {x,y,width,height,color,self.DrawCount + 1,border or 1}
	self.DrawCount = self.DrawCount + 1
end

function UI:DrawFilledRect(x,y,width,height,color)
	x = tointeger(x)
	y = tointeger(y)
	width = tointeger(width)
	height = tointeger(height)

	local frects = self.RenderCommandBuffer.FilledRects
	frects[#frects+1] = {x,y,self.DrawCount + 1,width,height,color}
	self.DrawCount = self.DrawCount + 1
end

function UI:DrawText(str,x,y,width,height,size,color,wrap,stencil)
	if str == "" then return end

	x = tointeger(x)
	y = tointeger(y)
	width = tointeger(width)
	height = tointeger(height)
	size = tointeger(size)

	local texts = self.RenderCommandBuffer.Texts
	texts[#texts+1] = {str,x,y,self.DrawCount + 5,width,height,size,color,wrap,stencil}
	self.DrawCount = self.DrawCount + 1
end

local CoreGUI = {
	Children = {}
}
setmetatable(CoreGUI.Children, {__mode="kv"}) --weakref to avoid holding leaked UI elements 

local GUIBox = {
	Type = "GUIBox"
}
local GUIMeta = {
	__index = function(_, k)
		
		local props = rawget(_, "props")
		if props[k] then
			return props[k]
		else
			local v = nil
			local types = rawget(_, "types")
			for i=1, #types do
				v = types[i][k]
				if v then break end 
			end
			return v
		end
		
	end,
	__newindex = function(_, k, v)
		local props = _.props
		local hooks = _.prophooks[k]
		local val = props[k]
		if hooks then
			for i=1, #hooks do
				hooks[i](val, v)
			end
		end
		props[k] = v
	end,
	__gc = function(_)
		for i,v in pairs(_.eventhooks) do
			v:Disconnect()
		end
	end
	--clean up hooks on garbage collection of object, essentially a destructor
}
setmetatable(GUIBox, {
	__call = function(_, parent)
		local self = {}

		if type(parent) == "GUIBox" then 
			self.Parent = parent
		else
			parent = {
				RealPos = vec2(),
				RealSize = vec2(UI.width, UI.height)
			}
		end

		local props = {
			Position = UITransform(0, 0, 0, 0),
			Size = UITransform(0, 0, 100, 100),
			Color = Color(.7,.7,.7,1.),
			BorderSize = 0,
			BorderColor = Color(1, 1, 1, 1),
			Children = {},
			Parent = nil,
			RealPosition = vec2(),
			RealSize = vec2(),
		}

		local prophooks = {
			Position = {
				function(old, new)
					local parent = props.Parent or {
						RealPosition = vec2()
					}
					props.RealPosition = props.Position.Scale * parent.RealPosition + props.Position.Offset
				end
			},
			Size = {
				function(old, new)
					local parent = props.Parent or {
						RealSize = vec2(UI.width, UI.height)
					}
					props.RealSize = new.Scale * parent.RealSize + new.Offset
				end
			},
			Parent = {
				function(old, new)
					print("PARENT " .. tostring(old) .. " : " .. tostring(new))
					if new == nil then
						new = CoreGUI
					else
						CoreGUI.Children[self] = nil
					end

					props.Parent = new
					if old then
						old.Children[self] = nil
					end
					new.Children = new.Children or {}
					new.Children[self] = self

					if new.ChildAdded then
						new.ChildAdded:Fire(self)
					end
					
					
					props.RealPosition = props.Position.Scale * new.RealPosition + props.Position.Offset
					props.RealSize = props.Size.Scale * new.RealSize + props.Size.Offset
				end
			}

		}

		self.props = props
		self.prophooks = prophooks
		self.eventhooks = {}
		self.types = {GUIBox}
		self.ChildAdded = Event()

		CoreGUI.Children[self] = self

		return setmetatable(self, GUIMeta)
	end,
})
function GUIBox:Draw(_drawchildren)
	local _drawchildren = _drawchildren == nil and true or false

	self:RecalculateBounds()

	--draw self
	--print("Draw GUIBox")

	if self.BorderSize > 0 then
		local p1 = self.RealPosition - vec2(math.ceil(self.BorderSize/2))
		local p2 =  self.RealSize + vec2(math.ceil(self.BorderSize))
		UI:DrawRect(p1.x, p1.y, p2.x, p2.y, self.BorderColor, math.ceil(self.BorderSize)+1)
	end
	UI:DrawFilledRect(self.RealPosition.x, self.RealPosition.y, self.RealSize.x, self.RealSize.y, self.Color)

	--draw children
	if _drawchildren then
		for i, v in pairs(self.Children) do
			v:Draw()
		end
	end
end
function GUIBox:Hook(event, func)
	local hhk =  event:Connect(func)
	table.insert(self.eventhooks, hhk)
	return hhk
end
function GUIBox:Unhook(hhk)
	for i,v in pairs(self.eventhooks) do
		if v == hhk then
			table.remove(self.eventhooks, i)
			hhk:Disconnect()
			return
		end
	end
	error("UI.lua:255: Attempt to unhook disconnected hook")
end
function GUIBox:GetChildren()
	local kids = {}
	for i,v in pairs(self.Children) do
		kids[#kids+1] = v
	end
	return kids
end
function GUIBox:Contains(vec)
	if vec.x >= self.RealPosition.x and vec.x <= self.RealPosition.x + self.RealSize.x and vec.y >= self.RealPosition.y and vec.y <= self.RealPosition.y + self.RealSize.y then
		return true
	end
	return false
end
function GUIBox:RecalculateBounds()
	local parent = self.Parent or {
		RealSize = vec2(UI.width, UI.height),
		RealPosition = vec2(0,0)
	}

	self.RealPosition = parent.RealPosition + self.Position.Scale * parent.RealSize + self.Position.Offset
	self.RealSize = self.Size.Scale * parent.RealSize + self.Size.Offset

	--[[for i,v in pairs(self.Children) do
		v:RecalculateBounds()
	end]]
end





local Text = {}
setmetatable(Text, {
	__call = function(_)
		local self = {}
		self.TextRect = vec2(0,0)
		self.Text = ""
		self.TextColor = Color(0,0,0,0)
		self.TextSize = 12
		self.MaxWidth = 100
		self.Wrapped = true
		self.Scissor = true
		return setmetatable(self, {__index = Text})
	end
})
function Text:CalcSize()
	local str = self.Text
	self.TextRect = vec2(GetTextRect(str, self.MaxWidth, self.TextSize))
	self.TextRect.y = math.max(self.TextRect.y, self.TextSize)
end
function Text:SetText(str)
	self.TextRect = vec2(GetTextRect(str, self.MaxWidth, self.TextSize))
	self.TextRect.y = math.max(self.TextRect.y, self.TextSize)
	self.Text = str
end

local ScrollBox = {}
setmetatable(ScrollBox, {
	__call = function(_)
		local self = GUIBox()

		self.RealClientArea = vec2(0,0)
		self.ScrollPos = 0
		self.types = {ScrollBox, GUIBox}
		self.StackDirection = 1 --TOPDOWN = 1 BOTTOMUP = 2
		self.Color = Color(0,0,0,0)
		
		self.CursorScrollStatus = false
		self.CursorScrollMoveHook = nil


		self:Hook(UI.OnScroll, function(dX, dY)
			if not self:Contains(UI.mousepos) then return end

			self.ScrollPos = clamp(self.ScrollPos + dY * (50 / self.RealClientArea.y), 0, 1)
		end)

		self:Hook(UI.OnMouseDown, function(button)
			if button ~= 0 then return end
			if not self:Contains(UI.mousepos) then return end 

			if UI.mousepos.x < self.RealPosition.x + self.RealSize.x - 10 then return end

			if self.CursorScrollMoveHook ~= nil then
				self.CursorScrollMoveHook:Disconnect()
				self.CursorScrollMoveHook = nil
			end

			self.CursorScrollMoveHook = UI.OnMouseMove:Connect(function(delta)
				self.ScrollPos = clamp(self.ScrollPos - delta.y / self.RealSize.y, 0, 1)
			end)
			self.CursorScrollStatus = true
		end)

		self:Hook(UI.OnMouseUp, function(button)
			if button ~= 0 then return end

			if self.CursorScrollStatus then
				self.CursorScrollStatus = false
				self.CursorScrollMoveHook:Disconnect()
				self.CursorScrollMoveHook = nil
			end
		end)

		self:Hook(self.ChildAdded, function(ui_instance)
			local totalheight = 0
			for i,v in pairs(self.Children) do
				totalheight = totalheight + v.RealSize.y
			end
			self.RealClientArea = vec2(self.RealSize.x, totalheight)
		end)
		
		return setmetatable(self, GUIMeta)
	end,
	__index = GUIBox
})
function ScrollBox:Draw()
	--print(self.ScrollPos)
	GUIBox.Draw(self, false)

	local accumpos = 0
	local children = self:GetChildren()
	table.sort(children, function(a,b) return (a.Order or 0) < (b.Order or 0) end)

	local diff = self.RealSize.y - self.RealClientArea.y

	local mX, mY = self.RealPosition.x, self.RealPosition.y

	for i, v in pairs(children) do
		local pY = mY + accumpos + diff * (1 -self.ScrollPos)
		local pX = mX
		if pY > mY and pY < UI.height then 
			local posY = pY
			--v:CalcSize()
			UI:DrawText(v.Text, pX, pY, self.RealSize.x, v.TextRect.y, v.TextSize, v.TextColor, v.Wrapped, v.Scissor)
		end
		
		accumpos = accumpos + v.TextRect.y
		
	end

	local boxheight =  self.RealSize.y / max(self.RealClientArea.y, self.RealSize.y) 
	local boxheightpx = self.RealSize.y * boxheight
	
	local p = UITransform(1, (1 - self.ScrollPos), -10, 0)
	local s = UITransform(0, boxheight, 10, 0)

	p = self.RealPosition + self.RealSize * p.Scale + p.Offset
	s = self.RealSize * s.Scale + s.Offset

	p.y = (1 - self.ScrollPos) * (self.RealSize.y - boxheightpx) + self.RealPosition.y

	UI:DrawFilledRect(p.x, p.y, s.x, s.y, Color(.5,.5,.5,1))
end
function ScrollBox:AddText(str)
	local elem = Text()
	elem.MaxWidth = self.RealSize.x
	elem.TextRect.x = self.RealSize.x
	elem.Parent = self
	self.Children[#self.Children+1] = elem
	elem:SetText(str)
	elem.Order = #self.Children
	elem.RealSize = elem.TextRect
	self.ChildAdded:Fire(elem)

	if elem.Order > 500 then
		local elem = self.Children[1]
		table.remove(self.Children, 1)
		for i, v in pairs(self.Children) do
			v.Order = v.Order - 1
		end
	end

	return elem
end
function ScrollBox:Clear()
	self.Children = {}
	self.ScrollPos = 0
end

local TextButtons = {}
setmetatable(TextButtons, {__mode="v"})
--weakref table of 

local TextButton = {}
setmetatable(TextButton, {
	__call = function(_)
		local self = GUIBox()

		local id = #TextButtons+1
		self.id = id
		TextButtons[id] = self

		self.Text = ""
		self.TextColor = Color(0,0,0,1)
		self.TextSize = 16

		self.Pressed = Event()
		self.Released = Event()

		self.types = {TextButton, GUIBox}
		return setmetatable(self, GUIMeta)
	end,
	__index = GUIBox
})
function TextButton:Draw()
	GUIBox.Draw(self)

	local w,h = GetTextRect(self.Text, self.RealSize.x, self.TextSize)
	UI:DrawText(self.Text, self.RealPosition.x + (self.RealSize.x - w) / 2, self.RealPosition.y, self.RealSize.x, self.RealSize.y, self.TextSize, self.TextColor, false, false)
end
UI.OnMouseDown:Connect(function(pX, pY)
	local b = nil
	local p = vec2(0,0)
	for i,v in pairs(TextButtons) do
		if v:Contains(UI.mousepos) then
			b = v
			p = v.RealPosition
		end
	end
	if b then
		b.Pressed:Fire(pX - p.x, pY - p.y)
	end
end)
UI.OnMouseUp:Connect(function(pX, pY)
	local b = nil
	local p = vec2(0,0)
	for i,v in pairs(TextButtons) do
		if v:Contains(UI.mousepos) then
			b = v
			p = v.RealPosition
		end
	end
	if b then
		b.Released:Fire(pX - p.x, pY - p.y)
	end
end)

local Label = {}
setmetatable(Label, {
	__call = function(_)
		local self = GUIBox()

		self.Text = " "
		self.TextColor = Color(1,1,1,1)
		self.TextSize = 16

		self.types = {Label, GUIBox}
		return setmetatable(self, GUIMeta)
	end,
	__index = GUIBox
})
function Label:Draw()
	GUIBox.Draw(self)

	UI:DrawText(self.Text, self.RealPosition.x, self.RealPosition.y, self.RealSize.x, self.RealSize.y, self.TextSize, self.TextColor, true, true)
end

local Sliders = {}
local Slider = {}
setmetatable(Slider, {
	__call = function(_)
		local self = TextButton()
		self.Text = " "
		self.Size = UITransform(0,0,100,20)
		self.Position = UITransform(0,0,0,0)
		self.Color = Color(0,0,0,0)

		local bar = GUIBox()
		bar.Size = UITransform(1, 0, 0, 6)
		bar.Position = UITransform(0, .5, 0, -3)
		bar.Color = Color(1,1,1,1)
		bar.BorderColor = Color(0,0,0,0)
		bar.BorderSize = 0

		local tab = GUIBox()
		tab.Size = UITransform(0, 0, 5, 16)
		tab.Position = UITransform(0, .5, 0, -8)
		tab.Color = Color(1,1,1,1)
		tab.BorderColor = Color(.5,.5,.5,1)
		tab.BorderSize = 1

		self.Bar = bar
		self.Tab = tab
		bar.Parent = self
		tab.Parent = bar

		self.Value = 0
		self.Min = 0
		self.Max = 100
		self.Step = 1
		self.Dragging = false

		self.types = {Slider, TextButton, GUIBox}
		self = setmetatable(self, GUIMeta)

		self:Hook(self.Pressed, function()
			print("pressed")
			self.Dragging = true
		end)

		self:Hook(UI.OnMouseUp, function()
			self.Dragging = false
		end)

		table.insert(Sliders, self)
		return self
	end,
	__index = TextButton
})
function Slider:Draw()
	if self.Dragging then
		local dX = UI.mousepos.x - self.RealPosition.x
		local f = dX / self.Bar.RealSize.x
		self.Value = clamp(self.Min + round((self.Max - self.Min) * f / self.Step) * self.Step, self.Min, self.Max)
		f = (self.Value - self.Min) / (self.Max - self.Min)
		self.Tab.Position = UITransform(f, .5, -3, -8)
	end

	TextButton.Draw(self)
	self.Bar:Draw()
	self.Tab:Draw()
end

local TextBox = {}
setmetatable(TextBox, {
	__call = function(_)
		local self = TextButton()

		self.Text = ""
		self.TextSize = 16
		self.TextColor = Color(1,1,1,1)

		self.CharHook = nil

		self:Hook(self.Pressed, function()
			if self.CharHook then return end 
			self.CharHook = UI.OnChar:Connect(function(char)
				self.Text = self.Text .. char
			end)
		end)
		
		self:Hook(UI.OnMouseDown, function()
			if not self:Contains(UI.mousepos) and self.CharHook then
				self.CharHook:Disconnect() --unhook from text input
				self.CharHook = nil
			end
		end)
		
		self:Hook(UI.OnKeyDown, function(key)
			if self.CharHook and key == 259 then
				self.Text = self.Text:sub(1, -2)
			end
		end)

		self.types = {TextBox, TextButton, GUIBox}
		setmetatable(self, GUIMeta)

		return self
	end
})
function TextBox:Draw()
	GUIBox.Draw(self)

	UI:DrawText(self.Text, self.RealPosition.x, self.RealPosition.y, self.RealSize.x, self.RealSize.y, self.TextSize, self.TextColor, true, true)
end

local fps = Label()
fps.Size = UITransform(0, 0, 100, 30)
fps.Position = UITransform(1,1,-100,-30)
fps.Color = Color(0,0,0,1)
fps.BorderSize = 1
fps.BorderColor = Color(0,0,0,1)
fps.TextColor = Color(1,1,1,1)

local ticks = {}

UI.OnDraw:Connect(function()

	local total = 0
	for i,v in pairs(ticks) do
		if v < os.clock() - 1 then
			table.remove(ticks, i)
		else
			total = total + 1
		end
	end
	table.insert(ticks, os.clock())

	fps.Text = tostring( tointeger(1 / (os.clock() - (ticks[#ticks-1] or 0))) )
	--debug application performance

	for i,v in pairs(CoreGUI.Children) do
		v:Draw()
	end

	for i,v in pairs(TextButtons) do
		v:Draw()
	end

	for i,v in pairs(Sliders) do
		v:Draw()
	end
	
end)

UI.GUIBox = GUIBox
UI.Label = Label
UI.ScrollBox = ScrollBox
UI.TextButton = TextButton
UI.Slider = Slider
UI.TextBox = TextBox

return UI