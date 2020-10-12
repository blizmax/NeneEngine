﻿/*Copyright reserved by KenLee@2018 hellokenlee@163.com*/
#include "Debug.h"
#include "Texture.h"
#include <FreeImage/FreeImage.h>

using namespace std;

shared_ptr<NNByte[]> Texture::LoadImage(const NNChar* filepath, NNUInt& width, NNUInt& height, NNColorFormat &format) {
	// 检查文件
	checkFileExist(filepath);
	// 图片格式
	FREE_IMAGE_FORMAT fileFormat = FIF_UNKNOWN;
	// 图片数据
	BYTE *pBits = nullptr;
	// 图片指针
	FIBITMAP *pImage = nullptr;
	// 获取格式
	fileFormat = FreeImage_GetFileType(filepath, 0);
	if (fileFormat == FIF_UNKNOWN) {
		fileFormat = FreeImage_GetFIFFromFilename(filepath);
	}
	if (fileFormat == FIF_UNKNOWN) {
		dLog("[Error] Unknown image type (%s)\n", filepath);
		return nullptr;
	}
	// 载入图片
	if (FreeImage_FIFSupportsReading(fileFormat)) {
		pImage = FreeImage_Load(fileFormat, filepath);
	}
	else {
		dLog("[Error] Unsupported image type (%s)\n", filepath);
	}
	if (!pImage) {
		dLog("[Error] Failed to load image! (%s)\n", filepath);
		return nullptr;
	}
	FREE_IMAGE_COLOR_TYPE color = FreeImage_GetColorType(pImage);
	// !TODO: 避免转换
	if (color != FIC_RGBALPHA) {
		FIBITMAP *tmp = pImage;
		pImage = FreeImage_ConvertTo32Bits(pImage);
		FreeImage_Unload(tmp);
		color = FreeImage_GetColorType(pImage);
	}
	// 获取数据
	pBits = FreeImage_GetBits(pImage);
	width = FreeImage_GetWidth(pImage);
	height = FreeImage_GetHeight(pImage);
	NNUInt pitch = FreeImage_GetPitch(pImage);
	switch (color)
	{
	case FIC_RGB:
		format = RGB;
		break;
	case FIC_RGBALPHA:
		format = RGBA;
		break;
	default:
		dLog("[Error] Wrong image color format!\n");
		break;
	}
	// 拷贝一份数据返回
	NNUInt BBP = FreeImage_GetBPP(pImage) / 8;
	BYTE *res = new BYTE[width * height * BBP];
	memcpy(res, pBits, width * height * BBP);
	// 释放原来的指针
	FreeImage_Unload(pImage);
	// 返回
	return std::unique_ptr<BYTE[]>(res);
}