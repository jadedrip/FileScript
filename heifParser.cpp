#include "stdafx.h"
#include "map.h"
#include "FileScript.h"
#include <libheif/heif.h>
#include <libexif/exif-loader.h>

using namespace std;

// static
uint8_t* getExifMetaData(const struct heif_image_handle* handle, size_t* size)
{
	heif_item_id metadata_id;
	int count = heif_image_handle_get_list_of_metadata_block_IDs(handle, "Exif",
																 &metadata_id, 1);
	for (int i = 0; i < count; i++) {
		size_t datasize = heif_image_handle_get_metadata_size(handle, metadata_id);
		uint8_t* data = static_cast<uint8_t*>(malloc(datasize));
		if (!data) {
			continue;
		}

		heif_error error = heif_image_handle_get_metadata(handle, metadata_id, data);
		if (error.code != heif_error_Ok) {
			free(data);
			continue;
		}

		*size = datasize;
		return data;
	}

	return nullptr;
}

void read_exif_content(ExifContent *ec, void *user_data);
void heif_parser(const char* filename, map_state* state) {

	std::shared_ptr<heif_context> ctx(heif_context_alloc(),
									  [](heif_context* c) { heif_context_free(c); });
	if (!ctx) {
		std::cerr << "Could not create HEIF context\n" << std::endl;
		return;
	}

	struct heif_error err;
	err = heif_context_read_from_file(ctx.get(), filename, nullptr);

	if (err.code != 0) {
		std::cerr << "Could not read HEIF file: " << err.message << "\n";
		return;
	}

	int numImages = heif_context_get_number_of_top_level_images(ctx.get());
	heif_item_id* IDs = (heif_item_id*)alloca(numImages * sizeof(heif_item_id));
	heif_context_get_list_of_top_level_image_IDs(ctx.get(), IDs, numImages);

	for (int i = 0; i < numImages; i++) {
		struct heif_image_handle* handle;
		struct heif_error err = heif_context_get_image_handle(ctx.get(), IDs[i], &handle);
		if (err.code) {
			std::cerr << err.message << "\n";
			return;
		}

		size_t dataSize = 0;
		uint8_t* data=getExifMetaData(handle, &dataSize);
		if (!data) continue;

		auto* ed = exif_data_new_from_data(data+4, dataSize-4);
		exif_data_foreach_content(ed, read_exif_content, state);
		exif_data_unref(ed);
	}

}

void init_heif_parser() {
	registerParser(".heic", heif_parser);
}
