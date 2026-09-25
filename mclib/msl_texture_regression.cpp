//---------------------------------------------------------------------------
// msl_texture_regression.cpp
//
// Regression coverage for per-instance multi-shape texture state and for
// the untextured vertex-color channel contract used by health bars.
//---------------------------------------------------------------------------//

#include "msl.h"
#include "camera.h"
#include "mlr/mlrclipper.hpp"
#include "dbasegui.h"

#include <stdio.h>

UserHeapPtr systemHeap = NULL;
CameraPtr eye = NULL;
MidLevelRenderer::MLRClipper *theClipper = NULL;
DWORD BaseVertexColor = 0x00000000;
int ObjectTextureSize = 128;
bool reloadBounds = false;
float gosFontScale = 1.0f;
HGOSFONT3D gosFontHandle = 0;
HSTRRES gosResourceHandle = 0;

Stuff::MemoryStream *effectStream = NULL;
bool useLOSAngle = false;
unsigned long MaxMinUV = 8;
bool justResaveAllMaps = false;
FastFile **fastFiles = NULL;
long numFastFiles = 0;
long maxFastFiles = 0;

static int fail (const char *message)
{
	fprintf(stderr, "%s\n", message);
	return 1;
}

// GL reads a little-endian AARRGGBB dword as B,G,R,A. Untextured shaders
// must apply the same .bgra swizzle as gos_tex_vertex.frag, or SB_RED
// health bars display blue.
static int check_untextured_bar_channels (void)
{
	DWORD submitted = SB_RED | 0xff000000;
	unsigned char gl_r = (unsigned char)(submitted & 0xff);
	unsigned char gl_g = (unsigned char)((submitted >> 8) & 0xff);
	unsigned char gl_b = (unsigned char)((submitted >> 16) & 0xff);
	unsigned char gl_a = (unsigned char)((submitted >> 24) & 0xff);
	unsigned char displayed_r = gl_b;
	unsigned char displayed_g = gl_g;
	unsigned char displayed_b = gl_r;
	unsigned char displayed_a = gl_a;

	if (submitted != 0xffff0000 || displayed_r != 0xff || displayed_g != 0x00 || displayed_b != 0x00 || displayed_a != 0xff)
		return fail("enemy health bar SB_RED would not display red after the untextured BGRA swizzle");

	submitted = SB_GREEN | 0xff000000;
	gl_r = (unsigned char)(submitted & 0xff);
	gl_g = (unsigned char)((submitted >> 8) & 0xff);
	gl_b = (unsigned char)((submitted >> 16) & 0xff);
	displayed_r = gl_b;
	displayed_g = gl_g;
	displayed_b = gl_r;
	if (displayed_r != 0x00 || displayed_g != 0xff || displayed_b != 0x00)
		return fail("friendly health bar SB_GREEN would not display green after the untextured BGRA swizzle");

	submitted = SB_BLUE | 0xff000000;
	gl_r = (unsigned char)(submitted & 0xff);
	gl_g = (unsigned char)((submitted >> 8) & 0xff);
	gl_b = (unsigned char)((submitted >> 16) & 0xff);
	displayed_r = gl_b;
	displayed_g = gl_g;
	displayed_b = gl_r;
	if (displayed_r != 0x00 || displayed_g != 0x00 || displayed_b != 0xff)
		return fail("neutral health bar SB_BLUE would not display blue after the untextured BGRA swizzle");

	return 0;
}

class TestTextureNode : public MC_TextureNode
{
	public:
		void setResolvedHandle (DWORD handle)
		{
			init();
			gosTextureHandle = handle;
		}
};

class TestTextureManager : public MC_TextureManager
{
	public:
		TestTextureManager (MC_TextureNode *nodes)
		{
			masterTextureNodes = nodes;
		}

		~TestTextureManager (void)
		{
			masterTextureNodes = NULL;
		}
};

class TestTGShape : public TG_Shape
{
	public:
		TestTGShape (TG_TypeNodePtr type)
		{
			myType = type;
		}
};

class TestTGTypeShape : public TG_TypeShape
{
	public:
		TestTGTypeShape (void)
		{
			strcpy(nodeId, "root");
			parentId[0] = '\0';

			numTextures = 1;
			listOfTextures = (TG_TinyTexturePtr)TG_Shape::tglHeap->Malloc(sizeof(TG_TinyTexture));
			gosASSERT(listOfTextures != NULL);
			listOfTextures[0].mcTextureNodeIndex = 10;
			listOfTextures[0].gosTextureHandle = 0xffffffff;
			listOfTextures[0].textureAlpha = false;
		}

		virtual TG_ShapePtr CreateFrom (void)
		{
			void *memarea = TG_Shape::tglHeap->Malloc(sizeof(TestTGShape));
			gosASSERT(memarea != NULL);
			return ::new(memarea) TestTGShape(this);
		}

		DWORD nodeIndex (void)
		{
			return listOfTextures[0].mcTextureNodeIndex;
		}

		DWORD resolvedHandle (void)
		{
			return listOfTextures[0].gosTextureHandle;
		}

		bool alpha (void)
		{
			return listOfTextures[0].textureAlpha;
		}
};

class TestTGTypeMultiShape : public TG_TypeMultiShape
{
	public:
		TestTGTypeMultiShape (TG_TypeNodePtr node, TG_TexturePtr texture)
		{
			numTG_TypeShapes = 1;
			listOfTypeShapes = (TG_TypeNodePtr *)TG_Shape::tglHeap->Malloc(sizeof(TG_TypeNodePtr));
			gosASSERT(listOfTypeShapes != NULL);
			listOfTypeShapes[0] = node;

			numTextures = 1;
			listOfTextures = (TG_TexturePtr)TG_Shape::tglHeap->Malloc(sizeof(TG_Texture));
			gosASSERT(listOfTextures != NULL);
			listOfTextures[0] = texture[0];
		}
};

static int expect_bound (TestTGTypeShape *node, DWORD index, DWORD resolved, bool alpha, const char *message)
{
	if (node->nodeIndex() != index || node->resolvedHandle() != resolved || node->alpha() != alpha)
		return fail(message);

	return 0;
}

int main (void)
{
	int result = check_untextured_bar_channels();
	if (result)
		return result;

	TG_Shape::tglHeap = new UserHeap;
	if (TG_Shape::tglHeap->init(1024 * 1024, "msl_texture_regression", false))
		return fail("failed to initialize TGL heap");

	systemHeap = TG_Shape::tglHeap;
	{
		TestTextureNode *nodes = new TestTextureNode[41];
		nodes[20].setResolvedHandle(0xA020);
		nodes[30].setResolvedHandle(0xA030);
		nodes[40].setResolvedHandle(0xA040);
		TestTextureManager manager(nodes);
		mcTextureManager = &manager;

	TG_Texture texture = {};
	texture.mcTextureNodeIndex = 10;
	{
		TestTGTypeShape *node = new TestTGTypeShape;
		TestTGTypeMultiShape type(node, &texture);
		TG_MultiShapePtr first = type.CreateFrom();
		TG_MultiShapePtr second = type.CreateFrom();

		if (!first || !second)
			return fail("CreateFrom did not return an instance");

		first->SetTextureHandle(0, 20);
		first->SetTextureAlpha(0, true);
		second->SetTextureHandle(0, 30);
		second->SetTextureAlpha(0, false);

		if (first->GetTextureHandle(0) != 20 || second->GetTextureHandle(0) != 30)
			return fail("instance texture handles were not stored independently");

		second->Render();
		result = expect_bound(node, 30, 0xA030, false, "second instance did not bind its resolved texture");
		if (result)
			return result;
		if (first->GetTextureHandle(0) != 20)
			return fail("rendering the second instance overwrote the first instance handle");

		first->RenderShadows();
		result = expect_bound(node, 20, 0xA020, true, "shadow render did not restore the first instance texture");
		if (result)
			return result;

		Stuff::LinearMatrix4D cameraOrigin = Stuff::LinearMatrix4D::Identity;
		TG_Shape::s_cameraOrigin = &cameraOrigin;
		Stuff::Point3D position;
		position.x = position.y = position.z = 0.0f;
		Stuff::UnitQuaternion rotation;
		rotation.w = 1.0f;
		rotation.x = rotation.y = rotation.z = 0.0f;
		second->TransformMultiShape(&position, &rotation);
		result = expect_bound(node, 30, 0xA030, false, "transform did not select the second instance texture");
		if (result)
			return result;
		if (first->GetTextureHandle(0) != 20)
			return fail("transform overwrote the other instance handle");

		TG_MultiShapePtr detached = first->Detach("root");
		if (!detached)
			return fail("Detach did not return an instance");
		detached->SetTextureHandle(0, 40);
		detached->SetTextureAlpha(0, false);
		detached->Render();
		result = expect_bound(node, 40, 0xA040, false, "detached instance did not bind its resolved texture");
		if (result)
			return result;
		if (first->GetTextureHandle(0) != 20)
			return fail("detached instance shared the parent texture handle");

		delete detached;
		delete second;
		delete first;
	}

		mcTextureManager = NULL;
		delete [] nodes;
	}
	systemHeap = NULL;
	delete TG_Shape::tglHeap;
	TG_Shape::tglHeap = NULL;
	return 0;
}
