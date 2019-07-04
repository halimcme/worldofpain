/**************************************************************************
*   File: xmlmap.c                                  Part of World of Pain *
*  Usage: Generate XML MMP format map of the world for mappers            *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 2019 World of Pain                                       *
***************************************************************************/


#define __XMLMAP_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "constants.h"
#include "olc.h"

#include <tinyxml2.h>
#include <iostream>
#include <boost/geometry.hpp>

namespace bg = boost::geometry;
using namespace std;
using namespace tinyxml2;

// external data

// local functions
void generateWorldMap();

// directional offsets:   NORTH   EAST   SOUTH     WEST     UP     DOWN
int dir_offsets[6][3] ={ {0,1,0},{1,0,0},{0,-1,0},{-1,0,0},{0,0,1},{0,0,-1} };

int sector_colors[] = {
  7,
  7,
  2,
  2,
  6,
  3,
  4,
  4,
  4,
  2,
  8,
  11,
  15,
  8,
  11,
  5
};


void generateWorldMap()
{
  XMLDocument doc;
  XMLDeclaration * decl = doc.NewDeclaration();
  doc.InsertFirstChild( decl );
  XMLNode * pRoot = doc.NewElement("map");
  doc.InsertEndChild(pRoot);
  XMLElement * areas = doc.NewElement("areas");
  pRoot->InsertEndChild(areas);
  for( auto const& [nr, zone] : zone_table )
  {
    if (nr == 255)
      continue;
    XMLElement * area = doc.NewElement("area");
    area->SetAttribute("id", nr);
    area->SetAttribute("name", zone->name);
    areas->InsertEndChild(area);

  }

  XMLElement * rooms = doc.NewElement("rooms");
  pRoot->InsertEndChild(rooms);
  bool roomHasExit; // only include rooms that have exits
  bool firstRoom = true; // first room of a zone (area)
  int prevRoomZone = 0; // the zone of the most recent room used for firstRoom
  int currentRoom = 0; // current room vnum
  map<int,bg::model::point<long, 3, bg::cs::cartesian>> coordsMap;
  bg::model::point<long, 3, bg::cs::cartesian> currentCoords;
  int furthestEast = 0;
  for( auto const& [nr, rm] : world )
  {
    if (rm->zone == 255)
      continue;
    
    currentRoom =  nr;
    XMLElement * room = doc.NewElement("room");
    room->SetAttribute("id", currentRoom);
    room->SetAttribute("area",  rm->zone);
    room->SetAttribute("title", rm->name);
    room->SetAttribute("environment", rm->sector_type);

    // is this the first room?
    if (rm->zone != prevRoomZone)
    {
      if (!coordsMap.empty())
      {
        for (XMLElement * r = rooms->FirstChildElement("room"); r != NULL; r=r->NextSiblingElement())
        {
          if (r->IntAttribute("area") != prevRoomZone)
            continue;
            
          if(r->FirstChildElement("coord"))
            continue;
          
          if (coordsMap.count(r->IntAttribute("id")))
          {
            XMLElement * c = doc.NewElement("coord");
            c->SetAttribute("x", coordsMap[r->IntAttribute("id")].get<0>());
            c->SetAttribute("y", coordsMap[r->IntAttribute("id")].get<1>());
            c->SetAttribute("z", coordsMap[r->IntAttribute("id")].get<2>());
            r->InsertEndChild(c);
          }
        }
      }
      firstRoom = true;
      coordsMap.clear();
      furthestEast = 0;
    } // does the current room have coords?
    else if (!coordsMap.count(currentRoom))
    { 
      // see if any exit rooms have coords
      for (int door = 0; door < NUM_OF_DIRS; door++)
      {
        if (W_EXIT(nr, door) && !world.count(W_EXIT(nr, door)->to_room))
          continue;
          
        if (W_EXIT(nr, door) && W_EXIT(nr, door)->to_room != NOWHERE &&
            world[W_EXIT(nr, door)->to_room]->zone != 255 )
        {
            int eRoomNum = W_EXIT(nr, door)->to_room;
            if (coordsMap.count(eRoomNum))
            {
              coordsMap[currentRoom].set<0>(coordsMap[eRoomNum].get<0>() + (-1 * dir_offsets[door][0]));
              coordsMap[currentRoom].set<1>(coordsMap[eRoomNum].get<1>() + (-1 * dir_offsets[door][1]));
              coordsMap[currentRoom].set<2>(coordsMap[eRoomNum].get<2>() + (-1 * dir_offsets[door][2]));

            }
        }
      }
    }
      
    roomHasExit = false;
    for (int door = 0; door < NUM_OF_DIRS; door++)
    {
        if (W_EXIT(nr, door) && !world.count(W_EXIT(nr, door)->to_room))
            continue;
            
        if (W_EXIT(nr, door) && W_EXIT(nr, door)->to_room != NOWHERE &&
            world[W_EXIT(nr, door)->to_room]->zone != 255 )
        {
            roomHasExit = true;
            int eRoomNum = W_EXIT(nr, door)->to_room;

            // only find exit coordinates if we are not the exit target!
            if (currentRoom != eRoomNum)
            { // first room always starts at 0,0,0
              if (firstRoom)
              {
                coordsMap[eRoomNum].set<0>(dir_offsets[door][0]); // x
                coordsMap[eRoomNum].set<1>(dir_offsets[door][1]); // y
                coordsMap[eRoomNum].set<2>(dir_offsets[door][2]); // z
              } // does the current room have a coordinate?
              else if (coordsMap.count(currentRoom))
              { // set exit room coords based on currentRoom
                coordsMap[eRoomNum].set<0>(coordsMap[currentRoom].get<0>() + dir_offsets[door][0]);
                coordsMap[eRoomNum].set<1>(coordsMap[currentRoom].get<1>() + dir_offsets[door][1]);
                coordsMap[eRoomNum].set<2>(coordsMap[currentRoom].get<2>() + dir_offsets[door][2]);
              } // no coords for current room
            }
            // skip adding the exit to the map if the door is closed
            if  (IS_SET(W_EXIT(nr, door)->exit_info, EX_CLOSED))
              continue;

            XMLElement * exit = doc.NewElement("exit");
            exit->SetAttribute("direction", dirs[door]);
            exit->SetAttribute("target", eRoomNum);
            room->InsertEndChild(exit);
            

        }
    }
    if (roomHasExit)
    {
      if (firstRoom)
      {
        coordsMap[nr].set<0>(0);
        coordsMap[nr].set<1>(0);
        coordsMap[nr].set<2>(0);
        XMLElement * coords = doc.NewElement("coord");
        coords->SetAttribute("x", 0);
        coords->SetAttribute("y", 0);
        coords->SetAttribute("z", 0);
        room->InsertEndChild(coords);
        firstRoom = false;
      }
      else if (coordsMap.count(currentRoom))
      {
        XMLElement * coords = doc.NewElement("coord");
        coords->SetAttribute("x", coordsMap[currentRoom].get<0>());
        coords->SetAttribute("y", coordsMap[currentRoom].get<1>());
        coords->SetAttribute("z", coordsMap[currentRoom].get<2>());
        room->InsertEndChild(coords);

      
      }
      
      rooms->InsertEndChild(room);
    }
    prevRoomZone = rm->zone;
  }

  XMLElement * sects = doc.NewElement("environments");
  pRoot->InsertEndChild(sects);

  for (int i = 0; i < NUM_ROOM_SECTORS; i++)
  {
    XMLElement * sect = doc.NewElement("environment");
    sect->SetAttribute("id", i);
    sect->SetAttribute("name", sector_types[i]);
    sect->SetAttribute("color", sector_colors[i]);
    sect->SetAttribute("htmlcolor", sector_html_colors[i]);
    sects->InsertEndChild(sect);
  }

  XMLError eResult = doc.SaveFile("/var/www/wop/maps/map.xml");

}

