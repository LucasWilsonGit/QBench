local unpack = table.unpack

local olderror = error
error = function(s)
	olderror(debug.traceback() .. "\n" .. s)
end

local function bitshiftL(x, y)
	return x * math.pow(2, y)
end
local function bitshiftR(x, y)
	return math.floor(x * math.pow(2, -y)) --discard extra bits
end
local function byteshiftL(x, y)
	return bitshiftL(x, y*8)
end
local function byteshiftR(x, y)
	return bitshiftR(x, y*8)
end

local _oldtype = type
function type(x)
	return (x and _oldtype(x) == "table" and x.Type) or _oldtype(x)
end

local Object = {
	Type = "UI.lua::Object"
}
setmetatable(Object, {
	__call = function(_, ...)
		error("Attempt to illegally instance class 'Object'")
	end
})

local Event = {
	Type = "Event"
}
setmetatable(Event, {
	__call = function(_, ...)
		local Hooks = {}
		local self = {}
		function self.Connect(_, fn)
			if type(_) == "function" then fn = _ end
			local id = #Hooks+1
			Hooks[id] = fn
			return {
				Disconnect = function() table.remove(Hooks, id) end 
			}
		end
		function self.Fire(_, ...)
			local args = {...}
			if _ ~= self then 
				table.insert(args, _, 1)
			end
			for i, Hook in pairs(Hooks) do 
				Hooks[i](unpack(args))
			end
		end
		return setmetatable(self, {__index=Event})
	end,
	__index = Object
})

local Color = {
	Type = "Color"
}
setmetatable(Color, {
	__call = function(_, ...)
		local args = {...}

		local self = {}

		if #args == 1 and type(args[1]) == "number"  then 
			local n = args[1]

			local r = byteshiftR(n, 3)
			n = n - byteshiftL(r, 3)

			local g = byteshiftR(n, 2)
			n = n - byteshiftL(g, 2)

			local b = byteshiftR(n, 1)
			n = n - byteshiftL(b, 1)

			local a = n
			--print("COLOR FROM INT: " .. tostring(a))
			self = {r/255,g/255,b/255,a/255}
		elseif #args == 4 and type(args[1]) == type(args[2]) and type(args[2]) == type(args[3]) and type(args[3]) == type(args[4]) and type(args[1]) == "number" then
			--sadly there's no sytax sugar for multiple expr comparison
			if args[1] > 1.0 then
				error("UI.lua::Color(r,g,b,a): expected r in range 0->1, got " .. tostring(r))
			elseif args[2] > 1.0 then
				error("UI.lua::Color(r,g,b,a): expected g in range 0->1, got " .. tostring(g))
			elseif args[3] > 1.0 then
				error("UI.lua::Color(r,g,b,a): expected b in range 0->1, got " .. tostring(b))
			elseif args[4] > 1.0 then
				error("UI.lua::Color(r,g,b,a): expected a in range 0->1, got " .. tostring(a))
			end
			self = {args[1], args[2], args[3], args[4]}
		else
			local typestr = ""
			for i,v in pairs(args) do
				typestr = typestr .. tostring(type(v)) .. ", "
			end
			error("UI.lua::Color(): expected args Color(HEX_RGBA) or Color(num_R,num_G,num_B,num_A) but got Color(" .. typestr:sub(1,#typestr-2) .. ")");
		end

		function self.Lerp(_, other, k)
			if (_ ~= self) then
				k = other
				other = self
			end

			if not (other.r and other.g and other.b and other.a) then
				error("Color:Lerp: arg[1] must be a Color object")
			end


		end

		return setmetatable(self, {__index = Color}) 
	end
})

local vec2 = {
	Type = "vec2"
}
local function makevec(x,y)
	return setmetatable(
		{x=x, y=y}, 
		{
			__add = function(a, b)
				if type(b) == "vec2" then 
					return makevec(a.x+b.x, a.y+b.y)
				else
					error("core.lua::vec2.__add() attempt to add vector and " .. type(b))
				end
			end,
			__sub = function(a,b)
				if type(b) == "vec2" then
					return makevec(a.x-b.x, a.y-b.y)
				else
					error("core.lua::vec2.__sub() attempt to subtract vector and " .. type(b))
				end
			end,
			__mul = function(a, b)
				if type(b) == "vec2" then
					return makevec(a.x * b.x, a.y * b.y)
				elseif type(b) == "number" then
					return makevec(a.x * b, a.y * b)
				else
					error("core.lua::vec2.___mul() attempt to multiply vector and " .. type(b))
				end
			end,
			__div = function(a,b)
				if type(b) == "vec2" then
					return makevec(a.x / b.x, a.y / b.y)
				elseif type(b) == "number" then 
					return makevec(a.x / b, a.y / b)
				else
					error("core.lua::vec2.__div() attempt to divide vector by " .. type(b))
				end
			end,
			__eq = function(a, b)
				if type(b) == "vec2" then 
					return a.x == b.x and a.y == b.y
				else
					return false
				end
			end,
			__unm = function(a,b)
				return makevec(-a.x, -a.y)
			end,
			__tostring = function(a)
				return "vec2(" .. tostring(a.x) .. ", " .. tostring(a.y) .. ")"
			end,
			__index = vec2
		}
	)
end

setmetatable(vec2, {
	__call = function(_, ...)
		local args = {...}
		for i,v in pairs(args) do
			if type(v) ~= "number" then 
				error("core.lua::vec2() unexpected argument " .. tostring(i) .. " expected type 'number' got '" .. typestr(v) .. "'")
			end
		end

		if #args == 0 then
			return makevec(0, 0)
		elseif #args == 1 then 
			return makevec(args[1], args[1])
		elseif #args == 2 then
			return makevec(args[1], args[2])
		else
			error("core.lua::vec2() too many arguments, expected at most 2 arguments, got " .. tostring(#args))
		end
	end,
})
function vec2:length() 
	return math.sqrt(self.x*self.x + self.y*self.y)
end

local UITransform = {
	Type = "UITransform"
}
local function makeUITransform(scale, offset)
	return setmetatable(
		{Scale = scale, Offset = offset}, 
		{
			__index = UITransform,
			__add = function(a, b)
				if type(b) ~= "UITransform" then 
					error("core.lua::UITransform.__add Attempt to add UITransform and " .. type(b))
				else
					return makeUITransform(a.Scale + b.Scale, a.Offset + b.Offset)
				end
			end,
			__sub = function(a, b)
				if type(b) ~= "UITransform" then
					error("core.lua::UITransform.__sub Attempt to subtract UITransform and " .. type(b))
				else
					return makeUITransform(a.Scale - b.Scale, a.Offset - b.Offset)
				end
			end,
			__tostring = function(_)
				return "UITransform( " .. tostring(_.Scale) .. ", " .. tostring(_.Offset) .. ")"
			end
		}
	)
end

setmetatable(UITransform, {
	__call = function(_, ...)
		local args = {...}
		if #args == 2 then 
			if type(args[1]) ~= "vec2" then 
				error("core.lua::UITransform() arg 1 'scale' must be of type 'vec2' found " .. type(args[1]))
			elseif type(args[2]) ~= "vec2" then
				error("core.lua::UITransform() arg 2 'offset' must be of type 'vec2' found " .. type(args[2]))
			else
				return makeUITransform(args[1], args[2])
			end
		elseif #args == 4 then
			for i,v in pairs(args) do
				if type(v) ~= "number" then
					error("core.lua::UITransform() arg " .. tostring(i) .. " expected type 'number' got " .. type(v))
				end
			end
			return makeUITransform(vec2(args[1], args[2]), vec2(args[3], args[4]))
		else
			error("core.lua:UITransform() expected 2 args (vec2 scale, vec2 offset) got " .. tostring(#args) .. " args.")
		end
	end
})

return {
	Object = Object,
	Color = Color,
	Event = Event,
	vec2 = vec2,
	UITransform = UITransform
}