// LICENSE HERE.

//
// client/rmlui/interfaces/RenderInterface.cpp
//
//
// RmlUI N&C System Interface implementation.
//

#include "../librmlui.h"

// RmlUI includes.
#include "../rmlui.h"

// QGL.
#include "../../src/refresh/gl/gl.h"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif


class RmlUiRendererGeometryHandler
{
public:
	GLuint VertexID, IndexID;
	int NumVertices;
	Rml::TextureHandle Texture;

	RmlUiRendererGeometryHandler() : VertexID(0), IndexID(0), NumVertices(0), Texture(0)
	{
	};

	~RmlUiRendererGeometryHandler()
	{
		if (VertexID)
			qglDeleteBuffersARB(1, &VertexID);

		if (IndexID)
			qglDeleteBuffersARB(1, &IndexID);

		VertexID = IndexID = 0;
	};
};

struct RmlUiRendererVertex
{
	float position[2];
	float texCoord[2];
	unsigned char color[4];
};

//
//=============================================================================
// RmlUiRenderInterface::RmlUiRenderInterface
// 
// The actual RmlUi render interface for OpenGL.
//=============================================================================
//
RmlUiRenderInterface::RmlUiRenderInterface() : m_width(1280), m_height(768), m_transform_enabled(false)
{

}

//
//=============================================================================
// CompileGeometry
// 
// Called by RmlUi when it wants to render geometry that it does not wish to 
// optimise.	
//=============================================================================
//
	// Called when the render viewport size has changed.
void RmlUiRenderInterface::SetViewportArea(int width, int height) {

}

// Called to initialize the renderer, enable certain OpenGL states.
void RmlUiRenderInterface::Initialize() {
	m_width = r_config.width;
	m_height = r_config.height;
}

void RmlUiRenderInterface::RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rml::TextureHandle texture, const Rml::Vector2f& translation)
{
	qglLoadIdentity();
	// Setup 2D rendering mode.
	qglViewport(0, 0, r_config.width, r_config.height);
	GL_Ortho(0, r_config.width, r_config.height, 0, -1, 1);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	//// Translate.
	qglTranslatef(translation.x, translation.y, 0);

	Rml::Vector<Rml::Vector2f> Positions(num_vertices);
	Rml::Vector<Rml::Colourb> Colors(num_vertices);
	Rml::Vector<Rml::Vector2f> TexCoords(num_vertices);

	for (int i = 0; i < num_vertices; i++)
	{
		Positions[i] = vertices[i].position;
		Colors[i] = vertices[i].colour;
		TexCoords[i] = vertices[i].tex_coord;
	};

	// Setup state bits.
	glStateBits_t bits = GLS_DEFAULT;
	////bits = (glStateBits_t)(GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_FALSE | GLS_CULL_DISABLE); // CPP: Cast
	bits = (glStateBits_t)(GLS_CULL_DISABLE); // CPP: Cast
	bits = (glStateBits_t)(bits | GLS_BLEND_BLEND); // CPP: Cast bits |= GLS_BLEND_BLEND;
	//bits = (glStateBits_t)(bits | GLS_ALPHATEST_ENABLE); // CPP: Cast

	// Setup array bits.
	if (texture) {
		qglActiveTextureARB(GL_TEXTURE0_ARB);
		qglBindTexture(GL_TEXTURE_2D, texture);
		qglEnable(GL_TEXTURE_2D);
		qglClientActiveTextureARB(GL_TEXTURE0_ARB);

		GL_StateBits(bits);
		GL_ArrayBits((glArrayBits_t)(GLA_VERTEX | GLA_TC | GLA_COLOR)); // CPP: Cast
	} else {
		qglActiveTextureARB(GL_TEXTURE0_ARB);
		qglBindTexture(GL_TEXTURE_2D, 0);
		qglEnable(GL_TEXTURE_2D);
		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		//qglDisable(GL_TEXTURE_2D);
		GL_StateBits(bits);
		GL_ArrayBits((glArrayBits_t)(GLA_VERTEX | GLA_COLOR)); // CPP: Cast
	}

	qglVertexPointer(2, GL_FLOAT, 0, &Positions[0]);
	qglColorPointer(4, GL_UNSIGNED_BYTE, 0, &Colors[0]);
	qglTexCoordPointer(2, GL_FLOAT, 0, &TexCoords[0]);

	GL_LockArrays(num_vertices);

	qglDrawElements(GL_TRIANGLES, num_indices, QGL_INDEX_ENUM, indices);

	GL_UnlockArrays();
}

//
//=============================================================================
// CompileGeometry
// 
// Called by RmlUi when it wants to compile geometry it believes will be static 
// for the forseeable future.		
//=============================================================================
//
// Rml::CompiledGeometryHandle RmlUiRenderInterface::CompileGeometry(Rml::Vertex* RMLUI_UNUSED_PARAMETER(vertices), int RMLUI_UNUSED_PARAMETER(num_vertices), int* RMLUI_UNUSED_PARAMETER(indices), int RMLUI_UNUSED_PARAMETER(num_indices), const Rml::TextureHandle RMLUI_UNUSED_PARAMETER(texture))
Rml::CompiledGeometryHandle RmlUiRenderInterface::CompileGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rml::TextureHandle texture)
{
	return (Rml::CompiledGeometryHandle) nullptr;
}

//
//=============================================================================
// RenderCompiledGeometry
// 
// Called by RmlUi when it wants to render application-compiled geometry.	
//=============================================================================
//
//void RmlUiRenderInterface::RenderCompiledGeometry(Rml::CompiledGeometryHandle RMLUI_UNUSED_PARAMETER(geometry), const Rml::Vector2f& RMLUI_UNUSED_PARAMETER(translation))
void RmlUiRenderInterface::RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry, const Rml::Vector2f& translation)
{

}

//
//=============================================================================
// ReleaseCompiledGeometry
// 
// Called by RmlUi when it wants to release application-compiled geometry.	
//=============================================================================
//
//void RmlUiRenderInterface::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle RMLUI_UNUSED_PARAMETER(geometry))
void RmlUiRenderInterface::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry)
{

}

//
//=============================================================================
// EnableScissorRegion
// 
// Called by RmlUi when it wants to enable or disable scissoring to clip content.	
//=============================================================================
//
void RmlUiRenderInterface::EnableScissorRegion(bool enable)
{
	//if (enable)
	//	qglEnable(GL_SCISSOR_TEST);
	//else
	//	qglDisable(GL_SCISSOR_TEST);
	// Disable renderer clip rect.
	//if (!enable)
	//	R_SetClipRect_GL(NULL);
	if (enable) {
		if (!m_transform_enabled) {
			qglEnable(GL_SCISSOR_TEST);
			qglDisable(GL_STENCIL_TEST);
		}
		else {
			qglDisable(GL_SCISSOR_TEST);
			qglEnable(GL_STENCIL_TEST);
		}
	} else {
		qglDisable(GL_SCISSOR_TEST);
		qglDisable(GL_STENCIL_TEST);
	}
}

//
//=============================================================================
// SetScissorRegion
// 
// Called by RmlUi when it wants to change the scissor region.		
//=============================================================================
//
void RmlUiRenderInterface::SetScissorRegion(int x, int y, int width, int height)
{
	//qglScissor(x, r_config.width - (y + height), width, height);

	if (!m_transform_enabled) {
		qglScissor(x, m_height - (y + height), width, height);
	} else {
		// clear the stencil buffer
		qglStencilMask(GLuint(-1));
		qglClear(GL_STENCIL_BUFFER_BIT);

		// fill the stencil buffer
		qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		qglDepthMask(GL_FALSE);
		qglStencilFunc(GL_NEVER, 1, GLuint(-1));
		qglStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);

		float fx = (float)x;
		float fy = (float)y;
		float fwidth = (float)width;
		float fheight = (float)height;

		// draw transformed quad
		GLfloat vertices[] = {
			fx, fy, 0,
			fx, fy + fheight, 0,
			fx + fwidth, fy + fheight, 0,
			fx + fwidth, fy, 0
		};
		qglDisableClientState(GL_COLOR_ARRAY);
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglVertexPointer(3, GL_FLOAT, 0, vertices);
		GLushort indices[] = { 1, 2, 0, 3 };
		qglDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, indices);
		qglEnableClientState(GL_COLOR_ARRAY);

		// prepare for drawing the real thing
		qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		qglDepthMask(GL_TRUE);
		qglStencilMask(0);
		qglStencilFunc(GL_EQUAL, 1, GLuint(-1));
	}
}

// Set to byte packing, or the compiler will expand our struct, which means it won't read correctly from file
#pragma pack(1) 
struct TGAHeader
{
	char  idLength;
	char  colourMapType;
	char  dataType;
	short int colourMapOrigin;
	short int colourMapLength;
	char  colourMapDepth;
	short int xOrigin;
	short int yOrigin;
	short int width;
	short int height;
	char  bitsPerPixel;
	char  imageDescriptor;
};
// Restore packing
#pragma pack()

//
//=============================================================================
// LoadTexture
// 
// Called by RmlUi when a texture is required by the library.		
//=============================================================================
//
bool RmlUiRenderInterface::LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle file_handle = file_interface->Open(source);
	if (!file_handle)
	{
		return false;
	}

	file_interface->Seek(file_handle, 0, SEEK_END);
	size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);

	RMLUI_ASSERTMSG(buffer_size > sizeof(TGAHeader), "Texture file size is smaller than TGAHeader, file must be corrupt or otherwise invalid");
	if (buffer_size <= sizeof(TGAHeader))
	{
		file_interface->Close(file_handle);
		return false;
	}

	char* buffer = new char[buffer_size];
	file_interface->Read(buffer, buffer_size, file_handle);
	file_interface->Close(file_handle);

	TGAHeader header;
	memcpy(&header, buffer, sizeof(TGAHeader));

	int color_mode = header.bitsPerPixel / 8;
	int image_size = header.width * header.height * 4; // We always make 32bit textures 

	if (header.dataType != 2)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24/32bit uncompressed TGAs are supported.");
		return false;
	}

	// Ensure we have at least 3 colors
	if (color_mode < 3)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24 and 32bit textures are supported");
		return false;
	}

	const char* image_src = buffer + sizeof(TGAHeader);
	unsigned char* image_dest = new unsigned char[image_size];

	// Targa is BGR, swap to RGB and flip Y axis
	for (long y = 0; y < header.height; y++)
	{
		long read_index = y * header.width * color_mode;
		long write_index = ((header.imageDescriptor & 32) != 0) ? read_index : (header.height - y - 1) * header.width * color_mode;
		for (long x = 0; x < header.width; x++)
		{
			image_dest[write_index] = image_src[read_index + 2];
			image_dest[write_index + 1] = image_src[read_index + 1];
			image_dest[write_index + 2] = image_src[read_index];
			if (color_mode == 4)
				image_dest[write_index + 3] = image_src[read_index + 3];
			else
				image_dest[write_index + 3] = 255;

			write_index += 4;
			read_index += color_mode;
		}
	}

	texture_dimensions.x = header.width;
	texture_dimensions.y = header.height;

	bool success = GenerateTexture(texture_handle, image_dest, texture_dimensions);

	delete[] image_dest;
	delete[] buffer;

	return success;
}

//
//=============================================================================
// GenerateTexture
// 
// Called by RmlUi when a texture is required to be built from an internally-
// generated sequence of pixels.
//=============================================================================
//
bool RmlUiRenderInterface::GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions)
{
	GLuint texture_id = 0;
	qglGenTextures(1, &texture_id);
	if (texture_id == 0)
	{
		Com_WPrintf("RmlUiRenderInterface::GenerateTexture failed to generate textures\n");
		return false;
	}

	qglBindTexture(GL_TEXTURE_2D, texture_id);

	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, source_dimensions.x, source_dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	texture_handle = (Rml::TextureHandle)texture_id;

	return true;
}

//
//=============================================================================
// ReleaseTexture
// 
// Called by RmlUi when a loaded texture is no longer required.		
//=============================================================================
//
void RmlUiRenderInterface::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	qglDeleteTextures(1, (GLuint*)&texture_handle);
}

//
//=============================================================================
// SetTransform
// 
// Called by RmlUi when it wants to set the current transform matrix to a new matrix.	
//=============================================================================
//
void RmlUiRenderInterface::SetTransform(const Rml::Matrix4f* transform)
{
	m_transform_enabled = (bool)transform;

	if (transform)
	{
		if (std::is_same<Rml::Matrix4f, Rml::ColumnMajorMatrix4f>::value)
			qglLoadMatrixf(transform->data());
		else if (std::is_same<Rml::Matrix4f, Rml::RowMajorMatrix4f>::value)
			qglLoadMatrixf(transform->Transpose().data());
	}
	else
		qglLoadIdentity();
}

//
//=============================================================================
// CaptureScreen
// 
// Called by RmlUi when it wants to capture the screen output.
//=============================================================================
//
RmlUiRenderInterface::Image RmlUiRenderInterface::CaptureScreen()
{
	Image image;
	image.num_components = 3;
	image.width = m_width;
	image.height = m_height;

	const int byte_size = image.width * image.height * image.num_components;
	image.data = Rml::UniquePtr<Rml::byte[]>(new Rml::byte[byte_size]);

	qglReadPixels(0, 0, image.width, image.height, GL_RGB, GL_UNSIGNED_BYTE, image.data.get());

	bool result = true;
	GLenum err;
	while ((err = qglGetError()) != GL_NO_ERROR)
	{
		result = false;
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not capture screenshot, got GL error: 0x%x", err);
	}

	if (!result)
		return Image();

	return image;
}
