#include "stdafx.h"
#include <libexif/exif-loader.h>
#include "FileScript.h"

using namespace std;
struct JpegData {
	std::map<string, string> properties;
	lua_State* state;
};

void read_exif_entry(ExifEntry *ee, void* user_data)
{
	JpegData* data = (JpegData*)user_data;
	char v[1024];
	//  strncpy(t, exif_tag_get_title_in_ifd(ee->tag, exif_entry_get_ifd(ee)), sizeof(t));  
	//  strncpy(t, exif_tag_get_title_in_ifd(ee->tag, *((ExifIfd*)ifd)), sizeof(t));  
	//trim t  
	ExifIfd ifd=exif_entry_get_ifd(ee);
	const char* name = exif_tag_get_name_in_ifd(ee->tag, ifd);
	const char* value = exif_entry_get_value(ee, v, sizeof(v));
	append_field(data->state, name, value);
	if(ifd==EXIF_IFD_GPS )
		data->properties[name] = value;

	printf("%s: %s\n", name, value);
	//printf("%s: %s\n"
	//	          , exif_tag_get_name_in_ifd(ee->tag, ifd)  
	//	//, exif_tag_get_title_in_ifd(ee->tag, ifd)
	//	//          , exif_tag_get_description_in_ifd(ee->tag, *((ExifIfd*)ifd))  
	//	, exif_entry_get_value(ee, v, sizeof(v)));
}

void read_exif_content(ExifContent *ec, void *user_data)
{
	ExifIfd ifd = exif_content_get_ifd(ec);
	if (ifd == EXIF_IFD_COUNT)
		throw new std::runtime_error("exif_content_get_ifd error");

	exif_content_foreach_entry(ec, read_exif_entry, user_data);
}

double parse_degree(const std::string& degree) {
	return 0.0;
}

void set_gps(lua_State* state, std::map<string, string>& properties) {
	if (properties.empty()) return;
	// GPSLatitude: 30, 15, 53.3556
	std::string& s_latitude =properties["GPSLatitude"];
	if (s_latitude.empty()) return;
	std::string& longitud = properties["GPSLongitude"];
	if (longitud.empty()) return;

	double latitude =parse_degree(s_latitude);
}

void jpeg_parser(const char* filename, lua_State* state) {
	auto *data = new JpegData();
	data->state = state;
	auto* ed=exif_data_new_from_file(filename);
	exif_data_foreach_content(ed, read_exif_content, data);
	exif_data_unref(ed);

	set_gps(state, data->properties);

	delete data;
}

void init_jpeg_parser() {
	register_parser(".jpg", jpeg_parser);
}