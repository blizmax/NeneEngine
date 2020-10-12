﻿/*Copyright reserved by KenLee@2018 hellokenlee@163.com*/
#ifdef NENE_GL
#include <vector>
#include "Debug.h"
#include "Texture2D.h"

using namespace std;

/** Implementation Functions >>> */

void GenerateTextures(vector<GLuint> IN tex_ids)
{

}

/** Implementation Functions <<< */

Texture2D::Texture2D() : Texture(), mTextureID(0), mMode(AS_COLOR) {}


Texture2D::~Texture2D() {
	if (mTextureID != 0) {
		glDeleteTextures(1, &mTextureID);
	}
}

shared_ptr<Texture2D> Texture2D::CreateFromMemory(const NNUInt& width, const NNUInt& height, const NNUInt& iformat, const NNUInt& format, const NNUInt& type, const void *pInitData) {
	//
	GLuint texID = 0;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
		glTexImage2D(GL_TEXTURE_2D, 0, (GLenum)iformat, width, height, 0, (GLenum)format, (GLenum)type, pInitData);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	//
	if (texID == 0) {
		return nullptr;
	}
	Texture2D* ret = new Texture2D();
	ret->mTextureID = texID;
	return shared_ptr<Texture2D>(ret);
}

shared_ptr<Texture2D> Texture2D::CreateMultisample(const NNUInt& width, const NNUInt& height, const NNUInt& samples, const NNUInt& iformat) {
	//
	GLuint texID = 0;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texID);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, (GLenum)iformat, width, height, false);
	glBindTexture(GL_TEXTURE_2D, 0);
	//
	if (texID == 0) {
		return nullptr;
	}
	Texture2D* ret = new Texture2D();
	ret->mTextureID = texID;
	return shared_ptr<Texture2D>(ret);
}


shared_ptr<Texture2D> Texture2D::Create(const NNChar* filepath) {
	return Create(vector<const NNChar*>({filepath}));
}


shared_ptr<Texture2D> Texture2D::Create(vector<const NNChar*> filepaths)
{
	//
	NNUInt texID = 0;
	glGenTextures(1, &texID);
	//
	if (texID == 0)
	{
		dLog("[Error] Cannot generate texture!\n");
		return nullptr;
	}
	//
	glBindTexture(GL_TEXTURE_2D, texID);
	{
		//
		for (NNUInt idx = 0; idx < filepaths.size(); ++idx)
		{
			//
			const char* filepath = filepaths[idx];
			std::shared_ptr<NNByte[]> image_data = nullptr;
			//
			GLuint width, height;
			NNColorFormat format;
			//
			image_data = LoadImage(filepath, width, height, format);
			if (image_data == nullptr || width == 0 || height == 0) {
				dLog("[Error] Broken image data! Could not load texture(%s)\n", filepath);
				continue;
			}
			//
			glTexImage2D(GL_TEXTURE_2D, idx, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, image_data.get());
		}
		//
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (GLint)filepaths.size() - 1);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	//
	Texture2D* ret = new Texture2D();
	ret->mTextureID = texID;
	return shared_ptr<Texture2D>(ret);
}

void Texture2D::Use(const NNUInt& slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, mTextureID);
}

void Texture2D::SetMode(NNTextureMode m) {
	if (m == mMode) {
		return;
	}
	glBindTexture(GL_TEXTURE_2D, mTextureID);
	switch (m)
	{
	case AS_COLOR:
		// 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	case AS_DEPTH:
		// 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
		break;
	case AS_STENCIL:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
		break;
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

#endif // NENE_GL