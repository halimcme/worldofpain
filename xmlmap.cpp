/**************************************************************************
 *   File: xmlmap.c                                  Part of World of Pain *
 *  Usage: Generate XML MMP format map of the world for mappers            *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 2022 World of Pain                                       *
 ***************************************************************************/

#define __XMLMAP_C__

#include "comm.h"
#include "conf.h"
#include "constants.h"
#include "db.h"
#include "olc.h"
#include "structs.h"
#include "sysdep.h"
#include "utils.h"

#include <boost/geometry.hpp>
#include <iostream>
#include <tinyxml2.h>

namespace bg = boost::geometry;
using namespace std;
using namespace tinyxml2;


// directional offsets:   NORTH   EAST   SOUTH     WEST     UP     DOWN
int dir_offsets[6][3] = {{0, 1, 0}, {1, 0, 0}, {0, -1, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};

int sector_colors[] = {
    8,  // inside
    7,  // city
    10, // field
    2,  // forest
    10, // hills
    13, // mountains
    12, // water (swim)
    6,  // water (no swim)
    14, // underwater
    12, // in flight
    8,  // cave
    11, // desert
    15, // arctic
    16, // space
    9,  // quicksand
    5   // swamp
};

typedef bg::model::point<long, 3, bg::cs::cartesian> coord;
typedef map<room_num, coord> cMap;
room_num findCoords(coord c, cMap coordsMap)
{
  for (auto i : coordsMap) {
    if (i.second.get<0>() == c.get<0>() && i.second.get<1>() == c.get<1>() && i.second.get<2>() == c.get<2>())
      return i.first;
  }
  return NOWHERE;
}

// generate the world map xml file
void generateWorldMap()
{
  XMLDocument doc;
  XMLDeclaration* decl = doc.NewDeclaration();
  doc.InsertFirstChild(decl);
  XMLNode* pRoot = doc.NewElement("map");
  doc.InsertEndChild(pRoot);
  XMLElement* areas = doc.NewElement("areas");
  pRoot->InsertEndChild(areas);
  for (auto const& [nr, zone] : zone_table) {
    if (nr == 255)
      continue;
    XMLElement* area = doc.NewElement("area");
    area->SetAttribute("id", nr);
    area->SetAttribute("name", zone->name);
    areas->InsertEndChild(area);
  }

  XMLElement* rooms = doc.NewElement("rooms");
  pRoot->InsertEndChild(rooms);
  bool roomHasExit;      // only include rooms that have exits
  bool firstRoom = true; // first room of a zone (area)
  int prevRoomZone = 0;  // the zone of the most recent room used for firstRoom
  int currentRoom = 0;   // current room vnum
  cMap coordsMap;
  coord currentCoords;
  int x, y, z;
  vector<room_num> zoneRooms;

  // WORLD LOOP of all rooms
  for (auto const [nr, rm] : world) {
    // skip Dark Tower, NOMAP rooms, and houses
    if (rm->zone == 255 || ROOM_FLAGGED(nr, ROOM_NOMAP) || ROOM_FLAGGED(nr, ROOM_HOUSE))
      continue;

    currentRoom = nr;
    XMLElement* room = doc.NewElement("room");
    room->SetAttribute("id", currentRoom);
    room->SetAttribute("area", rm->zone);
    room->SetAttribute("title", rm->name);
    room->SetAttribute("environment", rm->sector_type);

    // is this the first room of a zone? handle cleanup from previous zone
    if ((rm->zone != prevRoomZone || nr == world.rbegin()->first)
        && !coordsMap.empty()) { // loop through existing XML room elements to add
                                 // the coords
      // why is this necessary? -bliz
      int pass = 0;
      int totalC = 0;
      while (1) {
        int addedCoords = 0;
        for (auto zr : zoneRooms) {
          if (coordsMap.count(zr))
            continue;
          // see if any exit rooms have coords
          for (int door = 0; door < NUM_OF_DIRS; door++) {
            if (!W_EXIT(zr, door))
              continue;

            room_num eRoomNum = W_EXIT(zr, door)->to_room;

            if (eRoomNum == NOWHERE || !world.count(eRoomNum) || GET_ROOM_ZONE(eRoomNum) == 255)
              continue;

            if (coordsMap.count(eRoomNum)) {
              currentCoords.set<0>(coordsMap[eRoomNum].get<0>() + (-1 * dir_offsets[door][0]));
              currentCoords.set<1>(coordsMap[eRoomNum].get<1>() + (-1 * dir_offsets[door][1]));
              currentCoords.set<2>(coordsMap[eRoomNum].get<2>() + (-1 * dir_offsets[door][2]));

              room_num dup = findCoords(currentCoords, coordsMap);
              if (dup == NOWHERE) {
                coordsMap[zr].set<0>(coordsMap[eRoomNum].get<0>() + (-1 * dir_offsets[door][0]));
                coordsMap[zr].set<1>(coordsMap[eRoomNum].get<1>() + (-1 * dir_offsets[door][1]));
                coordsMap[zr].set<2>(coordsMap[eRoomNum].get<2>() + (-1 * dir_offsets[door][2]));
                addedCoords++;
              }
            }
          } // for (int door...
        }   // for (auto zr...
        pass++;
        totalC += addedCoords;
        if (addedCoords == 0) {
          break;
        }
      } // while(1)
      // see if any exit rooms have coords
      for (int door = 0; door < NUM_OF_DIRS; door++) {
        if (!W_EXIT(nr, door))
          continue;

        room_num eRoomNum = W_EXIT(nr, door)->to_room;

        if (eRoomNum == NOWHERE || !world.count(eRoomNum) || GET_ROOM_ZONE(eRoomNum) == 255)
          continue;

        if (coordsMap.count(eRoomNum)) {
          currentCoords.set<0>(coordsMap[eRoomNum].get<0>() + (-1 * dir_offsets[door][0]));
          currentCoords.set<1>(coordsMap[eRoomNum].get<1>() + (-1 * dir_offsets[door][1]));
          currentCoords.set<2>(coordsMap[eRoomNum].get<2>() + (-1 * dir_offsets[door][2]));

          room_num dup = findCoords(currentCoords, coordsMap);
          if (dup == NOWHERE) {
            coordsMap[currentRoom].set<0>(coordsMap[eRoomNum].get<0>() + (-1 * dir_offsets[door][0]));
            coordsMap[currentRoom].set<1>(coordsMap[eRoomNum].get<1>() + (-1 * dir_offsets[door][1]));
            coordsMap[currentRoom].set<2>(coordsMap[eRoomNum].get<2>() + (-1 * dir_offsets[door][2]));
          }
        }
      }
      for (XMLElement* r = rooms->FirstChildElement("room"); r != NULL; r = r->NextSiblingElement()) {
        // skip rooms where we aren't in the previous zone or alredy have coords
        if (r->IntAttribute("area") != prevRoomZone || r->FirstChildElement("coord"))
          continue;

        room_num lastRoom = r->IntAttribute("id");
        if (coordsMap.count(lastRoom)) {
          XMLElement* c = doc.NewElement("coord");
          c->SetAttribute("x", coordsMap[lastRoom].get<0>());
          c->SetAttribute("y", coordsMap[lastRoom].get<1>());
          c->SetAttribute("z", coordsMap[lastRoom].get<2>());
          r->InsertEndChild(c);
        }
      }
      // we've finished adding coords for the previous zone to the XML, start
      // fresh
      firstRoom = true;
      coordsMap.clear();
      zoneRooms.clear();
    } // if the current room doesn't have coords, try and find them
    else if (!coordsMap.count(currentRoom)) {
      // see if any exit rooms have coords
      for (int door = 0; door < NUM_OF_DIRS; door++) {
        if (!W_EXIT(nr, door))
          continue;

        room_num eRoomNum = W_EXIT(nr, door)->to_room;

        if (eRoomNum == NOWHERE || !world.count(eRoomNum) || GET_ROOM_ZONE(eRoomNum) == 255)
          continue;

        if (coordsMap.count(eRoomNum)) {
          currentCoords.set<0>(coordsMap[eRoomNum].get<0>() + (-1 * dir_offsets[door][0]));
          currentCoords.set<1>(coordsMap[eRoomNum].get<1>() + (-1 * dir_offsets[door][1]));
          currentCoords.set<2>(coordsMap[eRoomNum].get<2>() + (-1 * dir_offsets[door][2]));

          room_num dup = findCoords(currentCoords, coordsMap);
          if (dup == NOWHERE) {
            coordsMap[currentRoom].set<0>(coordsMap[eRoomNum].get<0>() + (-1 * dir_offsets[door][0]));
            coordsMap[currentRoom].set<1>(coordsMap[eRoomNum].get<1>() + (-1 * dir_offsets[door][1]));
            coordsMap[currentRoom].set<2>(coordsMap[eRoomNum].get<2>() + (-1 * dir_offsets[door][2]));
          }
        }
      }
    }

    zoneRooms.push_back(nr);
    roomHasExit = false;
    for (int door = 0; door < NUM_OF_DIRS; door++) {
      if (!W_EXIT(nr, door))
        continue;

      int eRoomNum = W_EXIT(nr, door)->to_room;

      if (eRoomNum == NOWHERE || !world.count(eRoomNum) || ROOM_FLAGGED(eRoomNum, ROOM_NOMAP)
          || ROOM_FLAGGED(eRoomNum, ROOM_HOUSE) || GET_ROOM_ZONE(eRoomNum) == 255 || currentRoom == eRoomNum)
        continue;

      roomHasExit = true;

      // first room always starts at 0,0,0
      if (firstRoom) {
        coordsMap[eRoomNum].set<0>(dir_offsets[door][0]); // x
        coordsMap[eRoomNum].set<1>(dir_offsets[door][1]); // y
        coordsMap[eRoomNum].set<2>(dir_offsets[door][2]); // z
      }                                                   // does the current room have a coordinate?
      else if (coordsMap.count(currentRoom)) {            // set exit room coords based on currentRoom
        currentCoords.set<0>(coordsMap[currentRoom].get<0>() + (-1 * dir_offsets[door][0]));
        currentCoords.set<1>(coordsMap[currentRoom].get<1>() + (-1 * dir_offsets[door][1]));
        currentCoords.set<2>(coordsMap[currentRoom].get<2>() + (-1 * dir_offsets[door][2]));

        room_num dup = findCoords(currentCoords, coordsMap);
        if (dup == NOWHERE) {
          coordsMap[eRoomNum].set<0>(coordsMap[currentRoom].get<0>() + dir_offsets[door][0]);
          coordsMap[eRoomNum].set<1>(coordsMap[currentRoom].get<1>() + dir_offsets[door][1]);
          coordsMap[eRoomNum].set<2>(coordsMap[currentRoom].get<2>() + dir_offsets[door][2]);
        }
      } // no coords for current room

      // skip adding the exit to the map if the door is closed
      if (IS_SET(W_EXIT(nr, door)->exit_info, EX_CLOSED))
        continue;

      XMLElement* exit = doc.NewElement("exit");
      exit->SetAttribute("direction", dirs[door]);
      exit->SetAttribute("target", eRoomNum);
      room->InsertEndChild(exit);
    }

    if (roomHasExit) {
      if (firstRoom) {
        coordsMap[nr].set<0>(0);
        coordsMap[nr].set<1>(0);
        coordsMap[nr].set<2>(0);
        XMLElement* coords = doc.NewElement("coord");
        coords->SetAttribute("x", 0);
        coords->SetAttribute("y", 0);
        coords->SetAttribute("z", 0);
        room->InsertEndChild(coords);
        firstRoom = false;
      } else if (coordsMap.count(currentRoom)) {
        XMLElement* coords = doc.NewElement("coord");
        coords->SetAttribute("x", coordsMap[currentRoom].get<0>());
        coords->SetAttribute("y", coordsMap[currentRoom].get<1>());
        coords->SetAttribute("z", coordsMap[currentRoom].get<2>());
        room->InsertEndChild(coords);
      }
      rooms->InsertEndChild(room);
    }
    prevRoomZone = rm->zone;
  } // end of WORLD LOOP of all rooms

  // delete rooms with no coords from the map
  int deletedRooms = 0;
  XMLElement* nextRoom;
  for (XMLElement* r = rooms->FirstChildElement("room"); r != NULL;) {
    nextRoom = r->NextSiblingElement();
    if (!r->FirstChildElement("coord")) {
      rooms->DeleteChild(r);
      deletedRooms++;
    }
    r = nextRoom;
  }
  do_log("XMLMap: deleted %d rooms without coords", deletedRooms);

  XMLElement* sects = doc.NewElement("environments");
  pRoot->InsertEndChild(sects);

  for (int i = 0; i < NUM_ROOM_SECTORS; i++) {
    XMLElement* sect = doc.NewElement("environment");
    sect->SetAttribute("id", i);
    sect->SetAttribute("name", sector_types[i]);
    sect->SetAttribute("color", sector_colors[i]);
    sect->SetAttribute("htmlcolor", sector_html_colors[i]);
    sects->InsertEndChild(sect);
  }

  auto mapFile = "/var/www/wop/maps/map.xml";
  XMLError eResult = doc.SaveFile(mapFile);
  if (eResult == XML_SUCCESS)
    do_log("Map file successfilly written to %s", mapFile);
  else
    do_log("Error writing map file %s: %s", mapFile, XMLDocument::ErrorIDToName(eResult));
}
