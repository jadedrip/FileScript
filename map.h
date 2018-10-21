#pragma once
#ifdef __cplusplus
extern "C" {
#endif
	typedef struct _MAP_STATE
	{
		void* data;
	}map_state;

	void map_append(map_state*, const char* name, const char* value);
	void map_remove(map_state*, const char* name);
	const char* map_get(map_state*, const char* name);
	unsigned long long map_size(map_state*);
#ifdef __cplusplus
}
#endif
