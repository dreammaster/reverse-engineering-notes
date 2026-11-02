# Actor worktypes and the turn scheduler

`run_actor_turns` (`sub_2AB7A`, 2AB7A) runs after every command. `actor_worktype_act` (`sub_29BDA`) is what a chosen actor does: it switches on the worktype through the
table at `cs:3F1B` (33 values). Worktypes come from the actor's `worktype` array (objlist +0x11F1); the schedule entry sets 0x81 ("go to your place") and then the entry's own worktype.

| Worktype | Meaning (Nuvie's name) | Handler |
|---|---|---|
| 0 | motionless (never acts) | - |
| 1 | follows the party (never acts on its own; moved with the leader) | - |
| 2 | the member the player controls | `run_actor_turns` ends the turn |
| 3 / 4 / 5 | combat front / rear / flank | `wt_combat_front` 2E454, `wt_combat_rear` 2E4B9, `wt_combat_flank` 2E7E2 |
| 6 | berserk | `wt_combat_berserk` 2EC2A |
| 7, 9, 0x13, 0x9A | retreat, shy, ... | `wt_combat_retreat` 2EEC2 |
| 8, 0x9B | assault (wild animals), attack the party | `wt_combat_assault` 2DF2E |
| 0xA | like | `wt_combat_like` 2F06F |
| 0xB | unfriendly | `wt_combat_unfriendly` 2F1A7 |
| 0xC, 0x8F | animal wander, wander around | `wt_wander` 2954B |
| 0xD | tangle vine | `wt_tangle` 2F4E6 |
| 0xE | immobile | `wt_immobile` 2F2DE |
| 0xF, 0x10, 0x8B-0x8E | guards walking east-west / north-south, walks | random choice via `sub_3B8F9`, then movement |
| 0x12 | (lookout?) | `wt_work_12` 29723 |
| 0x81, 0x82 | go to the schedule place | `walk_to_schedule_place` 28AA7 (+ `sub_29B97`) |
| 0x83-0x85 | follow a path | `wt_walk_path` 295ED |
| 0x86-0x8A | walk to a location, face north/east/south/west (set by schedule entries) | see `walk_to_schedule_place` |
| 0x90, 0x94 | work, ... | `wt_work` 29134 |
| 0x91 | sleep | handled in `advance_time` / sleeping checks |
| 0x96, 0x97 | beg, ... | (no call) |
| 0x98 | ? | `sub_11FF9` |
| 0x99 | ? | `wt_0x99` 2DD5E |

In the same routine: an actor in the party with status bit 4 clear runs `sub_2B04E` first; actors on another level or while `byte_404BA` (a time-stop style counter) is set simply lose their moves.

Not ported: all of these handlers. The stand-alone engine currently does schedule walking (own path finder), hostile hunting and random wandering for creatures only
(see `approximations.md` #1-#4, #20, #22).
