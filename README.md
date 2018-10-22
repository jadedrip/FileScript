# FileScript 文件枚举执行脚本

## 简介

写这个工具的初衷是为了整理我的照片，一家人用手机拍了无数照片，想按照日期、拍摄地点进行分类存放，于是就有了这个小工具。

工作原理是枚举指定目录（默认是当前目录）下的所有文件，然后调用 lua 脚本进行处理。而内部的 jpg 插件会读取 exif 信息，并注入到 lua 函数的参数中。
lua 可以通过内置的 move 函数来移动文件。

## 用法

	FileScript mv_jpg.lua [目录] -D out=(输出目录) 

目前的 lua 脚本会按 年份/目录_经度_维度 这样的格式分目录移动文件，你可以按需要自己定义。 

lua 目前支持 
	
	move (from, to)					-- 移动或改名（会自动创建目标目录）
	copy (from, to)					-- 复制
	splice(str, regex)				-- 根据正则式切分字符串
	saveData(key, data)				-- 为当前内容的文件关联保存一个数据（文件唯一性通过内容 hash 来确定），目前可以保存 bool，string
	loadData(key)					-- 读取 saveData 关联到当前文件的数据
	getGeoInfo(latitude, longitude)	-- 根据经纬度获取地理名称（默认通过 ali 地图api获取，如果为配置，仅支持中国）
	printUtf8(string)				-- 打印 utf8 字符串

目前发布测试版本 v0.9.2 下载地址：

https://github.com/jadedrip/FileScript/releases/download/0.9.2/file_script_0.9.2.zip

** 如果无法执行可能需要下载 vc 运行库： https://download.visualstudio.microsoft.com/download/pr/100349138/88b50ce70017bf10f2d56d60fcba6ab1/VC_redist.x86.exe **

## 配置

允许在 fileScript.conf 文件中进行配置，这文件 **必须是** utf-8 (无 BOM) 格式，目前可以配置经纬:

	{
		"location" : [
			{
				"latitude" : 15.870032,
				"longitude" : 100.992541,
				"distance" : 5000,
				"info" : {
					"poi": "泰国"
				}
			}
		]
	}

## 编译

Windows 环境下强烈推荐使用 [vcpkg](https://github.com/Microsoft/vcpkg) 来进行依赖包管理。

本程序使用了 boost, libexif, lua
并使用 Visual studio 2017 进行编译，并未使用平台相关的代码，理论上会比较容易移植到 linux 上。

## 展望
未来可以通过增加插件机制，通过增加更多的命令以及更多文件解析能力来增加使用范围。