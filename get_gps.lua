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
GPSLatitude: 30, 15, 53.3556		 <-- ¶È ·Ö Ãë
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
    printUtf8(indent.. "\"" .. key.."\" "..":".." ".."{")
  else
    print(indent .. "{")
  end

  key = ""
    for k,v in pairs(table) do
     if type(v) == "table" then
        key = k
        PrintTable(v, level + 1)
     elseif type(v) == "number" then
        local content = string.format("%s\"%s\" : %s,", indent .. "  ", tostring(k), tostring(v))
		printUtf8(content)  
  	else
        local content = string.format("%s\"%s\" : \"%s\",", indent .. "  ", tostring(k), tostring(v))
		printUtf8(content)  
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

	v=v-v%0.001
	return v
end

function getName( lat, lon )
	local tab = getGeoInfo(lat, lon)
	if tab["poi"] then
		return tab["poi"]
	end
	return tab["street"]
end

all = {}

function run( file )
--	PrintTable(file)

	filename = file["filename"]
	
	date=file["GPSDateStamp"]
	lat = file["GPSLatitude"]
	lon = file["GPSLongitude"]

	if not date then
		date=file["DateTime"]
	end

	if date then
		date = string.sub( date, 0, 7 )
		date = string.gsub( date, ":", "/" )

		if lat and lon then
			local geoLat=convStandardString(lat)
			local geoLon=convStandardString(lon)
			local name=getName(geoLat, geoLon)
			if not name then return end
			
			local data = {}
			data["latitude"] = geoLat
			data["longitude"] = geoLon
			data["amap"] = "http://ditu.amap.com/maps?q=".. geoLat .. "," .. geoLon
			local info =  { poi = "" }
			data["info"] = info
			-- ditu.amap.com/maps?q=35.170318,107.821668
			all[name] = data

			-- PrintTable(data)
		end
	end
end

function finish()
	PrintTable(all)
end