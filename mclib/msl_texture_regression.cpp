//---------------------------------------------------------------------------
// msl_texture_regression.cpp
//
// Regression coverage for per-instance multi-shape texture state.
//---------------------------------------------------------------------------//

#include "msl.h"
#include "camera.h"
#include "mlr/mlrclipper.hpp"

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

class TestTGShape : public TG_Shape
{
	public:
		TestTGShape (TG_TypeNodePtr type)
		{
			myType = type;
		}

		TG_TypeNodePtr getType (void)
		{
			return myType;
		}
};

class TestTGTypeNode : public TG_TypeNode
{
	public:
		TestTGTypeNode (TG_TexturePtr texture)
		{
			strcpy(nodeId, "root");
			parentId[0] = '\0';

			numTextures = 1;
			listOfTextures = (TG_TinyTexturePtr)TG_Shape::tglHeap->Malloc(sizeof(TG_TinyTexture));
			gosASSERT(listOfTextures != NULL);
			listOfTextures[0].mcTextureNodeIndex = texture->mcTextureNodeIndex;
			listOfTextures[0].gosTextureHandle = 0xffffffff;
			listOfTextures[0].textureAlpha = texture->textureAlpha;
		}

		virtual void destroy (void)
		{
			if (listOfTextures)
			{
				TG_Shape::tglHeap->Free(listOfTextures);
				listOfTextures = NULL;
			}
		}

		virtual TG_ShapePtr CreateFrom (void)
		{
			void *memarea = TG_Shape::tglHeap->Malloc(sizeof(TestTGShape));
			gosASSERT(memarea != NULL);
			return ::new(memarea) TestTGShape(this);
		}

		virtual long SetTextureHandle (DWORD textureNum, DWORD textureHandle)
		{
			if (textureNum >= numTextures)
				return -1;

			listOfTextures[textureNum].mcTextureNodeIndex = textureHandle;
			listOfTextures[textureNum].gosTextureHandle = 0xffffffff;
			return 0;
		}

		virtual long SetTextureAlpha (DWORD textureNum, bool alphaFlag)
		{
			if (textureNum >= numTextures)
				return -1;

			listOfTextures[textureNum].textureAlpha = alphaFlag;
			return 0;
		}

		DWORD getTextureHandle (void)
		{
			return listOfTextures[0].mcTextureNodeIndex;
		}

		bool getTextureAlpha (void)
		{
			return listOfTextures[0].textureAlpha;
		}

		TG_TinyTexturePtr listOfTextures;
		DWORD numTextures;
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

class TestTGMultiShape : public TG_MultiShape
{
	public:
		TestTGTypeNode *getChildType (void)
		{
			return (TestTGTypeNode *)((TestTGShape *)listOfShapes[0].node)->getType();
		}
};

int main (void)
{
	TG_Shape::tglHeap = new UserHeap;
	if (TG_Shape::tglHeap->init(1024 * 1024, "msl_texture_regression", false))
	{
		fprintf(stderr, "failed to initialize TGL heap\n");
		return 1;
	}

	TG_Texture texture = {};
	texture.mcTextureNodeIndex = 10;
	texture.gosTextureHandle = 0xffffffff;
	texture.textureAlpha = false;

	void *nodeMemory = TG_Shape::tglHeap->Malloc(sizeof(TestTGTypeNode));
	gosASSERT(nodeMemory != NULL);
	TestTGTypeNode *node = ::new(nodeMemory) TestTGTypeNode(&texture);
	{
		TestTGTypeMultiShape type(node, &texture);
		TG_MultiShapePtr first = type.CreateFrom();
		TG_MultiShapePtr second = type.CreateFrom();

		first->SetTextureHandle(0, 20);
		first->SetTextureAlpha(0, true);
		second->SetTextureHandle(0, 30);
		second->SetTextureAlpha(0, false);

		second->Render();
		if (node->getTextureHandle() != 30 || node->getTextureAlpha())
		{
			fprintf(stderr, "second instance did not select its texture state\n");
			return 1;
		}

		first->Render();
		if (node->getTextureHandle() != 20 || !node->getTextureAlpha())
		{
			fprintf(stderr, "first instance did not restore its texture state\n");
			return 1;
		}

		TG_MultiShapePtr detached = first->Detach("root");
		detached->SetTextureHandle(0, 40);
		detached->SetTextureAlpha(0, false);
		detached->Render();
		if (node->getTextureHandle() != 40 || node->getTextureAlpha())
		{
			fprintf(stderr, "detached instance did not retain its texture state\n");
			return 1;
		}

		delete detached;
		delete second;
		delete first;
	}

	delete TG_Shape::tglHeap;
	TG_Shape::tglHeap = NULL;

	return 0;
}
