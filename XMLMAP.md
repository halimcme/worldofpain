# MMP XML Maps 
This is a CircleMUD/tbaMUD implementation in C++ of the XML MMP format described here: https://wiki.mudlet.org/w/Standards:MMP. The coordinates system is not perfect but it does a decent job at mapping most zones when rooms don't overlap.
## Requirements
* Tinyxml2 library: http://leethomason.github.io/tinyxml2/
* Boost Geometry library: https://github.com/boostorg/geometry

Just add generateWorldMap(); after index_boot in your db.c boot_world function.
