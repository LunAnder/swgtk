/*
---------------------------------------------------------------------------------------
This source file is part of swgANH (Star Wars Galaxies - A New Hope - Server Emulator)
For more information, see http://www.swganh.org


Copyright (c) 2006 - 2010 The swgANH Team

---------------------------------------------------------------------------------------
*/

#include "QuadTree.h"


//======================================================================================================================
//
// Constructor
//

QuadTree::QuadTree(float lowX,float lowZ,float width,float height,unsigned char depth) :
QuadTreeNode(lowX,lowZ,width,height)
{
	// lets grow a tree
	for(unsigned char i = 0;i < depth;i++)
	{
		subDivide();
	}
}

//======================================================================================================================
//
// Deconstructor
//

QuadTree::~QuadTree()
{

}

//======================================================================================================================


