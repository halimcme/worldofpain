# Protocol code
This is primarily example code until the full WoP code is released, but it could serve as a starting point. This is how GMCP is implemented in the World of Pain.

Some definitions from type.h may be needed to compile, among other things.

In order to update stats on each game tick, I'm using a function inside comm.cpp's `void heartbeat(int pulse)`:
```
  if (!(pulse % PASSES_PER_SEC))
    oob_update();
```

The oob_update function WoP uses:
```
// update OOB (GMCP, etc.) variables. based on KaVir's plugin
static void oob_update(void)
{
  dPtr d;
  int PlayerCount = 0;
  char buf[MAX_STRING_LENGTH];
  extern const char* pc_class_types[];

  for (auto& d : descriptor_list) {
    if (d->connected == CON_PLAYING) {
      chPtr ch = d->character;
      if (d->close_me || !ch || ch->extract_me || IS_NPC(ch))
        continue;

      chPtr pOpponent = FIGHTING(ch);
      if (!GET_INVIS_LEV(ch))
        ++PlayerCount;

      OOBSetString(d, eOOB_CHARACTER_NAME, GET_NAME(ch));
      sprintf(buf, "%s %s", GET_NAME(ch), GET_TITLE(ch));
      OOBSetString(d, eOOB_CHAR_FULL_NAME, buf);
      OOBSetNumber(d, eOOB_ALIGNMENT, GET_ALIGNMENT(ch));
      OOBSetNumber(d, eOOB_EXPERIENCE, GET_EXP(ch));

      OOBSetNumber(d, eOOB_LEVEL, GET_LEVEL(ch));

      if (IS_SET(PLR_FLAGS(ch), PLR_MULTICLASS))
        sprinttype(GET_MULTICLASS(ch), pc_class_types, buf, sizeof(buf));
      else
        sprinttype(GET_CLASS(ch), pc_class_types, buf, sizeof(buf));

      OOBSetString(d, eOOB_CLASS, buf);

      OOBSetNumber(d, eOOB_WIMPY, GET_WIMP_LEV(ch));
      OOBSetNumber(d, eOOB_MONEY, GET_GOLD(ch));
      OOBSetNumber(d, eOOB_BANK, GET_BANK_GOLD(ch));

      OOBSetNumber(d, eOOB_AC, GET_AC(ch));

      // < 50 newbies can't see stats
      if (GET_LEVEL(ch) >= 50) {
        OOBSetNumber(d, eOOB_STR, GET_STR(ch));
        OOBSetNumber(d, eOOB_INT, GET_INT(ch));
        OOBSetNumber(d, eOOB_WIS, GET_WIS(ch));
        OOBSetNumber(d, eOOB_DEX, GET_DEX(ch));
        OOBSetNumber(d, eOOB_CON, GET_CON(ch));
        OOBSetNumber(d, eOOB_CHA, GET_CHA(ch));
      }

      OOBSetNumber(d, eOOB_HEALTH, GET_HIT(ch));
      OOBSetNumber(d, eOOB_HEALTH_MAX, GET_MAX_HIT(ch));
      OOBSetNumber(d, eOOB_MANA, GET_MANA(ch));
      OOBSetNumber(d, eOOB_MANA_MAX, GET_MAX_MANA(ch));
      OOBSetNumber(d, eOOB_MOVEMENT, GET_MOVE(ch));
      OOBSetNumber(d, eOOB_MOVEMENT_MAX, GET_MAX_MOVE(ch));

      if (FIGHTING(ch)) {
        OOBSetNumber(d, eOOB_OPPONENT_HEALTH, GET_HIT(FIGHTING(ch)));
        OOBSetNumber(d, eOOB_OPPONENT_HEALTH_MAX, GET_MAX_HIT(FIGHTING(ch)));
        OOBSetString(d, eOOB_OPPONENT_NAME, GET_NAME(FIGHTING(ch)));
        OOBSetNumber(d, eOOB_OPPONENT_LEVEL, GET_LEVEL(FIGHTING(ch)));
      } else {
        OOBSetNumber(d, eOOB_OPPONENT_HEALTH, 0);
        OOBSetNumber(d, eOOB_OPPONENT_HEALTH_MAX, 0);
        OOBSetString(d, eOOB_OPPONENT_NAME, "");
        OOBSetNumber(d, eOOB_OPPONENT_LEVEL, 0);
      }

      OOBUpdate(d);
    }

    /* Ideally this should be called once at startup, and again whenever
     * someone leaves or joins the mud.  But this works, and it keeps the
     * snippet simple.  Optimise as you see fit.
     */
    MSSPSetPlayers(PlayerCount);
  }
}
```


