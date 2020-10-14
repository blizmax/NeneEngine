/*Copyright reserved by KenLee@2020 hellokenlee@163.com*/
#ifdef NENE_GL
#include "Debug.h"
#include "Texture3D.h"

using namespace std;

/** GL Implementation >>> */

class Texture3DImpl
{
public:
	Texture3DImpl();
	~Texture3DImpl();
	void Use(const NNUInt& slot = 0);
public:
	GLuint m_texture_id;
};

Texture3DImpl::Texture3DImpl()
{

}

Texture3DImpl::~Texture3DImpl()
{
	if (m_texture_id != 0)
	{
		glDeleteTextures(1, &m_texture_id);
	}
}

void Texture3DImpl::Use(const NNUInt& slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_3D, m_texture_id);
}

/** GL Implementation <<< */

Texture3D::~Texture3D()
{
	if (m_impl != nullptr)
	{
		delete m_impl;
	}
}

shared_ptr<Texture3D> Texture3D::Create(const vector<string>& filepaths)
{
	return Create({filepaths});
}

shared_ptr<Texture3D> Texture3D::Create(const std::vector<std::vector<string>>& mipmapfilepaths)
{
	//
	NNUInt tex_id = 0;
	glGenTextures(1, &tex_id);
	if (tex_id == 0)
	{
		dLog("[Error] Cannot generate texture 3d!\n");
		return nullptr;
	}
	glBindTexture(GL_TEXTURE_3D, tex_id);
	{
		// Each mip map
		for (NNUInt mip = 0; mip < mipmapfilepaths.size(); ++mip)
		{
			// Read image data for a mip
			NNInt width = -1, height = -1, format = -1;
			vector<shared_ptr<NNByte[]>> images;
			for (NNUInt idx = 0; idx < mipmapfilepaths[mip].size(); ++idx)
			{
				const string& filepath = mipmapfilepaths[mip][idx];
				NNUInt currwidth, currheight;
				NNColorFormat currformat;
				shared_ptr<NNByte[]> currimage = LoadImage(filepath.c_str(), currwidth, currheight, currformat);
				width = (width == -1 ? currwidth : width);
				height = (height == -1 ? currheight : height);
				format = (format == -1 ? currformat : format);
				if (currimage == nullptr || width != currwidth || height != currheight || format != currformat)
				{
					dLog("[Error] Unaligned image for texture 3d!");
					return nullptr;
				}
				images.push_back(currimage);
			}
			// Copy all image data
			NNUInt depth = (NNUInt)images.size();
			NNUInt byteprepixel = 4;
			GLenum glimagedataformat = GL_BGRA;
			//
			shared_ptr<NNByte[]> imagedata = nullptr;
			NNUInt datasize = width * height * byteprepixel;
			imagedata = shared_ptr<NNByte[]>(new NNByte[datasize * depth]);
			for (NNUInt idx = 0; idx < depth; ++idx)
			{
				memcpy(imagedata.get() + datasize * idx, images[idx].get(), datasize);
			}
			//
			glTexImage3D(GL_TEXTURE_3D, mip, GL_RGBA, width, height, depth, 0, GL_BGRA, GL_UNSIGNED_BYTE, imagedata.get());
		}
		//
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, (GLint)mipmapfilepaths.size() - 1);
	}
	glBindTexture(GL_TEXTURE_3D, 0);
	//
	Texture3D* result = new Texture3D();
	result->m_impl = new Texture3DImpl();
	result->m_impl->m_texture_id = tex_id;
	return shared_ptr<Texture3D>(result);
}

void Texture3D::Use(const NNUInt& slot)
{
	m_impl->Use(slot);
}

#endif // NENE_GL