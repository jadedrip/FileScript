--[[
Flash: Flash did not fire, auto mode
FocalLength: 4.3 mm
MakerNote: 108 bytes undefined data
SubsecTime: 161869
SubSecTimeOriginal: 161869
SubSecTimeDigitized: 161869
FlashPixVersion: FlashPix Version 1.0
ColorSpace: sRGB
PixelXDimension: 4608
PixelYDimension: 3456
SensingMethod: One-chip color area sensor
SceneType: Directly photographed
ExposureMode: Auto exposure
WhiteBalance: Auto white balance
FocalLengthIn35mmFilm: 0
SceneCaptureType: Standard
GPSAltitudeRef:
GPSLatitudeRef: N
GPSLatitude: 30, 15, 53.3556		 <-- 度 分 秒
GPSLongitudeRef: E
GPSLongitude: 120,  9, 21.8700
GPSAltitude: 0.000
GPSTimeStamp: 06:26:11.00
GPSProcessingMethod: ASCII
GPSDateStamp: 2016:12:24
InteroperabilityIndex: R98
InteroperabilityVersion: 0100
]]
key = ""
function PrintTable(table , level)
  if not table then
      print("Nil table")
	  return
  end

  level = level or 1
  local indent = ""
  for i = 1, level do
    indent = indent.."  "
  end

  if key ~= "" then
    print(indent..key.." ".."=".." ".."{")
  else
    print(indent .. "{")
  end

  key = ""
  for k,v in pairs(table) do
     if type(v) == "table" then
        key = k
        PrintTable(v, level + 1)
     else
        local content = string.format("%s%s = %s", indent .. "  ",tostring(k), tostring(v))
      print(content)  
      end
  end
  print(indent .. "}")
end

function trim(s)
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

function convStandardString(str)
	local s=spliceString(str, ",")
	local v=tonumber( trim(s[1].."") )
	if s[2] then
		v=v+1/60*tonumber( trim(s[2]) )
	end
	if s[3] then
		v=v+1/3600*tonumber( trim(s[3]) )
	end
	-- print( "str " .. str .. " to " .. v )
	-- v=v-v%0.001
	return v
end

function getName( lat, lon )
	local tab = getGeoInfo(lat, lon)
	-- PrintTable(tab)
	if tab==nil then return "" end

	PrintTable(tab)
	if tab["city"] then
		return tab["city"]
	elseif tab["province"] then
		return tab["province"]
	end
	
	return tab["formatted_address"], name
end

function run( file )
	-- PrintTable(file)

	filename = file["filename"]
	shortname = file["shortname"]
	
	-- 判断文件是否已经被移动过了
	-- local d=loadData("moved") 
	
	-- if d then
	--	print ("File " .. filename .. " was already moved, skip it." )
	--	return 
	-- end

	if not out then
		out = "out/"
	end
	-- print ("out = " .. out)
	c = string.sub(out,-1,1)
	if c =="\\" or c == "/" then
		to = out
	else
		to = out .. "/"
	end

	date= file["GPSDateStamp"]
	lat = file["GPSLatitude"]
	lon = file["GPSLongitude"]

	if not date then
		date=file["DateTime"]
	end

	if date then
		date = string.sub( date, 0, 7 )
		date = string.gsub( date, ":", "/" )
		to = to .. date

		if lat and lon then
			local geoLat=convStandardString(lat)
			local geoLon=convStandardString(lon)
			local name=getName(geoLat, geoLon)

			if name then
				print( "Geo name " .. geoLat .. "," .. geoLon .. ":" .. name)
				to = to .. "/" .. name .. "/" 
			else
				local v=lat-lat%0.01
				print( filename .. " to " .. to)
				print( "Geo name " .. geoLat .. "," .. geoLon .. ":")
				to = to .. "/" .. v .. "/"
			end
			print( filename .. " at " .. geoLat .. "," .. geoLon .. " to " .. to )
		else
			to = to .. "/" 
		end

		move( filename , to )
		-- iPhone 会有 .mov 文件 
		move( shortname .. ".mov" , to )
		
		-- 保存一个标志
		-- saveData("moved", true )
	end
end
