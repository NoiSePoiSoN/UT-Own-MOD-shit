// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CustomDepthRendering.h: CustomDepth rendering implementation.
=============================================================================*/

#pragma once

/** 
* Set of distortion scene prims  
*/
class FCustomDepthPrimSet
{
public:

	/** 
	* Iterate over the prims and draw them
	* @param ViewInfo - current view used to draw items
	* @return true if anything was drawn
	*/
	bool DrawPrims(FRHICommandListImmediate& RHICmdList, const class FViewInfo& View);

	/**
	* Add a new primitive to the list of prims
	* @param PrimitiveSceneProxy - primitive info to add.
	* @param ViewInfo - used to transform bounds to view space
	*/
	void AddScenePrimitive(FPrimitiveSceneProxy* PrimitiveSceneProxy)
	{
		Prims.Add(PrimitiveSceneProxy);
	}
	/**
	* Adds a new primitives to the list of distortion prims
	* @param PrimitiveSceneProxies - primitive info to add.
	*/
	void Append(FPrimitiveSceneProxy** PrimitiveSceneProxies, int32 NumProxies)
	{
		Prims.Append(PrimitiveSceneProxies, NumProxies);
	}

	/** 
	* @return number of prims to render
	*/
	int32 NumPrims() const
	{
		return Prims.Num();
	}

private:
	/** list of prims added from the scene */
	TArray<FPrimitiveSceneProxy*, SceneRenderingAllocator> Prims;
};
