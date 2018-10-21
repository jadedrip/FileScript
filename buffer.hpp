#pragma once

template<typename T>
class Buffer
{
public:
	T* get(size_t sz)
	{
		if (!_data) {
			_dataSize = sz * sizeof(T);
			_data = (T*)malloc(_dataSize);
		} else if (_dataSize < sz) {
			_dataSize = sz * sizeof(T);
			_data = (T*)realloc(_data, _dataSize);
		}
		return _data;
	}
	~Buffer()
	{
		free(_data);
	}
private:
	size_t _dataSize = 0;
	T *_data = nullptr;
};
