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
function run( file )
	print ("out = " .. out)
	c = string.sub(out,-1,1)
	if c =="\\" or c == "/" then
		to = out
	else
		to = out .. "/"
	end

	date=file["GPSDateStamp"]
	lat = file["GPSLatitude"]
	lon = file["GPSLongitude"]
	if date  and lat  and lon  then
		date = string.sub( date, 0, 7 )
		date = string.gsub( date, ":", "/" )
		to = to .. date

		print ("lat " .. lat)
		lat = string.gsub( lat, "^(%d+),%s*(%d+),.*$", "_%1_%2" )
		lon =  string.gsub( lon, "^(%d+),%s*(%d+),.*$", "_%1_%2" )
		to = to .. lat .. lon
		move( file["filename"] , to )
	end

	print ( file["filename"] .. " to " .. to)
end
